#include "tape/PulseGenerator.h"

namespace tape {

void PulseGenerator::reset() {}

Pulse PulseGenerator::nextSilence(uint32_t durationUs) const {
  return Pulse{durationUs, false};
}

}  // namespace tape

