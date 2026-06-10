#include "audio/TapPlayer.h"

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

bool TapPlayer::play(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();
  file_ = SD.open(path, FILE_READ);
  if (!file_) {
    Serial.print("Unable to open TAP: ");
    Serial.println(path);
    return false;
  }

  totalDurationMs_ = estimateDurationMs(file_);
  if (totalDurationMs_ == 0) {
    Serial.println("Invalid or empty TAP");
    file_.close();
    return false;
  }

  file_.seek(0);
  blockIndex_ = 0;
  elapsedTenthsUs_ = 0;
  stopRequested_ = false;
  pauseActive_ = false;
  if (!initializeTimer()) {
    file_.close();
    Serial.println("Unable to initialize TAP pulse timer");
    return false;
  }

  playing_ = true;
  if (!loadNextBlock() || !startLoadedBlock()) {
    finish("TAP playback failed while loading the first block");
    return false;
  }

  Serial.print("Playing TAP: ");
  Serial.println(path);
  Serial.print("Estimated duration ms: ");
  Serial.println(totalDurationMs_);
  return true;
}

void TapPlayer::update() {
  if (!playing_ || stopRequested_) {
    return;
  }

  if (blockFinished_) {
    const uint32_t lateInterrupts = lateInterruptCount_;
    const uint32_t maxLatenessTicks = maxLatenessTicks_;
    releaseBlock();
    blockFinished_ = false;
    pauseActive_ = true;
    pauseUntilMs_ = millis() + pauseMs_;
    elapsedTenthsUs_ += static_cast<uint64_t>(pauseMs_) * 10000ULL;

    if (debugEnabled_) {
      Serial.print("[TAP] Block ");
      Serial.print(blockIndex_);
      Serial.print(" complete, pause=");
      Serial.print(pauseMs_);
      Serial.println(" ms");
      Serial.print("[TAP] Timer late interrupts=");
      Serial.print(lateInterrupts);
      Serial.print(" max lateness=");
      Serial.print(maxLatenessTicks / 10.0F, 1);
      Serial.println(" us");
    }
  }

  if (!pauseActive_ || static_cast<int32_t>(millis() - pauseUntilMs_) < 0) {
    return;
  }

  pauseActive_ = false;
  if (!file_.available()) {
    finish("TAP playback finished");
    return;
  }

  if (!loadNextBlock() || !startLoadedBlock()) {
    finish("TAP playback failed while loading a block");
  }
}

void TapPlayer::stop() {
  stopRequested_ = true;
  shutdownTimer();
  releaseBlock();
  if (file_) {
    file_.close();
  }
  playing_ = false;
  pauseActive_ = false;
  pulseStage_ = PulseStage::Idle;
  dacWrite(config::pins::SpeakerPwm, 128);
}

bool TapPlayer::isPlaying() const {
  return playing_;
}

uint32_t TapPlayer::elapsedMs() const {
  portENTER_CRITICAL(&TapTimerMux);
  const uint64_t elapsedTenthsUs = elapsedTenthsUs_;
  portEXIT_CRITICAL(&TapTimerMux);
  return static_cast<uint32_t>(elapsedTenthsUs / 10000ULL);
}

uint32_t TapPlayer::durationMs() const {
  return totalDurationMs_;
}

void TapPlayer::setTimingPermille(uint16_t permille) {
  timingPermille_ = constrain(permille, 900U, 1100U);
  Serial.print("TAP timing: ");
  Serial.print(timingPermille_ / 10.0F);
  Serial.println("%");
}

void TapPlayer::setInverted(bool inverted) {
  inverted_ = inverted;
  Serial.print("TAP output inverted: ");
  Serial.println(inverted_ ? "yes" : "no");
}

void TapPlayer::setAmplitude(uint8_t amplitude) {
  amplitude_ = constrain(amplitude, 8U, 120U);
  Serial.print("TAP DAC amplitude: ");
  Serial.print((static_cast<uint16_t>(amplitude_) * 100U) / 127U);
  Serial.println("%");
}

void TapPlayer::setPauseMs(uint16_t pauseMs) {
  pauseMs_ = constrain(pauseMs, 250U, 5000U);
  Serial.print("TAP block pause: ");
  Serial.print(pauseMs_);
  Serial.println(" ms");
}

void TapPlayer::setDebugEnabled(bool enabled) {
  debugEnabled_ = enabled;
}

bool TapPlayer::initializeTimer() {
  shutdownTimer();

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
    result = timer_isr_callback_add(TapTimerGroup, TapTimerIndex, timerCallback, this,
                                    ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);
  }

  if (result != ESP_OK) {
    Serial.print("TAP timer initialization failed: ");
    Serial.println(esp_err_to_name(result));
    timer_deinit(TapTimerGroup, TapTimerIndex);
    timerReady_ = false;
    return false;
  }

  timerReady_ = true;
  dacWrite(config::pins::SpeakerPwm, 128);

  Serial.print("TAP timer-driven DAC output ready on GPIO ");
  Serial.print(config::pins::SpeakerPwm);
  Serial.print(" with ");
  Serial.print(TapTimerTicksPerSecond);
  Serial.println(" timer ticks/s");
  return true;
}

void TapPlayer::shutdownTimer() {
  if (!timerReady_) {
    return;
  }

  timer_disable_intr(TapTimerGroup, TapTimerIndex);
  timer_pause(TapTimerGroup, TapTimerIndex);
  timer_isr_callback_remove(TapTimerGroup, TapTimerIndex);
  timer_deinit(TapTimerGroup, TapTimerIndex);
  timerReady_ = false;
  timerActive_ = false;
  dacWrite(config::pins::SpeakerPwm, 128);
}

bool TapPlayer::loadNextBlock() {
  releaseBlock();

  uint16_t blockLength = 0;
  while (file_.available()) {
    if (!readU16(file_, blockLength)) {
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

  if (file_.read(block, blockLength) != blockLength) {
    Serial.print("[TAP] Read error, expected bytes=");
    Serial.print(blockLength);
    Serial.print(" file position=");
    Serial.print(file_.position());
    Serial.print("/");
    Serial.println(file_.size());
    free(block);
    return false;
  }

  blockData_ = block;
  blockLength_ = blockLength;
  ++blockIndex_;
  return true;
}

bool TapPlayer::startLoadedBlock() {
  if (!timerReady_ || blockData_ == nullptr || blockLength_ == 0) {
    return false;
  }

  pilotTicks_ = adjustedTicks(PilotPulseTicks);
  syncOneTicks_ = adjustedTicks(SyncOnePulseTicks);
  syncTwoTicks_ = adjustedTicks(SyncTwoPulseTicks);
  zeroTicks_ = adjustedTicks(ZeroPulseTicks);
  oneTicks_ = adjustedTicks(OnePulseTicks);

  uint8_t checksum = 0;
  for (uint16_t index = 0; index < blockLength_; ++index) {
    checksum ^= blockData_[index];
  }

  pulseStage_ = PulseStage::Pilot;
  pilotPulsesRemaining_ = blockData_[0] < 0x80 ? HeaderPilotPulses : DataPilotPulses;
  byteIndex_ = 0;
  bitMask_ = 0x80;
  halfPulse_ = 0;
  blockFinished_ = false;
  levelHigh_ = true;
  lateInterruptCount_ = 0;
  maxLatenessTicks_ = 0;
  currentPulseTicks_ = nextPulseTicks();
  nextAlarmTicks_ = currentPulseTicks_;

  if (currentPulseTicks_ == 0) {
    return false;
  }

  if (debugEnabled_) {
    Serial.println();
    Serial.print("[TAP] Block ");
    Serial.print(blockIndex_);
    Serial.print(" type=");
    Serial.print(blockData_[0] < 0x80 ? "HEADER" : "DATA");
    Serial.print(" bytes=");
    Serial.print(blockLength_);
    Serial.print(" flag=0x");
    if (blockData_[0] < 0x10) {
      Serial.print('0');
    }
    Serial.print(blockData_[0], HEX);
    Serial.print(" checksum=");
    Serial.println(checksum == 0 ? "OK" : "INVALID");
    Serial.print("[TAP] Pilot=");
    Serial.print(pilotPulsesRemaining_ + 1U);
    Serial.print(" x ");
    Serial.print(pilotTicks_ / 10.0F, 1);
    Serial.print(" us, sync=");
    Serial.print(syncOneTicks_ / 10.0F, 1);
    Serial.print("/");
    Serial.print(syncTwoTicks_ / 10.0F, 1);
    Serial.print(" us, bit0=");
    Serial.print(zeroTicks_ / 10.0F, 1);
    Serial.print(" us x2, bit1=");
    Serial.print(oneTicks_ / 10.0F, 1);
    Serial.println(" us x2");
    Serial.print("[TAP] Output=DAC-TIMER timing=");
    Serial.print(timingPermille_ / 10.0F, 1);
    Serial.print("% level=");
    Serial.print((static_cast<uint16_t>(amplitude_) * 100U) / 127U);
    Serial.print("% inverted=");
    Serial.println(inverted_ ? "yes" : "no");
  }

  writeDacLevel(levelHigh_);
  timer_pause(TapTimerGroup, TapTimerIndex);
  timer_set_counter_value(TapTimerGroup, TapTimerIndex, 0);
  timer_set_alarm_value(TapTimerGroup, TapTimerIndex, currentPulseTicks_);
  timer_set_alarm(TapTimerGroup, TapTimerIndex, TIMER_ALARM_EN);
  timer_enable_intr(TapTimerGroup, TapTimerIndex);
  timerActive_ = true;
  return timer_start(TapTimerGroup, TapTimerIndex) == ESP_OK;
}

void TapPlayer::releaseBlock() {
  if (blockData_ == nullptr) {
    blockLength_ = 0;
    return;
  }

  uint8_t* block = nullptr;
  portENTER_CRITICAL(&TapTimerMux);
  block = blockData_;
  blockData_ = nullptr;
  blockLength_ = 0;
  portEXIT_CRITICAL(&TapTimerMux);
  free(block);
}

void TapPlayer::finish(const char* message) {
  shutdownTimer();
  releaseBlock();
  if (file_) {
    file_.close();
  }
  playing_ = false;
  pauseActive_ = false;
  pulseStage_ = PulseStage::Idle;
  dacWrite(config::pins::SpeakerPwm, 128);
  Serial.print(message);
  Serial.print(": blocks=");
  Serial.println(blockIndex_);
}

bool IRAM_ATTR TapPlayer::timerCallback(void* parameter) {
  return static_cast<TapPlayer*>(parameter)->handleTimerInterrupt();
}

bool IRAM_ATTR TapPlayer::handleTimerInterrupt() {
  const uint64_t edgeTicks =
      timer_group_get_counter_value_in_isr(TapTimerGroup, TapTimerIndex);
  portENTER_CRITICAL_ISR(&TapTimerMux);

  if (stopRequested_ || !timerActive_ || blockData_ == nullptr) {
    timer_group_set_counter_enable_in_isr(TapTimerGroup, TapTimerIndex, TIMER_PAUSE);
    timerActive_ = false;
    REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, 128);
    portEXIT_CRITICAL_ISR(&TapTimerMux);
    return false;
  }

  if (edgeTicks > nextAlarmTicks_) {
    const uint64_t lateTicks = edgeTicks - nextAlarmTicks_;
    if (lateTicks > 10) {
      ++lateInterruptCount_;
      if (lateTicks > maxLatenessTicks_) {
        maxLatenessTicks_ =
            lateTicks > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(lateTicks);
      }
    }
  }

  elapsedTenthsUs_ += currentPulseTicks_;
  levelHigh_ = !levelHigh_;
  writeDacLevel(levelHigh_);

  const uint16_t nextTicks = nextPulseTicks();
  if (nextTicks == 0) {
    timer_group_set_counter_enable_in_isr(TapTimerGroup, TapTimerIndex, TIMER_PAUSE);
    timerActive_ = false;
    blockFinished_ = true;
    REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, 128);
    portEXIT_CRITICAL_ISR(&TapTimerMux);
    return false;
  }

  currentPulseTicks_ = nextTicks;
  nextAlarmTicks_ += nextTicks;
  if (nextAlarmTicks_ <= edgeTicks) {
    // Recover from a genuinely missed deadline without creating an interrupt storm.
    nextAlarmTicks_ = edgeTicks + nextTicks;
  }
  timer_group_set_alarm_value_in_isr(TapTimerGroup, TapTimerIndex, nextAlarmTicks_);
  timer_group_enable_alarm_in_isr(TapTimerGroup, TapTimerIndex);
  portEXIT_CRITICAL_ISR(&TapTimerMux);
  return false;
}

uint16_t IRAM_ATTR TapPlayer::nextPulseTicks() {
  if (pulseStage_ == PulseStage::Pilot) {
    if (pilotPulsesRemaining_ > 0) {
      --pilotPulsesRemaining_;
      if (pilotPulsesRemaining_ == 0) {
        pulseStage_ = PulseStage::SyncOne;
      }
      return pilotTicks_;
    }
    pulseStage_ = PulseStage::SyncOne;
    return syncOneTicks_;
  }

  if (pulseStage_ == PulseStage::SyncOne) {
    pulseStage_ = PulseStage::SyncTwo;
    return syncOneTicks_;
  }

  if (pulseStage_ == PulseStage::SyncTwo) {
    pulseStage_ = PulseStage::Data;
    return syncTwoTicks_;
  }

  if (pulseStage_ == PulseStage::Data) {
    if (byteIndex_ >= blockLength_) {
      pulseStage_ = PulseStage::Done;
      return 0;
    }

    const uint16_t ticks =
        (blockData_[byteIndex_] & bitMask_) != 0 ? oneTicks_ : zeroTicks_;
    ++halfPulse_;
    if (halfPulse_ >= 2) {
      halfPulse_ = 0;
      bitMask_ >>= 1;
      if (bitMask_ == 0) {
        bitMask_ = 0x80;
        ++byteIndex_;
      }
    }
    return ticks;
  }

  return 0;
}

void IRAM_ATTR TapPlayer::writeDacLevel(bool high) {
  const bool physicalHigh = inverted_ ? !high : high;
  const uint8_t value =
      physicalHigh ? static_cast<uint8_t>(128U + amplitude_)
                   : static_cast<uint8_t>(128U - amplitude_);
  REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, value);
}

uint16_t TapPlayer::adjustedTicks(uint16_t ticks) const {
  const uint32_t adjusted =
      (static_cast<uint32_t>(ticks) * timingPermille_ + 500UL) / 1000UL;
  return static_cast<uint16_t>(constrain(adjusted, 1UL, 32767UL));
}

uint32_t TapPlayer::estimateDurationMs(File& file) {
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
        static_cast<uint64_t>((firstByte < 0x80) ? HeaderPilotPulses : DataPilotPulses) *
        adjustedTicks(PilotPulseTicks);
    blockTicks += adjustedTicks(SyncOnePulseTicks) + adjustedTicks(SyncTwoPulseTicks);

    auto addByteDuration = [&](uint8_t value) {
      for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        blockTicks +=
            2ULL * ((value & mask) ? adjustedTicks(OnePulseTicks)
                                   : adjustedTicks(ZeroPulseTicks));
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

    totalTicks += blockTicks + (static_cast<uint64_t>(pauseMs_) * 10000ULL);
  }

  return static_cast<uint32_t>(totalTicks / 10000ULL);
}

bool TapPlayer::readU16(File& file, uint16_t& value) {
  uint8_t bytes[2] = {};
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return false;
  }
  value = static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
  return true;
}

}  // namespace audio
