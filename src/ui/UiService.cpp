#include "ui/UiService.h"

#include <SD.h>

#include "ui/UiText.h"
#include "ui/UiTheme.h"

namespace ui {

bool UiService::begin() {
  Serial.println("Starting CYD display");
  display_.init();
  display_.setRotation(0);
  display_.setBrightness(255);
  components_.clear();
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(theme::TextPrimary, theme::Background);
  display_.setFont(&fonts::Font2);

  Serial.print("CYD display size: ");
  Serial.print(display_.width());
  Serial.print("x");
  Serial.println(display_.height());
  Serial.println("CYD display initialized");
  return true;
}

void UiService::setFooterSuffix(const char* suffix) {
  footerSuffix_ = suffix == nullptr ? "" : suffix;
}

UiAction UiService::pollAction() {
  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = display_.getTouch(&x, &y);

  if (!touched) {
    touchWasDown_ = false;
    return UiAction::None;
  }

  if (touchWasDown_) {
    return UiAction::None;
  }

  const uint32_t now = millis();
  if (now - lastTouchMs_ < 160) {
    return UiAction::None;
  }

  touchWasDown_ = true;
  lastTouchMs_ = now;

  const UiAction action = hitTest(x, y);
  Serial.print("Touch x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.print(y);
  Serial.print(" action=");
  Serial.println(static_cast<unsigned>(action));
  return action;
}

void UiService::showHome(const char* status) {
  resetTouchZones();

  const int screenW = display_.width();
  const int margin = 12;
  const int gap = 10;
  const int buttonW = (screenW - (margin * 2) - gap) / 2;
  const int buttonH = 62;
  const int left = margin;
  const int right = margin + buttonW + gap;

  components_.clear();
  drawHeader(text::AppName, text::ChooseSystem, true);

  drawHomeTile(left, 58, buttonW, buttonH, "ZX Spectrum", text::ZxTape,
               theme::Accent, UiIcon::Spectrum, UiAction::OpenTk90x);
  drawHomeTile(right, 58, buttonW, buttonH, "MSX", text::MsxTape,
               theme::Primary, UiIcon::Msx, UiAction::OpenMsx);
  drawHomeTile(left, 130, buttonW, buttonH, "WAV", text::WavAudio,
               theme::Info, UiIcon::Wave, UiAction::OpenWav);
  drawHomeTile(right, 130, buttonW, buttonH, text::Settings, text::DeviceSetup,
               theme::Secondary, UiIcon::Sliders, UiAction::OpenMenu);

  drawFooter(status == nullptr ? text::HomeReady : status, theme::Success);
}

void UiService::showFileBrowser(const char* title, const char* path, const storage::FileEntry* entries,
                                size_t entryCount, size_t offset, size_t totalCount, bool hasPrevious,
                                bool hasNext) {
  resetTouchZones();
  components_.clear();
  drawHeader(title, path);
  drawBackButton();

  const int listX = 10;
  const int rowY = 54;
  const int rowW = display_.width() - 20;
  const int rowH = 28;
  const int rowGap = 5;

  if (entryCount == 0) {
    display_.setTextDatum(textdatum_t::top_left);
    display_.setTextColor(theme::TextSecondary, theme::Background);
    display_.setFont(&fonts::Font2);
    display_.drawString(text::NoFiles, 18, 92);
  } else {
    for (size_t row = 0; row < 4; ++row) {
      const size_t index = offset + row;
      if (index >= entryCount) {
        break;
      }

      const storage::FileEntry& entry = entries[index];
      drawListItem(listX, rowY + static_cast<int>(row) * (rowH + rowGap), rowW,
                   rowH, entry.displayName.c_str(),
                   entry.isDirectory ? theme::Accent : theme::Primary,
                   entry.isDirectory ? UiIcon::Folder : UiIcon::File,
                   browserSelectAction(row));
    }
  }

  drawButton(10, 188, 84, 28, text::Previous, theme::Info, theme::TextPrimary,
             hasPrevious ? UiAction::BrowserPrevious : UiAction::None);
  drawButton(226, 188, 84, 28, text::Next, theme::Info, theme::TextPrimary,
             hasNext ? UiAction::BrowserNext : UiAction::None);

  char pageInfo[40];
  const size_t first = totalCount == 0 ? 0 : offset + 1;
  const size_t last = min(offset + static_cast<size_t>(4), totalCount);
  snprintf(pageInfo, sizeof(pageInfo), "%u-%u of %u", static_cast<unsigned>(first), static_cast<unsigned>(last),
           static_cast<unsigned>(totalCount));
  display_.setTextDatum(textdatum_t::middle_center);
  display_.setTextColor(theme::TextSecondary, theme::Background);
  display_.setFont(&fonts::Font2);
  display_.drawString(pageInfo, display_.width() / 2, 202);

  drawFooter(text::SelectFile, theme::Primary);
}

void UiService::showPlayer(const char* filename, const char* format,
                           const char* status, uint32_t elapsedMs,
                           uint32_t durationMs, bool playing,
                           const metadata::GameMetadata* metadata) {
  resetTouchZones();
  components_.clear();
  drawHeader(text::Player, format);
  drawBackButton();

  components_.drawPanel(12, 56, display_.width() - 24, 91);
  display_.setTextDatum(textdatum_t::top_left);
  const bool hasMetadata = metadata != nullptr && metadata->available;
  int textX = 22;
  int textWidth = display_.width() - 44;
  if (hasMetadata && metadata->coverPath.length() > 0 &&
      SD.exists(metadata->coverPath)) {
    display_.drawJpgFile(SD, metadata->coverPath.c_str(), 20, 62, 55, 78, 0, 0,
                         0.61f);
    textX = 84;
    textWidth = display_.width() - textX - 18;
  }

  if (hasMetadata) {
    display_.setFont(&fonts::Font2);
    drawTextFit(metadata->title, textX, 64, textWidth,
                theme::TextPrimary, theme::Surface);
    display_.setFont(&fonts::Font0);
    String release = metadata->platform;
    if (metadata->year > 0) {
      release += " - ";
      release += String(metadata->year);
    }
    drawTextFit(release, textX, 88, textWidth,
                theme::Accent, theme::Surface);
    drawTextFit(metadata->developer, textX, 105, textWidth,
                theme::TextSecondary, theme::Surface);
    drawTextFit(metadata->genres, textX, 122, textWidth,
                theme::TextSecondary, theme::Surface);
  } else {
    display_.setTextColor(theme::TextSecondary, theme::Surface);
    display_.setFont(&fonts::Font0);
    display_.drawString(text::File, 22, 70);
    display_.setFont(&fonts::Font2);
    drawTextFit(String(filename), 22, 88, display_.width() - 44,
                theme::TextPrimary, theme::Surface);
  }

  updatePlayerProgress(elapsedMs, durationMs, playing);

  drawButton(36, 178, 110, 34, text::Play, theme::Success, theme::TextPrimary,
             playing ? UiAction::None : UiAction::PlayerPlay, UiIcon::Play);
  drawButton(174, 178, 110, 34, text::Stop, theme::Danger, theme::TextPrimary,
             playing ? UiAction::PlayerStop : UiAction::None, UiIcon::Stop);
  drawFooter(status == nullptr ? text::Ready : status,
             playing ? theme::Primary : theme::Success);
}

void UiService::updatePlayerState(const char* status, bool playing) {
  resetTouchZones();
  drawBackButton();
  drawButton(36, 178, 110, 34, text::Play, theme::Success,
             theme::TextPrimary,
             playing ? UiAction::None : UiAction::PlayerPlay, UiIcon::Play);
  drawButton(174, 178, 110, 34, text::Stop, theme::Danger,
             theme::TextPrimary,
             playing ? UiAction::PlayerStop : UiAction::None, UiIcon::Stop);
  drawFooter(status == nullptr ? text::Ready : status,
             playing ? theme::Primary : theme::Success);
}

void UiService::updatePlayerProgress(uint32_t elapsedMs, uint32_t durationMs, bool playing) {
  char elapsed[8] = {};
  char duration[8] = {};
  char label[24] = {};
  components_.formatTime(elapsedMs, elapsed, sizeof(elapsed));
  components_.formatTime(durationMs, duration, sizeof(duration));
  snprintf(label, sizeof(label), "%s / %s", elapsed, durationMs == 0 ? "--:--" : duration);

  const int x = 22;
  const int y = 148;
  const int w = display_.width() - 44;
  const int barY = 166;
  const int barH = 8;

  display_.startWrite();
  display_.fillRect(x, y, w, 27, theme::Background);
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(playing ? theme::Primary : theme::TextSecondary,
                        theme::Background);
  display_.setFont(&fonts::Font2);
  display_.drawString(label, x, y);

  display_.fillRoundRect(x, barY, w, barH, 3, theme::ProgressTrack);
  if (durationMs > 0) {
    uint32_t fillW = (static_cast<uint64_t>(elapsedMs) * static_cast<uint32_t>(w)) / durationMs;
    if (fillW > static_cast<uint32_t>(w)) {
      fillW = w;
    }
    if (fillW > 0) {
      display_.fillRoundRect(x, barY, static_cast<int>(fillW), barH, 3,
                             theme::Progress);
    }
  }
  display_.endWrite();
}

void UiService::showSettings(const char* displayDriver, bool sdMounted, const char* wifiStatus) {
  resetTouchZones();
  components_.clear();
  drawHeader(text::Settings, text::Configuration);
  drawBackButton();

  components_.drawPanel(12, 58, display_.width() - 24, 84);
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(theme::TextSecondary, theme::Surface);
  display_.setFont(&fonts::Font2);
  display_.drawString("Display", 22, 68);
  display_.setTextColor(theme::TextPrimary, theme::Surface);
  display_.drawString(displayDriver, 112, 68);

  display_.setTextColor(theme::TextSecondary, theme::Surface);
  display_.drawString("SD card", 22, 94);
  display_.setTextColor(sdMounted ? theme::Success : theme::Danger, theme::Surface);
  display_.drawString(sdMounted ? text::Mounted : text::NotMounted, 112, 94);

  drawTextFit(String(wifiStatus == nullptr ? "Wi-Fi: -" : wifiStatus), 22, 112,
              display_.width() - 44, theme::TextSecondary, theme::Surface);

  drawButton(18, 140, 132, 34, text::WifiSetup, theme::Info, theme::TextPrimary,
             UiAction::OpenWifiSettings, UiIcon::Wifi);
  drawButton(168, 140, 132, 34, text::TapSetup, theme::Secondary,
             theme::TextPrimary, UiAction::OpenTapSettings, UiIcon::Sliders);
  drawButton(82, 182, 156, 28, text::AudioTest, theme::Accent,
             theme::TextPrimary, UiAction::OpenAudioTest, UiIcon::Wave);
  drawFooter(text::SettingsReady, theme::Primary);
}

void UiService::showAudioTest(uint16_t frequencyHz, uint8_t level, bool playing,
                              const char* status, bool error) {
  resetTouchZones();
  components_.clear();
  drawHeader(text::AudioTest, text::AudioDiagnostics);
  drawBackButton();

  components_.drawPanel(12, 58, display_.width() - 24, 56);
  display_.setTextDatum(textdatum_t::top_left);
  display_.setFont(&fonts::Font2);
  display_.setTextColor(theme::TextSecondary, theme::Surface);
  display_.drawString(text::TestTone, 22, 68);

  char frequency[20] = {};
  snprintf(frequency, sizeof(frequency), frequencyHz == 0 ? "Ready" : "%u Hz",
           static_cast<unsigned>(frequencyHz));
  display_.setTextColor(playing ? theme::Primary : theme::TextPrimary,
                        theme::Surface);
  display_.drawString(frequency, 118, 68);

  char levelText[18] = {};
  snprintf(levelText, sizeof(levelText), "%s %u%%", text::OutputLevel,
           static_cast<unsigned>((static_cast<uint16_t>(level) * 100U) / 255U));
  display_.setTextColor(theme::Accent, theme::Surface);
  display_.drawString(levelText, 22, 91);
  drawButton(198, 82, 48, 26, "-", theme::Info, theme::TextPrimary,
             UiAction::AudioLevelDown);
  drawButton(254, 82, 48, 26, "+", theme::Success, theme::TextPrimary,
             UiAction::AudioLevelUp);

  drawButton(12, 124, 92, 34, "1 kHz", theme::Primary, theme::TextPrimary,
             UiAction::AudioTone1000, UiIcon::Wave);
  drawButton(114, 124, 92, 34, "1200 Hz", theme::Info, theme::TextPrimary,
             UiAction::AudioTone1200, UiIcon::Wave);
  drawButton(216, 124, 92, 34, "2400 Hz", theme::Secondary,
             theme::TextPrimary, UiAction::AudioTone2400, UiIcon::Wave);

  drawButton(104, 170, 112, 34, text::Stop, theme::Danger,
             theme::TextPrimary,
             playing ? UiAction::AudioTestStop : UiAction::None,
             UiIcon::Stop);

  drawFooter(status == nullptr ? text::ToneReady : status,
             error ? theme::Danger
                   : (playing ? theme::Primary : theme::Success));
}

void UiService::showTapSettings(uint16_t timingPermille, uint8_t amplitude,
                                bool inverted, const char* status, bool error) {
  resetTouchZones();
  components_.clear();
  drawHeader(text::TapSetup, text::TapCompatibility);
  drawBackButton();

  components_.drawPanel(12, 58, display_.width() - 24, 120);
  display_.setTextDatum(textdatum_t::top_left);
  display_.setFont(&fonts::Font2);
  display_.setTextColor(theme::TextSecondary, theme::Surface);

  char timing[16] = {};
  snprintf(timing, sizeof(timing), "%.1f%%", timingPermille / 10.0F);
  display_.drawString(text::Timing, 22, 68);
  display_.setTextColor(theme::Primary, theme::Surface);
  display_.drawString(timing, 108, 68);
  drawButton(198, 62, 48, 28, "-", theme::Info, theme::TextPrimary,
             UiAction::TapTimingDown);
  drawButton(254, 62, 48, 28, "+", theme::Success, theme::TextPrimary,
             UiAction::TapTimingUp);

  display_.setTextColor(theme::TextSecondary, theme::Surface);
  display_.drawString(text::Level, 22, 106);
  display_.setTextColor(theme::Accent, theme::Surface);
  char level[16] = {};
  const uint16_t levelPercent = (static_cast<uint16_t>(amplitude) * 100U) / 127U;
  snprintf(level, sizeof(level), "%u%%", levelPercent);
  display_.drawString(level, 108, 106);
  drawButton(198, 100, 48, 28, "-", theme::Info, theme::TextPrimary,
             UiAction::TapLevelDown);
  drawButton(254, 100, 48, 28, "+", theme::Success, theme::TextPrimary,
             UiAction::TapLevelUp);

  display_.setTextColor(theme::TextSecondary, theme::Surface);
  display_.drawString(text::Polarity, 22, 144);
  drawButton(168, 138, 134, 30, inverted ? text::Inverted : text::Normal,
             inverted ? theme::Danger : theme::Primary, theme::TextPrimary,
             UiAction::TapInvert);

  drawButton(82, 182, 156, 28, text::RestoreDefaults, theme::Warning,
             theme::TextPrimary, UiAction::TapRestoreDefaults,
             UiIcon::Refresh);

  drawFooter(status == nullptr ? text::TapDefaults : status,
             status == nullptr ? theme::Primary
                               : (error ? theme::Danger : theme::Success));
}

void UiService::showWifiList(const network::WifiNetworkInfo* networks, size_t networkCount, size_t offset,
                             size_t totalCount, const char* status, bool hasPrevious, bool hasNext) {
  resetTouchZones();
  components_.clear();
  drawHeader("Wi-Fi", text::SelectNetwork);
  drawBackButton();

  if (networkCount == 0) {
    display_.setTextDatum(textdatum_t::top_left);
    display_.setTextColor(theme::TextSecondary, theme::Background);
    display_.setFont(&fonts::Font2);
    display_.drawString(text::NoNetworks, 18, 86);
  } else {
    const int listX = 10;
    const int rowY = 54;
    const int rowW = display_.width() - 20;
    const int rowH = 28;
    const int rowGap = 5;

    for (size_t row = 0; row < 4; ++row) {
      const size_t index = offset + row;
      if (index >= networkCount) {
        break;
      }

      String label = networks[index].ssid;
      label += " ";
      label += networks[index].rssi;
      label += "dBm";
      drawListItem(listX, rowY + static_cast<int>(row) * (rowH + rowGap), rowW,
                   rowH, label.c_str(),
                   networks[index].encrypted ? theme::Accent : theme::Primary,
                   networks[index].encrypted ? UiIcon::Lock : UiIcon::Wifi,
                   wifiSelectAction(row));
    }
  }

  drawButton(10, 188, 78, 28, text::Previous, theme::Info, theme::TextPrimary,
             hasPrevious ? UiAction::WifiPrevious : UiAction::None);
  drawButton(98, 188, 78, 28, text::Refresh, theme::Success, theme::TextPrimary,
             UiAction::WifiRescan, UiIcon::Refresh);
  drawButton(230, 188, 78, 28, text::Next, theme::Info, theme::TextPrimary,
             hasNext ? UiAction::WifiNext : UiAction::None);

  drawFooter(status == nullptr ? text::SelectNetwork : status, theme::Primary);
}

void UiService::showWifiPassword(const char* ssid, const char* password, const char* keyPage, const char* status) {
  resetTouchZones();
  components_.clear();
  drawHeader("Wi-Fi", ssid == nullptr ? "" : ssid);
  drawBackButton();

  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(theme::TextSecondary, theme::Background);
  display_.setFont(&fonts::Font2);
  drawTextFit(String(text::Password) + (password == nullptr ? "" : password), 12, 54, display_.width() - 24,
              theme::TextSecondary, theme::Background);

  const int keyColumns = 6;
  const int keyCount = 18;
  const int gap = 4;
  const int keyW = (display_.width() - 16 - ((keyColumns - 1) * gap)) / keyColumns;
  const int keyH = 24;
  const int startX = 8;
  const int startY = 80;

  for (size_t i = 0; i < keyCount; ++i) {
    const char key = (keyPage != nullptr && keyPage[i] != '\0') ? keyPage[i] : ' ';
    char label[2] = {key, '\0'};
    const int col = static_cast<int>(i % keyColumns);
    const int row = static_cast<int>(i / keyColumns);
    drawButton(startX + col * (keyW + gap), startY + row * (keyH + gap),
               keyW, keyH, label, theme::Info, theme::TextPrimary,
               key == ' ' ? UiAction::None : wifiKeyAction(i));
  }

  drawButton(8, 176, 72, 30, text::Keys, theme::Primary, theme::TextPrimary,
             UiAction::WifiKeyboardNext);
  drawButton(88, 176, 82, 30, text::Delete, theme::Danger, theme::TextPrimary,
             UiAction::WifiBackspace, UiIcon::Delete);
  drawButton(178, 176, 132, 30, text::Connect, theme::Success,
             theme::TextPrimary, UiAction::WifiConnect, UiIcon::Connect);

  drawFooter(status == nullptr ? text::EnterPassword : status, theme::Primary);
}

void UiService::showStatus(const char* message) {
  drawFooter(message, theme::Success);
  Serial.print("STATUS: ");
  Serial.println(message);
}

void UiService::showError(const char* message) {
  drawFooter(message, theme::Danger);
  Serial.print("ERROR: ");
  Serial.println(message);
}

void UiService::resetTouchZones() {
  touchZoneCount_ = 0;
}

void UiService::addTouchZone(int x, int y, int w, int h, UiAction action) {
  if (touchZoneCount_ >= MaxTouchZones || action == UiAction::None) {
    return;
  }

  touchZones_[touchZoneCount_].x = static_cast<int16_t>(x);
  touchZones_[touchZoneCount_].y = static_cast<int16_t>(y);
  touchZones_[touchZoneCount_].w = static_cast<int16_t>(w);
  touchZones_[touchZoneCount_].h = static_cast<int16_t>(h);
  touchZones_[touchZoneCount_].action = action;
  ++touchZoneCount_;
}

UiAction UiService::hitTest(uint16_t x, uint16_t y) const {
  for (size_t i = 0; i < touchZoneCount_; ++i) {
    const TouchZone& zone = touchZones_[i];
    if (x >= zone.x && x < zone.x + zone.w && y >= zone.y && y < zone.y + zone.h) {
      return zone.action;
    }
  }

  return UiAction::None;
}

UiAction UiService::browserSelectAction(size_t row) const {
  switch (row) {
    case 0:
      return UiAction::BrowserSelect0;
    case 1:
      return UiAction::BrowserSelect1;
    case 2:
      return UiAction::BrowserSelect2;
    case 3:
      return UiAction::BrowserSelect3;
    default:
      return UiAction::None;
  }
}

UiAction UiService::wifiSelectAction(size_t row) const {
  switch (row) {
    case 0:
      return UiAction::WifiSelect0;
    case 1:
      return UiAction::WifiSelect1;
    case 2:
      return UiAction::WifiSelect2;
    case 3:
      return UiAction::WifiSelect3;
    default:
      return UiAction::None;
  }
}

UiAction UiService::wifiKeyAction(size_t index) const {
  switch (index) {
    case 0:
      return UiAction::WifiKey0;
    case 1:
      return UiAction::WifiKey1;
    case 2:
      return UiAction::WifiKey2;
    case 3:
      return UiAction::WifiKey3;
    case 4:
      return UiAction::WifiKey4;
    case 5:
      return UiAction::WifiKey5;
    case 6:
      return UiAction::WifiKey6;
    case 7:
      return UiAction::WifiKey7;
    case 8:
      return UiAction::WifiKey8;
    case 9:
      return UiAction::WifiKey9;
    case 10:
      return UiAction::WifiKey10;
    case 11:
      return UiAction::WifiKey11;
    case 12:
      return UiAction::WifiKey12;
    case 13:
      return UiAction::WifiKey13;
    case 14:
      return UiAction::WifiKey14;
    case 15:
      return UiAction::WifiKey15;
    case 16:
      return UiAction::WifiKey16;
    case 17:
      return UiAction::WifiKey17;
    default:
      return UiAction::None;
  }
}

void UiService::drawHeader(const char* title, const char* subtitle, bool branded) {
  components_.drawHeader(title, subtitle, branded);
}

void UiService::drawBackButton() {
  components_.drawIconButton(8, 8, 36, 36, UiIcon::Back, theme::TextPrimary);
  addTouchZone(0, 0, 52, 52, UiAction::Back);
}

void UiService::drawButton(int x, int y, int w, int h, const char* label, uint16_t fill, uint16_t textColor,
                           UiAction action, UiIcon icon) {
  components_.drawButton(x, y, w, h, label, fill, textColor,
                         action != UiAction::None, icon);
  addTouchZone(x, y, w, h, action);
}

void UiService::drawHomeTile(int x, int y, int w, int h, const char* label,
                             const char* detail, uint16_t accent, UiIcon icon,
                             UiAction action) {
  components_.drawHomeTile(x, y, w, h, label, detail, accent, icon);
  addTouchZone(x, y, w, h, action);
}

void UiService::drawListItem(int x, int y, int w, int h, const char* label,
                             uint16_t accent, UiIcon icon, UiAction action) {
  components_.drawListItem(x, y, w, h, label, accent, icon,
                           action != UiAction::None);
  addTouchZone(x, y, w, h, action);
}

void UiService::drawTextFit(const String& text, int x, int y, int w, uint16_t textColor, uint16_t background) {
  components_.drawTextFit(text, x, y, w, textColor, background);
}

void UiService::drawFooter(const char* message, uint16_t color) {
  const int footerH = 24;
  const int y = display_.height() - footerH;
  String footerText(message == nullptr ? "" : message);
  if (footerSuffix_.length() > 0) {
    if (footerText.length() > 0) {
      footerText += " | ";
    }
    footerText += footerSuffix_;
  }

  display_.fillRect(0, y, display_.width(), footerH, theme::Surface);
  display_.fillRect(0, y, 4, footerH, color);
  display_.drawFastHLine(0, y, display_.width(), theme::Divider);
  display_.setTextDatum(textdatum_t::middle_left);
  display_.setTextColor(theme::TextPrimary, theme::Surface);
  display_.setFont(&fonts::Font2);
  drawTextFit(footerText, 10, y + 4, display_.width() - 18,
              theme::TextPrimary, theme::Surface);
  display_.setTextDatum(textdatum_t::top_left);
}

void UiService::formatTime(uint32_t ms, char* output, size_t outputSize) const {
  components_.formatTime(ms, output, outputSize);
}

}  // namespace ui
