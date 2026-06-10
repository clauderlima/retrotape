#include "audio/DacAudioOutput.h"

#include <SD.h>
#include <driver/timer.h>
#include <esp_err.h>
#include <soc/rtc_io_reg.h>
#include <soc/soc.h>

#include "config/pins.h"

namespace {

constexpr timer_group_t TapTimerGroup = TIMER_GROUP_0;
constexpr timer_idx_t TapTimerIndex = TIMER_0;
constexpr uint32_t TapTimerDivider = 8;
constexpr uint32_t TapTimerTicksPerSecond = TIMER_BASE_CLK / TapTimerDivider;
portMUX_TYPE TapTimerMux = portMUX_INITIALIZER_UNLOCKED;

}  // namespace

namespace audio {

constexpr uint8_t DacAudioOutput::CasHeaderMarker[DacAudioOutput::CasHeaderMarkerSize];

bool DacAudioOutput::begin() {
  writeIdleLevel();
  Serial.print("DAC audio output ready on GPIO ");
  Serial.println(config::pins::SpeakerPwm);
  return true;
}

void DacAudioOutput::update() {
  switch (playbackKind_) {
    case PlaybackKind::Wav:
      updateWav();
      break;
    case PlaybackKind::Tap:
      updateTap();
      break;
    case PlaybackKind::Cas:
      updateCas();
      break;
    case PlaybackKind::None:
    default:
      break;
  }
}

void DacAudioOutput::stop() {
  tapStopRequested_ = true;
  shutdownTapTimer();
  releaseTapBlock();

  if (wavFile_) {
    wavFile_.close();
  }
  if (tapFile_) {
    tapFile_.close();
  }
  if (casFile_) {
    casFile_.close();
  }
  wavPlaying_ = false;
  tapPlaying_ = false;
  casPlaying_ = false;
  playbackKind_ = PlaybackKind::None;
  framesRemaining_ = 0;
  tapPauseActive_ = false;
  tapPulseStage_ = TapPulseStage::Idle;
  casStage_ = CasStage::Idle;
  writeIdleLevel();
  Serial.println("Audio output stopped");
}

void DacAudioOutput::setVolume(uint8_t volume) {
  volume_ = volume;
  Serial.print("Audio volume set to ");
  Serial.println(volume_);
}

void DacAudioOutput::setTapTimingPermille(uint16_t permille) {
  tapTimingPermille_ = constrain(permille, 900U, 1100U);
  Serial.print("TAP timing: ");
  Serial.print(tapTimingPermille_ / 10.0F);
  Serial.println("%");
}

void DacAudioOutput::setTapInverted(bool inverted) {
  tapInverted_ = inverted;
  Serial.print("TAP output inverted: ");
  Serial.println(tapInverted_ ? "yes" : "no");
}

void DacAudioOutput::setTapAmplitude(uint8_t amplitude) {
  tapAmplitude_ = constrain(amplitude, 8U, 120U);
  Serial.print("TAP DAC amplitude: ");
  Serial.print((static_cast<uint16_t>(tapAmplitude_) * 100U) / 127U);
  Serial.println("%");
}

void DacAudioOutput::setTapPauseMs(uint16_t pauseMs) {
  tapPauseMs_ = constrain(pauseMs, 250U, 5000U);
  Serial.print("TAP block pause: ");
  Serial.print(tapPauseMs_);
  Serial.println(" ms");
}

void DacAudioOutput::setTapDebugEnabled(bool enabled) {
  tapDebugEnabled_ = enabled;
}

bool DacAudioOutput::playTestTone(uint16_t frequencyHz, uint32_t durationMs) {
  if (frequencyHz == 0 || durationMs == 0) {
    return false;
  }

  const uint32_t halfPeriodUs = 500000UL / frequencyHz;
  const uint32_t endAt = millis() + durationMs;
  uint8_t level = 64;

  while (static_cast<int32_t>(millis() - endAt) < 0) {
    dacWrite(config::pins::SpeakerPwm, scaleSample(level));
    level = level == 64 ? 192 : 64;
    delayMicroseconds(halfPeriodUs);
  }

  writeIdleLevel();
  return true;
}

bool DacAudioOutput::playWavFile(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();

  wavFile_ = SD.open(path, FILE_READ);
  if (!wavFile_) {
    Serial.print("Unable to open WAV: ");
    Serial.println(path);
    return false;
  }

  wavInfo_ = WavInfo{};
  if (!readWavInfo(wavFile_, wavInfo_)) {
    Serial.println("Invalid or unsupported WAV");
    wavFile_.close();
    writeIdleLevel();
    return false;
  }

  if (wavInfo_.audioFormat != 1 || (wavInfo_.bitsPerSample != 8 && wavInfo_.bitsPerSample != 16) ||
      (wavInfo_.channels != 1 && wavInfo_.channels != 2) || wavInfo_.sampleRate == 0) {
    Serial.println("Unsupported WAV format");
    wavFile_.close();
    writeIdleLevel();
    return false;
  }

  Serial.print("Playing WAV: ");
  Serial.println(path);
  Serial.print("Sample rate: ");
  Serial.println(wavInfo_.sampleRate);
  Serial.print("Channels: ");
  Serial.println(wavInfo_.channels);
  Serial.print("Bits: ");
  Serial.println(wavInfo_.bitsPerSample);

  frameBytes_ = (wavInfo_.bitsPerSample / 8) * wavInfo_.channels;
  totalFrames_ = wavInfo_.dataSize / frameBytes_;
  framesRemaining_ = totalFrames_;
  framesPlayed_ = 0;
  samplePeriodUs_ = 1000000UL / wavInfo_.sampleRate;
  if (samplePeriodUs_ == 0) {
    samplePeriodUs_ = 1;
  }
  nextSampleAtUs_ = micros();

  wavFile_.seek(wavInfo_.dataStart);
  wavPlaying_ = true;
  playbackKind_ = PlaybackKind::Wav;
  return true;
}

bool DacAudioOutput::playTapFile(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();

  tapFile_ = SD.open(path, FILE_READ);
  if (!tapFile_) {
    Serial.print("Unable to open TAP: ");
    Serial.println(path);
    return false;
  }

  tapTotalDurationMs_ = estimateTapDurationMs(tapFile_);
  if (tapTotalDurationMs_ == 0) {
    Serial.println("Invalid or empty TAP");
    tapFile_.close();
    return false;
  }

  tapFile_.seek(0);
  tapBlockIndex_ = 0;
  tapElapsedTenthsUs_ = 0;
  tapStopRequested_ = false;
  tapPauseActive_ = false;
  if (!initializeTapTimer()) {
    tapFile_.close();
    Serial.println("Unable to initialize TAP pulse timer");
    return false;
  }

  tapPlaying_ = true;
  playbackKind_ = PlaybackKind::Tap;
  if (!loadNextTapBlock() || !startLoadedTapBlock()) {
    finishTapPlayback("TAP playback failed while loading the first block");
    return false;
  }

  Serial.print("Playing TAP: ");
  Serial.println(path);
  Serial.print("Estimated duration ms: ");
  Serial.println(tapTotalDurationMs_);
  return true;
}

bool DacAudioOutput::playCasFile(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();

  casFile_ = SD.open(path, FILE_READ);
  if (!casFile_) {
    Serial.print("Unable to open CAS: ");
    Serial.println(path);
    return false;
  }

  casTotalDurationMs_ = estimateCasDurationMs(casFile_);
  if (casTotalDurationMs_ == 0) {
    Serial.println("Invalid or empty CAS");
    casFile_.close();
    return false;
  }

  casFile_.seek(0);
  casStage_ = CasStage::LoadNext;
  casPlaying_ = true;
  playbackKind_ = PlaybackKind::Cas;
  casLevelHigh_ = false;
  casElapsedUs_ = 0;
  casNextPulseAtUs_ = micros();
  writeTapeLevel(false);

  Serial.print("Playing CAS: ");
  Serial.println(path);
  Serial.print("Estimated duration ms: ");
  Serial.println(casTotalDurationMs_);
  return true;
}

bool DacAudioOutput::isPlaying() const {
  return wavPlaying_ || tapPlaying_ || casPlaying_;
}

uint32_t DacAudioOutput::playbackElapsedMs() const {
  if (playbackKind_ == PlaybackKind::None) {
    return 0;
  }

  if (playbackKind_ == PlaybackKind::Tap) {
    portENTER_CRITICAL(&TapTimerMux);
    const uint64_t elapsedTenthsUs = tapElapsedTenthsUs_;
    portEXIT_CRITICAL(&TapTimerMux);
    return static_cast<uint32_t>(elapsedTenthsUs / 10000ULL);
  }

  if (playbackKind_ == PlaybackKind::Cas) {
    return casElapsedUs_ / 1000;
  }

  if (wavInfo_.sampleRate == 0) {
    return 0;
  }

  return static_cast<uint32_t>((static_cast<uint64_t>(framesPlayed_) * 1000ULL) / wavInfo_.sampleRate);
}

uint32_t DacAudioOutput::playbackDurationMs() const {
  if (playbackKind_ == PlaybackKind::None) {
    return 0;
  }

  if (playbackKind_ == PlaybackKind::Tap) {
    return tapTotalDurationMs_;
  }

  if (playbackKind_ == PlaybackKind::Cas) {
    return casTotalDurationMs_;
  }

  if (wavInfo_.sampleRate == 0) {
    return 0;
  }

  return static_cast<uint32_t>((static_cast<uint64_t>(totalFrames_) * 1000ULL) / wavInfo_.sampleRate);
}

void DacAudioOutput::finishPlayback(const char* message) {
  if (wavFile_) {
    wavFile_.close();
  }
  if (casFile_) {
    casFile_.close();
  }
  wavPlaying_ = false;
  casPlaying_ = false;
  casStage_ = CasStage::Idle;
  playbackKind_ = PlaybackKind::None;
  writeIdleLevel();
  Serial.println(message);
}

void DacAudioOutput::updateWav() {
  if (!wavPlaying_) {
    return;
  }

  uint16_t framesThisCall = 0;
  uint32_t now = micros();

  while (wavPlaying_ && framesRemaining_ > 0 && static_cast<int32_t>(now - nextSampleAtUs_) >= 0 &&
         framesThisCall < 96) {
    if (!readAndWriteFrame()) {
      finishPlayback("WAV read error");
      return;
    }

    --framesRemaining_;
    ++framesPlayed_;
    ++framesThisCall;
    nextSampleAtUs_ += samplePeriodUs_;
    now = micros();
  }

  if (wavPlaying_ && framesRemaining_ == 0) {
    finishPlayback("WAV playback finished");
  }
}

void DacAudioOutput::updateTap() {
  if (!tapPlaying_ || tapStopRequested_) {
    return;
  }

  if (tapBlockFinished_) {
    const uint32_t lateInterrupts = tapLateInterruptCount_;
    const uint32_t maxLatenessTicks = tapMaxLatenessTicks_;
    releaseTapBlock();
    tapBlockFinished_ = false;
    tapPauseActive_ = true;
    tapPauseUntilMs_ = millis() + tapPauseMs_;
    tapElapsedTenthsUs_ += static_cast<uint64_t>(tapPauseMs_) * 10000ULL;

    if (tapDebugEnabled_) {
      Serial.print("[TAP] Block ");
      Serial.print(tapBlockIndex_);
      Serial.print(" complete, pause=");
      Serial.print(tapPauseMs_);
      Serial.println(" ms");
      Serial.print("[TAP] Timer late interrupts=");
      Serial.print(lateInterrupts);
      Serial.print(" max lateness=");
      Serial.print(maxLatenessTicks / 10.0F, 1);
      Serial.println(" us");
    }
  }

  if (!tapPauseActive_ || static_cast<int32_t>(millis() - tapPauseUntilMs_) < 0) {
    return;
  }

  tapPauseActive_ = false;
  if (!tapFile_.available()) {
    finishTapPlayback("TAP playback finished");
    return;
  }

  if (!loadNextTapBlock() || !startLoadedTapBlock()) {
    finishTapPlayback("TAP playback failed while loading a block");
  }
}

void DacAudioOutput::updateCas() {
  if (!casPlaying_) {
    return;
  }

  const uint32_t batchStartUs = micros();
  while (casPlaying_ && static_cast<uint32_t>(micros() - batchStartUs) < 3500UL) {
    if (casStage_ == CasStage::LoadNext && !beginNextCasUnit()) {
      finishPlayback("CAS playback finished");
      return;
    }

    uint32_t nowUs = micros();
    if (static_cast<int32_t>(nowUs - casNextPulseAtUs_) > 2000) {
      casNextPulseAtUs_ = nowUs;
    }

    while (static_cast<int32_t>(nowUs - casNextPulseAtUs_) < 0) {
      yield();
      nowUs = micros();
    }

    switch (casStage_) {
      case CasStage::Header:
        emitCasPulse(CasHeaderHalfPulseUs);
        if (--casHeaderHalfPulsesRemaining_ == 0) {
          casStage_ = CasStage::LoadNext;
        }
        break;
      case CasStage::Data:
        if (casBitHalfPulsesRemaining_ == 0 && !beginNextCasBit()) {
          casStage_ = CasStage::LoadNext;
          break;
        }

        emitCasPulse(casBitHalfPulseUs_);
        --casBitHalfPulsesRemaining_;
        break;
      case CasStage::Idle:
      case CasStage::LoadNext:
      default:
        break;
    }
  }
}

bool DacAudioOutput::readAndWriteFrame() {
  uint8_t frame[4] = {};
  if (wavFile_.read(frame, frameBytes_) != frameBytes_) {
    return false;
  }

  int32_t mixed = 0;
  if (wavInfo_.bitsPerSample == 8) {
    mixed = frame[0];
    if (wavInfo_.channels == 2) {
      mixed = (mixed + frame[1]) / 2;
    }
  } else {
    const int16_t left = static_cast<int16_t>(frame[0] | (frame[1] << 8));
    mixed = left;
    if (wavInfo_.channels == 2) {
      const int16_t right = static_cast<int16_t>(frame[2] | (frame[3] << 8));
      mixed = (mixed + right) / 2;
    }
    mixed = (mixed + 32768) >> 8;
  }

  dacWrite(config::pins::SpeakerPwm, scaleSample(static_cast<uint8_t>(mixed)));
  return true;
}

bool DacAudioOutput::initializeTapTimer() {
  shutdownTapTimer();

  timer_config_t configuration = {};
  configuration.alarm_en = TIMER_ALARM_DIS;
  configuration.counter_en = TIMER_PAUSE;
  configuration.intr_type = TIMER_INTR_LEVEL;
  configuration.counter_dir = TIMER_COUNT_UP;
  configuration.auto_reload = TIMER_AUTORELOAD_DIS;
  configuration.divider = TapTimerDivider;

  esp_err_t result = timer_init(TapTimerGroup, TapTimerIndex, &configuration);
  if (result == ESP_OK) {
    result = timer_set_counter_value(TapTimerGroup, TapTimerIndex, 0);
  }
  if (result == ESP_OK) {
    result = timer_isr_callback_add(TapTimerGroup, TapTimerIndex, tapTimerCallback, this,
                                    ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);
  }

  if (result != ESP_OK) {
    Serial.print("TAP timer initialization failed: ");
    Serial.println(esp_err_to_name(result));
    timer_deinit(TapTimerGroup, TapTimerIndex);
    tapTimerReady_ = false;
    return false;
  }

  tapTimerReady_ = true;
  dacWrite(config::pins::SpeakerPwm, 128);

  Serial.print("TAP timer-driven DAC output ready on GPIO ");
  Serial.print(config::pins::SpeakerPwm);
  Serial.print(" with ");
  Serial.print(TapTimerTicksPerSecond);
  Serial.println(" timer ticks/s");
  return true;
}

void DacAudioOutput::shutdownTapTimer() {
  if (!tapTimerReady_) {
    return;
  }

  timer_disable_intr(TapTimerGroup, TapTimerIndex);
  timer_pause(TapTimerGroup, TapTimerIndex);
  timer_isr_callback_remove(TapTimerGroup, TapTimerIndex);
  timer_deinit(TapTimerGroup, TapTimerIndex);
  tapTimerReady_ = false;
  tapTimerActive_ = false;
  dacWrite(config::pins::SpeakerPwm, 128);
}

bool DacAudioOutput::loadNextTapBlock() {
  releaseTapBlock();

  uint16_t blockLength = 0;
  while (tapFile_.available()) {
    if (!readU16(tapFile_, blockLength)) {
      return false;
    }
    if (blockLength != 0) {
      break;
    }
  }

  if (blockLength == 0) {
    return false;
  }

  auto* block = static_cast<uint8_t*>(malloc(blockLength));
  if (block == nullptr) {
    Serial.println("TAP playback stopped: insufficient memory");
    return false;
  }

  if (tapFile_.read(block, blockLength) != blockLength) {
    Serial.print("[TAP] Read error, expected bytes=");
    Serial.print(blockLength);
    Serial.print(" file position=");
    Serial.print(tapFile_.position());
    Serial.print("/");
    Serial.println(tapFile_.size());
    free(block);
    return false;
  }

  tapBlockData_ = block;
  tapBlockLength_ = blockLength;
  ++tapBlockIndex_;
  return true;
}

bool DacAudioOutput::startLoadedTapBlock() {
  if (!tapTimerReady_ || tapBlockData_ == nullptr || tapBlockLength_ == 0) {
    return false;
  }

  tapPilotTicks_ = adjustedTapTicks(ZxPilotPulseTicks);
  tapSyncOneTicks_ = adjustedTapTicks(ZxSyncOnePulseTicks);
  tapSyncTwoTicks_ = adjustedTapTicks(ZxSyncTwoPulseTicks);
  tapZeroTicks_ = adjustedTapTicks(ZxZeroPulseTicks);
  tapOneTicks_ = adjustedTapTicks(ZxOnePulseTicks);

  uint8_t checksum = 0;
  for (uint16_t index = 0; index < tapBlockLength_; ++index) {
    checksum ^= tapBlockData_[index];
  }

  tapPulseStage_ = TapPulseStage::Pilot;
  tapPilotPulsesRemaining_ =
      tapBlockData_[0] < 0x80 ? ZxHeaderPilotPulses : ZxDataPilotPulses;
  tapByteIndex_ = 0;
  tapBitMask_ = 0x80;
  tapHalfPulse_ = 0;
  tapBlockFinished_ = false;
  tapLevelHigh_ = true;
  tapLateInterruptCount_ = 0;
  tapMaxLatenessTicks_ = 0;
  tapCurrentPulseTicks_ = nextTapPulseTicks();
  tapNextAlarmTicks_ = tapCurrentPulseTicks_;

  if (tapCurrentPulseTicks_ == 0) {
    return false;
  }

  if (tapDebugEnabled_) {
    Serial.println();
    Serial.print("[TAP] Block ");
    Serial.print(tapBlockIndex_);
    Serial.print(" type=");
    Serial.print(tapBlockData_[0] < 0x80 ? "HEADER" : "DATA");
    Serial.print(" bytes=");
    Serial.print(tapBlockLength_);
    Serial.print(" flag=0x");
    if (tapBlockData_[0] < 0x10) {
      Serial.print('0');
    }
    Serial.print(tapBlockData_[0], HEX);
    Serial.print(" checksum=");
    Serial.println(checksum == 0 ? "OK" : "INVALID");
    Serial.print("[TAP] Pilot=");
    Serial.print(tapPilotPulsesRemaining_ + 1U);
    Serial.print(" x ");
    Serial.print(tapPilotTicks_ / 10.0F, 1);
    Serial.print(" us, sync=");
    Serial.print(tapSyncOneTicks_ / 10.0F, 1);
    Serial.print("/");
    Serial.print(tapSyncTwoTicks_ / 10.0F, 1);
    Serial.print(" us, bit0=");
    Serial.print(tapZeroTicks_ / 10.0F, 1);
    Serial.print(" us x2, bit1=");
    Serial.print(tapOneTicks_ / 10.0F, 1);
    Serial.println(" us x2");
    Serial.print("[TAP] Output=DAC-TIMER timing=");
    Serial.print(tapTimingPermille_ / 10.0F, 1);
    Serial.print("% level=");
    Serial.print((static_cast<uint16_t>(tapAmplitude_) * 100U) / 127U);
    Serial.print("% inverted=");
    Serial.println(tapInverted_ ? "yes" : "no");
  }

  writeTapDacLevel(tapLevelHigh_);
  timer_pause(TapTimerGroup, TapTimerIndex);
  timer_set_counter_value(TapTimerGroup, TapTimerIndex, 0);
  timer_set_alarm_value(TapTimerGroup, TapTimerIndex, tapCurrentPulseTicks_);
  timer_set_alarm(TapTimerGroup, TapTimerIndex, TIMER_ALARM_EN);
  timer_enable_intr(TapTimerGroup, TapTimerIndex);
  tapTimerActive_ = true;
  return timer_start(TapTimerGroup, TapTimerIndex) == ESP_OK;
}

void DacAudioOutput::releaseTapBlock() {
  if (tapBlockData_ == nullptr) {
    tapBlockLength_ = 0;
    return;
  }

  uint8_t* block = nullptr;
  portENTER_CRITICAL(&TapTimerMux);
  block = tapBlockData_;
  tapBlockData_ = nullptr;
  tapBlockLength_ = 0;
  portEXIT_CRITICAL(&TapTimerMux);
  free(block);
}

void DacAudioOutput::finishTapPlayback(const char* message) {
  shutdownTapTimer();
  releaseTapBlock();
  if (tapFile_) {
    tapFile_.close();
  }
  tapPlaying_ = false;
  tapPauseActive_ = false;
  tapPulseStage_ = TapPulseStage::Idle;
  playbackKind_ = PlaybackKind::None;
  writeIdleLevel();
  Serial.print(message);
  Serial.print(": blocks=");
  Serial.println(tapBlockIndex_);
}

bool IRAM_ATTR DacAudioOutput::tapTimerCallback(void* parameter) {
  return static_cast<DacAudioOutput*>(parameter)->handleTapTimerInterrupt();
}

bool IRAM_ATTR DacAudioOutput::handleTapTimerInterrupt() {
  const uint64_t edgeTicks =
      timer_group_get_counter_value_in_isr(TapTimerGroup, TapTimerIndex);
  portENTER_CRITICAL_ISR(&TapTimerMux);

  if (tapStopRequested_ || !tapTimerActive_ || tapBlockData_ == nullptr) {
    timer_group_set_counter_enable_in_isr(TapTimerGroup, TapTimerIndex, TIMER_PAUSE);
    tapTimerActive_ = false;
    REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, 128);
    portEXIT_CRITICAL_ISR(&TapTimerMux);
    return false;
  }

  if (edgeTicks > tapNextAlarmTicks_) {
    const uint64_t lateTicks = edgeTicks - tapNextAlarmTicks_;
    if (lateTicks > 10) {
      ++tapLateInterruptCount_;
      if (lateTicks > tapMaxLatenessTicks_) {
        tapMaxLatenessTicks_ =
            lateTicks > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(lateTicks);
      }
    }
  }

  tapElapsedTenthsUs_ += tapCurrentPulseTicks_;
  tapLevelHigh_ = !tapLevelHigh_;
  writeTapDacLevel(tapLevelHigh_);

  const uint16_t nextTicks = nextTapPulseTicks();
  if (nextTicks == 0) {
    timer_group_set_counter_enable_in_isr(TapTimerGroup, TapTimerIndex, TIMER_PAUSE);
    tapTimerActive_ = false;
    tapBlockFinished_ = true;
    REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, 128);
    portEXIT_CRITICAL_ISR(&TapTimerMux);
    return false;
  }

  tapCurrentPulseTicks_ = nextTicks;
  tapNextAlarmTicks_ += nextTicks;
  if (tapNextAlarmTicks_ <= edgeTicks) {
    // Recover from a genuinely missed deadline without creating an interrupt storm.
    tapNextAlarmTicks_ = edgeTicks + nextTicks;
  }
  timer_group_set_alarm_value_in_isr(TapTimerGroup, TapTimerIndex, tapNextAlarmTicks_);
  timer_group_enable_alarm_in_isr(TapTimerGroup, TapTimerIndex);
  portEXIT_CRITICAL_ISR(&TapTimerMux);
  return false;
}

uint16_t IRAM_ATTR DacAudioOutput::nextTapPulseTicks() {
  if (tapPulseStage_ == TapPulseStage::Pilot) {
    if (tapPilotPulsesRemaining_ > 0) {
      --tapPilotPulsesRemaining_;
      if (tapPilotPulsesRemaining_ == 0) {
        tapPulseStage_ = TapPulseStage::SyncOne;
      }
      return tapPilotTicks_;
    }
    tapPulseStage_ = TapPulseStage::SyncOne;
    return tapSyncOneTicks_;
  }

  if (tapPulseStage_ == TapPulseStage::SyncOne) {
    tapPulseStage_ = TapPulseStage::SyncTwo;
    return tapSyncOneTicks_;
  }

  if (tapPulseStage_ == TapPulseStage::SyncTwo) {
    tapPulseStage_ = TapPulseStage::Data;
    return tapSyncTwoTicks_;
  }

  if (tapPulseStage_ == TapPulseStage::Data) {
    if (tapByteIndex_ >= tapBlockLength_) {
      tapPulseStage_ = TapPulseStage::Done;
      return 0;
    }

    const uint16_t ticks =
        (tapBlockData_[tapByteIndex_] & tapBitMask_) != 0 ? tapOneTicks_ : tapZeroTicks_;
    ++tapHalfPulse_;
    if (tapHalfPulse_ >= 2) {
      tapHalfPulse_ = 0;
      tapBitMask_ >>= 1;
      if (tapBitMask_ == 0) {
        tapBitMask_ = 0x80;
        ++tapByteIndex_;
      }
    }
    return ticks;
  }

  return 0;
}

void IRAM_ATTR DacAudioOutput::writeTapDacLevel(bool high) {
  const bool physicalHigh = tapInverted_ ? !high : high;
  const uint8_t value =
      physicalHigh ? static_cast<uint8_t>(128U + tapAmplitude_)
                   : static_cast<uint8_t>(128U - tapAmplitude_);
  REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, value);
}

uint16_t DacAudioOutput::adjustedTapTicks(uint16_t ticks) const {
  const uint32_t adjusted =
      (static_cast<uint32_t>(ticks) * tapTimingPermille_ + 500UL) / 1000UL;
  return static_cast<uint16_t>(constrain(adjusted, 1UL, 32767UL));
}

uint32_t DacAudioOutput::estimateTapDurationMs(File& file) {
  uint64_t totalTicks = 0;
  uint16_t blockLength = 0;

  file.seek(0);
  while (file.available()) {
    if (!readU16(file, blockLength)) {
      return 0;
    }
    if (blockLength == 0) {
      continue;
    }

    const int firstByte = file.read();
    if (firstByte < 0) {
      return 0;
    }

    uint64_t blockTicks =
        static_cast<uint64_t>((firstByte < 0x80) ? ZxHeaderPilotPulses : ZxDataPilotPulses) *
        adjustedTapTicks(ZxPilotPulseTicks);
    blockTicks += adjustedTapTicks(ZxSyncOnePulseTicks) + adjustedTapTicks(ZxSyncTwoPulseTicks);

    auto addByteDuration = [&](uint8_t value) {
      for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        blockTicks +=
            2ULL * ((value & mask) ? adjustedTapTicks(ZxOnePulseTicks)
                                   : adjustedTapTicks(ZxZeroPulseTicks));
      }
    };

    addByteDuration(static_cast<uint8_t>(firstByte));
    for (uint16_t i = 1; i < blockLength; ++i) {
      const int value = file.read();
      if (value < 0) {
        return 0;
      }
      addByteDuration(static_cast<uint8_t>(value));
    }

    totalTicks += blockTicks + (static_cast<uint64_t>(tapPauseMs_) * 10000ULL);
  }

  return static_cast<uint32_t>(totalTicks / 10000ULL);
}

bool DacAudioOutput::beginNextCasUnit() {
  uint32_t headerHalfPulses = 0;
  if (detectCasHeader(headerHalfPulses)) {
    casHeaderHalfPulsesRemaining_ = headerHalfPulses;
    casStage_ = CasStage::Header;
    Serial.print("CAS header cycles: ");
    Serial.println(headerHalfPulses / 2);
    return true;
  }

  const int value = casFile_.read();
  if (value < 0) {
    return false;
  }

  beginCasByte(static_cast<uint8_t>(value));
  return true;
}

bool DacAudioOutput::detectCasHeader(uint32_t& halfPulses) {
  const uint32_t position = casFile_.position();
  if ((position % CasHeaderMarkerSize) != 0) {
    return false;
  }

  uint8_t marker[CasHeaderMarkerSize] = {};
  if (casFile_.read(marker, sizeof(marker)) != sizeof(marker)) {
    casFile_.seek(position);
    return false;
  }

  if (memcmp(marker, CasHeaderMarker, sizeof(marker)) != 0) {
    casFile_.seek(position);
    return false;
  }

  const bool longHeader = isCasLongHeader(casFile_, position);
  halfPulses = static_cast<uint32_t>(longHeader ? CasLongHeaderCycles : CasShortHeaderCycles) * 2UL;
  casFile_.seek(position + CasHeaderMarkerSize);
  return true;
}

bool DacAudioOutput::isCasLongHeader(File& file, uint32_t markerPosition) {
  const uint32_t dataPosition = markerPosition + CasHeaderMarkerSize;
  if (!file.seek(dataPosition)) {
    return false;
  }

  uint8_t preview[CasTypeRunSize] = {};
  if (file.read(preview, sizeof(preview)) != sizeof(preview)) {
    file.seek(dataPosition);
    return false;
  }

  const uint8_t typeByte = preview[0];
  if (typeByte != 0xD0 && typeByte != 0xD3 && typeByte != 0xEA) {
    file.seek(dataPosition);
    return false;
  }

  for (uint8_t i = 1; i < sizeof(preview); ++i) {
    if (preview[i] != typeByte) {
      file.seek(dataPosition);
      return false;
    }
  }

  file.seek(dataPosition);
  return true;
}

void DacAudioOutput::beginCasByte(uint8_t value) {
  casFrame_ = static_cast<uint16_t>((0x03U << 9) | (static_cast<uint16_t>(value) << 1));
  casFrameBitsRemaining_ = CasDataBitsPerByte;
  casBitHalfPulsesRemaining_ = 0;
  casStage_ = CasStage::Data;
}

bool DacAudioOutput::beginNextCasBit() {
  if (casFrameBitsRemaining_ == 0) {
    return false;
  }

  const bool bit = (casFrame_ & 0x01U) != 0;
  casFrame_ >>= 1;
  --casFrameBitsRemaining_;
  casBitHalfPulsesRemaining_ = bit ? 4 : 2;
  casBitHalfPulseUs_ = bit ? CasOneHalfPulseUs : CasZeroHalfPulseUs;
  return true;
}

void DacAudioOutput::emitCasPulse(uint32_t durationUs) {
  casLevelHigh_ = !casLevelHigh_;
  writeTapeLevel(casLevelHigh_);
  casElapsedUs_ += durationUs;
  casNextPulseAtUs_ += durationUs;
}

uint32_t DacAudioOutput::estimateCasDurationMs(File& file) {
  uint64_t totalUs = 0;
  file.seek(0);

  while (file.available()) {
    const uint32_t position = file.position();
    bool markerFound = false;

    if ((position % CasHeaderMarkerSize) == 0) {
      uint8_t marker[CasHeaderMarkerSize] = {};
      if (file.read(marker, sizeof(marker)) != sizeof(marker)) {
        return 0;
      }

      markerFound = memcmp(marker, CasHeaderMarker, sizeof(marker)) == 0;
      if (markerFound) {
        const bool longHeader = isCasLongHeader(file, position);
        const uint16_t cycles = longHeader ? CasLongHeaderCycles : CasShortHeaderCycles;
        totalUs += static_cast<uint64_t>(cycles) * 2ULL * CasHeaderHalfPulseUs;
        file.seek(position + CasHeaderMarkerSize);
        continue;
      }

      file.seek(position);
    }

    const int value = file.read();
    if (value < 0) {
      return 0;
    }
    totalUs += casByteDurationUs(static_cast<uint8_t>(value));
  }

  return static_cast<uint32_t>(totalUs / 1000ULL);
}

uint32_t DacAudioOutput::casByteDurationUs(uint8_t value) const {
  uint32_t duration = 2UL * CasZeroHalfPulseUs;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    duration += (value & (1U << bit)) ? (4UL * CasOneHalfPulseUs) : (2UL * CasZeroHalfPulseUs);
  }
  duration += 8UL * CasOneHalfPulseUs;
  return duration;
}

bool DacAudioOutput::readWavInfo(File& file, WavInfo& info) {
  char id[4] = {};
  uint32_t size = 0;

  if (!readFourCc(file, id) || memcmp(id, "RIFF", 4) != 0) {
    return false;
  }

  if (!readU32(file, size)) {
    return false;
  }

  if (!readFourCc(file, id) || memcmp(id, "WAVE", 4) != 0) {
    return false;
  }

  bool foundFmt = false;
  bool foundData = false;

  while (file.available() && (!foundFmt || !foundData)) {
    if (!readFourCc(file, id) || !readU32(file, size)) {
      return false;
    }

    const uint32_t payloadStart = file.position();

    if (memcmp(id, "fmt ", 4) == 0) {
      uint32_t byteRate = 0;
      uint16_t blockAlign = 0;
      if (!readU16(file, info.audioFormat) || !readU16(file, info.channels) || !readU32(file, info.sampleRate) ||
          !readU32(file, byteRate) || !readU16(file, blockAlign) || !readU16(file, info.bitsPerSample)) {
        return false;
      }
      foundFmt = true;
    } else if (memcmp(id, "data", 4) == 0) {
      info.dataStart = payloadStart;
      info.dataSize = size;
      foundData = true;
    }

    if (foundFmt && foundData) {
      break;
    }

    const uint32_t nextChunk = payloadStart + size + (size & 1);
    if (!file.seek(nextChunk)) {
      return false;
    }
  }

  return foundFmt && foundData;
}

bool DacAudioOutput::readFourCc(File& file, char id[4]) {
  return file.read(reinterpret_cast<uint8_t*>(id), 4) == 4;
}

bool DacAudioOutput::readU16(File& file, uint16_t& value) {
  uint8_t bytes[2] = {};
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return false;
  }
  value = static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
  return true;
}

bool DacAudioOutput::readU32(File& file, uint32_t& value) {
  uint8_t bytes[4] = {};
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return false;
  }
  value = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
          (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
  return true;
}

uint8_t DacAudioOutput::scaleSample(uint8_t sample) const {
  const int16_t centered = static_cast<int16_t>(sample) - 128;
  const int16_t scaled = 128 + ((centered * volume_) / 255);
  if (scaled < 0) return 0;
  if (scaled > 255) return 255;
  return static_cast<uint8_t>(scaled);
}

void DacAudioOutput::writeTapeLevel(bool high) {
  dacWrite(config::pins::SpeakerPwm, scaleSample(high ? 255 : 0));
}

void DacAudioOutput::writeIdleLevel() {
  dacWrite(config::pins::SpeakerPwm, 128);
}

}  // namespace audio
