#pragma once

#include <Arduino.h>

namespace audio {

class DacOutputDriver {
 public:
  bool begin();
  void setVolume(uint8_t volume);
  uint8_t volume() const;
  bool playTestTone(uint16_t frequencyHz, uint32_t durationMs);
  void updateTestTone();
  void stopTestTone();
  bool isTestTonePlaying() const;
  uint32_t testToneElapsedMs() const;
  uint32_t testToneDurationMs() const;
  void writeSample(uint8_t sample);
  void writeLevel(bool high);
  void writeIdle();

 private:
  uint8_t scaleSample(uint8_t sample) const;

  uint8_t volume_ = 180;
  bool testTonePlaying_ = false;
  bool testToneHigh_ = false;
  uint32_t testToneStartedAtMs_ = 0;
  uint32_t testToneDurationMs_ = 0;
  uint32_t testToneNextEdgeUs_ = 0;
  uint32_t testToneHalfPeriodUs_ = 0;
};

}  // namespace audio
