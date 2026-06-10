# RetroTape

RetroTape turns an ESP32-2432S028 / Cheap Yellow Display into a touch-controlled
digital cassette player for vintage computers.

## Current features

- 320x240 landscape interface for the TPM408-2.8 display using LovyanGFX;
- XPT2046 resistive touch input;
- SD card browser with dedicated TK90X/ZX, MSX, and WAV views;
- WAV PCM playback through the ESP32 internal DAC;
- hardware-timed TAP playback for ZX Spectrum and TK90X;
- 1200-baud MSX BIOS CAS playback;
- elapsed time, duration, progress bar, and immediate Stop control;
- adjustable TAP timing, output level, and polarity;
- persistent TAP settings with one-touch restoration of the validated profile;
- non-blocking 1 kHz, 1200 Hz, and 2400 Hz output diagnostics;
- persistent DAC output level shared by WAV, CAS, and test tones;
- on-device Wi-Fi scan and password keyboard;
- web upload and file listing for `.tap` and `.cas` files;
- fallback `RetroTape` access point when no saved Wi-Fi is available.

## Target hardware

- ESP32-2432S028 or ESP32-2432S028R;
- ESP32-WROOM;
- TPM408-2.8 320x240 display;
- XPT2046 touch controller;
- microSD card;
- onboard SC8002B audio amplifier driven from ESP32 DAC2 on GPIO 26.

Board revisions vary. See [Hardware](docs/HARDWARE.md) before changing display,
touch, SD, or audio pins.

## Build

Requirements:

- Visual Studio Code;
- PlatformIO IDE extension.

Open the repository folder in VS Code, wait for PlatformIO to detect
`platformio.ini`, and use **PlatformIO: Build** or **Upload**.

Command-line build:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
```

## SD card layout

Use a FAT32-formatted card. The firmware creates these directories:

```text
/msx
/tk90x
/wav
```

| Platform | Format | Directory |
| --- | --- | --- |
| MSX | `.cas` | `/msx` |
| TK90X / ZX Spectrum | `.tap` | `/tk90x` |
| General audio | `.wav` | `/wav` or other browsable folders |

## Wi-Fi and web upload

1. Open **Menu** on the device.
2. Open **Wi-Fi setup**.
3. Select a network.
4. Enter its password and connect.
5. Open the IP address shown in the display footer.

Without saved credentials, the device starts:

```text
SSID: RetroTape
Password: 12345678
Address: http://192.168.4.1
```

The web page lists existing MSX and TK90X/ZX files and uploads new files to
their matching directories.

## Audio connection

The board speaker connector is the bridged output of the SC8002B amplifier. Its
two pins are active outputs; neither speaker pin is ground. Follow the tested
connection described in [Hardware](docs/HARDWARE.md).

For the successful TK90X hardware test, capacitor **C5 on the TK90X mainboard**
was replaced with **100 uF**. This is a modification to the computer, not to the
RetroTape board.

Validated TAP settings:

```text
Timing: 100.0%
Level: 31%
Polarity: Normal
```

## Project structure

```text
src/
  app/       Application state and browser navigation
  audio/     DAC driver and WAV, TAP, CAS players
  config/    Board configuration
  hardware/  Display and touch configuration
  network/   Wi-Fi and web file server
  settings/  Persistent validated user settings
  storage/   SD card access
  tape/      File format detection
  ui/        Screens, text, theme, and reusable components
docs/        Hardware, architecture, baseline, and regression notes
```

See [Architecture](docs/ARCHITECTURE.md), [Validated baseline](docs/BASELINE.md),
and [Regression checklist](docs/REGRESSION_CHECKLIST.md) before changing
hardware-timed TAP playback.

## Documentation

- [Project brief](docs/PROJECT_BRIEF.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Hardware](docs/HARDWARE.md)
- [Validated baseline](docs/BASELINE.md)
- [Regression checklist](docs/REGRESSION_CHECKLIST.md)
- [Roadmap](docs/ROADMAP.md)
- [Analysis](docs/ANALYSIS.md)
- [References](docs/REFERENCES.md)

## Format status

| Format | Status |
| --- | --- |
| WAV | PCM 8/16-bit, mono/stereo |
| TAP | Hardware-timed DAC playback |
| CAS | MSX BIOS 1200-baud playback |
| TZX | Planned |
| TSX/TSZ | Planned |

## License

RetroTape is distributed under the GNU General Public License v3.0. See
[LICENSE](LICENSE).
