#include "ui/UiComponents.h"

#include "ui/UiTheme.h"

namespace ui {

UiComponents::UiComponents(hardware::CydDisplay& display) : display_(display) {}

void UiComponents::clear() {
  display_.fillScreen(theme::Background);
}

void UiComponents::drawHeader(const char* title, const char* subtitle) {
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(theme::TextPrimary, theme::Background);
  display_.setFont(&fonts::Font2);
  display_.drawString(title, theme::PageMargin + 62, 10);

  display_.setTextColor(theme::TextSecondary, theme::Background);
  display_.drawString(subtitle, theme::PageMargin + 62, 30);
  display_.drawFastHLine(theme::PageMargin, 48,
                         display_.width() - (theme::PageMargin * 2), theme::Divider);
}

void UiComponents::drawButton(int x, int y, int w, int h, const char* label, uint16_t fill,
                              uint16_t textColor, bool enabled) {
  const uint16_t stroke = enabled ? theme::TextPrimary : theme::Disabled;
  const uint16_t text = enabled ? textColor : theme::Disabled;

  display_.fillRoundRect(x, y, w, h, theme::ButtonRadius, fill);
  display_.drawRoundRect(x, y, w, h, theme::ButtonRadius, stroke);

  String fitted = label;
  display_.setTextColor(text, fill);
  display_.setFont(&fonts::Font2);
  if (display_.textWidth(fitted) > w - 12) {
    while (fitted.length() > 3 && display_.textWidth(fitted + "..") > w - 12) {
      fitted.remove(fitted.length() - 1);
    }
    fitted += "..";
  }

  display_.setTextDatum(textdatum_t::middle_center);
  display_.drawString(fitted, x + (w / 2), y + (h / 2));
  display_.setTextDatum(textdatum_t::top_left);
}

void UiComponents::drawTextFit(const String& text, int x, int y, int w,
                               uint16_t textColor, uint16_t background) {
  String fitted = text;
  display_.setTextDatum(textdatum_t::top_left);
  display_.setTextColor(textColor, background);
  display_.setFont(&fonts::Font2);

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

}  // namespace ui
