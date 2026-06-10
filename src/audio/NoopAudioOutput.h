#pragma once

#include <Arduino.h>

#include "audio/AudioOutput.h"

namespace audio {

class NoopAudioOutput : public AudioOutput {
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
};

}  // namespace audio
