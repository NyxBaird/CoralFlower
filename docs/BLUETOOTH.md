# CoralFlower Bluetooth MIDI fork

This is an experimental native BLE-MIDI build for the FM-1, based on
CoralFlower 0.7.1. On 2026-11-15 it was flashed onto a physical FM-1 and
exchanged MIDI in both directions with a Windows Bluetooth client. WIDI Bud Pro
group reconnection also passed a subsequent software-reboot test. It remains an experimental build.

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
- No Bluetooth audio, saved security bonds, or encrypted pairing in
  this build. Hosts that insist on pairing/encryption will require further work.

## Switch between WIDI Bud Pro and OP-1 field (0.7.2-btdev)

Press **GLO**, turn **SELECT** to **Bluetooth**, then turn **Knob 1 (TARGET)**:

- **WIDI Bud**: peripheral mode, preserving the saved group on the Bud.
- **OP-1 field**: scan for a BLE-MIDI advertiser named `OP-1` or `OP-1 ...`,
  connect as central, discover MIDI and subscribe. Enable advertising in the
  OP-1 COM screen with the blue encoder button. This supports one nearby OP-1;
  it does not present a picker for multiple OP-1 units.
- **Off**: stop the radio link.

**LINK** shows Waiting, Searching, Connecting, Connected or Needs pairing.
The target is saved in an existing settings padding byte, preserving record size
and the previous sounds/patterns. The two targets are exclusive: OP-1 mode stops
advertising so the Bud cannot take the link. Target switches send pedal-off and
all-notes-off before disconnecting. USB MIDI remains available in every mode.
The OP-1 connection retries after link loss; security-required peers are not
supported by this minimal central build. Selecting a different target does not
change MIDI channels: chord/bass/layer/drums still use GLO > Channels.

USB serial diagnostics: `bt` reads link state and MIDI counters; `bt widi`,
`bt op1`, `bt off` select and save the same target as the panel.

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

The group is stored on the **Bud Pro**, not the FM-1. A phone is not required:
[CME's SysEx reference](https://cme-pro.com/midi-implementation/WIDI-Bud-Pro.html)
documents configuration over MIDI. USB requests successfully saved the verified
H4 and FM-1 addresses in slots 1 and 2 (settings 7 and 8), then selected central
role (setting 6 = 2). Back up settings and identify each device before writing.
The H4 was already a forced peripheral. The flasher does not automate this setup.
Another computer/phone can occupy the FM-1's single BLE link; disconnect it first.

## Build

Use Linux x86-64 (WSL works), Python 3 with Pillow and fontTools, and the JieLi
toolchain described in the main README. No full JieLi Bluetooth SDK is required.

```sh
./build.sh --ble --release 0.7.2-btdev
./build.sh --release 0.7.2-usbdev  # same instrument without the radio
```

The outputs are `build/coralflower-0.7.2-btdev.fwsc` and
`build/coralflower-0.7.2-usbdev.fwsc`. The original boot stub and loader remain
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

- Installed `0.7.2-btdev` using the USB command-line installer; running firmware
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
  This initial failure was resolved by the group setup described below.

### WIDI group follow-up, same day

Read-only USB requests identified the Bud and verified its connected peer against
the H4 WIDI Core serial/address reply. The Bud initially used auto role and had
four unused group slots. After saving the two addresses and central role, both
connected. Readback after a Bud software reboot confirmed the settings persisted.
H4 reconnected first; FM-1 was still scanning at the first 12-second check but
connected at the later check. Three test note-on/off pairs sent through Bud USB
produced the FM-1's expected transposed note-on/off replies, both before and after
the reboot. FM-1 was not connected by USB during this test. No firmware change was
needed. Private configuration backups and raw logs remain local under build/.
Physical power-cycle, both startup orders and prolonged load remain untested.

### OP-1 target selector tests, 2026-11-16

Both Bluetooth and USB-only 0.7.2 builds compiled, with unchanged recovery stub.
Routing, link-loss and target-switch host tests passed. The target-switch test
covers the note-release grace period, peripheral/central/off transitions, excluding
unrelated named devices, address type, connection timeout and cancelled switches.
On hardware, direct OP-1 service discovery/subscription reached MIDI-ready; switching
to WIDI peripheral mode reached MIDI-ready again. After switching back, OP-1 no
longer appeared in a PC BLE scan, so advertising had to be checked/re-enabled.
Do not treat the initial link as proof of unattended OP-1 reconnection.
Backup objects for sounds and patterns matched before/after installation; only
the target byte and its settings checksum changed.

After advertising was re-enabled, the OP-1 connected again and the FM-1's BLE
counters recorded MIDI in both directions (RX 7, TX 7 while the user played).
The installed unit was left in OP-1 mode. Audible output was not independently
measured. The OP-1 may require advertising to be enabled again when returning
from WIDI mode; the FM-1 cannot connect to a peer that is not advertising.
