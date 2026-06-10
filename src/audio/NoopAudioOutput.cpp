#include "audio/NoopAudioOutput.h"

namespace audio {

bool NoopAudioOutput::begin() {
  Serial.println("Audio output started in noop mode");
  return true;
}

void NoopAudioOutput::update() {}

void NoopAudioOutput::stop() {
  Serial.println("Audio output stopped");
}

void NoopAudioOutput::setVolume(uint8_t volume) {
  Serial.print("Audio volume set to ");
  Serial.println(volume);
}

bool NoopAudioOutput::playTestTone(uint16_t frequencyHz, uint32_t durationMs) {
  Serial.print("Audio test tone requested: ");
  Serial.print(frequencyHz);
  Serial.print(" Hz for ");
  Serial.print(durationMs);
  Serial.println(" ms");
  return true;
}

bool NoopAudioOutput::playWavFile(const char* path) {
  Serial.print("WAV playback requested in noop mode: ");
  Serial.println(path);
  return true;
}

bool NoopAudioOutput::playTapFile(const char* path) {
  Serial.print("TAP playback requested in noop mode: ");
  Serial.println(path);
  return true;
}

bool NoopAudioOutput::playCasFile(const char* path) {
  Serial.print("CAS playback requested in noop mode: ");
  Serial.println(path);
  return true;
}

bool NoopAudioOutput::isPlaying() const {
  return false;
}

uint32_t NoopAudioOutput::playbackElapsedMs() const {
  return 0;
}

uint32_t NoopAudioOutput::playbackDurationMs() const {
  return 0;
}

void NoopAudioOutput::setTapTimingPermille(uint16_t permille) {
  Serial.print("TAP timing requested: ");
  Serial.println(permille);
}

void NoopAudioOutput::setTapInverted(bool inverted) {
  Serial.print("TAP inversion requested: ");
  Serial.println(inverted ? "yes" : "no");
}

void NoopAudioOutput::setTapAmplitude(uint8_t amplitude) {
  Serial.print("TAP amplitude requested: ");
  Serial.println(amplitude);
}

}  // namespace audio
