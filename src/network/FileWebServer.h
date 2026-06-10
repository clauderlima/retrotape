#pragma once

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

#include "storage/SdCardService.h"
#include "network/WifiService.h"

namespace network {

class FileWebServer {
 public:
  FileWebServer(storage::SdCardService& storage, WifiService& wifi);

  bool begin();
  void loop();
  bool isRunning() const;
  String footerText() const;

 private:
  void configureRoutes();
  void handleRoot();
  void handleUploadComplete();
  void handleUpload();
  void handleNotFound();
  void redirectToRoot(const char* query);
  void sendPageHeader(const char* title);
  void sendPageFooter();
  void sendUploadForm();
  void sendFileList(const char* title, const char* directory, const char* extension);
  const char* directoryForPlatform(const String& platform) const;
  const char* extensionForPlatform(const String& platform) const;
  bool isAllowedExtension(const String& filename, const char* extension) const;
  String sanitizeFilename(String filename) const;
  String htmlEscape(String value) const;
  String sizeLabel(uint32_t bytes) const;

  storage::SdCardService& storage_;
  WifiService& wifi_;
  WebServer server_;
  File uploadFile_;
  String uploadPath_;
  bool running_ = false;
  bool uploadFailed_ = false;
  bool uploadHadFile_ = false;
};

}  // namespace network
