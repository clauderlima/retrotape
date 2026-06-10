#pragma once

#include <Arduino.h>

#include "hardware/CydDisplay.h"

namespace ui {

enum class UiIcon : uint8_t {
  None,
  Back,
  Cassette,
  Computer,
  Spectrum,
  Msx,
  Wave,
  Sliders,
  Folder,
  File,
  Wifi,
  Lock,
  Play,
  Stop,
  Refresh,
  Delete,
  Connect,
};

class UiComponents {
 public:
  explicit UiComponents(hardware::CydDisplay& display);

  void clear();
  void drawHeader(const char* title, const char* subtitle, bool branded = false);
  void drawPanel(int x, int y, int w, int h);
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill,
                  uint16_t textColor, bool enabled, UiIcon icon = UiIcon::None);
  void drawHomeTile(int x, int y, int w, int h, const char* label, const char* detail,
                    uint16_t accent, UiIcon icon);
  void drawListItem(int x, int y, int w, int h, const char* label, uint16_t accent,
                    UiIcon icon, bool enabled = true);
  void drawIconButton(int x, int y, int w, int h, UiIcon icon, uint16_t accent,
                      bool enabled = true);
  void drawTextFit(const String& text, int x, int y, int w, uint16_t textColor,
                   uint16_t background);
  void formatTime(uint32_t ms, char* output, size_t outputSize) const;

 private:
  void drawIcon(UiIcon icon, int x, int y, uint16_t color);
  void drawCassette(int x, int y, int w, int h, uint16_t color);
  void drawSpectrumIcon(int x, int y, uint16_t color);
  void drawMsxIcon(int x, int y, uint16_t color);

  hardware::CydDisplay& display_;
};

}  // namespace ui
