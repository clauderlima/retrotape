#pragma once

#include <Arduino.h>
#include <FS.h>

#include "audio/DacOutputDriver.h"

namespace audio {

class WavPlayer {
 public:
  explicit WavPlayer(DacOutputDriver& output);

  bool play(const char* path);
  void update();
  void stop();
  bool isPlaying() const;
  uint32_t elapsedMs() const;
  uint32_t durationMs() const;

 private:
  struct WavInfo {
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataStart = 0;
    uint32_t dataSize = 0;
  };

  void finish(const char* message);
  bool readAndWriteFrame();
  bool readWavInfo(File& file, WavInfo& info);
  bool readFourCc(File& file, char id[4]);
  bool readU16(File& file, uint16_t& value);
  bool readU32(File& file, uint32_t& value);

  DacOutputDriver& output_;
  File file_;
  WavInfo info_;
  bool playing_ = false;
  uint16_t frameBytes_ = 0;
  uint32_t totalFrames_ = 0;
  uint32_t framesRemaining_ = 0;
  uint32_t framesPlayed_ = 0;
  uint32_t samplePeriodUs_ = 0;
  uint32_t nextSampleAtUs_ = 0;
};

}  // namespace audio
