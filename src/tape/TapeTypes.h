#pragma once

#include <Arduino.h>

namespace tape {

enum class TapeFormat {
  Unknown,
  Wav,
  Tap,
  Tzx,
  Cas,
  Tsx,
  Tsz,
};

enum class PlayerStatus {
  Idle,
  Ready,
  Playing,
  Paused,
  Stopped,
  Finished,
  Error,
};

struct TapeTiming {
  uint32_t pilotPulseUs = 0;
  uint32_t syncOneUs = 0;
  uint32_t syncTwoUs = 0;
  uint32_t zeroPulseUs = 0;
  uint32_t onePulseUs = 0;
  uint32_t pauseAfterBlockMs = 0;
};

struct TapeBlock {
  uint32_t offset = 0;
  uint32_t size = 0;
  uint16_t index = 0;
  TapeFormat format = TapeFormat::Unknown;
  TapeTiming timing;
  String label;
};

}  // namespace tape

