#include "audio/DacOutputDriver.h"

#include "config/pins.h"

namespace audio {

bool DacOutputDriver::begin() {
  writeIdle();
  Serial.print("DAC audio output ready on GPIO ");
  Serial.println(config::pins::SpeakerPwm);
  return true;
}

void DacOutputDriver::setVolume(uint8_t volume) {
  volume_ = volume;
  Serial.print("Audio volume set to ");
  Serial.println(volume_);
}

uint8_t DacOutputDriver::volume() const {
  return volume_;
}

bool DacOutputDriver::playTestTone(uint16_t frequencyHz, uint32_t durationMs) {
  if (frequencyHz == 0 || durationMs == 0) {
    return false;
  }

  const uint32_t halfPeriodUs = 500000UL / frequencyHz;
  const uint32_t endAt = millis() + durationMs;
  uint8_t level = 64;

  while (static_cast<int32_t>(millis() - endAt) < 0) {
    writeSample(level);
    level = level == 64 ? 192 : 64;
    delayMicroseconds(halfPeriodUs);
  }

  writeIdle();
  return true;
}

void DacOutputDriver::writeSample(uint8_t sample) {
  dacWrite(config::pins::SpeakerPwm, scaleSample(sample));
}

void DacOutputDriver::writeLevel(bool high) {
  writeSample(high ? 255 : 0);
}

void DacOutputDriver::writeIdle() {
  dacWrite(config::pins::SpeakerPwm, 128);
}

uint8_t DacOutputDriver::scaleSample(uint8_t sample) const {
  const int16_t centered = static_cast<int16_t>(sample) - 128;
  const int16_t scaled = 128 + ((centered * volume_) / 255);
  if (scaled < 0) {
    return 0;
  }
  if (scaled > 255) {
    return 255;
  }
  return static_cast<uint8_t>(scaled);
}

}  // namespace audio
