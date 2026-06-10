#pragma once

#include <Arduino.h>

namespace app {

enum class BrowserMode : uint8_t {
  Tk90x,
  Msx,
  Wav,
};

class BrowserNavigation {
 public:
  void open(BrowserMode mode);
  void enter(const String& path);
  bool goToParent();

  BrowserMode mode() const;
  const String& path() const;
  const char* title() const;
  void extensions(const char* const*& values, size_t& count) const;

 private:
  const char* rootPath() const;

  BrowserMode mode_ = BrowserMode::Tk90x;
  String path_ = "/tk90x";
};

}  // namespace app
