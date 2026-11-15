# CoralFlower Bluetooth MIDI fork

This is an experimental native BLE-MIDI peripheral build for the FM-1, based on
CoralFlower 0.7.1. On 2026-11-15 it was flashed onto a physical FM-1 and
exchanged MIDI in both directions with a Windows Bluetooth client. WIDI Bud Pro
auto-connect has not passed hardware testing. It remains an experimental build.

## What is implemented

- Standard BLE-MIDI service and characteristic; advertises as `CoralFlower XXXX`.
- Advertising starts at boot and resumes after link loss. USB MIDI stays available.
- The address and name suffix are derived from the flash chip's factory UID and
  remain stable across boots and firmware updates. No new flash storage is used.
  An unreadable/blank UID leaves Bluetooth off and shows `BLUETOOTH UID ERROR`.
- Chords, bass, layer, drums and clock output use the existing MIDI channel/routing
  settings over both transports. Channel messages and clock/transport are accepted
  from Bluetooth. Incoming clock currently uses arrival times, not BLE timestamps.
- Overflow/disconnection releases held notes. Wireless SysEx is excluded from the
  input router, so it cannot invoke the USB firmware updater.
- Bluetooth stays off in safe mode, and the baseband is stopped before USB updates.
- No central role, Bluetooth audio, saved security bonds, or encrypted pairing in
  this build. Hosts that insist on pairing/encryption will require further work.

## WIDI Bud Pro: automatic connection

The Bud Pro initiates the connection as the central; CoralFlower advertises as the
peripheral. This avoids two devices trying to be the central. A remembered WIDI
group requires a stable peripheral address, which this fork provides.

One-time setup, following [CME's Bud Pro guide](https://www.cme-pro.com/widi-bud-pro-start-guide-bluetooth-midi-dongle/):

1. With the FM-1 off, connect WIDI App to the Bud Pro. Clear an unwanted existing
   group if necessary. Enable **Group Auto Learn** on the Bud Pro, then close the app.
2. Turn on the FM-1 with this Bluetooth build. Keep other BLE-MIDI peripherals off
   during learning so the Bud does not remember the wrong instrument.
3. Once connected, press the Bud Pro's button to store the group. CME describes a
   green LED while pressed and turquoise after release for the central.
4. Power-cycle both devices. The Bud should reconnect to the same FM-1 automatically.
   Use the Bud Pro's USB MIDI port in Ableton; OS Bluetooth pairing is not needed.

The group is stored on the **Bud Pro**, not the FM-1. This code cannot configure its
group remotely. In the 2026-11-15 test, the attached Bud Pro did not establish a
connection and its USB test notes did not reach the FM-1. Its role/group settings
were not verified; this does not isolate configuration from interoperability.
Another computer/phone can occupy the FM-1's single BLE link; disconnect it first.

## Build

Use Linux x86-64 (WSL works), Python 3 with Pillow and fontTools, and the JieLi
toolchain described in the main README. No full JieLi Bluetooth SDK is required.

```sh
./build.sh --ble --release 0.7.1-btdev
./build.sh --release 0.7.1-usbdev  # same instrument without the radio
```

The outputs are `build/coralflower-0.7.1-btdev.fwsc` and
`build/coralflower-0.7.1-usbdev.fwsc`. The original boot stub and loader remain
unchanged. Keep the USB-only build available while testing.

## Flasher

On Windows, double-click `Flash-CoralFlower.cmd` (Python 3 required), or run
`python tools/launch_flasher.py` on any desktop. Open the printed localhost address
in Chrome or Edge if your default browser does not support Web MIDI with SysEx.
Choose the `.fwsc`, check the connection, and install. The page does not upload files.

File CRCs/layout and the recovery-compatible loader are checked before flashing.
Only the known official V15 checksum is accepted for stock restoration. A checksum
is not a source authenticity guarantee: choose firmware from a trusted source.
Supported backups are downloaded before writing; confirm the file has saved to
continue. Backup failure stops the update. Stock firmware needs an explicit
acknowledgement because its sounds cannot be backed up. Multiple candidate devices
are refused. Interrupted updates can be retried with the same file.

## Validation still required on hardware

WIDI learn/reconnect in either power-on order; physical power-cycle recovery; bidirectional notes/CC/bend/clock; sustained chords plus
sequencer/effects without audio glitches; link loss without stuck notes; USB update
from the BLE build back to USB-only. Do not interpret host tests as these results.

Source versions and licensing: [BLUETOOTH-SOURCES.md](BLUETOOTH-SOURCES.md).

## Hardware observations â€” 2026-11-15

- Installed `0.7.1-btdev` using the USB command-line installer; running firmware
  identity confirmed after the update. The browser flasher was not used for this test.
- Before/after backup object payloads matched exactly (only firmware label and
  backup creation time differed). Private backups remain local.
- Windows BLE discovery received the standard MIDI service advertisement.
- A Windows Bleak client connected, subscribed to MIDI notifications, sent a
  note-on/off pair, and received the instrument's outgoing note-on/off pair.
  The firmware's BLE receive counter advanced by two and MIDI-ready became true.
- On client disconnect the FM-1 returned to advertising. A supported USB software
  restart reset uptime and preserved the BLE address; another BLE MIDI exchange
  succeeded afterward. This was not a physical power-cycle test.
- With the PC BLE client disconnected, WIDI USB note-on/off did not advance the
  FM-1 BLE receive counter. The FM-1 remained advertising, not connected.
  WIDI auto-connect and sustained performance are not verified.
