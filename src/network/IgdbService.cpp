#include "network/IgdbService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ctime>

#include "network/IgdbCertificates.h"

namespace network {

namespace {
constexpr char PreferencesNamespace[] = "rt-igdb";
constexpr char ClientIdKey[] = "client_id";
constexpr char ClientSecretKey[] = "secret";
constexpr char AccessTokenKey[] = "token";
constexpr char TokenEndpoint[] = "https://id.twitch.tv/oauth2/token";
constexpr char GamesEndpoint[] = "https://api.igdb.com/v4/games";
constexpr uint16_t ZxSpectrumPlatformId = 26;
constexpr uint16_t MsxPlatformId = 27;
constexpr uint16_t Msx2PlatformId = 53;
constexpr time_t MinimumValidTime = 1700000000;
}  // namespace

IgdbService::IgdbService(metadata::GameMetadataService& metadata)
    : metadata_(metadata) {}

bool IgdbService::begin() {
  loadCredentials();
  Serial.println(isConfigured() ? "IGDB credentials loaded"
                                : "IGDB integration not configured");
  return true;
}

bool IgdbService::isConfigured() const {
  return clientId_.length() > 0 && clientSecret_.length() > 0;
}

String IgdbService::clientIdLabel() const {
  if (clientId_.length() <= 6) {
    return isConfigured() ? "configured" : "not configured";
  }
  return clientId_.substring(0, 3) + "..." +
         clientId_.substring(clientId_.length() - 3);
}

bool IgdbService::saveCredentials(const String& clientId,
                                  const String& clientSecret) {
  String cleanId = clientId;
  String cleanSecret = clientSecret;
  cleanId.trim();
  cleanSecret.trim();
  if (cleanId.length() < 6 || cleanSecret.length() < 8) {
    return false;
  }

  clientId_ = cleanId;
  clientSecret_ = cleanSecret;
  accessToken_ = "";
  return saveStoredValues();
}

bool IgdbService::clearCredentials() {
  clientId_ = "";
  clientSecret_ = "";
  accessToken_ = "";

  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, false)) {
    return false;
  }
  const bool ok = preferences.clear();
  preferences.end();
  return ok;
}

size_t IgdbService::searchGames(const String& query, const String& platform,
                                metadata::GameMetadata* results,
                                size_t maxResults, String& error) {
  if (results == nullptr || maxResults == 0) {
    error = "No result buffer";
    return 0;
  }
  const uint16_t requestedPlatform = platformId(platform);
  if (requestedPlatform == 0) {
    error = "Unsupported platform";
    return 0;
  }

  String body = "fields name,first_release_date,cover.image_id,"
                "genres.name,involved_companies.company.name,"
                "involved_companies.developer,involved_companies.publisher,"
                "platforms.name; search \"";
  body += escapeQuery(query);
  body += "\"; where platforms = (";
  if (platform == "msx") {
    body += String(MsxPlatformId) + "," + String(Msx2PlatformId);
  } else {
    body += String(requestedPlatform);
  }
  body += ") & version_parent = null; limit ";
  body += String(maxResults);
  body += ";";

  size_t count = 0;
  if (!requestGames(body, results, maxResults, count, error)) {
    return 0;
  }
  return count;
}

bool IgdbService::enrichGame(const String& gamePath, const String& platform,
                             uint32_t gameId, String& error) {
  metadata::GameMetadata game;
  size_t count = 0;
  const String body =
      "fields name,summary,first_release_date,cover.image_id,genres.name,"
      "involved_companies.company.name,involved_companies.developer,"
      "involved_companies.publisher,platforms.name; where id = " +
      String(gameId) + "; limit 1;";
  if (!requestGames(body, &game, 1, count, error) || count != 1) {
    if (error.length() == 0) {
      error = "Game not found";
    }
    return false;
  }

  game.available = true;
  game.gamePath = gamePath;
  game.platform = platformName(platform);
  if (game.coverImageId.length() > 0) {
    game.coverPath = metadata_.coverPathForGame(gamePath);
    if (!downloadCover(game.coverImageId, game.coverPath, error)) {
      Serial.print("Cover download skipped: ");
      Serial.println(error);
      game.coverPath = "";
      error = "";
    }
  }

  if (!metadata_.saveForGame(gamePath, game)) {
    error = "Could not save metadata";
    return false;
  }
  return true;
}

bool IgdbService::loadCredentials() {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, true)) {
    return false;
  }
  clientId_ = preferences.getString(ClientIdKey, "");
  clientSecret_ = preferences.getString(ClientSecretKey, "");
  accessToken_ = preferences.getString(AccessTokenKey, "");
  preferences.end();
  return isConfigured();
}

bool IgdbService::saveStoredValues() {
  Preferences preferences;
  if (!preferences.begin(PreferencesNamespace, false)) {
    return false;
  }
  bool ok = true;
  ok = preferences.putString(ClientIdKey, clientId_) > 0 && ok;
  ok = preferences.putString(ClientSecretKey, clientSecret_) > 0 && ok;
  if (accessToken_.length() > 0) {
    ok = preferences.putString(AccessTokenKey, accessToken_) > 0 && ok;
  } else {
    preferences.remove(AccessTokenKey);
  }
  preferences.end();
  return ok;
}

bool IgdbService::ensureClock(String& error) {
  if (time(nullptr) >= MinimumValidTime) {
    return true;
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  const uint32_t startedAt = millis();
  while (time(nullptr) < MinimumValidTime && millis() - startedAt < 10000) {
    delay(100);
  }
  if (time(nullptr) < MinimumValidTime) {
    error = "Could not synchronize time for secure connection";
    return false;
  }
  return true;
}

bool IgdbService::ensureAccessToken(String& error) {
  if (accessToken_.length() > 0) {
    return true;
  }
  if (!isConfigured()) {
    error = "IGDB credentials are not configured";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    error = "Internet connection is unavailable";
    return false;
  }
  if (!ensureClock(error)) {
    return false;
  }

  WiFiClientSecure client;
  client.setCACert(certificates::IgdbRootCa);
  HTTPClient http;
  if (!http.begin(client, TokenEndpoint)) {
    error = "Could not start Twitch authentication";
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const String body = "client_id=" + formEncode(clientId_) +
                      "&client_secret=" + formEncode(clientSecret_) +
                      "&grant_type=client_credentials";
  const int status = http.POST(body);
  const String response = http.getString();
  http.end();
  if (status != HTTP_CODE_OK) {
    error = "Twitch authentication failed (" + String(status) + ")";
    return false;
  }

  JsonDocument document;
  if (deserializeJson(document, response)) {
    error = "Invalid Twitch response";
    return false;
  }
  accessToken_ = document["access_token"] | "";
  if (accessToken_.length() == 0) {
    error = "Twitch did not return a token";
    return false;
  }
  saveStoredValues();
  return true;
}

bool IgdbService::requestGames(const String& queryBody,
                               metadata::GameMetadata* results,
                               size_t maxResults, size_t& resultCount,
                               String& error, bool retryAuthentication) {
  resultCount = 0;
  if (!ensureAccessToken(error)) {
    return false;
  }

  WiFiClientSecure client;
  client.setCACert(certificates::IgdbRootCa);
  HTTPClient http;
  if (!http.begin(client, GamesEndpoint)) {
    error = "Could not connect to IGDB";
    return false;
  }
  http.addHeader("Client-ID", clientId_);
  http.addHeader("Authorization", "Bearer " + accessToken_);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "text/plain");
  const int status = http.POST(queryBody);
  const String response = http.getString();
  http.end();

  if (status == HTTP_CODE_UNAUTHORIZED && retryAuthentication) {
    accessToken_ = "";
    saveStoredValues();
    return requestGames(queryBody, results, maxResults, resultCount, error,
                        false);
  }
  if (status != HTTP_CODE_OK) {
    error = "IGDB request failed (" + String(status) + ")";
    return false;
  }

  JsonDocument document;
  const DeserializationError jsonError = deserializeJson(document, response);
  if (jsonError || !document.is<JsonArray>()) {
    error = "Invalid IGDB response";
    return false;
  }

  for (JsonObject item : document.as<JsonArray>()) {
    if (resultCount >= maxResults) {
      break;
    }
    metadata::GameMetadata& game = results[resultCount];
    game = metadata::GameMetadata{};
    game.available = true;
    game.igdbId = item["id"] | 0U;
    game.title = item["name"] | "";
    game.summary = item["summary"] | "";
    game.coverImageId = item["cover"]["image_id"] | "";

    const uint32_t releaseTimestamp = item["first_release_date"] | 0U;
    if (releaseTimestamp > 0) {
      const time_t releaseTime = static_cast<time_t>(releaseTimestamp);
      struct tm released = {};
      gmtime_r(&releaseTime, &released);
      game.year = static_cast<uint16_t>(released.tm_year + 1900);
    }

    for (JsonObject genre : item["genres"].as<JsonArray>()) {
      const char* name = genre["name"] | "";
      if (name[0] == '\0') {
        continue;
      }
      if (game.genres.length() > 0) {
        game.genres += ", ";
      }
      game.genres += name;
      if (game.genres.length() > 54) {
        break;
      }
    }

    String publisher;
    for (JsonObject company : item["involved_companies"].as<JsonArray>()) {
      const String name = company["company"]["name"] | "";
      if (name.length() == 0) {
        continue;
      }
      if (company["developer"] | false) {
        game.developer = name;
        break;
      }
      if ((company["publisher"] | false) && publisher.length() == 0) {
        publisher = name;
      }
    }
    if (game.developer.length() == 0) {
      game.developer = publisher;
    }

    if (game.igdbId > 0 && game.title.length() > 0) {
      ++resultCount;
    }
  }
  return true;
}

bool IgdbService::downloadCover(const String& imageId,
                                const String& destination, String& error) {
  if (imageId.length() == 0) {
    return true;
  }
  const String url = "https://images.igdb.com/igdb/image/upload/"
                     "t_cover_small/" +
                     imageId + ".jpg";

  WiFiClientSecure client;
  client.setCACert(certificates::IgdbRootCa);
  HTTPClient http;
  if (!http.begin(client, url)) {
    error = "Could not connect to image server";
    return false;
  }
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    error = "Cover download failed (" + String(status) + ")";
    http.end();
    return false;
  }

  if (SD.exists(destination)) {
    SD.remove(destination);
  }
  File file = SD.open(destination, FILE_WRITE);
  if (!file) {
    error = "Could not create cover file";
    http.end();
    return false;
  }
  const int written = http.writeToStream(&file);
  file.close();
  http.end();
  if (written <= 0) {
    SD.remove(destination);
    error = "Could not store cover";
    return false;
  }
  return true;
}

String IgdbService::platformName(const String& platform) const {
  return platform == "msx" ? "MSX" : "ZX Spectrum";
}

uint16_t IgdbService::platformId(const String& platform) const {
  if (platform == "msx") {
    return MsxPlatformId;
  }
  if (platform == "tk90x" || platform == "zx") {
    return ZxSpectrumPlatformId;
  }
  return 0;
}

String IgdbService::escapeQuery(String value) const {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\r", " ");
  value.replace("\n", " ");
  value.trim();
  return value;
}

String IgdbService::formEncode(const String& value) const {
  String encoded;
  encoded.reserve(value.length() * 3);
  const char hex[] = "0123456789ABCDEF";
  for (size_t index = 0; index < value.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(value.charAt(index));
    const bool safe = (character >= 'a' && character <= 'z') ||
                      (character >= 'A' && character <= 'Z') ||
                      (character >= '0' && character <= '9') ||
                      character == '-' || character == '_' || character == '.';
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

}  // namespace network
