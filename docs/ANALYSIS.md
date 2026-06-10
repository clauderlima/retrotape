# Initial Research and Preparation

- Project: RetroTape-ESP32-CYD
- Research date: June 7, 2026

## Purpose

The initial research reviewed established digital cassette projects and
identified concepts suitable for a clean, modular ESP32 implementation. Local
reference clones were used only for study and are excluded from the firmware
repository.

## Repository findings

### MaxDuino

Reviewed commit `12c2178`. No clear repository-wide license was found.

MaxDuino is an efficient AVR-era firmware organized around Arduino
`setup()`/`loop()`, physical buttons, a small display, SdFat, timer interrupts,
and pin-level audio. `MaxProcessing.cpp` supports many TZX block types,
`casProcessing.cpp` demonstrates byte-to-pulse state machines, and its
two-page buffer limits contention between the producer and interrupt consumer.

The architecture is tightly coupled to global state, macros, AVR hardware, and
its display/menu implementation. RetroTape therefore uses only the concepts:

- parser to pulse-buffer flow;
- timing and block-state tables;
- pause, polarity, and extension handling;
- short interrupt routines.

No MaxDuino source code is copied.

### TZXDuino dev-hp

Reviewed commit `f6aebbc`. No clear license was found in the reviewed files.

This project clearly documents an ISR plus main-loop producer model. Its
waveform buffer is divided into producer and consumer regions, and the interrupt
only reads durations and changes the output level. Expensive display work is
reduced during playback to avoid underruns.

RetroTape adopted the architectural principle of keeping timing-critical output
independent from UI and storage work, without copying implementation code.

### POWADCR

Reviewed commit `6f0599f`. The project is explicitly GPL-3.0.

POWADCR is the closest modern-hardware reference: ESP32, PlatformIO, SD_MMC,
digital audio, and an HMI. It pre-processes TAP, TZX, TSX, PZX, and CSW into
descriptors, then produces PCM audio through format-specific processors.

Useful concepts:

- pre-parsed block descriptors;
- predictive and stream buffering;
- centralized signal configuration;
- PCM/I2S generation;
- separate format processors.

Its code is not copied. RetroTape is GPL-3.0, but shared licensing alone does
not replace provenance, attribution, and a deliberate integration review.

### SD Tape Player

Reviewed commit `ed9a0ae`. No clear license was found.

This project is a tested Arduino Nano PCB for CASDuino/TZXDuino. It is useful as
a connector and hardware-layout reference but does not provide reusable firmware
for RetroTape.

## Concepts adopted

- strict separation of UI, storage, parsing, and output;
- non-blocking player state machines;
- hardware timer output for timing-sensitive pulses;
- explicit playback progress, pause, Stop, and polarity state;
- fixed diagnostic tones and adjustable output level;
- block-at-a-time SD reading;
- format-specific players behind a common audio facade.

## Clean-room implementation

RetroTape reimplements:

- CYD display, touch, and pin configuration;
- SD card browsing and filtering;
- application state and touch navigation;
- DAC sample output and test tones;
- WAV parsing and playback;
- standard TAP block playback and ZX ROM timings;
- MSX CAS byte encoding;
- settings persistence;
- Wi-Fi and web upload behavior.

Future TZX and TSX support must also be implemented from public format
specifications rather than third-party source.

## Technical risks identified

- CYD pin maps and panel controllers vary by revision.
- The onboard bridged amplifier is not a ground-referenced line output.
- Vintage EAR/CASSETTE inputs vary in sensitivity and filtering.
- Display, touch, SD, and network work can introduce timing latency.
- Complex TZX/TSX control flow can substantially increase state complexity.
- MSX CAS containers do not preserve every original gap and timing distinction.
- Unlicensed or GPL source can create licensing conflicts if copied.

## How the implementation addressed the risks

- The TPM408-2.8 display and XPT2046 orientation were validated on hardware.
- Hardware wiring and the bridged P4 warning are documented.
- Standard TAP uses a 10 MHz hardware timer and direct DAC register writes.
- TAP edges are scheduled from an absolute deadline to avoid accumulated drift.
- UI progress updates are limited during timing-sensitive playback.
- WAV, TAP, CAS, and diagnostics are isolated behind `DacAudioOutput`.
- A tagged functional baseline and regression checklist protect known behavior.
- User settings are validated before reaching the audio layer.

## Original incremental plan

1. Create the PlatformIO base and CYD pin map.
2. Initialize serial, display, touch, and SD.
3. Add application states and filtered file browsing.
4. Build a minimal Home, browser, player, and settings UI.
5. Add the audio abstraction and diagnostic tones.
6. Implement WAV.
7. Implement standard TAP and ZX timings.
8. Implement initial MSX CAS.
9. Add Wi-Fi and web upload.
10. Refactor and validate before adding TZX/TSX.

The first nine items are now implemented. Current and future work is tracked in
`docs/ROADMAP.md`.
