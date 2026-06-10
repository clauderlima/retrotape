#include "tape/TsxParser.h"

namespace tape {

bool TsxParser::canParse(TapeFormat format) const {
  return format == TapeFormat::Tsx || format == TapeFormat::Tsz;
}

}  // namespace tape

