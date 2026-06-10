#pragma once

#include <Arduino.h>

namespace metadata {

struct GameMetadata {
  bool available = false;
  uint32_t igdbId = 0;
  String gamePath;
  String title;
  String platform;
  String developer;
  String genres;
  String summary;
  String coverImageId;
  String coverPath;
  uint16_t year = 0;
};

class GameMetadataService {
 public:
  bool begin();
  bool loadForGame(const String& gamePath, GameMetadata& metadata) const;
  bool saveForGame(const String& gamePath, const GameMetadata& metadata) const;
  String metadataPathForGame(const String& gamePath) const;
  String coverPathForGame(const String& gamePath) const;

 private:
  uint32_t pathHash(const String& path) const;
};

}  // namespace metadata
