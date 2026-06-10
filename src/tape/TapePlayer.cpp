#include "tape/TapePlayer.h"

#include "tape/TapeFormatDetector.h"

namespace tape {

TapePlayer::TapePlayer(audio::AudioOutput& output) : output_(output) {}

bool TapePlayer::load(const String& path) {
  path_ = path;
  format_ = TapeFormatDetector::detectFromPath(path);
  status_ = format_ == TapeFormat::Unknown ? PlayerStatus::Error : PlayerStatus::Ready;
  return status_ == PlayerStatus::Ready;
}

void TapePlayer::play() {
  if (status_ != PlayerStatus::Ready && status_ != PlayerStatus::Paused) {
    status_ = PlayerStatus::Error;
    return;
  }

  status_ = PlayerStatus::Playing;
}

void TapePlayer::stop() {
  output_.stop();
  status_ = PlayerStatus::Stopped;
}

PlayerStatus TapePlayer::status() const {
  return status_;
}

}  // namespace tape

