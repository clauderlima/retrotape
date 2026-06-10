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

  stopTestTone();
  testToneHalfPeriodUs_ = 500000UL / frequencyHz;
  if (testToneHalfPeriodUs_ == 0) {
    return false;
  }
  testToneDurationMs_ = durationMs;
  testToneStartedAtMs_ = millis();
  testToneNextEdgeUs_ = micros();
  testToneHigh_ = false;
  testTonePlaying_ = true;

  Serial.print("Audio test tone started: ");
  Serial.print(frequencyHz);
  Serial.print(" Hz for ");
  Serial.print(durationMs);
  Serial.println(" ms");
  return true;
}

void DacOutputDriver::updateTestTone() {
  if (!testTonePlaying_) {
    return;
  }

  if (millis() - testToneStartedAtMs_ >= testToneDurationMs_) {
    stopTestTone();
    return;
  }

  const uint32_t nowUs = micros();
  if (static_cast<int32_t>(nowUs - testToneNextEdgeUs_) < 0) {
    return;
  }

  const uint32_t lateUs = nowUs - testToneNextEdgeUs_;
  const uint32_t elapsedPeriods = (lateUs / testToneHalfPeriodUs_) + 1;
  if ((elapsedPeriods & 1U) != 0) {
    testToneHigh_ = !testToneHigh_;
    writeSample(testToneHigh_ ? 192 : 64);
  }
  testToneNextEdgeUs_ += elapsedPeriods * testToneHalfPeriodUs_;
}

void DacOutputDriver::stopTestTone() {
  if (testTonePlaying_) {
    Serial.println("Audio test tone stopped");
  }
  testTonePlaying_ = false;
  testToneHigh_ = false;
  writeIdle();
}

bool DacOutputDriver::isTestTonePlaying() const {
  return testTonePlaying_;
}

uint32_t DacOutputDriver::testToneElapsedMs() const {
  if (!testTonePlaying_) {
    return 0;
  }
  const uint32_t elapsed = millis() - testToneStartedAtMs_;
  return elapsed > testToneDurationMs_ ? testToneDurationMs_ : elapsed;
}

uint32_t DacOutputDriver::testToneDurationMs() const {
  return testToneDurationMs_;
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
