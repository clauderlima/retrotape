#pragma once

#include <LovyanGFX.hpp>

namespace ui::theme {

constexpr uint16_t Background = TFT_BLACK;
constexpr uint16_t Surface = 0x18E3;
constexpr uint16_t SurfaceRaised = 0x2145;
constexpr uint16_t Primary = TFT_DARKCYAN;
constexpr uint16_t Success = TFT_DARKGREEN;
constexpr uint16_t Warning = TFT_ORANGE;
constexpr uint16_t Danger = TFT_MAROON;
constexpr uint16_t TextPrimary = TFT_WHITE;
constexpr uint16_t TextSecondary = TFT_LIGHTGREY;
constexpr uint16_t Disabled = TFT_DARKGREY;
constexpr uint16_t Divider = TFT_DARKGREY;
constexpr uint16_t Progress = TFT_GREEN;

constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 240;
constexpr int PageMargin = 12;
constexpr int HeaderHeight = 50;
constexpr int FooterHeight = 24;
constexpr int ButtonRadius = 5;
constexpr int MinimumTouchHeight = 28;

}  // namespace ui::theme
