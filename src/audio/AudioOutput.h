#pragma once

#include <cstdint>

namespace audio {

class AudioOutput {
 public:
  virtual ~AudioOutput() = default;

  virtual bool begin() = 0;
  virtual void update() = 0;
  virtual void stop() = 0;
  virtual void setVolume(uint8_t volume) = 0;
  virtual bool playTestTone(uint16_t frequencyHz, uint32_t durationMs) = 0;
  virtual bool playWavFile(const char* path) = 0;
  virtual bool playTapFile(const char* path) = 0;
  virtual bool playCasFile(const char* path) = 0;
  virtual bool isPlaying() const = 0;
  virtual uint32_t playbackElapsedMs() const = 0;
  virtual uint32_t playbackDurationMs() const = 0;
  virtual void setTapTimingPermille(uint16_t permille) = 0;
  virtual void setTapInverted(bool inverted) = 0;
  virtual void setTapAmplitude(uint8_t amplitude) = 0;
};

}  // namespace audio
