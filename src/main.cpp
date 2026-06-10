#include <Arduino.h>

#include "app/AppController.h"
#include "audio/DacAudioOutput.h"
#include "network/FileWebServer.h"
#include "network/WifiService.h"
#include "storage/SdCardService.h"
#include "ui/UiService.h"

storage::SdCardService sdCard;
ui::UiService uiService;
audio::DacAudioOutput audioOutput;
network::WifiService wifiService;
network::FileWebServer fileWebServer(sdCard, wifiService);
app::AppController appController(sdCard, uiService, audioOutput, wifiService, fileWebServer);

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
