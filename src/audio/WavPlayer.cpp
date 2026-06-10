#include "audio/WavPlayer.h"

#include <SD.h>
#include <cstring>

namespace audio {

WavPlayer::WavPlayer(DacOutputDriver& output) : output_(output) {}

bool WavPlayer::play(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();
  file_ = SD.open(path, FILE_READ);
  if (!file_) {
    Serial.print("Unable to open WAV: ");
    Serial.println(path);
    return false;
  }

  info_ = WavInfo{};
  if (!readWavInfo(file_, info_)) {
    Serial.println("Invalid or unsupported WAV");
    file_.close();
    output_.writeIdle();
    return false;
  }

  if (info_.audioFormat != 1 || (info_.bitsPerSample != 8 && info_.bitsPerSample != 16) ||
      (info_.channels != 1 && info_.channels != 2) || info_.sampleRate == 0) {
    Serial.println("Unsupported WAV format");
    file_.close();
    output_.writeIdle();
    return false;
  }

  Serial.print("Playing WAV: ");
  Serial.println(path);
  Serial.print("Sample rate: ");
  Serial.println(info_.sampleRate);
  Serial.print("Channels: ");
  Serial.println(info_.channels);
  Serial.print("Bits: ");
  Serial.println(info_.bitsPerSample);

  frameBytes_ = (info_.bitsPerSample / 8) * info_.channels;
  totalFrames_ = info_.dataSize / frameBytes_;
  framesRemaining_ = totalFrames_;
  framesPlayed_ = 0;
  samplePeriodUs_ = 1000000UL / info_.sampleRate;
  if (samplePeriodUs_ == 0) {
    samplePeriodUs_ = 1;
  }
  nextSampleAtUs_ = micros();

  file_.seek(info_.dataStart);
  playing_ = true;
  return true;
}

void WavPlayer::update() {
  if (!playing_) {
    return;
  }

  uint16_t framesThisCall = 0;
  uint32_t now = micros();
  while (playing_ && framesRemaining_ > 0 && static_cast<int32_t>(now - nextSampleAtUs_) >= 0 &&
         framesThisCall < 96) {
    if (!readAndWriteFrame()) {
      finish("WAV read error");
      return;
    }

    --framesRemaining_;
    ++framesPlayed_;
    ++framesThisCall;
    nextSampleAtUs_ += samplePeriodUs_;
    now = micros();
  }

  if (playing_ && framesRemaining_ == 0) {
    finish("WAV playback finished");
  }
}

void WavPlayer::stop() {
  if (file_) {
    file_.close();
  }
  playing_ = false;
  framesRemaining_ = 0;
  output_.writeIdle();
}

bool WavPlayer::isPlaying() const {
  return playing_;
}

uint32_t WavPlayer::elapsedMs() const {
  if (info_.sampleRate == 0) {
    return 0;
  }
  return static_cast<uint32_t>((static_cast<uint64_t>(framesPlayed_) * 1000ULL) / info_.sampleRate);
}

uint32_t WavPlayer::durationMs() const {
  if (info_.sampleRate == 0) {
    return 0;
  }
  return static_cast<uint32_t>((static_cast<uint64_t>(totalFrames_) * 1000ULL) / info_.sampleRate);
}

void WavPlayer::finish(const char* message) {
  stop();
  Serial.println(message);
}

bool WavPlayer::readAndWriteFrame() {
  uint8_t frame[4] = {};
  if (file_.read(frame, frameBytes_) != frameBytes_) {
    return false;
  }

  int32_t mixed = 0;
  if (info_.bitsPerSample == 8) {
    mixed = frame[0];
    if (info_.channels == 2) {
      mixed = (mixed + frame[1]) / 2;
    }
  } else {
    const int16_t left = static_cast<int16_t>(frame[0] | (frame[1] << 8));
    mixed = left;
    if (info_.channels == 2) {
      const int16_t right = static_cast<int16_t>(frame[2] | (frame[3] << 8));
      mixed = (mixed + right) / 2;
    }
    mixed = (mixed + 32768) >> 8;
  }

  output_.writeSample(static_cast<uint8_t>(mixed));
  return true;
}

bool WavPlayer::readWavInfo(File& file, WavInfo& info) {
  char id[4] = {};
  uint32_t size = 0;

  if (!readFourCc(file, id) || memcmp(id, "RIFF", 4) != 0 || !readU32(file, size) ||
      !readFourCc(file, id) || memcmp(id, "WAVE", 4) != 0) {
    return false;
  }

  bool foundFmt = false;
  bool foundData = false;
  while (file.available() && (!foundFmt || !foundData)) {
    if (!readFourCc(file, id) || !readU32(file, size)) {
      return false;
    }

    const uint32_t payloadStart = file.position();
    if (memcmp(id, "fmt ", 4) == 0) {
      uint32_t byteRate = 0;
      uint16_t blockAlign = 0;
      if (!readU16(file, info.audioFormat) || !readU16(file, info.channels) ||
          !readU32(file, info.sampleRate) || !readU32(file, byteRate) ||
          !readU16(file, blockAlign) || !readU16(file, info.bitsPerSample)) {
        return false;
      }
      foundFmt = true;
    } else if (memcmp(id, "data", 4) == 0) {
      info.dataStart = payloadStart;
      info.dataSize = size;
      foundData = true;
    }

    if (foundFmt && foundData) {
      break;
    }

    const uint32_t nextChunk = payloadStart + size + (size & 1);
    if (!file.seek(nextChunk)) {
      return false;
    }
  }

  return foundFmt && foundData;
}

bool WavPlayer::readFourCc(File& file, char id[4]) {
  return file.read(reinterpret_cast<uint8_t*>(id), 4) == 4;
}

bool WavPlayer::readU16(File& file, uint16_t& value) {
  uint8_t bytes[2] = {};
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return false;
  }
  value = static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
  return true;
}

bool WavPlayer::readU32(File& file, uint32_t& value) {
  uint8_t bytes[4] = {};
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return false;
  }
  value = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
          (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
  return true;
}

}  // namespace audio
