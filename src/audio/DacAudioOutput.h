#pragma once

#include <Arduino.h>

#include "audio/AudioOutput.h"
#include "audio/CasPlayer.h"
#include "audio/DacOutputDriver.h"
#include "audio/TapPlayer.h"
#include "audio/WavPlayer.h"

namespace audio {

class DacAudioOutput : public AudioOutput {
 public:
  DacAudioOutput();

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
    TestTone,
  };

  void clearFinishedPlayback();

  DacOutputDriver output_;
  WavPlayer wavPlayer_;
  TapPlayer tapPlayer_;
  CasPlayer casPlayer_;
  PlaybackKind playbackKind_ = PlaybackKind::None;
};

}  // namespace audio
