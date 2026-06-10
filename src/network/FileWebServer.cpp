#include "network/FileWebServer.h"

#include <SD.h>
#include <cstring>

namespace network {

FileWebServer::FileWebServer(storage::SdCardService& storage, WifiService& wifi,
                             IgdbService& igdb,
                             metadata::GameMetadataService& metadata)
    : storage_(storage),
      wifi_(wifi),
      igdb_(igdb),
      metadata_(metadata),
      server_(80) {}

bool FileWebServer::begin() {
  if (!wifi_.isConnected() && !wifi_.isAccessPointMode()) {
    Serial.println("Web server not started: WiFi unavailable");
    return false;
  }

  if (storage_.isMounted()) {
    storage_.ensureStandardDirectories();
    metadata_.begin();
  }
  igdb_.begin();

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
  server_.on("/igdb", HTTP_GET, [this]() { handleIgdbSettings(); });
  server_.on("/igdb/save", HTTP_POST, [this]() { handleIgdbSettingsSave(); });
  server_.on("/igdb/clear", HTTP_POST, [this]() { handleIgdbClear(); });
  server_.on("/identify", HTTP_GET, [this]() { handleIdentify(); });
  server_.on("/identify/save", HTTP_POST, [this]() { handleIdentifySave(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void FileWebServer::handleRoot() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  sendPageHeader("RetroTape");

  if (server_.hasArg("ok")) {
    server_.sendContent("<p class=\"ok\">Upload completed.</p>");
  } else if (server_.hasArg("enriched")) {
    server_.sendContent("<p class=\"ok\">Game information and cover saved.</p>");
  } else if (server_.hasArg("igdb_saved")) {
    server_.sendContent("<p class=\"ok\">Personal IGDB credentials saved.</p>");
  } else if (server_.hasArg("igdb_cleared")) {
    server_.sendContent("<p class=\"ok\">IGDB credentials removed.</p>");
  } else if (server_.hasArg("error")) {
    server_.sendContent("<p class=\"error\">Upload failed. Check the platform, file extension, and SD card.</p>");
  } else if (server_.hasArg("metadata_error")) {
    server_.sendContent("<p class=\"error\">Game information could not be saved.</p>");
  } else if (server_.hasArg("igdb_error")) {
    server_.sendContent("<p class=\"error\">The IGDB credentials were not valid.</p>");
  }

  sendIgdbStatus();

  server_.sendContent("<section><h2>Upload files</h2>");
  if (!storage_.isMounted()) {
    server_.sendContent("<p class=\"error\">SD card is not mounted.</p>");
  } else {
    sendUploadForm();
  }
  server_.sendContent("</section>");

  server_.sendContent("<section><h2>Files on the SD card</h2><div class=\"grid\">");
  sendFileList("MSX", "/msx", ".cas");
  sendFileList("ZX Spectrum", "/tk90x", ".tap");
  server_.sendContent("</div></section>");

  sendPageFooter();
  server_.sendContent("");
}

void FileWebServer::handleUploadComplete() {
  const bool ok = uploadHadFile_ && !uploadFailed_;
  const String identifyPath =
      ok && uploadedFileCount_ == 1 ? lastUploadedPath_ : "";
  uploadHadFile_ = false;
  uploadFailed_ = false;
  uploadPath_ = "";
  uploadedFileCount_ = 0;
  lastUploadedPath_ = "";

  if (identifyPath.length() > 0 && igdb_.isConfigured() &&
      wifi_.isConnected()) {
    redirectTo("/identify?path=" + urlEncode(identifyPath));
    return;
  }
  redirectToRoot(ok ? "ok=1" : "error=1");
}

void FileWebServer::handleUpload() {
  HTTPUpload& upload = server_.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (!uploadHadFile_) {
      uploadFailed_ = false;
      uploadedFileCount_ = 0;
      lastUploadedPath_ = "";
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
    } else if (uploadPath_.length() > 0) {
      lastUploadedPath_ = uploadPath_;
      ++uploadedFileCount_;
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

void FileWebServer::handleIgdbSettings() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  sendPageHeader("IGDB settings");

  server_.sendContent("<section><div class=\"section-head\"><div><h2>Personal IGDB integration</h2>");
  server_.sendContent("<p class=\"muted\">Credentials stay in this ESP32 and are never added to the RetroTape repository.</p></div>");
  server_.sendContent("<a class=\"link-button secondary\" href=\"/\">Back</a></div>");
  server_.sendContent("<p>Create a Twitch application, then enter its Client ID and Client Secret below. Use this page only on a trusted local network because the RetroTape page itself uses local HTTP.</p>");
  server_.sendContent("<p><a href=\"https://dev.twitch.tv/console/apps/create\" target=\"_blank\" rel=\"noreferrer\">Open Twitch application registration</a></p>");
  server_.sendContent("<form method=\"post\" action=\"/igdb/save\">");
  server_.sendContent("<label for=\"client_id\">Twitch Client ID</label><input id=\"client_id\" name=\"client_id\" autocomplete=\"off\" required>");
  server_.sendContent("<label for=\"client_secret\">Twitch Client Secret</label><input id=\"client_secret\" name=\"client_secret\" type=\"password\" autocomplete=\"new-password\" required>");
  server_.sendContent("<button type=\"submit\">Save personal credentials</button></form>");
  if (igdb_.isConfigured()) {
    server_.sendContent("<form method=\"post\" action=\"/igdb/clear\"><button class=\"danger\" type=\"submit\">Remove IGDB credentials</button></form>");
  }
  server_.sendContent("</section>");
  sendPageFooter();
  server_.sendContent("");
}

void FileWebServer::handleIgdbSettingsSave() {
  const bool ok =
      igdb_.saveCredentials(server_.arg("client_id"),
                            server_.arg("client_secret"));
  redirectToRoot(ok ? "igdb_saved=1" : "igdb_error=1");
}

void FileWebServer::handleIgdbClear() {
  const bool ok = igdb_.clearCredentials();
  redirectToRoot(ok ? "igdb_cleared=1" : "metadata_error=1");
}

void FileWebServer::handleIdentify() {
  const String gamePath = server_.arg("path");
  if (!isValidGamePath(gamePath)) {
    server_.send(400, "text/plain", "Invalid game path");
    return;
  }

  String query = server_.arg("q");
  if (query.length() == 0) {
    query = searchNameFromPath(gamePath);
  }

  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  sendPageHeader("Identify game");
  server_.sendContent("<section><div class=\"section-head\"><div><h2>Identify game</h2><p class=\"muted\">");
  server_.sendContent(htmlEscape(gamePath));
  server_.sendContent("</p></div><a class=\"link-button secondary\" href=\"/\">Skip</a></div>");
  server_.sendContent("<form method=\"get\" action=\"/identify\"><input type=\"hidden\" name=\"path\" value=\"");
  server_.sendContent(htmlEscape(gamePath));
  server_.sendContent("\"><label for=\"q\">Search title</label><div class=\"search-row\"><input id=\"q\" name=\"q\" value=\"");
  server_.sendContent(htmlEscape(query));
  server_.sendContent("\" required><button type=\"submit\">Search IGDB</button></div></form></section>");

  if (!igdb_.isConfigured()) {
    server_.sendContent("<section><p class=\"error\">Configure personal IGDB credentials before searching.</p><a class=\"link-button\" href=\"/igdb\">Configure IGDB</a></section>");
  } else if (!wifi_.isConnected()) {
    server_.sendContent("<section><p class=\"error\">Internet access is unavailable. The game file is already stored on the SD card.</p></section>");
  } else {
    metadata::GameMetadata results[5];
    String error;
    const size_t count =
        igdb_.searchGames(query, platformForPath(gamePath), results, 5, error);
    server_.sendContent("<section><h2>Possible matches</h2><div class=\"games\">");
    for (size_t index = 0; index < count; ++index) {
      sendGameCandidate(results[index], gamePath);
    }
    server_.sendContent("</div>");
    if (count == 0) {
      server_.sendContent("<p class=\"muted\">");
      server_.sendContent(error.length() > 0 ? htmlEscape(error)
                                             : "No matching games found.");
      server_.sendContent("</p>");
    }
    server_.sendContent("</section>");
  }

  sendPageFooter();
  server_.sendContent("");
}

void FileWebServer::handleIdentifySave() {
  const String gamePath = server_.arg("path");
  const uint32_t gameId =
      static_cast<uint32_t>(server_.arg("game_id").toInt());
  if (!isValidGamePath(gamePath) || gameId == 0) {
    redirectToRoot("metadata_error=1");
    return;
  }

  String error;
  const bool ok =
      igdb_.enrichGame(gamePath, platformForPath(gamePath), gameId, error);
  if (!ok) {
    Serial.print("IGDB enrichment failed: ");
    Serial.println(error);
  }
  redirectToRoot(ok ? "enriched=1" : "metadata_error=1");
}

void FileWebServer::handleNotFound() {
  server_.send(404, "text/plain", "Not found");
}

void FileWebServer::redirectTo(const String& location) {
  server_.sendHeader("Location", location, true);
  server_.send(303, "text/plain", "");
}

void FileWebServer::redirectToRoot(const char* query) {
  String location = "/";
  if (query != nullptr && query[0] != '\0') {
    location += "?";
    location += query;
  }
  redirectTo(location);
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
  server_.sendContent("select,input,button{font:inherit}select,input{width:100%;box-sizing:border-box;padding:10px;background:#0f151b;color:#eef3f8;border:1px solid #3b4c5b;border-radius:6px}");
  server_.sendContent("button{margin-top:12px;padding:10px 14px;border:0;border-radius:6px;background:#28a17b;color:white;font-weight:700}");
  server_.sendContent("button.danger{background:#ad3f49}.link-button{display:inline-block;padding:9px 12px;border-radius:6px;background:#28a17b;color:white;text-decoration:none;font-weight:700}.link-button.secondary{background:#354451}");
  server_.sendContent(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:14px}.panel{background:#101820;border:1px solid #2c3a46;border-radius:8px;padding:12px}");
  server_.sendContent("ul{list-style:none;margin:0;padding:0}li{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:9px 0;border-bottom:1px solid #26323d}.file-title{display:block}.file-name{display:block;color:#8294a2;font-size:.78rem;margin-top:2px}");
  server_.sendContent(".section-head,.file-actions,.search-row{display:flex;align-items:center;justify-content:space-between;gap:12px}.search-row input{flex:1}.search-row button{width:auto;margin:0}.games{display:grid;grid-template-columns:repeat(auto-fit,minmax(245px,1fr));gap:12px}.game{display:grid;grid-template-columns:72px 1fr;gap:12px;background:#101820;border:1px solid #2c3a46;border-radius:8px;padding:10px}.game img{width:72px;height:102px;object-fit:cover;background:#26323d;border-radius:4px}.game button{width:100%}");
  server_.sendContent("small,.muted{color:#9fb1bf}.ok{color:#7ee6a6}.error{color:#ff9a9a}footer{color:#9fb1bf;padding:12px 20px;text-align:center}@media(max-width:520px){.section-head{align-items:flex-start}.search-row{display:block}.search-row button{width:100%;margin-top:10px}}");
  server_.sendContent("</style></head><body><header><h1>RetroTape</h1><small>File server - ");
  server_.sendContent(wifi_.isAccessPointMode() ? "AP " : "WiFi ");
  server_.sendContent(wifi_.ipAddressText());
  server_.sendContent("</small></header><main>");
}

void FileWebServer::sendPageFooter() {
  server_.sendContent("</main><footer>MSX accepts .cas files; ZX Spectrum accepts .tap files.</footer></body></html>");
}

void FileWebServer::sendUploadForm() {
  server_.sendContent("<form method=\"post\" action=\"/upload\" enctype=\"multipart/form-data\">");
  server_.sendContent("<label for=\"platform\">Computer</label><select id=\"platform\" name=\"platform\">");
  server_.sendContent("<option value=\"msx\">MSX (.cas)</option><option value=\"tk90x\">ZX Spectrum (.tap)</option>");
  server_.sendContent("</select><label for=\"file\">Files</label><input id=\"file\" name=\"file\" type=\"file\" multiple accept=\".cas,.tap\">");
  server_.sendContent("<small>When one game is uploaded, RetroTape opens the IGDB identification step automatically.</small>");
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
      String gamePath(directory);
      if (!gamePath.endsWith("/")) {
        gamePath += "/";
      }
      gamePath += name;
      metadata::GameMetadata game;
      const bool hasMetadata = metadata_.loadForGame(gamePath, game);

      server_.sendContent("<li><span><strong class=\"file-title\">");
      server_.sendContent(htmlEscape(hasMetadata ? game.title : name));
      server_.sendContent("</strong>");
      if (hasMetadata) {
        server_.sendContent("<small class=\"file-name\">");
        server_.sendContent(htmlEscape(name));
        if (game.year > 0) {
          server_.sendContent(" - ");
          server_.sendContent(String(game.year));
        }
        server_.sendContent("</small>");
      }
      server_.sendContent("</span><span class=\"file-actions\"><small>");
      server_.sendContent(sizeLabel(static_cast<uint32_t>(file.size())));
      server_.sendContent("</small><a class=\"link-button secondary\" href=\"/identify?path=");
      server_.sendContent(urlEncode(gamePath));
      server_.sendContent("\">");
      server_.sendContent(hasMetadata ? "Change" : "Identify");
      server_.sendContent("</a></span></li>");
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

void FileWebServer::sendIgdbStatus() {
  server_.sendContent("<section><div class=\"section-head\"><div><h2>Game library</h2><p class=\"muted\">IGDB: ");
  server_.sendContent(igdb_.isConfigured()
                          ? "configured (" + htmlEscape(igdb_.clientIdLabel()) + ")"
                          : "not configured");
  server_.sendContent("</p></div><a class=\"link-button secondary\" href=\"/igdb\">");
  server_.sendContent(igdb_.isConfigured() ? "IGDB settings" : "Configure IGDB");
  server_.sendContent("</a></div></section>");
}

void FileWebServer::sendGameCandidate(
    const metadata::GameMetadata& game, const String& gamePath) {
  server_.sendContent("<article class=\"game\">");
  if (game.coverImageId.length() > 0) {
    server_.sendContent("<img loading=\"lazy\" src=\"https://images.igdb.com/igdb/image/upload/t_cover_small/");
    server_.sendContent(htmlEscape(game.coverImageId));
    server_.sendContent(".jpg\" alt=\"\">");
  } else {
    server_.sendContent("<div></div>");
  }
  server_.sendContent("<div><h3>");
  server_.sendContent(htmlEscape(game.title));
  server_.sendContent("</h3><small>");
  if (game.year > 0) {
    server_.sendContent(String(game.year));
  }
  if (game.developer.length() > 0) {
    server_.sendContent(game.year > 0 ? " - " : "");
    server_.sendContent(htmlEscape(game.developer));
  }
  server_.sendContent("</small>");
  if (game.genres.length() > 0) {
    server_.sendContent("<p class=\"muted\">");
    server_.sendContent(htmlEscape(game.genres));
    server_.sendContent("</p>");
  }
  server_.sendContent("<form method=\"post\" action=\"/identify/save\"><input type=\"hidden\" name=\"path\" value=\"");
  server_.sendContent(htmlEscape(gamePath));
  server_.sendContent("\"><input type=\"hidden\" name=\"game_id\" value=\"");
  server_.sendContent(String(game.igdbId));
  server_.sendContent("\"><button type=\"submit\">Use this game</button></form></div></article>");
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

String FileWebServer::platformForPath(const String& path) const {
  if (path.startsWith("/msx/")) {
    return "msx";
  }
  if (path.startsWith("/tk90x/")) {
    return "tk90x";
  }
  return "";
}

bool FileWebServer::isValidGamePath(const String& path) const {
  if (path.indexOf("..") >= 0 || !SD.exists(path)) {
    return false;
  }
  const String platform = platformForPath(path);
  const char* extension = extensionForPlatform(platform);
  return extension != nullptr && isAllowedExtension(path, extension);
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

String FileWebServer::searchNameFromPath(String path) const {
  const int slash = path.lastIndexOf('/');
  if (slash >= 0) {
    path = path.substring(slash + 1);
  }
  const int dot = path.lastIndexOf('.');
  if (dot > 0) {
    path = path.substring(0, dot);
  }

  String cleaned;
  bool insideTag = false;
  for (size_t index = 0; index < path.length(); ++index) {
    const char value = path.charAt(index);
    if (value == '[' || value == '{') {
      insideTag = true;
      continue;
    }
    if (value == ']' || value == '}') {
      insideTag = false;
      continue;
    }
    if (insideTag) {
      continue;
    }
    cleaned += value == '_' || value == '-' ? ' ' : value;
  }
  while (cleaned.indexOf("  ") >= 0) {
    cleaned.replace("  ", " ");
  }
  cleaned.trim();
  return cleaned;
}

String FileWebServer::htmlEscape(String value) const {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

String FileWebServer::urlEncode(const String& value) const {
  String encoded;
  const char hex[] = "0123456789ABCDEF";
  for (size_t index = 0; index < value.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(value.charAt(index));
    const bool safe = (character >= 'a' && character <= 'z') ||
                      (character >= 'A' && character <= 'Z') ||
                      (character >= '0' && character <= '9') ||
                      character == '-' || character == '_' || character == '.' ||
                      character == '~';
    if (safe) {
      encoded += static_cast<char>(character);
    } else {
      encoded += '%';
      encoded += hex[character >> 4];
      encoded += hex[character & 0x0F];
    }
  }
  return encoded;
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
