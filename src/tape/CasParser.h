#pragma once

#include "tape/TapeTypes.h"

namespace tape {

class CasParser {
 public:
  bool canParse(TapeFormat format) const;
};

}  // namespace tape

