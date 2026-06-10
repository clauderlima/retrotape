#include "network/WifiService.h"

#include <Preferences.h>
#include <WiFi.h>

#include "config/wifi.h"

namespace network {

namespace {
constexpr char PreferencesNamespace[] = "retrotape";
constexpr char SsidKey[] = "ssid";
constexpr char PasswordKey[] = "pass";
}  // namespace

bool WifiService::begin() {
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setHostname(config::wifi::Hostname);

  String ssid;
  String password;
  if (loadCredentials(ssid, password) && connectStation(ssid, password)) {
    accessPointMode_ = false;
    currentSsid_ = ssid;
    return true;
  }

  return startFallbackAccessPoint();
}

bool WifiService::connectAndSave(const String& ssid, const String& password) {
  if (ssid.length() == 0) {
    return false;
  }

  WiFi.softAPdisconnect(true);
  if (!connectStation(ssid, password)) {
    startFallbackAccessPoint();
    return false;
  }

  accessPointMode_ = false;
  currentSsid_ = ssid;
  return saveCredentials(ssid, password);
}

size_t WifiService::scanNetworks(WifiNetworkInfo* networks, size_t maxNetworks) {
  if (networks == nullptr || maxNetworks == 0) {
    return 0;
  }

  WiFi.mode(accessPointMode_ ? WIFI_AP_STA : WIFI_STA);
  const int found = WiFi.scanNetworks(false, true);
  if (found <= 0) {
    return 0;
  }

  size_t count = 0;
  for (int i = 0; i < found && count < maxNetworks; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }

    bool duplicate = false;
    for (size_t existing = 0; existing < count; ++existing) {
      if (networks[existing].ssid == ssid) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    networks[count].ssid = ssid;
    networks[count].rssi = WiFi.RSSI(i);
    networks[count].encrypted = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    ++count;
  }

  WiFi.scanDelete();
  return count;
}

void WifiService::clearCredentials() {
  Preferences preferences;
  if (preferences.begin(PreferencesNamespace, false)) {
    preferences.remove(SsidKey);
    preferences.remove(PasswordKey);
    preferences.end();
  }
}

bool WifiService::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiService::isAccessPointMode() const {
  return accessPointMode_;
}

String WifiService::ipAddressText() const {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  if (accessPointMode_) {
    return WiFi.softAPIP().toString();
  }
  return "sem IP";
}

String WifiService::footerText() const {
  if (isConnected()) {
    return String("Web ") + ipAddressText();
  }
  if (accessPointMode_) {
    return String("AP ") + ipAddressText();
  }
  return "WiFi off";
}

String WifiService::statusText() const {
  if (isConnected()) {
    return String("WiFi: ") + currentSsid_ + " " + ipAddressText();
  }
  if (accessPointMode_) {
    return String("AP: ") + config::wifi::FallbackApSsid + " " + ipAddressText();
  }
  return "WiFi: desconectado";
}

String WifiService::currentSsid() const {
  return currentSsid_;
}

bool WifiService::loadCredentials(String& ssid, String& password) {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, true)) {
    return false;
  }

  ssid = preferences.getString(SsidKey, "");
  password = preferences.getString(PasswordKey, "");
  preferences.end();
  return ssid.length() > 0;
}

bool WifiService::saveCredentials(const String& ssid, const String& password) {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, false)) {
    return false;
  }

  preferences.putString(SsidKey, ssid);
  preferences.putString(PasswordKey, password);
  preferences.end();
  return true;
}

bool WifiService::connectStation(const String& ssid, const String& password) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config::wifi::Hostname);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting WiFi: ");
  Serial.println(ssid);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < config::wifi::ConnectTimeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    WiFi.disconnect(true);
    return false;
  }

  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool WifiService::startFallbackAccessPoint() {
  if (!config::wifi::EnableFallbackAccessPoint) {
    accessPointMode_ = false;
    return false;
  }

  WiFi.mode(WIFI_AP);
  const bool ok = WiFi.softAP(config::wifi::FallbackApSsid, config::wifi::FallbackApPassword);
  accessPointMode_ = ok;
  currentSsid_ = ok ? config::wifi::FallbackApSsid : "";

  if (ok) {
    Serial.print("Fallback AP ready: ");
    Serial.print(config::wifi::FallbackApSsid);
    Serial.print(" IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Fallback AP failed");
  }

  return ok;
}

}  // namespace network
