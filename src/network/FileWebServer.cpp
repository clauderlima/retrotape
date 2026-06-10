#include "network/FileWebServer.h"

#include <SD.h>
#include <cstring>

namespace network {

FileWebServer::FileWebServer(storage::SdCardService& storage, WifiService& wifi)
    : storage_(storage), wifi_(wifi), server_(80) {}

bool FileWebServer::begin() {
  if (!wifi_.isConnected() && !wifi_.isAccessPointMode()) {
    Serial.println("Web server not started: WiFi unavailable");
    return false;
  }

  if (storage_.isMounted()) {
    storage_.ensureStandardDirectories();
  }

  configureRoutes();
  server_.begin();
  running_ = true;

  Serial.print("Web server ready at http://");
  Serial.println(wifi_.ipAddressText());
  return true;
}

void FileWebServer::loop() {
  if (running_) {
    server_.handleClient();
  }
}

bool FileWebServer::isRunning() const {
  return running_;
}

String FileWebServer::footerText() const {
  if (!running_) {
    return "WiFi off";
  }
  return wifi_.footerText();
}

void FileWebServer::configureRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/upload", HTTP_POST, [this]() { handleUploadComplete(); }, [this]() { handleUpload(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void FileWebServer::handleRoot() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  sendPageHeader("RetroTape");

  if (server_.hasArg("ok")) {
    server_.sendContent("<p class=\"ok\">Upload completed.</p>");
  } else if (server_.hasArg("error")) {
    server_.sendContent("<p class=\"error\">Upload failed. Check the platform, file extension, and SD card.</p>");
  }

  server_.sendContent("<section><h2>Upload files</h2>");
  if (!storage_.isMounted()) {
    server_.sendContent("<p class=\"error\">SD card is not mounted.</p>");
  } else {
    sendUploadForm();
  }
  server_.sendContent("</section>");

  server_.sendContent("<section><h2>Files on the SD card</h2><div class=\"grid\">");
  sendFileList("MSX", "/msx", ".cas");
  sendFileList("TK90X / ZX", "/tk90x", ".tap");
  server_.sendContent("</div></section>");

  sendPageFooter();
  server_.sendContent("");
}

void FileWebServer::handleUploadComplete() {
  const bool ok = uploadHadFile_ && !uploadFailed_;
  uploadHadFile_ = false;
  uploadFailed_ = false;
  uploadPath_ = "";
  redirectToRoot(ok ? "ok=1" : "error=1");
}

void FileWebServer::handleUpload() {
  HTTPUpload& upload = server_.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (!uploadHadFile_) {
      uploadFailed_ = false;
    }
    uploadHadFile_ = true;
    uploadPath_ = "";

    if (!storage_.isMounted()) {
      uploadFailed_ = true;
      return;
    }

    String platform = server_.arg("platform");
    String filename = sanitizeFilename(upload.filename);
    if (platform.length() == 0) {
      String lowerName = filename;
      lowerName.toLowerCase();
      platform = lowerName.endsWith(".cas") ? "msx" : "tk90x";
    }

    const char* directory = directoryForPlatform(platform);
    const char* extension = extensionForPlatform(platform);
    if (directory == nullptr || extension == nullptr || !isAllowedExtension(filename, extension)) {
      uploadFailed_ = true;
      return;
    }

    storage_.ensureDirectory(directory);
    uploadPath_ = String(directory) + "/" + filename;
    if (SD.exists(uploadPath_)) {
      SD.remove(uploadPath_);
    }

    uploadFile_ = SD.open(uploadPath_, FILE_WRITE);
    if (!uploadFile_) {
      uploadFailed_ = true;
      uploadPath_ = "";
      return;
    }

    Serial.print("Upload started: ");
    Serial.println(uploadPath_);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile_ && !uploadFailed_) {
      const size_t written = uploadFile_.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        uploadFailed_ = true;
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile_) {
      uploadFile_.close();
    }
    if (uploadFailed_ && uploadPath_.length() > 0) {
      SD.remove(uploadPath_);
    }
    Serial.print(uploadFailed_ ? "Upload failed: " : "Upload finished: ");
    Serial.println(uploadPath_);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile_) {
      uploadFile_.close();
    }
    if (uploadPath_.length() > 0) {
      SD.remove(uploadPath_);
    }
    uploadFailed_ = true;
  }
}

void FileWebServer::handleNotFound() {
  server_.send(404, "text/plain", "Not found");
}

void FileWebServer::redirectToRoot(const char* query) {
  String location = "/";
  if (query != nullptr && query[0] != '\0') {
    location += "?";
    location += query;
  }
  server_.sendHeader("Location", location, true);
  server_.send(303, "text/plain", "");
}

void FileWebServer::sendPageHeader(const char* title) {
  server_.sendContent("<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  server_.sendContent("<meta charset=\"utf-8\"><title>");
  server_.sendContent(title);
  server_.sendContent("</title><style>");
  server_.sendContent("body{font-family:system-ui,Segoe UI,Arial,sans-serif;margin:0;background:#101418;color:#eef3f8}");
  server_.sendContent("header{padding:18px 20px;background:#1d6f67}main{max-width:880px;margin:auto;padding:18px}");
  server_.sendContent("section{background:#182029;border:1px solid #2c3a46;border-radius:8px;margin:0 0 16px;padding:16px}");
  server_.sendContent("h1,h2,h3{margin:0 0 12px}label{display:block;margin:10px 0 6px;color:#b9c8d4}");
  server_.sendContent("select,input,button{font:inherit}select,input[type=file]{width:100%;box-sizing:border-box;padding:10px;background:#0f151b;color:#eef3f8;border:1px solid #3b4c5b;border-radius:6px}");
  server_.sendContent("button{margin-top:12px;padding:10px 14px;border:0;border-radius:6px;background:#28a17b;color:white;font-weight:700}");
  server_.sendContent(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:14px}.panel{background:#101820;border:1px solid #2c3a46;border-radius:8px;padding:12px}");
  server_.sendContent("ul{list-style:none;margin:0;padding:0}li{display:flex;justify-content:space-between;gap:10px;padding:8px 0;border-bottom:1px solid #26323d}");
  server_.sendContent("small{color:#9fb1bf}.ok{color:#7ee6a6}.error{color:#ff9a9a}footer{color:#9fb1bf;padding:12px 20px;text-align:center}");
  server_.sendContent("</style></head><body><header><h1>RetroTape</h1><small>File server - ");
  server_.sendContent(wifi_.isAccessPointMode() ? "AP " : "WiFi ");
  server_.sendContent(wifi_.ipAddressText());
  server_.sendContent("</small></header><main>");
}

void FileWebServer::sendPageFooter() {
  server_.sendContent("</main><footer>MSX accepts .cas files; TK90X / ZX accepts .tap files.</footer></body></html>");
}

void FileWebServer::sendUploadForm() {
  server_.sendContent("<form method=\"post\" action=\"/upload\" enctype=\"multipart/form-data\">");
  server_.sendContent("<label for=\"platform\">Computer</label><select id=\"platform\" name=\"platform\">");
  server_.sendContent("<option value=\"msx\">MSX (.cas)</option><option value=\"tk90x\">TK90X / ZX (.tap)</option>");
  server_.sendContent("</select><label for=\"file\">Files</label><input id=\"file\" name=\"file\" type=\"file\" multiple accept=\".cas,.tap\">");
  server_.sendContent("<button type=\"submit\">Upload to SD card</button></form>");
}

void FileWebServer::sendFileList(const char* title, const char* directory, const char* extension) {
  server_.sendContent("<div class=\"panel\"><h3>");
  server_.sendContent(title);
  server_.sendContent("</h3>");

  if (!storage_.isMounted()) {
    server_.sendContent("<small>SD card is not mounted.</small></div>");
    return;
  }

  File dir = SD.open(directory);
  if (!dir || !dir.isDirectory()) {
    server_.sendContent("<small>Empty folder.</small></div>");
    return;
  }

  bool hasFiles = false;
  server_.sendContent("<ul>");
  File file = dir.openNextFile();
  while (file) {
    String name(file.name());
    const int slash = name.lastIndexOf('/');
    if (slash >= 0) {
      name = name.substring(slash + 1);
    }

    if (!file.isDirectory() && isAllowedExtension(name, extension)) {
      hasFiles = true;
      server_.sendContent("<li><span>");
      server_.sendContent(htmlEscape(name));
      server_.sendContent("</span><small>");
      server_.sendContent(sizeLabel(static_cast<uint32_t>(file.size())));
      server_.sendContent("</small></li>");
    }
    file.close();
    file = dir.openNextFile();
  }
  server_.sendContent("</ul>");

  if (!hasFiles) {
    server_.sendContent("<small>No files.</small>");
  }
  dir.close();
  server_.sendContent("</div>");
}

const char* FileWebServer::directoryForPlatform(const String& platform) const {
  if (platform == "msx") {
    return "/msx";
  }
  if (platform == "tk90x") {
    return "/tk90x";
  }
  return nullptr;
}

const char* FileWebServer::extensionForPlatform(const String& platform) const {
  if (platform == "msx") {
    return ".cas";
  }
  if (platform == "tk90x") {
    return ".tap";
  }
  return nullptr;
}

bool FileWebServer::isAllowedExtension(const String& filename, const char* extension) const {
  if (extension == nullptr) {
    return false;
  }
  String lowerName(filename);
  lowerName.toLowerCase();
  return lowerName.endsWith(extension);
}

String FileWebServer::sanitizeFilename(String filename) const {
  filename.replace("\\", "/");
  const int slash = filename.lastIndexOf('/');
  if (slash >= 0) {
    filename = filename.substring(slash + 1);
  }

  String sanitized;
  sanitized.reserve(filename.length());
  for (size_t i = 0; i < filename.length(); ++i) {
    const char c = filename.charAt(i);
    const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' ||
                         c == '_' || c == '-';
    sanitized += allowed ? c : '_';
  }

  if (sanitized.length() == 0 || sanitized == "." || sanitized == "..") {
    sanitized = "upload.bin";
  }
  return sanitized;
}

String FileWebServer::htmlEscape(String value) const {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

String FileWebServer::sizeLabel(uint32_t bytes) const {
  if (bytes >= 1024UL * 1024UL) {
    return String(bytes / (1024UL * 1024UL)) + " MB";
  }
  if (bytes >= 1024UL) {
    return String(bytes / 1024UL) + " KB";
  }
  return String(bytes) + " B";
}

}  // namespace network
