#pragma once

#include <Arduino.h>

#include "tape/TapeTypes.h"

namespace tape {

class TapeFormatDetector {
 public:
  static TapeFormat detectFromPath(const String& path);
  static const char* toString(TapeFormat format);
};

}  // namespace tape

