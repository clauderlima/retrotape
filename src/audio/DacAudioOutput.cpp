#include "audio/DacAudioOutput.h"

namespace audio {

DacAudioOutput::DacAudioOutput()
    : wavPlayer_(output_), casPlayer_(output_) {}

bool DacAudioOutput::begin() {
  return output_.begin();
}

void DacAudioOutput::update() {
  switch (playbackKind_) {
    case PlaybackKind::Wav:
      wavPlayer_.update();
      break;
    case PlaybackKind::Tap:
      tapPlayer_.update();
      break;
    case PlaybackKind::Cas:
      casPlayer_.update();
      break;
    case PlaybackKind::None:
    default:
      return;
  }
  clearFinishedPlayback();
}

void DacAudioOutput::stop() {
  wavPlayer_.stop();
  tapPlayer_.stop();
  casPlayer_.stop();
  playbackKind_ = PlaybackKind::None;
  output_.writeIdle();
  Serial.println("Audio output stopped");
}

void DacAudioOutput::setVolume(uint8_t volume) {
  output_.setVolume(volume);
}

bool DacAudioOutput::playTestTone(uint16_t frequencyHz, uint32_t durationMs) {
  stop();
  return output_.playTestTone(frequencyHz, durationMs);
}

bool DacAudioOutput::playWavFile(const char* path) {
  stop();
  if (!wavPlayer_.play(path)) {
    return false;
  }
  playbackKind_ = PlaybackKind::Wav;
  return true;
}

bool DacAudioOutput::playTapFile(const char* path) {
  stop();
  if (!tapPlayer_.play(path)) {
    return false;
  }
  playbackKind_ = PlaybackKind::Tap;
  return true;
}

bool DacAudioOutput::playCasFile(const char* path) {
  stop();
  if (!casPlayer_.play(path)) {
    return false;
  }
  playbackKind_ = PlaybackKind::Cas;
  return true;
}

bool DacAudioOutput::isPlaying() const {
  switch (playbackKind_) {
    case PlaybackKind::Wav:
      return wavPlayer_.isPlaying();
    case PlaybackKind::Tap:
      return tapPlayer_.isPlaying();
    case PlaybackKind::Cas:
      return casPlayer_.isPlaying();
    case PlaybackKind::None:
    default:
      return false;
  }
}

uint32_t DacAudioOutput::playbackElapsedMs() const {
  switch (playbackKind_) {
    case PlaybackKind::Wav:
      return wavPlayer_.elapsedMs();
    case PlaybackKind::Tap:
      return tapPlayer_.elapsedMs();
    case PlaybackKind::Cas:
      return casPlayer_.elapsedMs();
    case PlaybackKind::None:
    default:
      return 0;
  }
}

uint32_t DacAudioOutput::playbackDurationMs() const {
  switch (playbackKind_) {
    case PlaybackKind::Wav:
      return wavPlayer_.durationMs();
    case PlaybackKind::Tap:
      return tapPlayer_.durationMs();
    case PlaybackKind::Cas:
      return casPlayer_.durationMs();
    case PlaybackKind::None:
    default:
      return 0;
  }
}

void DacAudioOutput::setTapTimingPermille(uint16_t permille) {
  tapPlayer_.setTimingPermille(permille);
}

void DacAudioOutput::setTapInverted(bool inverted) {
  tapPlayer_.setInverted(inverted);
}

void DacAudioOutput::setTapAmplitude(uint8_t amplitude) {
  tapPlayer_.setAmplitude(amplitude);
}

void DacAudioOutput::setTapPauseMs(uint16_t pauseMs) {
  tapPlayer_.setPauseMs(pauseMs);
}

void DacAudioOutput::setTapDebugEnabled(bool enabled) {
  tapPlayer_.setDebugEnabled(enabled);
}

void DacAudioOutput::clearFinishedPlayback() {
  if (!isPlaying()) {
    playbackKind_ = PlaybackKind::None;
  }
}

}  // namespace audio
