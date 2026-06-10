#pragma once

#include <cstdint>

namespace config::pins {

// Common ESP32-2432S028R / Cheap Yellow Display pin map.
// Board revisions vary, so these values must be confirmed on the target unit.

constexpr int TftMiso = 12;
constexpr int TftMosi = 13;
constexpr int TftSclk = 14;
constexpr int TftCs = 15;
constexpr int TftDc = 2;
constexpr int TftRst = -1;
constexpr int TftBacklight = 21;

constexpr int TouchIrq = 36;
constexpr int TouchMosi = 32;
constexpr int TouchMiso = 39;
constexpr int TouchSclk = 25;
constexpr int TouchCs = 33;

constexpr int SdMiso = 19;
constexpr int SdMosi = 23;
constexpr int SdSclk = 18;
constexpr int SdCs = 5;
constexpr uint32_t SdInitialFrequency = 20000000UL;

constexpr int SpeakerPwm = 26;

constexpr int LedRed = 4;
constexpr int LedGreen = 16;
constexpr int LedBlue = 17;
constexpr int Ldr = 34;

}  // namespace config::pins

