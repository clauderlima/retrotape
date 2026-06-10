#pragma once

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

#include "metadata/GameMetadata.h"
#include "network/IgdbService.h"
#include "network/WifiService.h"
#include "storage/SdCardService.h"

namespace network {

class FileWebServer {
 public:
  FileWebServer(storage::SdCardService& storage, WifiService& wifi,
                IgdbService& igdb,
                metadata::GameMetadataService& metadata);

  bool begin();
  void loop();
  bool isRunning() const;
  String footerText() const;

 private:
  void configureRoutes();
  void handleRoot();
  void handleUploadComplete();
  void handleUpload();
  void handleIgdbSettings();
  void handleIgdbSettingsSave();
  void handleIgdbClear();
  void handleIdentify();
  void handleIdentifySave();
  void handleNotFound();
  void redirectToRoot(const char* query);
  void redirectTo(const String& location);
  void sendPageHeader(const char* title);
  void sendPageFooter();
  void sendUploadForm();
  void sendFileList(const char* title, const char* directory, const char* extension);
  void sendIgdbStatus();
  void sendGameCandidate(const metadata::GameMetadata& game,
                         const String& gamePath);
  const char* directoryForPlatform(const String& platform) const;
  const char* extensionForPlatform(const String& platform) const;
  String platformForPath(const String& path) const;
  bool isValidGamePath(const String& path) const;
  bool isAllowedExtension(const String& filename, const char* extension) const;
  String sanitizeFilename(String filename) const;
  String searchNameFromPath(String path) const;
  String htmlEscape(String value) const;
  String urlEncode(const String& value) const;
  String sizeLabel(uint32_t bytes) const;

  storage::SdCardService& storage_;
  WifiService& wifi_;
  IgdbService& igdb_;
  metadata::GameMetadataService& metadata_;
  WebServer server_;
  File uploadFile_;
  String uploadPath_;
  String lastUploadedPath_;
  size_t uploadedFileCount_ = 0;
  bool running_ = false;
  bool uploadFailed_ = false;
  bool uploadHadFile_ = false;
};

}  // namespace network
