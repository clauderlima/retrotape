#pragma once

#include <Arduino.h>

#include "app/BrowserNavigation.h"
#include "audio/AudioOutput.h"
#include "network/FileWebServer.h"
#include "network/WifiService.h"
#include "storage/SdCardService.h"
#include "tape/TapeFormatDetector.h"
#include "ui/UiService.h"

namespace app {

enum class AppState {
  Home,
  FileBrowser,
  Player,
  Settings,
  TapSettings,
  WifiList,
  WifiPassword,
  Error,
};

class AppController {
 public:
  AppController(storage::SdCardService& storage, ui::UiService& ui, audio::AudioOutput& audio,
                network::WifiService& wifi, network::FileWebServer& webServer);

  void begin();
  void loop();
  AppState state() const;

 private:
  static constexpr size_t MaxBrowserEntries = 48;
  static constexpr size_t BrowserRows = 4;
  static constexpr size_t MaxWifiNetworks = 16;
  static constexpr size_t WifiRows = 4;
  static constexpr size_t WifiKeysPerPage = 18;
  static constexpr size_t WifiKeyPageCount = 5;

  void setState(AppState nextState);
  void handleAction(ui::UiAction action);
  bool handleNavigationAction(ui::UiAction action);
  bool handleTapSettingsAction(ui::UiAction action);
  bool handleBrowserAction(ui::UiAction action);
  bool handlePlayerAction(ui::UiAction action);
  bool handleWifiAction(ui::UiAction action);
  void serviceAudio();
  void openBrowser(BrowserMode mode);
  void refreshBrowser();
  void showBrowser();
  void showPlayerScreen(const char* status, bool playing);
  void showTapSettingsScreen();
  void scanWifiNetworks(const char* status);
  void showWifiListScreen(const char* status);
  void showWifiPasswordScreen(const char* status);
  void selectWifiRow(size_t row);
  void appendWifiKey(ui::UiAction action);
  void connectSelectedWifi();
  uint8_t wifiKeyIndex(ui::UiAction action) const;
  const char* wifiKeyPage() const;
  void selectBrowserRow(size_t row);
  void goBack();
  void goParentDirectory();
  void setPlayerFile(const storage::FileEntry& entry);
  const char* selectedFormatName() const;
  void updateWebFooter(bool force = false);
  void printHeartbeat();

  storage::SdCardService& storage_;
  ui::UiService& ui_;
  audio::AudioOutput& audio_;
  network::WifiService& wifi_;
  network::FileWebServer& webServer_;
  AppState state_ = AppState::Home;
  BrowserNavigation browserNavigation_;
  bool storageReady_ = false;
  bool audioReady_ = false;
  bool wifiReady_ = false;
  bool webReady_ = false;
  String selectedName_;
  String selectedPath_;
  tape::TapeFormat selectedFormat_ = tape::TapeFormat::Unknown;
  storage::FileEntry browserEntries_[MaxBrowserEntries];
  network::WifiNetworkInfo wifiNetworks_[MaxWifiNetworks];
  size_t browserEntryCount_ = 0;
  size_t browserOffset_ = 0;
  size_t wifiNetworkCount_ = 0;
  size_t wifiOffset_ = 0;
  size_t wifiKeyPage_ = 0;
  String selectedWifiSsid_;
  String wifiPassword_;
  bool playerWasPlaying_ = false;
  uint32_t lastPlayerProgressMs_ = 0;
  uint32_t lastWebFooterMs_ = 0;
  uint32_t lastHeartbeatMs_ = 0;
  uint16_t tapTimingPermille_ = 1000;
  uint8_t tapAmplitude_ = 40;
  bool tapInverted_ = false;
};

}  // namespace app
