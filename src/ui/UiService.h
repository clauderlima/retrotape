#pragma once

#include <Arduino.h>

#include "hardware/CydDisplay.h"
#include "network/WifiService.h"
#include "storage/SdCardService.h"
#include "ui/UiComponents.h"

namespace ui {

enum class UiAction : uint8_t {
  None,
  OpenTk90x,
  OpenMsx,
  OpenWav,
  OpenMenu,
  Back,
  BrowserPrevious,
  BrowserNext,
  BrowserSelect0,
  BrowserSelect1,
  BrowserSelect2,
  BrowserSelect3,
  PlayerPlay,
  PlayerStop,
  OpenWifiSettings,
  OpenTapSettings,
  OpenAudioTest,
  AudioTone1000,
  AudioTone1200,
  AudioTone2400,
  AudioLevelDown,
  AudioLevelUp,
  AudioTestStop,
  TapTimingDown,
  TapTimingUp,
  TapLevelDown,
  TapLevelUp,
  TapInvert,
  TapRestoreDefaults,
  WifiRescan,
  WifiPrevious,
  WifiNext,
  WifiSelect0,
  WifiSelect1,
  WifiSelect2,
  WifiSelect3,
  WifiKey0,
  WifiKey1,
  WifiKey2,
  WifiKey3,
  WifiKey4,
  WifiKey5,
  WifiKey6,
  WifiKey7,
  WifiKey8,
  WifiKey9,
  WifiKey10,
  WifiKey11,
  WifiKey12,
  WifiKey13,
  WifiKey14,
  WifiKey15,
  WifiKey16,
  WifiKey17,
  WifiKeyboardNext,
  WifiBackspace,
  WifiConnect,
};

class UiService {
 public:
  bool begin();
  void setFooterSuffix(const char* suffix);
  UiAction pollAction();
  void showHome(const char* status);
  void showFileBrowser(const char* title, const char* path, const storage::FileEntry* entries, size_t entryCount,
                       size_t offset, size_t totalCount, bool hasPrevious, bool hasNext);
  void showPlayer(const char* filename, const char* format, const char* status, uint32_t elapsedMs = 0,
                  uint32_t durationMs = 0, bool playing = false);
  void updatePlayerProgress(uint32_t elapsedMs, uint32_t durationMs, bool playing);
  void showSettings(const char* displayDriver, bool sdMounted, const char* wifiStatus);
  void showAudioTest(uint16_t frequencyHz, uint8_t level, bool playing,
                     const char* status, bool error = false);
  void showTapSettings(uint16_t timingPermille, uint8_t amplitude, bool inverted,
                       const char* status = nullptr, bool error = false);
  void showWifiList(const network::WifiNetworkInfo* networks, size_t networkCount, size_t offset, size_t totalCount,
                    const char* status, bool hasPrevious, bool hasNext);
  void showWifiPassword(const char* ssid, const char* password, const char* keyPage, const char* status);
  void showStatus(const char* message);
  void showError(const char* message);

 private:
  struct TouchZone {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
    UiAction action = UiAction::None;
  };

  void resetTouchZones();
  void addTouchZone(int x, int y, int w, int h, UiAction action);
  UiAction hitTest(uint16_t x, uint16_t y) const;
  UiAction browserSelectAction(size_t row) const;
  UiAction wifiSelectAction(size_t row) const;
  UiAction wifiKeyAction(size_t index) const;
  void drawHeader(const char* title, const char* subtitle, bool branded = false);
  void drawBackButton();
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill,
                  uint16_t textColor, UiAction action, UiIcon icon = UiIcon::None);
  void drawHomeTile(int x, int y, int w, int h, const char* label, const char* detail,
                    uint16_t accent, UiIcon icon, UiAction action);
  void drawListItem(int x, int y, int w, int h, const char* label, uint16_t accent,
                    UiIcon icon, UiAction action);
  void drawTextFit(const String& text, int x, int y, int w, uint16_t textColor, uint16_t background);
  void drawFooter(const char* message, uint16_t color);
  void formatTime(uint32_t ms, char* output, size_t outputSize) const;

  static constexpr size_t MaxTouchZones = 24;

  hardware::CydDisplay display_;
  UiComponents components_{display_};
  String footerSuffix_;
  TouchZone touchZones_[MaxTouchZones];
  size_t touchZoneCount_ = 0;
  uint32_t lastTouchMs_ = 0;
  bool touchWasDown_ = false;
};

}  // namespace ui
