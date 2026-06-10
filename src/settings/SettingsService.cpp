#include "settings/SettingsService.h"

#include <Preferences.h>

namespace settings {

namespace {
constexpr char PreferencesNamespace[] = "rt-settings";
constexpr char VersionKey[] = "version";
constexpr char TimingKey[] = "tap_timing";
constexpr char AmplitudeKey[] = "tap_level";
constexpr char InvertedKey[] = "tap_invert";
constexpr uint8_t SettingsVersion = 1;
constexpr uint16_t MinimumTimingPermille = 950;
constexpr uint16_t MaximumTimingPermille = 1050;
constexpr uint8_t MinimumAmplitude = 8;
constexpr uint8_t MaximumAmplitude = 120;
}  // namespace

bool SettingsService::begin() {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, true)) {
    Serial.println("Settings namespace not found; creating defaults");
    tapProfile_ = TapProfile{};
    return writeTapProfile();
  }

  const uint8_t version = preferences.getUChar(VersionKey, 0);
  TapProfile loaded;
  if (version == SettingsVersion) {
    loaded.timingPermille =
        preferences.getUShort(TimingKey, TapProfile::DefaultTimingPermille);
    loaded.amplitude =
        preferences.getUChar(AmplitudeKey, TapProfile::DefaultAmplitude);
    loaded.inverted =
        preferences.getBool(InvertedKey, TapProfile::DefaultInverted);
  }
  preferences.end();

  tapProfile_ = validated(loaded);
  const bool corrected =
      tapProfile_.timingPermille != loaded.timingPermille ||
      tapProfile_.amplitude != loaded.amplitude ||
      tapProfile_.inverted != loaded.inverted;
  if (version != SettingsVersion || corrected) {
    return writeTapProfile();
  }

  Serial.print("TAP settings loaded: timing=");
  Serial.print(tapProfile_.timingPermille);
  Serial.print(" level=");
  Serial.print(tapProfile_.amplitude);
  Serial.print(" inverted=");
  Serial.println(tapProfile_.inverted ? "yes" : "no");
  return true;
}

const TapProfile& SettingsService::tapProfile() const {
  return tapProfile_;
}

bool SettingsService::saveTapProfile(const TapProfile& profile) {
  const TapProfile next = validated(profile);
  if (next.timingPermille == tapProfile_.timingPermille &&
      next.amplitude == tapProfile_.amplitude &&
      next.inverted == tapProfile_.inverted) {
    return true;
  }

  const TapProfile previous = tapProfile_;
  tapProfile_ = next;
  if (writeTapProfile()) {
    return true;
  }
  tapProfile_ = previous;
  return false;
}

bool SettingsService::restoreTapDefaults() {
  const TapProfile previous = tapProfile_;
  tapProfile_ = TapProfile{};
  if (writeTapProfile()) {
    return true;
  }
  tapProfile_ = previous;
  return false;
}

TapProfile SettingsService::validated(const TapProfile& profile) {
  TapProfile result = profile;
  if (result.timingPermille < MinimumTimingPermille ||
      result.timingPermille > MaximumTimingPermille) {
    result.timingPermille = TapProfile::DefaultTimingPermille;
  }
  if (result.amplitude < MinimumAmplitude ||
      result.amplitude > MaximumAmplitude) {
    result.amplitude = TapProfile::DefaultAmplitude;
  }
  return result;
}

bool SettingsService::writeTapProfile() {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, false)) {
    Serial.println("Could not open settings storage");
    return false;
  }

  bool ok = true;
  ok = preferences.putUChar(VersionKey, SettingsVersion) > 0 && ok;
  ok = preferences.putUShort(TimingKey, tapProfile_.timingPermille) > 0 && ok;
  ok = preferences.putUChar(AmplitudeKey, tapProfile_.amplitude) > 0 && ok;
  ok = preferences.putBool(InvertedKey, tapProfile_.inverted) > 0 && ok;
  preferences.end();

  Serial.println(ok ? "TAP settings saved" : "TAP settings save failed");
  return ok;
}

}  // namespace settings
