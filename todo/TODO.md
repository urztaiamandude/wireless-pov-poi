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

## Android App Enhancements (Future)

- [ ] Motion control - Use phone gyroscope/accelerometer to control patterns (tilt phone to change speed/direction)
- [ ] Phone microphone audio-reactive patterns - Use phone mic instead of hardware mic on Teensy, process audio on phone and send pattern commands
- [ ] Visual sequence builder - Drag-and-drop timeline editor for creating sequences
- [ ] Pattern editor/creator - Visual pattern design tool with custom color gradients and timing
- [ ] Background services - Keep connection alive in background, auto-reconnect on WiFi changes
- [ ] Home screen widgets - Quick controls without opening app, one-tap pattern switching
- [ ] Advanced image processing - Use Android image libraries (OpenCV, etc.) for better filters, dithering, edge detection

