# Architecture

## Goals

RetroTape separates application flow, user interface, storage, networking, tape
format detection, and audio generation. The main reason for these boundaries is
timing safety: display, touch, SD card, and web server work must not disturb TAP
pulse generation.

```text
Touch -> UiService -> AppController -> player facade -> format player -> DAC
                         |                  |
                         |                  +-> progress and status
                         +-> storage, Wi-Fi, and web server

Browser -> FileWebServer -> SD card
```

## Entry point

`src/main.cpp` creates the services and delegates Arduino `setup()` and `loop()`
to `AppController`.

## Application layer

`src/app/AppController` owns the application state machine and coordinates the
services. Touch actions are grouped by responsibility:

- navigation and screen changes;
- file browser actions;
- player actions;
- TAP compatibility settings;
- Wi-Fi setup and keyboard input.

`BrowserNavigation` owns the current browser mode, root directory, path, title,
and extension filters. Returning from a platform root goes directly to Home.

## User interface

`src/ui/UiService` draws screens and maps touch coordinates to `UiAction`
values. It does not perform storage, networking, or playback operations.

- `UiTheme.h` contains shared colors and dimensions.
- `UiText.h` contains display strings.
- `UiComponents` provides reusable headers, buttons, fitted text, and time
  formatting.

The visual redesign and cassette logo are intentionally isolated from the
application and audio code.

## Storage and format detection

`SdCardService` mounts the SD card, creates standard directories, lists files,
and filters entries by extension.

`TapeFormatDetector` identifies supported formats from their file names. It is
the only active class under `src/tape`; unfinished parser placeholders were
removed in Phase 1 so the source tree represents actual behavior.

## Audio

`AudioOutput` is the interface consumed by the application. `DacAudioOutput` is
a facade that selects one format-specific player:

- `WavPlayer`: PCM WAV playback;
- `TapPlayer`: ZX Spectrum and TK90X TAP pulse generation;
- `CasPlayer`: MSX BIOS CAS playback;
- `DacOutputDriver`: shared DAC setup and sample output.

`NoopAudioOutput` remains available as a harmless implementation for tests or
future host-side development.

### TAP timing contract

The hardware-validated TAP implementation remains isolated in `TapPlayer`.
It uses ESP32 timer group 0, timer 0, at 10 MHz. The interrupt handler writes
the internal DAC register and schedules pulse edges from an absolute timer
deadline to avoid cumulative drift.

ROM pulse timings are preserved:

- pilot: 2168 T-states;
- sync 1: 667 T-states;
- sync 2: 735 T-states;
- bit 0: 855 T-states per half-wave;
- bit 1: 1710 T-states per half-wave.

The validated TK90X baseline is timing 100.0%, UI level 31% (internal amplitude
40), and normal polarity. `Stop` disables the timer before releasing the file
and block buffer.

## Networking

`WifiService` scans networks, connects in station mode, saves credentials in
ESP32 Preferences, and provides a fallback access point.

`FileWebServer` serves an English upload/listing page on port 80. It accepts
`.cas` files for `/msx` and `.tap` files for `/tk90x`. The web server is serviced
only while audio is idle to protect playback timing.

## Concurrency

TAP pulse edges are produced by a hardware timer interrupt. File loading, UI,
touch, Wi-Fi, and web requests run from the main loop. WAV and CAS players are
non-blocking and advance during regular `update()` calls.

Future TZX/TSX support should use a format-specific player or parser feeding
the same output boundary rather than adding format logic back into the facade.
