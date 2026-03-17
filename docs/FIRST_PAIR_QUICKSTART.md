# First Pair Quickstart (Beginner Friendly)

This guide is for building and validating your **first fully functional pair** of Nebula Poi.

It is written for people who may be new to microcontrollers. You do not need to understand firmware internals to use this checklist.

## What You Will Achieve

By the end of this guide, you will be able to:

- Power on each poi and connect to it directly
- Open the Web UI and control patterns/images
- Move each poi from direct-connect mode to your home/work WiFi
- Pair two poi together and verify synchronization

## Before You Start

### Hardware Checklist (Per Poi)

- 1x Teensy 4.1 (POV engine)
- 1x ESP32-S3 (WiFi/Web UI controller)
- 1x APA102 LED strip (32 LEDs)
- Stable 5V power source (2-3A recommended)
- Common ground between all boards
- Safe mounting for spin tests

### Safety Checklist

- Start at low brightness (for example, 40-80)
- Verify all wiring before power-on
- Keep clear space before spin testing
- Avoid looking directly into LEDs at full brightness

## Phase 1 - Bring Up One Poi

### 1) Power and Boot

1. Power the poi.
2. Wait 10-20 seconds for boot.
3. On your phone/laptop, look for WiFi network `POV-POI-WiFi`.
4. Connect using password `povpoi123`.

Expected result:

- Device connects to the poi WiFi successfully.

### 2) Open the Web UI

1. Open a browser.
2. Go to `http://192.168.4.1` (or `http://povpoi.local` if available).
3. Confirm the dashboard/status is visible.

Expected result:

- You can see controls for mode, brightness, frame rate, and image/pattern actions.

### 3) Basic Control Test

1. Set mode to pattern mode.
2. Change pattern type.
3. Change brightness.
4. Change frame rate.

Expected result:

- LED behavior changes quickly after each action.

### 4) Image Upload Test

1. Open Image Lab.
2. Upload one image.
3. Display the uploaded image.

Expected result:

- Image appears on the strip when spinning.

If image orientation looks wrong, see `docs/POV_DISPLAY_ORIENTATION_GUIDE.md`.

## Phase 2 - Move Poi to Local WiFi

This makes the poi easier to access without manually reconnecting to its AP every time.

1. While still connected to the poi AP, open Advanced Settings.
2. Enter your local WiFi SSID/password.
3. Press Connect.
4. Wait for the UI to report a station (LAN) IP.
5. Reconnect your phone/laptop to the same local WiFi network.
6. Open the poi using its LAN IP shown in the UI.

Expected result:

- Web UI works from normal LAN WiFi, not only direct AP mode.

Repeat Phase 1 and Phase 2 for the second poi.

## Phase 3 - Pair Two Poi Together

Use this after both poi are individually working.

1. Make sure both poi are powered on.
2. Open Web UI for Poi A.
3. Go to sync/pairing section and discover peers.
4. Pair with Poi B.
5. Trigger a sync (images/patterns/settings).
6. Change pattern or brightness from Poi A.
7. Confirm Poi B follows (in mirror/sync mode).

Expected result:

- Both poi show coordinated content and settings.

For deeper pairing details, see `docs/POI_PAIRING.md`.

## Release-Ready Validation Checklist

Use this as your "go/no-go" list for first release:

- [ ] One poi can be controlled in AP mode via `http://192.168.4.1`
- [ ] One poi can join local WiFi and stay reachable via LAN IP
- [ ] Image upload and display works reliably
- [ ] Pattern switching, brightness, and frame rate work reliably
- [ ] Second poi passes the same checks
- [ ] Pairing and sync between two poi works reliably
- [ ] Both poi recover correctly after power cycle
- [ ] Spin test done safely at low, medium, and intended brightness

## Common Beginner Issues (Quick Fixes)

- **"I can connect to WiFi but UI does not load"**
  - Use `http://192.168.4.1` directly
  - Disable cellular data temporarily on phone
  - Ensure you are connected to the correct network

- **"UI loads but LEDs do not respond"**
  - Check Teensy-ESP32 serial wiring (TX/RX crossed)
  - Check shared ground
  - Check LED data/clock wiring and power

- **"Worked in AP mode, not working on local WiFi"**
  - Confirm both phone/laptop and poi are on same LAN
  - Use LAN IP shown in UI, not `192.168.4.1`
  - Reboot poi and router, then retry

- **"Pairing is inconsistent"**
  - Test each poi standalone first
  - Keep devices close during first pairing
  - Re-run discovery and sync after both are stable

## Suggested Next Step After First Pair

Once this guide is fully green, capture your exact hardware and firmware versions in your release notes so users can reproduce your successful setup.

For bench sessions and live testing, use the printable one-page checklist:

- `docs/FIRST_PAIR_FIELD_CHECKLIST.md`
