#pragma once

#include <LovyanGFX.hpp>

namespace ui::theme {

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8U) << 8U) |
                               ((green & 0xFCU) << 3U) |
                               (blue >> 3U));
}

constexpr uint16_t Background = rgb565(12, 14, 16);
constexpr uint16_t Surface = rgb565(27, 31, 35);
constexpr uint16_t SurfaceRaised = rgb565(38, 43, 47);
constexpr uint16_t SurfaceMuted = rgb565(48, 54, 58);
constexpr uint16_t Primary = rgb565(38, 194, 160);
constexpr uint16_t Secondary = rgb565(241, 105, 87);
constexpr uint16_t Accent = rgb565(242, 190, 68);
constexpr uint16_t Info = rgb565(82, 151, 213);
constexpr uint16_t Success = rgb565(75, 184, 112);
constexpr uint16_t Warning = rgb565(238, 160, 55);
constexpr uint16_t Danger = rgb565(220, 75, 80);
constexpr uint16_t TextPrimary = rgb565(246, 243, 236);
constexpr uint16_t TextSecondary = rgb565(174, 180, 181);
constexpr uint16_t Disabled = rgb565(96, 102, 105);
constexpr uint16_t Divider = rgb565(59, 65, 69);
constexpr uint16_t ProgressTrack = rgb565(45, 50, 54);
constexpr uint16_t Progress = Primary;

constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 240;
constexpr int PageMargin = 12;
constexpr int HeaderHeight = 52;
constexpr int FooterHeight = 24;
constexpr int ButtonRadius = 6;
constexpr int PanelRadius = 6;
constexpr int MinimumTouchHeight = 28;

}  // namespace ui::theme
