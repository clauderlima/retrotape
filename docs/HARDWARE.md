# Hardware

## Target board

ESP32-2432S028 / ESP32-2432S028R, commonly called the Cheap Yellow Display
(CYD). Board revisions vary, so pin assignments must be confirmed against the
actual unit before hardware modifications are made.

## Validated display

The tested module is marked **TPM408-2.8**. A generic ILI9341 configuration
produced mirrored and overlapping output. The validated firmware uses
LovyanGFX `Panel_ILI9342` with a 320 x 240 memory and panel size.

| TFT signal | GPIO |
| --- | --- |
| MISO | 12 |
| MOSI | 13 |
| SCLK | 14 |
| CS | 15 |
| DC | 2 |
| RST | -1 |
| Backlight | 21 |

## XPT2046 touch

| Touch signal | GPIO |
| --- | --- |
| IRQ | 36 |
| MOSI | 32 |
| MISO | 39 |
| SCLK | 25 |
| CS | 33 |

Touch uses software SPI to avoid contention with the microSD hardware bus. On
the tested TPM408-2.8 assembly, the touch sensor is physically oriented in
portrait relative to the landscape display. `offset_rotation = 3` aligns touch
coordinates with the visible controls.

## microSD

| SD signal | GPIO |
| --- | --- |
| MISO | 19 |
| MOSI | 23 |
| SCLK | 18 |
| CS | 5 |

Use a FAT32 card. RetroTape creates `/tk90x`, `/msx`, and `/wav`.

## Onboard audio path

| Function | GPIO |
| --- | --- |
| ESP32 DAC2 | 26 |

GPIO 26 feeds the onboard SC8002B amplifier input. RetroTape uses the DAC for
WAV, CAS, TAP, and audio diagnostics.

TAP playback is special: ESP32 timer group 0, timer 0 runs at 10 MHz and the
interrupt writes DAC levels directly. This gives 0.1 us resolution and keeps
Stop responsive without depending on I2S or DMA.

## Speaker connector warning

`SPEAK/P4` is the bridged output of the SC8002B amplifier. Both P4 terminals
are active outputs. Neither terminal is board ground, and neither should be
shorted to ground.

The single-ended development connection was:

```text
one P4 output -> 1 uF to 10 uF coupling capacitor
              -> 4.7 kOhm to 10 kOhm series resistor
              -> computer TIP / EAR

CYD board GND ---------------------------------> computer sleeve / signal GND
other P4 output: leave disconnected
```

For an electrolytic capacitor, place its positive side toward P4. A 10 kOhm
potentiometer can be used as an attenuator. Begin at a low level and increase
gradually while observing the waveform when possible.

A pre-amplifier connection taken directly from GPIO 26 would be electrically
more predictable, but GPIO 26 is not exposed on the normal CYD headers and
requires a physical board modification.

## TK90X / ZX Spectrum

Put the computer in `LOAD ""` before starting a TAP file. Standard ROM timings
are:

| Signal | T-states | RetroTape baseline |
| --- | ---: | ---: |
| Pilot half-pulse | 2168 | 619.4 us |
| Sync 1 | 667 | 190.6 us |
| Sync 2 | 735 | 210.0 us |
| Bit 0 half-pulse | 855 | 244.3 us |
| Bit 1 half-pulse | 1710 | 488.6 us |

The validated settings are timing 100.0%, TAP level 31%, and normal polarity.

### Tested TK90X modification

The tested TK90X only loaded successfully after **C5 on the TK90X mainboard**
was changed to **100 uF**. This capacitor is inside the computer; it is not the
external RetroTape coupling capacitor. Other revisions may behave differently.
Check the original circuit and capacitor polarity before repeating this change.

## MSX

Use the same attenuated connection with the MSX cassette input. RetroTape
currently generates BIOS CAS data at 1200 baud. Commands depend on the recorded
file:

```text
CLOAD
BLOAD"CAS:",R
```

## External I2S DAC

A future PCM5102A or similar I2S output can provide a cleaner, unamplified line
signal. It remains a planned option; the current validated board uses DAC2 and
the onboard amplifier.

## Safety and validation

- Start with a low output level.
- Never use the second bridged speaker output as ground.
- Do not connect an amplified output to a sensitive input without attenuation.
- Confirm AC coupling and near-zero average voltage at the computer input.
- Validate amplitude, frequency, and distortion with an oscilloscope.
- Expect pin and backlight differences across CYD revisions.

## Hardware references

- [CYD community pin map](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md)
- [Archived CYD schematics](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/OriginalDocumentation/5-Schematic)
- [ESP3D Sunton ESP32-2432S028R notes](https://esp3d.io/esp3d-tft/version_1x/hardware/esp32/sunton-28-2432/)
- [ESP-IDF timer driver](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/timer.html)
