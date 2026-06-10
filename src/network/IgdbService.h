#pragma once

#include <Arduino.h>

#include "metadata/GameMetadata.h"

namespace network {

class IgdbService {
 public:
  explicit IgdbService(metadata::GameMetadataService& metadata);

  bool begin();
  bool isConfigured() const;
  String clientIdLabel() const;
  bool saveCredentials(const String& clientId, const String& clientSecret);
  bool clearCredentials();
  size_t searchGames(const String& query, const String& platform,
                     metadata::GameMetadata* results, size_t maxResults,
                     String& error);
  bool enrichGame(const String& gamePath, const String& platform,
                  uint32_t gameId, String& error);

 private:
  bool loadCredentials();
  bool saveStoredValues();
  bool ensureClock(String& error);
  bool ensureAccessToken(String& error);
  bool requestGames(const String& queryBody, metadata::GameMetadata* results,
                    size_t maxResults, size_t& resultCount, String& error,
                    bool retryAuthentication = true);
  bool downloadCover(const String& imageId, const String& destination,
                     String& error);
  String platformName(const String& platform) const;
  uint16_t platformId(const String& platform) const;
  String escapeQuery(String value) const;
  String formEncode(const String& value) const;

  metadata::GameMetadataService& metadata_;
  String clientId_;
  String clientSecret_;
  String accessToken_;
};

}  // namespace network
