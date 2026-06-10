#pragma once

#include <Arduino.h>

namespace settings {

struct TapProfile {
  static constexpr uint16_t DefaultTimingPermille = 1000;
  static constexpr uint8_t DefaultAmplitude = 40;
  static constexpr bool DefaultInverted = false;

  uint16_t timingPermille = DefaultTimingPermille;
  uint8_t amplitude = DefaultAmplitude;
  bool inverted = DefaultInverted;
};

class SettingsService {
 public:
  static constexpr uint8_t DefaultAudioVolume = 180;

  bool begin();
  const TapProfile& tapProfile() const;
  bool saveTapProfile(const TapProfile& profile);
  bool restoreTapDefaults();
  uint8_t audioVolume() const;
  bool saveAudioVolume(uint8_t volume);

 private:
  static TapProfile validated(const TapProfile& profile);
  bool writeSettings();

  TapProfile tapProfile_;
  uint8_t audioVolume_ = DefaultAudioVolume;
};

}  // namespace settings
