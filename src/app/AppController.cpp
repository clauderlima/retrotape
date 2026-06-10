#include "app/AppController.h"

#include "ui/UiText.h"

namespace app {

namespace {
const char* const WifiKeyPages[] = {
    "abcdefghijklmnopqr",
    "stuvwxyz0123456789",
    "ABCDEFGHIJKLMNOPQR",
    "STUVWXYZ.-_@#*$/!&",
    "?+=:;,%()[]{}^~<>`",
};
}  // namespace

AppController::AppController(storage::SdCardService& storage, ui::UiService& ui, audio::AudioOutput& audio,
                             network::WifiService& wifi, network::FileWebServer& webServer)
    : storage_(storage), ui_(ui), audio_(audio), wifi_(wifi), webServer_(webServer) {}

void AppController::begin() {
  ui_.begin();
  ui_.showStatus("Starting services");

  audioReady_ = audio_.begin();
  audio_.setTapTimingPermille(tapTimingPermille_);
  audio_.setTapAmplitude(tapAmplitude_);
  audio_.setTapInverted(tapInverted_);
  storageReady_ = storage_.begin();
  wifiReady_ = wifi_.begin();
  webReady_ = webServer_.begin();
  updateWebFooter(true);

  if (!audioReady_) {
    ui_.showError("Audio not ready");
  }

  if (!storageReady_) {
    ui_.showError("SD not mounted");
  } else {
    storage_.ensureStandardDirectories();
    storage::FileEntry entries[8];
    const char* extensions[] = {".tap", ".tzx", ".cas", ".tsx", ".tsz", ".wav"};
    const size_t entryCount = storage_.listDirectory("/", entries, 8, extensions, 6);

    for (size_t i = 0; i < entryCount; ++i) {
      Serial.print(entries[i].isDirectory ? "[DIR] " : "[FILE] ");
      Serial.println(entries[i].path);
    }
  }

  setState(AppState::Home);
}

void AppController::loop() {
  serviceAudio();
  if (!audio_.isPlaying()) {
    webServer_.loop();
  }
  updateWebFooter();
  handleAction(ui_.pollAction());
  serviceAudio();
  printHeartbeat();
}

AppState AppController::state() const {
  return state_;
}

void AppController::setState(AppState nextState) {
  state_ = nextState;

  switch (state_) {
    case AppState::Home:
      ui_.showHome(storageReady_ ? ui::text::ChooseSystem : ui::text::SdNotMounted);
      break;
    case AppState::FileBrowser:
      showBrowser();
      break;
    case AppState::Player:
      showPlayerScreen(audio_.isPlaying() ? ui::text::Playing : ui::text::Ready,
                       audio_.isPlaying());
      break;
    case AppState::Settings:
      ui_.showSettings("TPM408-2.8 ILI9342", storageReady_, wifi_.statusText().c_str());
      break;
    case AppState::TapSettings:
      showTapSettingsScreen();
      break;
    case AppState::WifiList:
      showWifiListScreen(ui::text::SelectNetwork);
      break;
    case AppState::WifiPassword:
      showWifiPasswordScreen(ui::text::EnterPassword);
      break;
    case AppState::Error:
      ui_.showError(ui::text::ApplicationError);
      break;
  }
}

void AppController::handleAction(ui::UiAction action) {
  if (action == ui::UiAction::None) {
    return;
  }

  handleNavigationAction(action) || handleTapSettingsAction(action) ||
      handleBrowserAction(action) || handlePlayerAction(action) ||
      handleWifiAction(action);
}

bool AppController::handleNavigationAction(ui::UiAction action) {
  switch (action) {
    case ui::UiAction::OpenTk90x:
      openBrowser(BrowserMode::Tk90x);
      return true;
    case ui::UiAction::OpenMsx:
      openBrowser(BrowserMode::Msx);
      return true;
    case ui::UiAction::OpenWav:
      openBrowser(BrowserMode::Wav);
      return true;
    case ui::UiAction::OpenMenu:
      setState(AppState::Settings);
      return true;
    case ui::UiAction::OpenWifiSettings:
      scanWifiNetworks(ui::text::NetworksFound);
      return true;
    case ui::UiAction::OpenTapSettings:
      setState(AppState::TapSettings);
      return true;
    case ui::UiAction::Back:
      goBack();
      return true;
    default:
      return false;
  }
}

bool AppController::handleTapSettingsAction(ui::UiAction action) {
  switch (action) {
    case ui::UiAction::TapTimingDown:
      tapTimingPermille_ = tapTimingPermille_ > 950 ? tapTimingPermille_ - 5 : 950;
      audio_.setTapTimingPermille(tapTimingPermille_);
      showTapSettingsScreen();
      return true;
    case ui::UiAction::TapTimingUp:
      tapTimingPermille_ = tapTimingPermille_ < 1050 ? tapTimingPermille_ + 5 : 1050;
      audio_.setTapTimingPermille(tapTimingPermille_);
      showTapSettingsScreen();
      return true;
    case ui::UiAction::TapLevelDown:
      tapAmplitude_ = tapAmplitude_ > 16 ? tapAmplitude_ - 8 : 8;
      audio_.setTapAmplitude(tapAmplitude_);
      showTapSettingsScreen();
      return true;
    case ui::UiAction::TapLevelUp:
      tapAmplitude_ = tapAmplitude_ < 112 ? tapAmplitude_ + 8 : 120;
      audio_.setTapAmplitude(tapAmplitude_);
      showTapSettingsScreen();
      return true;
    case ui::UiAction::TapInvert:
      tapInverted_ = !tapInverted_;
      audio_.setTapInverted(tapInverted_);
      showTapSettingsScreen();
      return true;
    default:
      return false;
  }
}

bool AppController::handleBrowserAction(ui::UiAction action) {
  switch (action) {
    case ui::UiAction::BrowserPrevious:
      browserOffset_ = browserOffset_ > BrowserRows ? browserOffset_ - BrowserRows : 0;
      showBrowser();
      return true;
    case ui::UiAction::BrowserNext:
      if (browserOffset_ + BrowserRows < browserEntryCount_) {
        browserOffset_ += BrowserRows;
      }
      showBrowser();
      return true;
    case ui::UiAction::BrowserSelect0:
      selectBrowserRow(0);
      return true;
    case ui::UiAction::BrowserSelect1:
      selectBrowserRow(1);
      return true;
    case ui::UiAction::BrowserSelect2:
      selectBrowserRow(2);
      return true;
    case ui::UiAction::BrowserSelect3:
      selectBrowserRow(3);
      return true;
    default:
      return false;
  }
}

bool AppController::handlePlayerAction(ui::UiAction action) {
  switch (action) {
    case ui::UiAction::PlayerPlay:
      if (selectedFormat_ == tape::TapeFormat::Wav) {
        if (audio_.playWavFile(selectedPath_.c_str())) {
          playerWasPlaying_ = true;
          lastPlayerProgressMs_ = 0;
          showPlayerScreen("Playing WAV...", true);
        } else {
          showPlayerScreen("WAV error", false);
        }
        return true;
      }

      if (selectedFormat_ == tape::TapeFormat::Tap) {
        if (audio_.playTapFile(selectedPath_.c_str())) {
          playerWasPlaying_ = true;
          lastPlayerProgressMs_ = 0;
          showPlayerScreen("Playing TAP...", true);
        } else {
          showPlayerScreen("TAP error", false);
        }
        return true;
      }

      if (selectedFormat_ == tape::TapeFormat::Cas) {
        if (audio_.playCasFile(selectedPath_.c_str())) {
          playerWasPlaying_ = true;
          lastPlayerProgressMs_ = 0;
          showPlayerScreen("Playing CAS...", true);
        } else {
          showPlayerScreen("CAS error", false);
        }
        return true;
      }

      showPlayerScreen("Unsupported format", false);
      return true;
    case ui::UiAction::PlayerStop:
      audio_.stop();
      playerWasPlaying_ = false;
      showPlayerScreen(ui::text::Stopped, false);
      return true;
    default:
      return false;
  }
}

bool AppController::handleWifiAction(ui::UiAction action) {
  switch (action) {
    case ui::UiAction::WifiRescan:
      scanWifiNetworks(ui::text::NetworksUpdated);
      return true;
    case ui::UiAction::WifiPrevious:
      wifiOffset_ = wifiOffset_ > WifiRows ? wifiOffset_ - WifiRows : 0;
      showWifiListScreen(ui::text::SelectNetwork);
      return true;
    case ui::UiAction::WifiNext:
      if (wifiOffset_ + WifiRows < wifiNetworkCount_) {
        wifiOffset_ += WifiRows;
      }
      showWifiListScreen(ui::text::SelectNetwork);
      return true;
    case ui::UiAction::WifiSelect0:
      selectWifiRow(0);
      return true;
    case ui::UiAction::WifiSelect1:
      selectWifiRow(1);
      return true;
    case ui::UiAction::WifiSelect2:
      selectWifiRow(2);
      return true;
    case ui::UiAction::WifiSelect3:
      selectWifiRow(3);
      return true;
    case ui::UiAction::WifiKey0:
    case ui::UiAction::WifiKey1:
    case ui::UiAction::WifiKey2:
    case ui::UiAction::WifiKey3:
    case ui::UiAction::WifiKey4:
    case ui::UiAction::WifiKey5:
    case ui::UiAction::WifiKey6:
    case ui::UiAction::WifiKey7:
    case ui::UiAction::WifiKey8:
    case ui::UiAction::WifiKey9:
    case ui::UiAction::WifiKey10:
    case ui::UiAction::WifiKey11:
    case ui::UiAction::WifiKey12:
    case ui::UiAction::WifiKey13:
    case ui::UiAction::WifiKey14:
    case ui::UiAction::WifiKey15:
    case ui::UiAction::WifiKey16:
    case ui::UiAction::WifiKey17:
      appendWifiKey(action);
      return true;
    case ui::UiAction::WifiKeyboardNext:
      wifiKeyPage_ = (wifiKeyPage_ + 1) % WifiKeyPageCount;
      showWifiPasswordScreen(ui::text::EnterPassword);
      return true;
    case ui::UiAction::WifiBackspace:
      if (wifiPassword_.length() > 0) {
        wifiPassword_.remove(wifiPassword_.length() - 1);
      }
      showWifiPasswordScreen(ui::text::EnterPassword);
      return true;
    case ui::UiAction::WifiConnect:
      connectSelectedWifi();
      return true;
    default:
      return false;
  }
}

void AppController::serviceAudio() {
  audio_.update();

  if (state_ != AppState::Player) {
    playerWasPlaying_ = audio_.isPlaying();
    return;
  }

  const bool playing = audio_.isPlaying();
  const uint32_t now = millis();

  const uint32_t progressIntervalMs =
      selectedFormat_ == tape::TapeFormat::Tap ? 1000UL : 500UL;
  if (playing && now - lastPlayerProgressMs_ >= progressIntervalMs) {
    lastPlayerProgressMs_ = now;
    ui_.updatePlayerProgress(audio_.playbackElapsedMs(), audio_.playbackDurationMs(), true);
  }

  if (playerWasPlaying_ && !playing) {
    playerWasPlaying_ = false;
    showPlayerScreen(ui::text::Finished, false);
  }
}

void AppController::openBrowser(BrowserMode mode) {
  if (!storageReady_) {
    ui_.showHome(ui::text::SdNotMounted);
    return;
  }

  browserNavigation_.open(mode);
  browserOffset_ = 0;
  refreshBrowser();
  setState(AppState::FileBrowser);
}

void AppController::refreshBrowser() {
  browserEntryCount_ = 0;
  browserOffset_ = 0;

  if (!storageReady_) {
    return;
  }

  const char* const* extensions = nullptr;
  size_t extensionCount = 0;
  browserNavigation_.extensions(extensions, extensionCount);
  browserEntryCount_ =
      storage_.listDirectory(browserNavigation_.path().c_str(), browserEntries_,
                             MaxBrowserEntries, extensions, extensionCount);
}

void AppController::showBrowser() {
  const bool hasPrevious = browserOffset_ > 0;
  const bool hasNext = browserOffset_ + BrowserRows < browserEntryCount_;
  ui_.showFileBrowser(browserNavigation_.title(), browserNavigation_.path().c_str(),
                      browserEntries_, browserEntryCount_, browserOffset_, browserEntryCount_,
                      hasPrevious, hasNext);
}

void AppController::showPlayerScreen(const char* status, bool playing) {
  ui_.showPlayer(selectedName_.c_str(), selectedFormatName(), status, audio_.playbackElapsedMs(),
                 audio_.playbackDurationMs(), playing);
}

void AppController::showTapSettingsScreen() {
  ui_.showTapSettings(tapTimingPermille_, tapAmplitude_, tapInverted_);
}

void AppController::scanWifiNetworks(const char* status) {
  ui_.showStatus(ui::text::ScanNetworks);
  wifiNetworkCount_ = wifi_.scanNetworks(wifiNetworks_, MaxWifiNetworks);
  wifiOffset_ = 0;
  state_ = AppState::WifiList;
  showWifiListScreen(status);
}

void AppController::showWifiListScreen(const char* status) {
  const bool hasPrevious = wifiOffset_ > 0;
  const bool hasNext = wifiOffset_ + WifiRows < wifiNetworkCount_;
  ui_.showWifiList(wifiNetworks_, wifiNetworkCount_, wifiOffset_, wifiNetworkCount_, status, hasPrevious, hasNext);
}

void AppController::showWifiPasswordScreen(const char* status) {
  ui_.showWifiPassword(selectedWifiSsid_.c_str(), wifiPassword_.c_str(), wifiKeyPage(), status);
}

void AppController::selectWifiRow(size_t row) {
  const size_t index = wifiOffset_ + row;
  if (index >= wifiNetworkCount_) {
    return;
  }

  selectedWifiSsid_ = wifiNetworks_[index].ssid;
  wifiPassword_ = "";
  wifiKeyPage_ = 0;
  state_ = AppState::WifiPassword;
  showWifiPasswordScreen(ui::text::EnterPassword);
}

void AppController::appendWifiKey(ui::UiAction action) {
  if (wifiPassword_.length() >= 63) {
    showWifiPasswordScreen(ui::text::PasswordTooLong);
    return;
  }

  const uint8_t index = wifiKeyIndex(action);
  const char* page = wifiKeyPage();
  if (index < WifiKeysPerPage && page[index] != '\0') {
    wifiPassword_ += page[index];
  }
  showWifiPasswordScreen(ui::text::EnterPassword);
}

void AppController::connectSelectedWifi() {
  showWifiPasswordScreen(ui::text::Connecting);
  const bool connected = wifi_.connectAndSave(selectedWifiSsid_, wifiPassword_);
  updateWebFooter(true);
  if (!webReady_) {
    webReady_ = webServer_.begin();
  }
  state_ = AppState::Settings;
  ui_.showSettings("TPM408-2.8 ILI9342", storageReady_, wifi_.statusText().c_str());
  ui_.showStatus(connected ? ui::text::WifiConnected : ui::text::WifiFailed);
}

uint8_t AppController::wifiKeyIndex(ui::UiAction action) const {
  const uint8_t first = static_cast<uint8_t>(ui::UiAction::WifiKey0);
  const uint8_t value = static_cast<uint8_t>(action);
  if (value < first || value > static_cast<uint8_t>(ui::UiAction::WifiKey17)) {
    return 255;
  }
  return value - first;
}

const char* AppController::wifiKeyPage() const {
  return WifiKeyPages[wifiKeyPage_ % WifiKeyPageCount];
}

void AppController::selectBrowserRow(size_t row) {
  const size_t index = browserOffset_ + row;
  if (index >= browserEntryCount_) {
    return;
  }

  const storage::FileEntry& entry = browserEntries_[index];
  if (entry.isDirectory) {
    browserNavigation_.enter(entry.path);
    refreshBrowser();
    setState(AppState::FileBrowser);
    return;
  }

  setPlayerFile(entry);
  setState(AppState::Player);
}

void AppController::goBack() {
  switch (state_) {
    case AppState::FileBrowser:
      goParentDirectory();
      break;
    case AppState::Player:
      audio_.stop();
      playerWasPlaying_ = false;
      setState(AppState::FileBrowser);
      break;
    case AppState::Settings:
      setState(AppState::Home);
      break;
    case AppState::TapSettings:
      setState(AppState::Settings);
      break;
    case AppState::WifiList:
      setState(AppState::Settings);
      break;
    case AppState::WifiPassword:
      setState(AppState::WifiList);
      break;
    case AppState::Error:
      setState(AppState::Home);
      break;
    case AppState::Home:
    default:
      break;
  }
}

void AppController::goParentDirectory() {
  if (!browserNavigation_.goToParent()) {
    setState(AppState::Home);
    return;
  }

  refreshBrowser();
  setState(AppState::FileBrowser);
}

void AppController::setPlayerFile(const storage::FileEntry& entry) {
  selectedName_ = entry.name;
  selectedPath_ = entry.path;
  selectedFormat_ = tape::TapeFormatDetector::detectFromPath(selectedPath_);

  Serial.print("Selected file: ");
  Serial.println(selectedPath_);
}

const char* AppController::selectedFormatName() const {
  return tape::TapeFormatDetector::toString(selectedFormat_);
}

void AppController::updateWebFooter(bool force) {
  const uint32_t now = millis();
  if (!force && now - lastWebFooterMs_ < 3000) {
    return;
  }

  lastWebFooterMs_ = now;
  ui_.setFooterSuffix(webReady_ ? webServer_.footerText().c_str() : wifi_.footerText().c_str());
}

void AppController::printHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatMs_ < 5000) {
    return;
  }

  lastHeartbeatMs_ = now;
  Serial.println("RetroTape heartbeat");
}

}  // namespace app
