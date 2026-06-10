# Roadmap

## Completed foundation

### Research and preparation

- reviewed MaxDuino, TZXDuino, POWADCR, and hardware references;
- recorded licensing constraints;
- chose conceptual reimplementation instead of copying unlicensed or GPL code.

### CYD hardware

- validated TPM408-2.8 with LovyanGFX ILI9342 profile;
- aligned XPT2046 touch with the landscape display;
- mounted FAT32 microSD;
- validated GPIO 26 DAC and the onboard amplifier path.

### Core firmware

- modular application, UI, storage, network, settings, and audio layers;
- 2 x 2 Home navigation and touch file browser;
- WAV, standard TAP, and MSX CAS playback;
- elapsed time, progress, Play, Stop, and direct platform-root navigation;
- Wi-Fi setup, fallback access point, and web uploads;
- persistent TAP settings and validated-profile restoration;
- non-blocking audio diagnostics at 1 kHz, 1200 Hz, and 2400 Hz;
- persistent global DAC level for WAV, CAS, and diagnostics.

### Refactoring phases

- Phase 0: recorded and tagged the hardware-validated baseline;
- Phase 1: separated audio players, UI components, and browser navigation;
- Phase 2: introduced the current visual system and cassette identity;
- Phase 3: added validated persistent settings;
- Phase 4: added responsive output diagnostics;
- Phase 5: standardized public documentation in English.

## Near-term validation

- complete the hardware regression checklist after every playback change;
- confirm persisted settings across a full power cycle;
- validate WAV variants on the real output;
- validate CAS loading on target MSX computers;
- test web uploads and reconnect behavior on multiple networks;
- record tested file hashes, cable details, amplitudes, and computer revisions.

## Planned format work

### TZX

Implement from the public TZX specification, beginning with common blocks:

- ID 10 standard-speed data;
- ID 11 turbo data;
- ID 12 pure tone;
- ID 13 pulse sequence;
- ID 14 pure data;
- ID 20 pause or stop.

Unsupported blocks must stop safely and display a clear error. Complex control
flow, CSW, direct recording, generalized data, and metadata can follow later.

### TSX / TSZ

Add support only after the reusable TZX block engine is stable. Prioritize the
MSX Kansas City data block ID 4B and verify it on real MSX hardware.

## Future hardware and product work

- optional PCM5102A I2S output;
- file delete and rename operations in the web interface;
- downloadable diagnostics and playback logs;
- release packaging and version display;
- additional computers and cassette formats after the existing targets remain
  regression-free.
