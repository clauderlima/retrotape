#pragma once

#include <Arduino.h>

#include "tape/TapeTypes.h"

namespace tape {

class TapParser {
 public:
  bool canParse(TapeFormat format) const;
};

}  // namespace tape

