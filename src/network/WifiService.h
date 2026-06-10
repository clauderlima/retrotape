#pragma once

#include <Arduino.h>
#include <IPAddress.h>

namespace network {

struct WifiNetworkInfo {
  String ssid;
  int32_t rssi = 0;
  bool encrypted = false;
};

class WifiService {
 public:
  bool begin();
  bool connectAndSave(const String& ssid, const String& password);
  size_t scanNetworks(WifiNetworkInfo* networks, size_t maxNetworks);
  void clearCredentials();
  bool isConnected() const;
  bool isAccessPointMode() const;
  String ipAddressText() const;
  String footerText() const;
  String statusText() const;
  String currentSsid() const;

 private:
  bool loadCredentials(String& ssid, String& password);
  bool saveCredentials(const String& ssid, const String& password);
  bool connectStation(const String& ssid, const String& password);
  bool startFallbackAccessPoint();

  bool accessPointMode_ = false;
  String currentSsid_;
};

}  // namespace network
