#pragma once

#include <Arduino.h>
#include <FS.h>

#include "audio/DacOutputDriver.h"

namespace audio {

class CasPlayer {
 public:
  explicit CasPlayer(DacOutputDriver& output);

  bool play(const char* path);
  void update();
  void stop();
  bool isPlaying() const;
  uint32_t elapsedMs() const;
  uint32_t durationMs() const;

 private:
  enum class Stage : uint8_t {
    Idle,
    LoadNext,
    Header,
    Data,
  };

  bool beginNextUnit();
  bool detectHeader(uint32_t& halfPulses);
  bool isLongHeader(File& file, uint32_t markerPosition);
  void beginByte(uint8_t value);
  bool beginNextBit();
  void emitPulse(uint32_t durationUs);
  uint32_t estimateDurationMs(File& file);
  uint32_t byteDurationUs(uint8_t value) const;
  void finish(const char* message);

  static constexpr uint8_t HeaderMarkerSize = 8;
  static constexpr uint8_t TypeRunSize = 10;
  static constexpr uint32_t HeaderHalfPulseUs = 208;
  static constexpr uint32_t ZeroHalfPulseUs = 417;
  static constexpr uint32_t OneHalfPulseUs = 208;
  static constexpr uint16_t ShortHeaderCycles = 4000;
  static constexpr uint16_t LongHeaderCycles = 16000;
  static constexpr uint8_t DataBitsPerByte = 11;
  static constexpr uint8_t HeaderMarker[HeaderMarkerSize] = {
      0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74,
  };

  DacOutputDriver& output_;
  File file_;
  bool playing_ = false;
  Stage stage_ = Stage::Idle;
  uint32_t headerHalfPulsesRemaining_ = 0;
  uint16_t frame_ = 0;
  uint8_t frameBitsRemaining_ = 0;
  uint8_t bitHalfPulsesRemaining_ = 0;
  uint32_t bitHalfPulseUs_ = 0;
  uint32_t nextPulseAtUs_ = 0;
  uint32_t elapsedUs_ = 0;
  uint32_t totalDurationMs_ = 0;
  bool levelHigh_ = false;
};

}  // namespace audio
