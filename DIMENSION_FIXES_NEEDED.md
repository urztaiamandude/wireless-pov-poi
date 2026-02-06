# Dimension Labeling Fixes Needed

## Problem Statement

Throughout the repository, dimensions are incorrectly labeled. The LED strip is **VERTICAL** (31 LEDs tall), but many places incorrectly state "31 pixels wide".

## Correct Specification

- **Width**: Variable (calculated proportionally from original image)
- **Height**: 31 pixels (FIXED - matches 31 display LEDs)
- **Format**: Always Width×Height (W×H)
- **LED Orientation**: Vertical strip, 31 LEDs from bottom to top
- **Display**: LEDs show vertical columns, scrolling left to right as poi spins

## Files Requiring Fixes

### Documentation Files
- [ ] README.md - "31 pixels wide" → "31 pixels tall"
- [ ] docs/README.md - "31 pixels wide" → "31 pixels tall"
- [ ] docs/IMAGE_CONVERSION.md - Multiple instances
- [ ] TESTING.md - "31 pixels wide" → "31 pixels tall"
- [ ] examples/README.md - "31 pixels wide" → "31 pixels tall"
- [ ] examples/GUI_GUIDE.md - Dimension labels need clarification
- [ ] examples/BUILD_INSTALLER.md - Dimension references
- [ ] examples/ORIENTATION_FIX.md - Verify dimension labels
- [ ] DEMO_CONTENT.md - "31x31 pixels" needs W×H clarification

### Firmware Documentation
- [ ] teensy_firmware/README.md - "31x64 pixels max" needs clarification
- [ ] firmware/teensy41/README.md - "31x64 pixel POV images" needs clarification
- [ ] FIRMWARE_ARCHITECTURE.md - "31x64 max" needs clarification
- [ ] ESP32_COMMAND_IMPLEMENTATION.md - Protocol dimension references

### API Documentation
- [ ] docs/API.md - Command format dimension references
- [ ] docs/SD_CARD_STORAGE.md - File format dimension references

### Code Files (Comments)
- [ ] examples/image_converter.py - Already fixed ✓
- [ ] examples/image_converter_gui.py - Already fixed ✓

## Common Errors to Fix

### Pattern 1: "31 pixels wide"
**Wrong:** "Images should be 31 pixels wide"
**Right:** "Images should be 31 pixels tall (height)"

### Pattern 2: "31x64" without clarification
**Wrong:** "31x64 pixels max"
**Right:** "Width×Height format, with height fixed at 31 pixels (max width ~200)"

### Pattern 3: Ambiguous dimension references
**Wrong:** "Resize to 31 pixels"
**Right:** "Resize to 31 pixels tall (height)"

### Pattern 4: "width × height" when meaning "height × width"
**Wrong:** "31 pixels wide × variable height"
**Right:** "Variable width × 31 pixels tall (height)"

## Fix Strategy

1. **Phase 1**: Fix all markdown documentation files
2. **Phase 2**: Fix code comments in Python files
3. **Phase 3**: Verify firmware comments (C++/Arduino)
4. **Phase 4**: Create comprehensive dimension reference guide

## Dimension Reference Guide (To Be Created)

Create a new file `DIMENSIONS_REFERENCE.md` that clearly explains:
- LED strip orientation (vertical)
- Image dimensions (W×H with H=31 fixed)
- How images are displayed (vertical columns, left to right)
- Scaling logic (proportional scale factor)
- Maximum dimensions and why

## Status

- [x] Problem identified
- [x] Tracking document created
- [ ] Phase 1: Documentation fixes
- [ ] Phase 2: Python code fixes
- [ ] Phase 3: Firmware comment fixes
- [ ] Phase 4: Reference guide created
- [ ] All changes committed and pushed
