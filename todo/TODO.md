# Project TODOs

- [x] Implement distinct Teensy POV patterns for all ESP32 UI pattern IDs (Fire, Comet, Breathing, Strobe, Meteor, Wipe, Plasma) instead of mapping them onto the 4 core patterns. ✅ COMPLETED
  - [x] Fire pattern (ID 4) - heat simulation rising upward
  - [x] Comet pattern (ID 5) - bouncing bright head with fading tail
  - [x] Breathing pattern (ID 6) - smooth pulsing brightness
  - [x] Strobe pattern (ID 7) - quick on/off flashes
  - [x] Meteor pattern (ID 8) - falling particle with sparkly tail
  - [x] Wipe pattern (ID 9) - progressive fill then clear
  - [x] Plasma pattern (ID 10) - organic color mixing
  - [ ] Audio-reactive patterns (IDs 11-15) - Requires audio input hardware on pin A0. Currently falls back to sparkle pattern.
- [x] Add ESP32 HTTP endpoints and wiring to drive Teensy SD image management commands (save/load/list/delete) and surface results in the web UI. ✅ COMPLETED

