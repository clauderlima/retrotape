#pragma once

#include <cstdint>

namespace tape {

struct Pulse {
  uint32_t durationUs = 0;
  bool high = false;
};

class PulseGenerator {
 public:
  void reset();
  Pulse nextSilence(uint32_t durationUs) const;
};

}  // namespace tape

