#include "tape/TzxParser.h"

namespace tape {

bool TzxParser::canParse(TapeFormat format) const {
  return format == TapeFormat::Tzx;
}

}  // namespace tape

