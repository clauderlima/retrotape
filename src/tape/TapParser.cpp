#include "tape/TapParser.h"

namespace tape {

bool TapParser::canParse(TapeFormat format) const {
  return format == TapeFormat::Tap;
}

}  // namespace tape

