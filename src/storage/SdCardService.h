#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

namespace storage {

struct FileEntry {
  String name;
  String displayName;
  String path;
  bool isDirectory = false;
  uint32_t size = 0;
};

class SdCardService {
 public:
  bool begin();
  bool isMounted() const;
  bool ensureDirectory(const char* path);
  bool ensureStandardDirectories();
  size_t listDirectory(const char* path, FileEntry* entries, size_t maxEntries,
                       const char* const* extensions = nullptr, size_t extensionCount = 0);

 private:
  bool matchesExtension(const char* filename, const char* const* extensions, size_t extensionCount) const;

  SPIClass sdSpi_ = SPIClass(VSPI);
  bool mounted_ = false;
};

}  // namespace storage
