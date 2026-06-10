#pragma once

#include "tape/TapeTypes.h"

namespace tape {

class TsxParser {
 public:
  bool canParse(TapeFormat format) const;
};

}  // namespace tape

