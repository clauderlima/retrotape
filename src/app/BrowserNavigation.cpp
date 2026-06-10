#include "app/BrowserNavigation.h"

namespace app {

void BrowserNavigation::open(BrowserMode mode) {
  mode_ = mode;
  path_ = rootPath();
}

void BrowserNavigation::enter(const String& path) {
  path_ = path;
}

bool BrowserNavigation::goToParent() {
  const String root = rootPath();
  if (path_ == root || path_ == "/") {
    return false;
  }

  const int slash = path_.lastIndexOf('/');
  path_ = slash <= 0 ? "/" : path_.substring(0, slash);
  if (!path_.startsWith(root)) {
    path_ = root;
  }
  return true;
}

BrowserMode BrowserNavigation::mode() const {
  return mode_;
}

const String& BrowserNavigation::path() const {
  return path_;
}

const char* BrowserNavigation::title() const {
  switch (mode_) {
    case BrowserMode::Tk90x:
      return "ZX Spectrum";
    case BrowserMode::Msx:
      return "MSX";
    case BrowserMode::Wav:
      return "WAV";
  }
  return "Files";
}

void BrowserNavigation::extensions(const char* const*& values, size_t& count) const {
  static const char* const tk90xExtensions[] = {".tap"};
  static const char* const msxExtensions[] = {".cas"};
  static const char* const wavExtensions[] = {".wav"};

  switch (mode_) {
    case BrowserMode::Tk90x:
      values = tk90xExtensions;
      count = sizeof(tk90xExtensions) / sizeof(tk90xExtensions[0]);
      break;
    case BrowserMode::Msx:
      values = msxExtensions;
      count = sizeof(msxExtensions) / sizeof(msxExtensions[0]);
      break;
    case BrowserMode::Wav:
      values = wavExtensions;
      count = sizeof(wavExtensions) / sizeof(wavExtensions[0]);
      break;
  }
}

const char* BrowserNavigation::rootPath() const {
  switch (mode_) {
    case BrowserMode::Tk90x:
      return "/tk90x";
    case BrowserMode::Msx:
      return "/msx";
    case BrowserMode::Wav:
      return "/";
  }
  return "/";
}

}  // namespace app
