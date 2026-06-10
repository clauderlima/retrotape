#include "storage/SdCardService.h"

#include "config/pins.h"

namespace storage {

bool SdCardService::begin() {
  Serial.println("Mounting SD card");
  sdSpi_.begin(config::pins::SdSclk, config::pins::SdMiso, config::pins::SdMosi, config::pins::SdCs);
  mounted_ = SD.begin(config::pins::SdCs, sdSpi_, config::pins::SdInitialFrequency);

  if (mounted_) {
    Serial.println("SD card mounted");
    ensureStandardDirectories();
  } else {
    Serial.println("SD card mount failed");
  }

  return mounted_;
}

bool SdCardService::isMounted() const {
  return mounted_;
}

bool SdCardService::ensureDirectory(const char* path) {
  if (!mounted_ || path == nullptr || path[0] != '/') {
    return false;
  }

  if (SD.exists(path)) {
    return true;
  }

  const bool created = SD.mkdir(path);
  Serial.print(created ? "Created SD directory: " : "Unable to create SD directory: ");
  Serial.println(path);
  return created;
}

bool SdCardService::ensureStandardDirectories() {
  bool ok = true;
  ok = ensureDirectory("/msx") && ok;
  ok = ensureDirectory("/tk90x") && ok;
  ok = ensureDirectory("/wav") && ok;
  return ok;
}

size_t SdCardService::listDirectory(const char* path, FileEntry* entries, size_t maxEntries,
                                    const char* const* extensions, size_t extensionCount) {
  if (!mounted_ || entries == nullptr || maxEntries == 0) {
    return 0;
  }

  File directory = SD.open(path);
  if (!directory || !directory.isDirectory()) {
    return 0;
  }

  size_t count = 0;
  File file = directory.openNextFile();
  while (file && count < maxEntries) {
    const bool directoryEntry = file.isDirectory();
    const char* rawName = file.name();
    String name(rawName);
    const int slash = name.lastIndexOf('/');
    if (slash >= 0) {
      name = name.substring(slash + 1);
    }

    if (directoryEntry || matchesExtension(name.c_str(), extensions, extensionCount)) {
      entries[count].name = name;
      entries[count].displayName = name;
      entries[count].path = String(path);
      if (!entries[count].path.endsWith("/")) {
        entries[count].path += "/";
      }
      entries[count].path += name;
      entries[count].isDirectory = directoryEntry;
      entries[count].size = static_cast<uint32_t>(file.size());
      ++count;
    }

    file.close();
    file = directory.openNextFile();
  }

  directory.close();
  return count;
}

bool SdCardService::matchesExtension(const char* filename, const char* const* extensions, size_t extensionCount) const {
  if (extensions == nullptr || extensionCount == 0) {
    return true;
  }

  String lowerName(filename);
  lowerName.toLowerCase();

  for (size_t i = 0; i < extensionCount; ++i) {
    String extension(extensions[i]);
    extension.toLowerCase();
    if (!extension.startsWith(".")) {
      extension = "." + extension;
    }
    if (lowerName.endsWith(extension)) {
      return true;
    }
  }

  return false;
}

}  // namespace storage
