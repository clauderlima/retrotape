#include "ui/UiComponents.h"

#include "ui/UiTheme.h"

namespace ui {

UiComponents::UiComponents(hardware::CydDisplay& display) : display_(display) {}

void UiComponents::clear() {
  display_.fillScreen(theme::Background);
}

void UiComponents::drawHeader(const char* title, const char* subtitle, bool branded) {
  display_.fillRect(0, 0, display_.width(), theme::HeaderHeight, theme::Surface);

  const int textX = branded ? 58 : 54;
  if (branded) {
    drawCassette(10, 12, 38, 27, theme::Accent);
    display_.setFont(&fonts::Font4);
    display_.setTextColor(theme::TextPrimary, theme::Surface);
    display_.setTextDatum(textdatum_t::top_left);
    display_.drawString(title, textX, 6);
    display_.setFont(&fonts::Font0);
    display_.setTextColor(theme::TextSecondary, theme::Surface);
    display_.drawString(subtitle, textX, 34);
  } else {
    display_.setTextDatum(textdatum_t::top_left);
    display_.setFont(&fonts::Font2);
    display_.setTextColor(theme::TextPrimary, theme::Surface);
    display_.drawString(title, textX, 6);
    display_.setFont(&fonts::Font0);
    display_.setTextColor(theme::TextSecondary, theme::Surface);
    drawTextFit(subtitle == nullptr ? "" : subtitle, textX, 30,
                display_.width() - textX - theme::PageMargin,
                theme::TextSecondary, theme::Surface);
  }

  display_.drawFastHLine(0, theme::HeaderHeight - 1, display_.width(), theme::Divider);
}

void UiComponents::drawPanel(int x, int y, int w, int h) {
  display_.fillRoundRect(x, y, w, h, theme::PanelRadius, theme::Surface);
  display_.drawRoundRect(x, y, w, h, theme::PanelRadius, theme::Divider);
}

void UiComponents::drawButton(int x, int y, int w, int h, const char* label,
                              uint16_t accent, uint16_t textColor, bool enabled,
                              UiIcon icon) {
  const uint16_t surface = enabled ? theme::SurfaceRaised : theme::Surface;
  const uint16_t stroke = enabled ? theme::Divider : theme::SurfaceMuted;
  const uint16_t text = enabled ? textColor : theme::Disabled;
  const uint16_t iconColor = enabled ? accent : theme::Disabled;

  display_.fillRoundRect(x, y, w, h, theme::ButtonRadius, surface);
  display_.drawRoundRect(x, y, w, h, theme::ButtonRadius, stroke);
  if (enabled) {
    display_.fillRoundRect(x + 1, y + 1, 4, h - 2, 2, accent);
  }

  String fitted = label == nullptr ? "" : label;
  display_.setTextColor(text, surface);
  display_.setFont(&fonts::Font2);

  int textX = x + (w / 2);
  int available = w - 16;
  if (icon != UiIcon::None) {
    drawIcon(icon, x + 14, y + (h / 2), iconColor);
    textX += 9;
    available -= 30;
  }

  if (display_.textWidth(fitted) > available) {
    while (fitted.length() > 3 && display_.textWidth(fitted + "..") > available) {
      fitted.remove(fitted.length() - 1);
    }
    fitted += "..";
  }

  display_.setTextDatum(textdatum_t::middle_center);
  display_.drawString(fitted, textX, y + (h / 2));
  display_.setTextDatum(textdatum_t::top_left);
}

void UiComponents::drawHomeTile(int x, int y, int w, int h, const char* label,
                                const char* detail, uint16_t accent, UiIcon icon) {
  display_.fillRoundRect(x, y, w, h, theme::PanelRadius, theme::Surface);
  display_.drawRoundRect(x, y, w, h, theme::PanelRadius, theme::Divider);
  display_.fillRoundRect(x + 1, y + 1, 5, h - 2, 2, accent);
  drawIcon(icon, x + 22, y + (h / 2), accent);

  display_.setTextDatum(textdatum_t::top_left);
  display_.setFont(&fonts::Font2);
  display_.setTextColor(theme::TextPrimary, theme::Surface);
  drawTextFit(label, x + 48, y + 15, w - 56, theme::TextPrimary, theme::Surface);
  display_.setFont(&fonts::Font0);
  display_.setTextColor(theme::TextSecondary, theme::Surface);
  drawTextFit(detail, x + 48, y + 38, w - 56, theme::TextSecondary, theme::Surface);
}

void UiComponents::drawListItem(int x, int y, int w, int h, const char* label,
                                uint16_t accent, UiIcon icon, bool enabled) {
  const uint16_t surface = enabled ? theme::Surface : theme::Background;
  const uint16_t color = enabled ? accent : theme::Disabled;
  display_.fillRoundRect(x, y, w, h, theme::ButtonRadius, surface);
  display_.drawRoundRect(x, y, w, h, theme::ButtonRadius,
                         enabled ? theme::Divider : theme::SurfaceMuted);
  drawIcon(icon, x + 17, y + (h / 2), color);
  display_.setFont(&fonts::Font2);
  drawTextFit(label == nullptr ? "" : label, x + 34, y + 7, w - 45,
              enabled ? theme::TextPrimary : theme::Disabled, surface);
}

void UiComponents::drawIconButton(int x, int y, int w, int h, UiIcon icon,
                                  uint16_t accent, bool enabled) {
  const uint16_t surface = enabled ? theme::SurfaceRaised : theme::Surface;
  display_.fillRoundRect(x, y, w, h, theme::ButtonRadius, surface);
  display_.drawRoundRect(x, y, w, h, theme::ButtonRadius,
                         enabled ? theme::Divider : theme::SurfaceMuted);
  drawIcon(icon, x + (w / 2), y + (h / 2), enabled ? accent : theme::Disabled);
}

void UiComponents::drawTextFit(const String& text, int x, int y, int w,
                               uint16_t textColor, uint16_t background) {
  String fitted = text;
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(textColor, background);

  if (display_.textWidth(fitted) > w) {
    while (fitted.length() > 3 && display_.textWidth(fitted + "..") > w) {
      fitted.remove(fitted.length() - 1);
    }
    fitted += "..";
  }

  display_.drawString(fitted, x, y);
}

void UiComponents::formatTime(uint32_t ms, char* output, size_t outputSize) const {
  const uint32_t totalSeconds = ms / 1000;
  const uint32_t minutes = totalSeconds / 60;
  const uint32_t seconds = totalSeconds % 60;
  snprintf(output, outputSize, "%02u:%02u", static_cast<unsigned>(minutes),
           static_cast<unsigned>(seconds));
}

void UiComponents::drawCassette(int x, int y, int w, int h, uint16_t color) {
  const int centerY = y + (h * 9 / 20);
  const int leftHubX = x + (w * 3 / 10);
  const int rightHubX = x + (w * 7 / 10);
  const int hubRadius = w >= 34 ? 4 : 3;
  const int windowX = x + 4;
  const int windowY = y + 4;
  const int windowW = w - 8;
  const int windowH = h / 2;
  const int lowerY = y + h - 4;

  display_.fillRoundRect(x, y, w, h, 4, theme::SurfaceRaised);
  display_.drawRoundRect(x, y, w, h, 4, color);
  display_.fillRoundRect(windowX, windowY, windowW, windowH, 2, theme::Background);
  display_.drawRoundRect(windowX, windowY, windowW, windowH, 2, theme::Divider);

  display_.drawFastHLine(leftHubX, centerY, rightHubX - leftHubX, color);
  display_.fillCircle(leftHubX, centerY, hubRadius, color);
  display_.fillCircle(rightHubX, centerY, hubRadius, color);
  display_.fillCircle(leftHubX, centerY, hubRadius - 2, theme::Background);
  display_.fillCircle(rightHubX, centerY, hubRadius - 2, theme::Background);

  display_.drawLine(x + (w / 3), y + h - 9, x + (w / 4), lowerY, color);
  display_.drawFastHLine(x + (w / 4), lowerY, w / 2, color);
  display_.drawLine(x + (w * 2 / 3), y + h - 9, x + (w * 3 / 4), lowerY, color);
  display_.fillCircle(x + 4, y + h - 4, 1, theme::TextSecondary);
  display_.fillCircle(x + w - 5, y + h - 4, 1, theme::TextSecondary);
}

void UiComponents::drawSpectrumIcon(int x, int y, uint16_t color) {
  display_.fillRoundRect(x - 12, y - 9, 24, 18, 2, theme::SurfaceMuted);
  display_.drawRoundRect(x - 12, y - 9, 24, 18, 2, color);
  display_.fillRect(x - 9, y - 5, 18, 8, theme::Background);
  display_.drawFastHLine(x - 7, y - 3, 14, theme::TextSecondary);
  display_.drawFastHLine(x - 7, y, 14, theme::TextSecondary);
  display_.drawFastVLine(x - 3, y - 4, 7, theme::Divider);
  display_.drawFastVLine(x + 2, y - 4, 7, theme::Divider);

  display_.drawLine(x + 2, y + 4, x + 7, y + 8, theme::Danger);
  display_.drawLine(x + 4, y + 4, x + 9, y + 8, theme::Accent);
  display_.drawLine(x + 6, y + 4, x + 11, y + 8, theme::Success);
  display_.drawLine(x, y + 4, x + 5, y + 8, theme::Info);
}

void UiComponents::drawMsxIcon(int x, int y, uint16_t color) {
  const int top = y - 7;
  const int bottom = y + 7;
  const int middle = y;

  display_.drawFastVLine(x - 12, top, 15, color);
  display_.drawFastVLine(x - 6, top, 15, color);
  display_.drawLine(x - 11, top, x - 9, middle - 1, color);
  display_.drawLine(x - 7, top, x - 9, middle - 1, color);

  display_.drawFastHLine(x - 3, top, 7, color);
  display_.drawFastVLine(x - 3, top, 7, color);
  display_.drawFastHLine(x - 3, middle, 7, color);
  display_.drawFastVLine(x + 3, middle, 8, color);
  display_.drawFastHLine(x - 3, bottom, 7, color);

  display_.drawLine(x + 6, top, x + 12, bottom, color);
  display_.drawLine(x + 12, top, x + 6, bottom, color);
}

void UiComponents::drawIcon(UiIcon icon, int x, int y, uint16_t color) {
  switch (icon) {
    case UiIcon::Back:
      display_.drawLine(x + 5, y - 7, x - 3, y, color);
      display_.drawLine(x - 3, y, x + 5, y + 7, color);
      display_.drawFastHLine(x - 3, y, 13, color);
      break;
    case UiIcon::Cassette:
      drawCassette(x - 13, y - 9, 26, 18, color);
      break;
    case UiIcon::Computer:
      display_.drawRoundRect(x - 9, y - 8, 18, 13, 2, color);
      display_.drawFastHLine(x - 5, y + 8, 10, color);
      display_.drawFastVLine(x, y + 5, 4, color);
      break;
    case UiIcon::Spectrum:
      drawSpectrumIcon(x, y, color);
      break;
    case UiIcon::Msx:
      drawMsxIcon(x, y, color);
      break;
    case UiIcon::Wave:
      display_.drawLine(x - 10, y, x - 6, y, color);
      display_.drawLine(x - 6, y, x - 3, y - 7, color);
      display_.drawLine(x - 3, y - 7, x + 2, y + 7, color);
      display_.drawLine(x + 2, y + 7, x + 6, y, color);
      display_.drawLine(x + 6, y, x + 10, y, color);
      break;
    case UiIcon::Sliders:
      display_.drawFastHLine(x - 9, y - 6, 18, color);
      display_.drawFastHLine(x - 9, y, 18, color);
      display_.drawFastHLine(x - 9, y + 6, 18, color);
      display_.fillCircle(x - 3, y - 6, 2, color);
      display_.fillCircle(x + 4, y, 2, color);
      display_.fillCircle(x, y + 6, 2, color);
      break;
    case UiIcon::Folder:
      display_.fillRoundRect(x - 9, y - 5, 18, 12, 2, color);
      display_.fillRect(x - 7, y - 8, 8, 4, color);
      break;
    case UiIcon::File:
      display_.drawRect(x - 7, y - 9, 14, 18, color);
      display_.drawLine(x + 2, y - 9, x + 7, y - 4, color);
      break;
    case UiIcon::Wifi:
      display_.drawCircle(x, y + 6, 2, color);
      display_.drawArc(x, y + 6, 7, 6, 215, 325, color);
      display_.drawArc(x, y + 6, 11, 10, 215, 325, color);
      break;
    case UiIcon::Lock:
      display_.drawRoundRect(x - 7, y - 1, 14, 11, 2, color);
      display_.drawArc(x, y, 6, 5, 180, 360, color);
      break;
    case UiIcon::Play:
      display_.fillTriangle(x - 5, y - 8, x - 5, y + 8, x + 8, y, color);
      break;
    case UiIcon::Stop:
      display_.fillRect(x - 6, y - 6, 12, 12, color);
      break;
    case UiIcon::Refresh:
      display_.drawArc(x, y, 9, 8, 35, 315, color);
      display_.fillTriangle(x + 8, y - 5, x + 10, y + 2, x + 3, y, color);
      break;
    case UiIcon::Delete:
      display_.drawRect(x - 6, y - 5, 12, 13, color);
      display_.drawFastHLine(x - 8, y - 8, 16, color);
      display_.drawFastHLine(x - 3, y - 10, 6, color);
      break;
    case UiIcon::Connect:
      display_.drawFastHLine(x - 9, y, 15, color);
      display_.drawLine(x + 2, y - 5, x + 8, y, color);
      display_.drawLine(x + 8, y, x + 2, y + 5, color);
      break;
    case UiIcon::None:
    default:
      break;
  }
}

}  // namespace ui
