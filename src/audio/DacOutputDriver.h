#pragma once

#include <Arduino.h>

namespace audio {

class DacOutputDriver {
 public:
  bool begin();
  void setVolume(uint8_t volume);
  uint8_t volume() const;
  bool playTestTone(uint16_t frequencyHz, uint32_t durationMs);
  void writeSample(uint8_t sample);
  void writeLevel(bool high);
  void writeIdle();

 private:
  uint8_t scaleSample(uint8_t sample) const;

  uint8_t volume_ = 180;
};

}  // namespace audio
