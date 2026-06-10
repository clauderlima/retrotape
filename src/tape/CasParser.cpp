#include "tape/CasParser.h"

namespace tape {

bool CasParser::canParse(TapeFormat format) const {
  return format == TapeFormat::Cas;
}

}  // namespace tape

