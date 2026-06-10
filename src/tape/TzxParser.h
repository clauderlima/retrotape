#pragma once

#include "tape/TapeTypes.h"

namespace tape {

class TzxParser {
 public:
  bool canParse(TapeFormat format) const;
};

}  // namespace tape

