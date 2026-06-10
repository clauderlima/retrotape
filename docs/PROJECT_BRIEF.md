# Project Brief

## Product

RetroTape is firmware for the ESP32-2432S028 / Cheap Yellow Display that turns
the board into a modern digital cassette player for vintage computers.

Initial targets:

- TK90X and ZX Spectrum;
- MSX;
- WAV playback for direct audio and signal tests.

## User flow

1. The user powers on the CYD.
2. A touch interface presents the available computer families.
3. The user selects TK90X/ZX, MSX, or WAV.
4. RetroTape lists matching files from the SD card.
5. The user selects a file and opens the player.
6. Play generates the required signal through the ESP32 audio output.
7. The signal is connected to the computer EAR/CASSETTE input.

## Version 1 scope

- ESP32-2432S028 / ESP32-2432S028R;
- PlatformIO and Arduino Framework;
- C++17;
- LovyanGFX user interface;
- FAT32 microSD browsing;
- PCM WAV playback;
- standard ZX Spectrum TAP playback;
- MSX BIOS CAS playback;
- touch-controlled Play, Stop, navigation, settings, and diagnostics;
- Wi-Fi configuration and web file upload.

## Version 2 candidates

- common TZX blocks;
- TSX/TSZ support;
- external I2S DAC output;
- additional file-management operations;
- support for more cassette formats.

## Architecture rule

User interface, file parsing, storage, and signal generation must remain
separate. The intended flow is:

```text
SD file -> format player/parser -> pulse or sample generation -> AudioOutput
                                      ^
                                      |
                              progress and status
```

Timing-critical playback must not depend on display refreshes, web requests, or
long blocking operations in the main loop.

## Product priorities

1. Preserve the hardware-validated TAP baseline.
2. Keep all playback controls responsive.
3. Make setup and file selection understandable on a 320 x 240 display.
4. Validate format support on real target computers.
5. Add complex formats only after the current formats remain stable.
