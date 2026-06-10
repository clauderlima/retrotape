#include <Arduino.h>

#include "app/AppController.h"
#include "audio/DacAudioOutput.h"
#include "metadata/GameMetadata.h"
#include "network/FileWebServer.h"
#include "network/IgdbService.h"
#include "network/WifiService.h"
#include "settings/SettingsService.h"
#include "storage/SdCardService.h"
#include "ui/UiService.h"

storage::SdCardService sdCard;
ui::UiService uiService;
audio::DacAudioOutput audioOutput;
network::WifiService wifiService;
metadata::GameMetadataService gameMetadataService;
network::IgdbService igdbService(gameMetadataService);
network::FileWebServer fileWebServer(sdCard, wifiService, igdbService,
                                     gameMetadataService);
settings::SettingsService settingsService;
app::AppController appController(sdCard, uiService, audioOutput, wifiService,
                                 fileWebServer, settingsService,
                                 gameMetadataService);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("RetroTape-ESP32-CYD boot");

  appController.begin();
}

void loop() {
  appController.loop();
  yield();
}
