# Technical References

This document records the sources studied for RetroTape and the rules governing
their use.

## Source-use policy

- Code without a clear project license must not be copied.
- RetroTape is distributed under GPL-3.0. Even so, third-party GPL code is not
  copied without explicit provenance, compatibility review, and attribution.
- Third-party projects are currently conceptual references only.
- Format parsing and timings are reimplemented from public specifications.

## Studied repositories

### MaxDuino

- Repository: [rcmolina/MaxDuino](https://github.com/rcmolina/MaxDuino)
- Reviewed commit: `12c2178`
- License finding: no clear repository-wide license was found.
- Use in RetroTape: conceptual reference only.

Reviewed files:

- `MaxDuino.ino`
- `MaxProcessing.cpp`
- `casProcessing.cpp`
- `isr.cpp`
- `buffer.cpp` and `buffer.h`
- `TimerCounter.cpp`
- `file_utils.cpp`
- `CheckForExt.cpp`
- `constants.h`
- `processing_state.h`
- `README.md`
- `FILE_TYPES.md`

Useful concepts include pulse timing tables, parser-to-buffer flow, pause and
polarity state, extension detection, and keeping ISR work small. Parser,
buffer, ISR, menu, and configuration code must not be copied without a new
license review.

### TZXDuino dev-hp

- Repository: [dev-hp/TZXDuino](https://gitlab.com/dev-hp/TZXDuino)
- Reviewed commit: `f6aebbc`
- License finding: no clear license was found in the reviewed files.
- Use in RetroTape: conceptual reference only.

Reviewed files:

- `README.md`
- `TZXDuino.ino`
- `TZXProcessing.ino`
- `Storage.ino` and `Storage.h`
- `Display.ino`
- `Buttons.ino`
- `TZXDuino.h`
- `userconfig.h`

Its producer/consumer waveform buffer and minimal interrupt handler are useful
architectural references. No code is copied.

### POWADCR

- Repository: [hash6iron/powadcr](https://github.com/hash6iron/powadcr)
- Reviewed commit: `6f0599f`
- License: GPL-3.0.
- Use in RetroTape: conceptual reference only; no POWADCR source is included.

Reviewed files:

- `platformio.ini`
- `README.md` and `LICENSE`
- `src/config.h`
- `src/globales.h`
- `src/powadcr.cpp`
- `src/TAPprocessor.h`
- `src/TZXprocessor.h`
- `src/TSXprocessor.h`
- `src/PZXprocessor.h`
- `src/ZXProcessor.h`
- `src/SmartRadioBuffer.h`
- `src/PredictiveRadioBuffer.h`
- `HMI.h`

Useful concepts include pre-parsed block descriptors, PCM/I2S generation,
predictive buffering, audio configuration, and separation of format processors.
No parser, audio, UI, or vendored library code is copied.

### SD Tape Player

- Repository: [GadgetReboot/SD_Tape_Player](https://github.com/GadgetReboot/SD_Tape_Player)
- Reviewed commit: `ed9a0ae`
- License finding: no clear license was found.
- Use in RetroTape: hardware and connector concepts only.

Reviewed files:

- `README.md`
- KiCad schematic and PCB files;
- `SD_Tape_Player-sch.pdf`.

## Public format specifications

### ZX Spectrum TAP

- [Sinclair Wiki TAP format](https://sinclair.wiki.zxnet.co.uk/wiki/TAP_format)
- [World of Spectrum TAP format](https://worldofspectrum.net/zx-modules/fileformats/tapformat.html)

A TAP file is a sequence of blocks. Each block starts with a two-byte
little-endian payload length. The payload normally contains a flag byte and XOR
checksum, but the container itself does not interpret them.

### ZX Spectrum TZX

- [TZX format specification v1.20](https://worldofspectrum.net/TZXformat.html)

TZX preserves pulse timing and complex block structure. Initial candidates are
IDs 10, 11, 12, 13, 14, and 20. More complex blocks require explicit design
and hardware regression testing.

### MSX CAS and TSX

- [MSX Wiki emulation file formats](https://www.msx.org/wiki/Emulation_related_file_formats)
- [MSX2 Technical Handbook cassette interface](https://konamiman.github.io/MSX2-Technical-Handbook/md/Chapter5a.html)

MSX CAS stores decoded BIOS cassette bytes. The known header sequence is
`1F A6 DE BA CC 13 7D 74`. At 1200 baud, a zero bit uses one 1200 Hz cycle and
a one bit uses two 2400 Hz cycles. Bytes are emitted with one start bit, eight
LSB-first data bits, and two stop bits.

TSX extends TZX with MSX-oriented blocks, notably ID 4B Kansas City data.

## ESP32 and CYD references

- [ESP-IDF timer driver](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/timer.html)
- [CYD community pin map](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md)
- [Archived CYD schematic](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/OriginalDocumentation/5-Schematic)
- [SC8002B datasheet](https://www.alldatasheet.com/datasheet-pdf/pdf/1146797/FUMAN/SC8002B.html)

The ESP32 APB clock divided by eight gives a 10 MHz timer and 0.1 us ticks.
GPIO 26 is DAC2 and feeds the CYD amplifier. P4 is the bridged amplifier output,
not a GPIO or ground-referenced line output.

## Current decisions

- RetroTape uses LovyanGFX rather than LVGL for the validated firmware.
- The tested pin map is recorded in `src/config/pins.h` and `docs/HARDWARE.md`.
- Current output uses ESP32 DAC2 and the onboard amplifier.
- PCM5102A I2S remains a future option.
- New format support must be clean-room implementation from public specs.
- RetroTape is licensed under GPL-3.0.
