# First Pair Field Checklist (Printable)

Use this one-page checklist during bench setup and live spin testing.

Date: __________  
Builder: __________  
Poi IDs/Names: ________________________________

## 0) Safety (Do First)

- [ ] Clear test area
- [ ] Wiring inspected (no loose/exposed shorts)
- [ ] Low brightness set for first test (40-80)
- [ ] Secure mounting confirmed before spin
- [ ] Eye safety: no direct stare at full-bright LEDs

## 1) Poi A - AP Mode Bring-Up

- [ ] Power on Poi A
- [ ] Phone/laptop joins `POV-POI-WiFi` (`povpoi123`)
- [ ] Open `http://192.168.4.1`
- [ ] Dashboard/status visible
- [ ] Pattern mode changes LED output
- [ ] Brightness slider updates LEDs
- [ ] Frame rate updates LEDs
- [ ] One image uploads and displays

Notes (Poi A AP): ______________________________________________

## 2) Poi A - Local WiFi Handoff

- [ ] In Advanced Settings, enter local SSID/password
- [ ] Press Connect and wait for LAN IP
- [ ] Reconnect phone/laptop to same LAN
- [ ] Open Poi A using LAN IP
- [ ] Controls still responsive over LAN

Poi A LAN IP: ______________________

## 3) Poi B - AP Mode Bring-Up

- [ ] Power on Poi B
- [ ] Join Poi B AP
- [ ] Open `http://192.168.4.1`
- [ ] Dashboard/status visible
- [ ] Pattern/brightness/frame rate working
- [ ] One image uploads and displays

Notes (Poi B AP): ______________________________________________

## 4) Poi B - Local WiFi Handoff

- [ ] Enter local SSID/password on Poi B
- [ ] Reconnect to LAN and open Poi B by IP
- [ ] Controls still responsive over LAN

Poi B LAN IP: ______________________

## 5) Pair + Sync Test

- [ ] Open Poi A UI and discover peers
- [ ] Pair Poi A with Poi B
- [ ] Run sync (images/patterns/settings)
- [ ] Change pattern on Poi A and verify Poi B follows
- [ ] Change brightness on Poi A and verify Poi B follows
- [ ] Power-cycle both poi and verify pair recovers

## 6) Spin Validation

- [ ] Static control test at low brightness
- [ ] Spin test at low brightness
- [ ] Spin test at medium brightness
- [ ] Intended release brightness tested safely
- [ ] No resets/flicker/dropouts during spin

## 7) Release Go/No-Go

- [ ] AP mode reliable on both poi
- [ ] LAN mode reliable on both poi
- [ ] Pairing and sync reliable
- [ ] Image and pattern workflows reliable
- [ ] Reboot recovery reliable

Decision:  [ ] GO   [ ] NO-GO

Top issues to fix before release:

1. ______________________________________________
2. ______________________________________________
3. ______________________________________________
