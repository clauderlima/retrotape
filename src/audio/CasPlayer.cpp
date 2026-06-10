#include "audio/CasPlayer.h"

#include <SD.h>
#include <cstring>

namespace audio {

constexpr uint8_t CasPlayer::HeaderMarker[CasPlayer::HeaderMarkerSize];

CasPlayer::CasPlayer(DacOutputDriver& output) : output_(output) {}

bool CasPlayer::play(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  stop();
  file_ = SD.open(path, FILE_READ);
  if (!file_) {
    Serial.print("Unable to open CAS: ");
    Serial.println(path);
    return false;
  }

  totalDurationMs_ = estimateDurationMs(file_);
  if (totalDurationMs_ == 0) {
    Serial.println("Invalid or empty CAS");
    file_.close();
    return false;
  }

  file_.seek(0);
  stage_ = Stage::LoadNext;
  playing_ = true;
  levelHigh_ = false;
  elapsedUs_ = 0;
  nextPulseAtUs_ = micros();
  output_.writeLevel(false);

  Serial.print("Playing CAS: ");
  Serial.println(path);
  Serial.print("Estimated duration ms: ");
  Serial.println(totalDurationMs_);
  return true;
}

void CasPlayer::update() {
  if (!playing_) {
    return;
  }

  const uint32_t batchStartUs = micros();
  while (playing_ && static_cast<uint32_t>(micros() - batchStartUs) < 3500UL) {
    if (stage_ == Stage::LoadNext && !beginNextUnit()) {
      finish("CAS playback finished");
      return;
    }

    uint32_t nowUs = micros();
    if (static_cast<int32_t>(nowUs - nextPulseAtUs_) > 2000) {
      nextPulseAtUs_ = nowUs;
    }

    while (static_cast<int32_t>(nowUs - nextPulseAtUs_) < 0) {
      yield();
      nowUs = micros();
    }

    switch (stage_) {
      case Stage::Header:
        emitPulse(HeaderHalfPulseUs);
        if (--headerHalfPulsesRemaining_ == 0) {
          stage_ = Stage::LoadNext;
        }
        break;
      case Stage::Data:
        if (bitHalfPulsesRemaining_ == 0 && !beginNextBit()) {
          stage_ = Stage::LoadNext;
          break;
        }
        emitPulse(bitHalfPulseUs_);
        --bitHalfPulsesRemaining_;
        break;
      case Stage::Idle:
      case Stage::LoadNext:
      default:
        break;
    }
  }
}

void CasPlayer::stop() {
  if (file_) {
    file_.close();
  }
  playing_ = false;
  stage_ = Stage::Idle;
  output_.writeIdle();
}

bool CasPlayer::isPlaying() const {
  return playing_;
}

uint32_t CasPlayer::elapsedMs() const {
  return elapsedUs_ / 1000;
}

uint32_t CasPlayer::durationMs() const {
  return totalDurationMs_;
}

bool CasPlayer::beginNextUnit() {
  uint32_t headerHalfPulses = 0;
  if (detectHeader(headerHalfPulses)) {
    headerHalfPulsesRemaining_ = headerHalfPulses;
    stage_ = Stage::Header;
    Serial.print("CAS header cycles: ");
    Serial.println(headerHalfPulses / 2);
    return true;
  }

  const int value = file_.read();
  if (value < 0) {
    return false;
  }

  beginByte(static_cast<uint8_t>(value));
  return true;
}

bool CasPlayer::detectHeader(uint32_t& halfPulses) {
  const uint32_t position = file_.position();
  if ((position % HeaderMarkerSize) != 0) {
    return false;
  }

  uint8_t marker[HeaderMarkerSize] = {};
  if (file_.read(marker, sizeof(marker)) != sizeof(marker)) {
    file_.seek(position);
    return false;
  }

  if (memcmp(marker, HeaderMarker, sizeof(marker)) != 0) {
    file_.seek(position);
    return false;
  }

  const bool longHeader = isLongHeader(file_, position);
  halfPulses = static_cast<uint32_t>(longHeader ? LongHeaderCycles : ShortHeaderCycles) * 2UL;
  file_.seek(position + HeaderMarkerSize);
  return true;
}

bool CasPlayer::isLongHeader(File& file, uint32_t markerPosition) {
  const uint32_t dataPosition = markerPosition + HeaderMarkerSize;
  if (!file.seek(dataPosition)) {
    return false;
  }

  uint8_t preview[TypeRunSize] = {};
  if (file.read(preview, sizeof(preview)) != sizeof(preview)) {
    file.seek(dataPosition);
    return false;
  }

  const uint8_t typeByte = preview[0];
  if (typeByte != 0xD0 && typeByte != 0xD3 && typeByte != 0xEA) {
    file.seek(dataPosition);
    return false;
  }

  for (uint8_t i = 1; i < sizeof(preview); ++i) {
    if (preview[i] != typeByte) {
      file.seek(dataPosition);
      return false;
    }
  }

  file.seek(dataPosition);
  return true;
}

void CasPlayer::beginByte(uint8_t value) {
  frame_ = static_cast<uint16_t>((0x03U << 9) | (static_cast<uint16_t>(value) << 1));
  frameBitsRemaining_ = DataBitsPerByte;
  bitHalfPulsesRemaining_ = 0;
  stage_ = Stage::Data;
}

bool CasPlayer::beginNextBit() {
  if (frameBitsRemaining_ == 0) {
    return false;
  }

  const bool bit = (frame_ & 0x01U) != 0;
  frame_ >>= 1;
  --frameBitsRemaining_;
  bitHalfPulsesRemaining_ = bit ? 4 : 2;
  bitHalfPulseUs_ = bit ? OneHalfPulseUs : ZeroHalfPulseUs;
  return true;
}

void CasPlayer::emitPulse(uint32_t durationUs) {
  levelHigh_ = !levelHigh_;
  output_.writeLevel(levelHigh_);
  elapsedUs_ += durationUs;
  nextPulseAtUs_ += durationUs;
}

uint32_t CasPlayer::estimateDurationMs(File& file) {
  uint64_t totalUs = 0;
  file.seek(0);

  while (file.available()) {
    const uint32_t position = file.position();
    if ((position % HeaderMarkerSize) == 0) {
      uint8_t marker[HeaderMarkerSize] = {};
      if (file.read(marker, sizeof(marker)) != sizeof(marker)) {
        return 0;
      }

      if (memcmp(marker, HeaderMarker, sizeof(marker)) == 0) {
        const bool longHeader = isLongHeader(file, position);
        const uint16_t cycles = longHeader ? LongHeaderCycles : ShortHeaderCycles;
        totalUs += static_cast<uint64_t>(cycles) * 2ULL * HeaderHalfPulseUs;
        file.seek(position + HeaderMarkerSize);
        continue;
      }
      file.seek(position);
    }

    const int value = file.read();
    if (value < 0) {
      return 0;
    }
    totalUs += byteDurationUs(static_cast<uint8_t>(value));
  }

  return static_cast<uint32_t>(totalUs / 1000ULL);
}

uint32_t CasPlayer::byteDurationUs(uint8_t value) const {
  uint32_t duration = 2UL * ZeroHalfPulseUs;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    duration += (value & (1U << bit)) ? (4UL * OneHalfPulseUs) : (2UL * ZeroHalfPulseUs);
  }
  duration += 8UL * OneHalfPulseUs;
  return duration;
}

void CasPlayer::finish(const char* message) {
  stop();
  Serial.println(message);
}

}  // namespace audio
