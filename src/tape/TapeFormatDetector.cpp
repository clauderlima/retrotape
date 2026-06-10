#include "tape/TapeFormatDetector.h"

namespace tape {

TapeFormat TapeFormatDetector::detectFromPath(const String& path) {
  String lowerPath(path);
  lowerPath.toLowerCase();

  if (lowerPath.endsWith(".wav")) return TapeFormat::Wav;
  if (lowerPath.endsWith(".tap")) return TapeFormat::Tap;
  if (lowerPath.endsWith(".tzx")) return TapeFormat::Tzx;
  if (lowerPath.endsWith(".cas")) return TapeFormat::Cas;
  if (lowerPath.endsWith(".tsx")) return TapeFormat::Tsx;
  if (lowerPath.endsWith(".tsz")) return TapeFormat::Tsz;

  return TapeFormat::Unknown;
}

const char* TapeFormatDetector::toString(TapeFormat format) {
  switch (format) {
    case TapeFormat::Wav:
      return "WAV";
    case TapeFormat::Tap:
      return "TAP";
    case TapeFormat::Tzx:
      return "TZX";
    case TapeFormat::Cas:
      return "CAS";
    case TapeFormat::Tsx:
      return "TSX";
    case TapeFormat::Tsz:
      return "TSZ";
    case TapeFormat::Unknown:
    default:
      return "Unknown";
  }
}

}  // namespace tape

