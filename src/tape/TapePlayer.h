#pragma once

#include <Arduino.h>

#include "audio/AudioOutput.h"
#include "tape/TapeTypes.h"

namespace tape {

class TapePlayer {
 public:
  explicit TapePlayer(audio::AudioOutput& output);

  bool load(const String& path);
  void play();
  void stop();
  PlayerStatus status() const;

 private:
  audio::AudioOutput& output_;
  String path_;
  TapeFormat format_ = TapeFormat::Unknown;
  PlayerStatus status_ = PlayerStatus::Idle;
};

}  // namespace tape

