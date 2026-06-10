#include "metadata/GameMetadata.h"

#include <ArduinoJson.h>
#include <SD.h>

namespace metadata {

namespace {
constexpr char MetadataDirectory[] = "/metadata";
}  // namespace

bool GameMetadataService::begin() {
  if (SD.exists(MetadataDirectory)) {
    return true;
  }
  return SD.mkdir(MetadataDirectory);
}

bool GameMetadataService::loadForGame(const String& gamePath,
                                      GameMetadata& metadata) const {
  metadata = GameMetadata{};
  const String path = metadataPathForGame(gamePath);
  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, file);
  file.close();
  if (error) {
    Serial.print("Metadata read failed: ");
    Serial.println(error.c_str());
    return false;
  }

  const String storedPath = document["game_path"] | "";
  if (storedPath.length() == 0 || storedPath != gamePath) {
    Serial.println("Metadata path mismatch");
    return false;
  }

  metadata.available = true;
  metadata.igdbId = document["igdb_id"] | 0U;
  metadata.gamePath = storedPath;
  metadata.title = document["title"] | "";
  metadata.platform = document["platform"] | "";
  metadata.developer = document["developer"] | "";
  metadata.genres = document["genres"] | "";
  metadata.summary = document["summary"] | "";
  metadata.coverImageId = document["cover_image_id"] | "";
  metadata.coverPath = document["cover_path"] | "";
  metadata.year = document["year"] | 0U;
  return metadata.title.length() > 0;
}

bool GameMetadataService::saveForGame(
    const String& gamePath, const GameMetadata& metadata) const {
  const String path = metadataPathForGame(gamePath);
  if (SD.exists(path)) {
    SD.remove(path);
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    return false;
  }

  JsonDocument document;
  document["version"] = 1;
  document["igdb_id"] = metadata.igdbId;
  document["game_path"] = gamePath;
  document["title"] = metadata.title;
  document["platform"] = metadata.platform;
  document["developer"] = metadata.developer;
  document["genres"] = metadata.genres;
  document["summary"] = metadata.summary;
  document["cover_image_id"] = metadata.coverImageId;
  document["cover_path"] = metadata.coverPath;
  document["year"] = metadata.year;

  const bool ok = serializeJsonPretty(document, file) > 0;
  file.close();
  return ok;
}

String GameMetadataService::metadataPathForGame(
    const String& gamePath) const {
  char filename[32] = {};
  snprintf(filename, sizeof(filename), "/metadata/%08lx.json",
           static_cast<unsigned long>(pathHash(gamePath)));
  return String(filename);
}

String GameMetadataService::coverPathForGame(const String& gamePath) const {
  char filename[32] = {};
  snprintf(filename, sizeof(filename), "/metadata/%08lx.jpg",
           static_cast<unsigned long>(pathHash(gamePath)));
  return String(filename);
}

uint32_t GameMetadataService::pathHash(const String& path) const {
  uint32_t hash = 2166136261UL;
  for (size_t index = 0; index < path.length(); ++index) {
    char value = path.charAt(index);
    if (value >= 'A' && value <= 'Z') {
      value = static_cast<char>(value + ('a' - 'A'));
    }
    hash ^= static_cast<uint8_t>(value);
    hash *= 16777619UL;
  }
  return hash;
}

}  // namespace metadata
