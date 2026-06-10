#pragma once

#include <Arduino.h>

#include "hardware/CydDisplay.h"

namespace ui {

class UiComponents {
 public:
  explicit UiComponents(hardware::CydDisplay& display);

  void clear();
  void drawHeader(const char* title, const char* subtitle);
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill,
                  uint16_t textColor, bool enabled);
  void drawTextFit(const String& text, int x, int y, int w, uint16_t textColor,
                   uint16_t background);
  void formatTime(uint32_t ms, char* output, size_t outputSize) const;

 private:
  hardware::CydDisplay& display_;
};

}  // namespace ui
