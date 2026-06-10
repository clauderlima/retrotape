#include "settings/SettingsService.h"

#include <Preferences.h>

namespace settings {

namespace {
constexpr char PreferencesNamespace[] = "rt-settings";
constexpr char VersionKey[] = "version";
constexpr char TimingKey[] = "tap_timing";
constexpr char AmplitudeKey[] = "tap_level";
constexpr char InvertedKey[] = "tap_invert";
constexpr char AudioVolumeKey[] = "audio_level";
constexpr uint8_t SettingsVersion = 1;
constexpr uint16_t MinimumTimingPermille = 950;
constexpr uint16_t MaximumTimingPermille = 1050;
constexpr uint8_t MinimumAmplitude = 8;
constexpr uint8_t MaximumAmplitude = 120;
constexpr uint8_t MinimumAudioVolume = 32;
}  // namespace

bool SettingsService::begin() {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, true)) {
    Serial.println("Settings namespace not found; creating defaults");
    tapProfile_ = TapProfile{};
    return writeSettings();
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
    audioVolume_ =
        preferences.getUChar(AudioVolumeKey, DefaultAudioVolume);
  }
  preferences.end();

  tapProfile_ = validated(loaded);
  const bool audioCorrected = audioVolume_ < MinimumAudioVolume;
  if (audioCorrected) {
    audioVolume_ = DefaultAudioVolume;
  }
  const bool corrected =
      tapProfile_.timingPermille != loaded.timingPermille ||
      tapProfile_.amplitude != loaded.amplitude ||
      tapProfile_.inverted != loaded.inverted;
  if (version != SettingsVersion || corrected || audioCorrected) {
    return writeSettings();
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
  if (writeSettings()) {
    return true;
  }
  tapProfile_ = previous;
  return false;
}

bool SettingsService::restoreTapDefaults() {
  const TapProfile previous = tapProfile_;
  tapProfile_ = TapProfile{};
  if (writeSettings()) {
    return true;
  }
  tapProfile_ = previous;
  return false;
}

uint8_t SettingsService::audioVolume() const {
  return audioVolume_;
}

bool SettingsService::saveAudioVolume(uint8_t volume) {
  const uint8_t next = volume < MinimumAudioVolume ? MinimumAudioVolume : volume;
  if (next == audioVolume_) {
    return true;
  }

  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, false)) {
    return false;
  }
  const bool ok = preferences.putUChar(AudioVolumeKey, next) > 0;
  preferences.end();
  if (ok) {
    audioVolume_ = next;
    Serial.println("Audio output level saved");
  }
  return ok;
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

bool SettingsService::writeSettings() {
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
  ok = preferences.putUChar(AudioVolumeKey, audioVolume_) > 0 && ok;
  preferences.end();

  Serial.println(ok ? "Settings saved" : "Settings save failed");
  return ok;
}

}  // namespace settings
