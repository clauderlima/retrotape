# Regression Checklist

Use this checklist after each refactoring phase and before creating a release.
Record the firmware revision, board revision, computer model, file name, cable,
and audio settings with every hardware result.

## Build and startup

- [ ] PlatformIO release build completes without errors.
- [ ] Firmware size remains within the configured flash partition.
- [ ] Device boots without a reset loop.
- [ ] Display initializes in 320 x 240 landscape orientation.
- [ ] Touch coordinates match the visible controls.
- [ ] microSD mounts successfully.
- [ ] Standard directories `/tk90x`, `/msx`, and `/wav` exist.

## Navigation and display

- [ ] Home opens TK90X/ZX, MSX, WAV, and Settings.
- [ ] Back from a platform root returns directly to Home.
- [ ] Back from a subdirectory returns to its parent directory.
- [ ] Back from Player returns to the originating file list.
- [ ] Long file names do not overlap adjacent controls.
- [ ] Empty folders show a clear empty state.
- [ ] Disabled controls cannot be activated.
- [ ] Footer status and network address remain readable.

## TAP and TK90X/ZX

- [ ] Default TAP settings are 100.0%, 31%, normal polarity.
- [ ] TAP playback starts from the selected file.
- [ ] Pilot, sync, data, and inter-block pause are present.
- [ ] Player elapsed time and progress update during playback.
- [ ] Stop interrupts playback promptly.
- [ ] The validated TAP file loads on the tested TK90X.
- [ ] Serial diagnostics report valid checksums.
- [ ] Serial diagnostics do not report significant repeated timer lateness.
- [ ] Oscilloscope pilot frequency is approximately 807 Hz at 100.0%.
- [ ] Output remains approximately centered around 0 V after AC coupling.

Hardware note for the validated machine:

- [ ] Confirm that TK90X mainboard capacitor C5 is the tested 100 uF part.

## CAS and MSX

- [ ] CAS files appear only in the MSX browser.
- [ ] CAS playback starts and reaches the end of the file.
- [ ] Stop interrupts CAS playback.
- [ ] Elapsed time and progress are plausible.
- [ ] A known CAS file loads on the target MSX.

## WAV

- [ ] WAV files appear in the WAV browser.
- [ ] PCM 8-bit mono playback works.
- [ ] PCM 16-bit mono playback works.
- [ ] Stereo input is mixed or selected correctly.
- [ ] Unsupported WAV formats show an error without crashing.
- [ ] Stop interrupts WAV playback.
- [ ] Elapsed time and progress are accurate.

## Wi-Fi and web server

- [ ] Saved Wi-Fi credentials reconnect after reboot.
- [ ] Network scan lists available SSIDs without duplicates.
- [ ] Open and encrypted networks are distinguishable.
- [ ] Password entry supports all required characters.
- [ ] Failed connection returns to a usable state.
- [ ] Fallback access point starts when station connection is unavailable.
- [ ] The IP address appears on the device.
- [ ] Web page opens from another device.
- [ ] `.tap` upload is stored in `/tk90x`.
- [ ] `.cas` upload is stored in `/msx`.
- [ ] Existing files are listed with correct sizes.
- [ ] Invalid extensions are rejected.
- [ ] Audio playback is not disrupted by web server activity.

## Persistence

- [ ] TAP timing persists after reboot.
- [ ] TAP output level persists after reboot.
- [ ] TAP polarity persists after reboot.
- [ ] Restore-default action returns to the validated TAP profile.

## Final hardware evidence

- [ ] Record a short video of the device UI during playback.
- [ ] Record an oscilloscope capture of the TAP pilot and data pulses.
- [ ] Record the successful computer load result.
- [ ] Store the tested file checksum with the test notes.

