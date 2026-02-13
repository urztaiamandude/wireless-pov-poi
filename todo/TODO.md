# Project TODOs

## Completed

- [x] Implement distinct Teensy POV patterns for all ESP32 UI pattern IDs (Fire, Comet, Breathing, Strobe, Meteor, Wipe, Plasma) instead of mapping them onto the 4 core patterns.
  - [x] Fire pattern (ID 4) - heat simulation rising upward
  - [x] Comet pattern (ID 5) - bouncing bright head with fading tail
  - [x] Breathing pattern (ID 6) - smooth pulsing brightness
  - [x] Strobe pattern (ID 7) - quick on/off flashes
  - [x] Meteor pattern (ID 8) - falling particle with sparkly tail
  - [x] Wipe pattern (ID 9) - progressive fill then clear
  - [x] Plasma pattern (ID 10) - organic color mixing
- [x] Add ESP32 HTTP endpoints and wiring to drive Teensy SD image management commands (save/load/list/delete) and surface results in the web UI.
- [x] Implement image/pattern listing in ESP32 `/api/sync/data` endpoint.
- [x] Fix Flutter `Model` class to extend `ChangeNotifier` (was broken with `ChangeNotifierProvider`).
- [x] Fix `isIntialized` typo in `ble_uart.dart` and `welcome.dart`.
- [x] Remove non-existent `fonts/` asset directory from Flutter `pubspec.yaml`.
- [x] Generate 10 default pattern BMP files for Flutter app database seeding.
- [x] Remove dead empty `initState()` override in `welcome.dart`.

## Remaining - Requires Hardware

- [ ] **Audio-reactive patterns (IDs 11-15)** - Requires MAX9814 microphone hardware on pin A0. Currently falls back to sparkle pattern.
- [ ] **PlatformIO firmware integration testing** - Feature-complete code, awaiting hardware validation with physical Teensy 4.1 + ESP32 setup. All code compiles without errors.
- [ ] **IMU/gyroscope rotation detection** - Would enable auto-speed POV rendering. Requires additional hardware sensor.

## Remaining - Software (Needs User Decision)

- [ ] **Flutter app: Settings page** - Currently a placeholder (`home.dart:24`). Decide what settings to expose (brightness defaults, BLE scan timeout, theme, etc.).
- [ ] **Flutter app: Pattern import** - "Import Pattern" button is a stub (`home.dart:107`). Needs image picker integration and BMP conversion pipeline.
- [ ] **Flutter app: Missing platform configs** - Only Windows and Web are configured. Android/iOS/Linux/macOS platform directories are missing. Run `flutter create .` in the app directory to regenerate if needed.
- [ ] **Flutter app: PWA icons** - `web/manifest.json` references icon files (`icons/Icon-192.png`, etc.) that don't exist. Generate or add app icons.
- [ ] **Duplicate `wireless-pov-poi/` subdirectory** - A full mirror of the repository exists inside itself at `wireless-pov-poi/wireless-pov-poi/`. Should be removed to avoid confusion (`git rm -r wireless-pov-poi/`).
- [ ] **ESP32 SPIFFS static files** - Routes registered for `/style.css` and `/script.js` but no SPIFFS files exist. These are non-functional fallback routes (all CSS/JS is inline in the HTML). Remove the dead routes or add the files.

## Future Enhancements (Not Blocking)

- [ ] Battery power management system
- [ ] Music synchronization across multiple poi
- [ ] Multiple poi wireless synchronization (peer-to-peer)
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Cloud storage for patterns/images

