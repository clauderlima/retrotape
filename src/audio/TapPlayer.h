#pragma once

#include <Arduino.h>
#include <FS.h>

namespace audio {

class TapPlayer {
 public:
  bool play(const char* path);
  void update();
  void stop();
  bool isPlaying() const;
  uint32_t elapsedMs() const;
  uint32_t durationMs() const;

  void setTimingPermille(uint16_t permille);
  void setInverted(bool inverted);
  void setAmplitude(uint8_t amplitude);
  void setPauseMs(uint16_t pauseMs);
  void setDebugEnabled(bool enabled);

 private:
  enum class PulseStage : uint8_t {
    Idle,
    Pilot,
    SyncOne,
    SyncTwo,
    Data,
    Done,
  };

  bool initializeTimer();
  void shutdownTimer();
  bool loadNextBlock();
  bool startLoadedBlock();
  void releaseBlock();
  void finish(const char* message);
  static bool IRAM_ATTR timerCallback(void* parameter);
  bool IRAM_ATTR handleTimerInterrupt();
  uint16_t IRAM_ATTR nextPulseTicks();
  void IRAM_ATTR writeDacLevel(bool high);
  uint16_t adjustedTicks(uint16_t ticks) const;
  uint32_t estimateDurationMs(File& file);
  bool readU16(File& file, uint16_t& value);

  static constexpr uint16_t PilotPulseTicks = 6194;
  static constexpr uint16_t SyncOnePulseTicks = 1906;
  static constexpr uint16_t SyncTwoPulseTicks = 2100;
  static constexpr uint16_t ZeroPulseTicks = 2443;
  static constexpr uint16_t OnePulseTicks = 4886;
  static constexpr uint32_t DefaultPauseMs = 1000;
  static constexpr uint16_t HeaderPilotPulses = 8063;
  static constexpr uint16_t DataPilotPulses = 3223;

  File file_;
  uint8_t* blockData_ = nullptr;
  uint16_t blockLength_ = 0;
  uint16_t blockIndex_ = 0;
  volatile bool playing_ = false;
  volatile bool stopRequested_ = false;
  volatile bool blockFinished_ = false;
  volatile bool timerActive_ = false;
  volatile uint64_t elapsedTenthsUs_ = 0;
  volatile PulseStage pulseStage_ = PulseStage::Idle;
  volatile uint16_t pilotPulsesRemaining_ = 0;
  volatile uint16_t byteIndex_ = 0;
  volatile uint8_t bitMask_ = 0x80;
  volatile uint8_t halfPulse_ = 0;
  volatile bool levelHigh_ = false;
  volatile uint16_t currentPulseTicks_ = 0;
  volatile uint64_t nextAlarmTicks_ = 0;
  volatile uint32_t lateInterruptCount_ = 0;
  volatile uint32_t maxLatenessTicks_ = 0;
  uint32_t totalDurationMs_ = 0;
  uint32_t pauseUntilMs_ = 0;
  bool pauseActive_ = false;
  bool timerReady_ = false;
  uint16_t pilotTicks_ = PilotPulseTicks;
  uint16_t syncOneTicks_ = SyncOnePulseTicks;
  uint16_t syncTwoTicks_ = SyncTwoPulseTicks;
  uint16_t zeroTicks_ = ZeroPulseTicks;
  uint16_t oneTicks_ = OnePulseTicks;
  uint16_t timingPermille_ = 1000;
  uint16_t pauseMs_ = DefaultPauseMs;
  uint8_t amplitude_ = 40;
  bool inverted_ = false;
  bool debugEnabled_ = true;
};

}  // namespace audio
