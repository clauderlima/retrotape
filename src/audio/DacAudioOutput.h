#pragma once

#include <Arduino.h>
#include <FS.h>

#include "audio/AudioOutput.h"

namespace audio {

class DacAudioOutput : public AudioOutput {
 public:
  bool begin() override;
  void update() override;
  void stop() override;
  void setVolume(uint8_t volume) override;
  bool playTestTone(uint16_t frequencyHz, uint32_t durationMs) override;
  bool playWavFile(const char* path) override;
  bool playTapFile(const char* path) override;
  bool playCasFile(const char* path) override;
  bool isPlaying() const override;
  uint32_t playbackElapsedMs() const override;
  uint32_t playbackDurationMs() const override;
  void setTapTimingPermille(uint16_t permille) override;
  void setTapInverted(bool inverted) override;
  void setTapAmplitude(uint8_t amplitude) override;
  void setTapPauseMs(uint16_t pauseMs);
  void setTapDebugEnabled(bool enabled);

 private:
  enum class PlaybackKind : uint8_t {
    None,
    Wav,
    Tap,
    Cas,
  };

  enum class CasStage : uint8_t {
    Idle,
    LoadNext,
    Header,
    Data,
  };

  enum class TapPulseStage : uint8_t {
    Idle,
    Pilot,
    SyncOne,
    SyncTwo,
    Data,
    Done,
  };

  struct WavInfo {
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataStart = 0;
    uint32_t dataSize = 0;
  };

  void finishPlayback(const char* message);
  bool readAndWriteFrame();
  void updateWav();
  void updateTap();
  void updateCas();
  bool initializeTapTimer();
  void shutdownTapTimer();
  bool loadNextTapBlock();
  bool startLoadedTapBlock();
  void releaseTapBlock();
  void finishTapPlayback(const char* message);
  static bool IRAM_ATTR tapTimerCallback(void* parameter);
  bool IRAM_ATTR handleTapTimerInterrupt();
  uint16_t IRAM_ATTR nextTapPulseTicks();
  void IRAM_ATTR writeTapDacLevel(bool high);
  uint16_t adjustedTapTicks(uint16_t ticks) const;
  uint32_t estimateTapDurationMs(File& file);
  bool beginNextCasUnit();
  bool detectCasHeader(uint32_t& halfPulses);
  bool isCasLongHeader(File& file, uint32_t markerPosition);
  void beginCasByte(uint8_t value);
  bool beginNextCasBit();
  void emitCasPulse(uint32_t durationUs);
  uint32_t estimateCasDurationMs(File& file);
  uint32_t casByteDurationUs(uint8_t value) const;
  bool readWavInfo(File& file, WavInfo& info);
  bool readFourCc(File& file, char id[4]);
  bool readU16(File& file, uint16_t& value);
  bool readU32(File& file, uint32_t& value);
  uint8_t scaleSample(uint8_t sample) const;
  void writeTapeLevel(bool high);
  void writeIdleLevel();

  static constexpr uint16_t ZxPilotPulseTicks = 6194;
  static constexpr uint16_t ZxSyncOnePulseTicks = 1906;
  static constexpr uint16_t ZxSyncTwoPulseTicks = 2100;
  static constexpr uint16_t ZxZeroPulseTicks = 2443;
  static constexpr uint16_t ZxOnePulseTicks = 4886;
  static constexpr uint32_t ZxPauseMs = 1000;
  static constexpr uint16_t ZxHeaderPilotPulses = 8063;
  static constexpr uint16_t ZxDataPilotPulses = 3223;
  static constexpr uint8_t CasHeaderMarkerSize = 8;
  static constexpr uint8_t CasTypeRunSize = 10;
  static constexpr uint32_t CasHeaderHalfPulseUs = 208;
  static constexpr uint32_t CasZeroHalfPulseUs = 417;
  static constexpr uint32_t CasOneHalfPulseUs = 208;
  static constexpr uint16_t CasShortHeaderCycles = 4000;
  static constexpr uint16_t CasLongHeaderCycles = 16000;
  static constexpr uint8_t CasDataBitsPerByte = 11;
  static constexpr uint8_t CasHeaderMarker[CasHeaderMarkerSize] = {0x1F, 0xA6, 0xDE, 0xBA,
                                                                   0xCC, 0x13, 0x7D, 0x74};

  PlaybackKind playbackKind_ = PlaybackKind::None;
  File wavFile_;
  WavInfo wavInfo_;
  bool wavPlaying_ = false;
  uint16_t frameBytes_ = 0;
  uint32_t totalFrames_ = 0;
  uint32_t framesRemaining_ = 0;
  uint32_t framesPlayed_ = 0;
  uint32_t samplePeriodUs_ = 0;
  uint32_t nextSampleAtUs_ = 0;

  File tapFile_;
  uint8_t* tapBlockData_ = nullptr;
  uint16_t tapBlockLength_ = 0;
  uint16_t tapBlockIndex_ = 0;
  volatile bool tapPlaying_ = false;
  volatile bool tapStopRequested_ = false;
  volatile bool tapBlockFinished_ = false;
  volatile bool tapTimerActive_ = false;
  volatile uint64_t tapElapsedTenthsUs_ = 0;
  volatile TapPulseStage tapPulseStage_ = TapPulseStage::Idle;
  volatile uint16_t tapPilotPulsesRemaining_ = 0;
  volatile uint16_t tapByteIndex_ = 0;
  volatile uint8_t tapBitMask_ = 0x80;
  volatile uint8_t tapHalfPulse_ = 0;
  volatile bool tapLevelHigh_ = false;
  volatile uint16_t tapCurrentPulseTicks_ = 0;
  volatile uint64_t tapNextAlarmTicks_ = 0;
  volatile uint32_t tapLateInterruptCount_ = 0;
  volatile uint32_t tapMaxLatenessTicks_ = 0;
  uint32_t tapTotalDurationMs_ = 0;
  uint32_t tapPauseUntilMs_ = 0;
  bool tapPauseActive_ = false;
  bool tapTimerReady_ = false;
  uint16_t tapPilotTicks_ = ZxPilotPulseTicks;
  uint16_t tapSyncOneTicks_ = ZxSyncOnePulseTicks;
  uint16_t tapSyncTwoTicks_ = ZxSyncTwoPulseTicks;
  uint16_t tapZeroTicks_ = ZxZeroPulseTicks;
  uint16_t tapOneTicks_ = ZxOnePulseTicks;
  uint16_t tapTimingPermille_ = 1000;
  uint16_t tapPauseMs_ = ZxPauseMs;
  uint8_t tapAmplitude_ = 40;
  bool tapInverted_ = false;
  bool tapDebugEnabled_ = true;

  File casFile_;
  bool casPlaying_ = false;
  CasStage casStage_ = CasStage::Idle;
  uint32_t casHeaderHalfPulsesRemaining_ = 0;
  uint16_t casFrame_ = 0;
  uint8_t casFrameBitsRemaining_ = 0;
  uint8_t casBitHalfPulsesRemaining_ = 0;
  uint32_t casBitHalfPulseUs_ = 0;
  uint32_t casNextPulseAtUs_ = 0;
  uint32_t casElapsedUs_ = 0;
  uint32_t casTotalDurationMs_ = 0;
  bool casLevelHigh_ = false;

  uint8_t volume_ = 180;
};

}  // namespace audio
