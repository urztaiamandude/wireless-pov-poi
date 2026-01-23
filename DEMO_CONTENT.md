# Demo Content Guide

This document describes the built-in demo images, patterns, and sequences available in the Nebula POI firmware.

## Overview

The firmware comes pre-loaded with demo content that displays automatically on startup. This allows you to test the device immediately without uploading custom content.

## Demo Images

The firmware includes 3 pre-loaded demo images (31x31 pixels):

### Image 0: Smiley Face 😊
- **Description**: A yellow smiley face with eyes and smile
- **Colors**: Yellow on black background
- **Use**: Great for testing basic image display
- **Access**: Set mode to "Image Display" (mode 1) with index 0

### Image 1: Rainbow Gradient 🌈
- **Description**: A smooth rainbow gradient across the entire image
- **Colors**: Full spectrum HSV gradient
- **Use**: Perfect for testing color accuracy and smooth gradients
- **Access**: Set mode to "Image Display" (mode 1) with index 1

### Image 2: Heart ❤️
- **Description**: A red heart shape
- **Colors**: Red on black background
- **Use**: Ideal for romantic displays or testing shape rendering
- **Access**: Set mode to "Image Display" (mode 1) with index 2

## Demo Patterns

The firmware includes 5 pre-configured animated patterns:

### Pattern 0: Rainbow 🌈
- **Type**: Rainbow (type 0)
- **Description**: Animated rainbow that cycles through all hues
- **Colors**: Full spectrum
- **Speed**: 50 (medium)
- **Access**: Set mode to "Pattern Display" (mode 2) with index 0

### Pattern 1: Fire 🔥
- **Type**: Fire (type 4)
- **Description**: Realistic fire effect with flickering flames
- **Colors**: Orange-red to yellow
- **Speed**: 120 (fast)
- **Access**: Set mode to "Pattern Display" (mode 2) with index 1

### Pattern 2: Comet ☄️
- **Type**: Comet (type 5)
- **Description**: Bright comet with fading tail bouncing up and down
- **Colors**: Cyan to blue
- **Speed**: 80 (medium-fast)
- **Access**: Set mode to "Pattern Display" (mode 2) with index 2

### Pattern 3: Breathing 💨
- **Type**: Breathing (type 6)
- **Description**: Smooth pulsing effect like breathing
- **Colors**: Purple
- **Speed**: 60 (medium)
- **Access**: Set mode to "Pattern Display" (mode 2) with index 3

### Pattern 4: Plasma 🌊
- **Type**: Plasma (type 10)
- **Description**: Organic flowing plasma effect
- **Colors**: Green to magenta
- **Speed**: 40 (medium-slow)
- **Access**: Set mode to "Pattern Display" (mode 2) with index 4

## Demo Sequence

The firmware includes 1 pre-configured sequence that cycles through multiple items:

### Sequence 0: Demo Mix 🎬
- **Description**: Cycles through demo images and patterns
- **Loop**: Yes (repeats continuously)
- **Duration**: 10 seconds total (2 seconds per item)
- **Items**:
  1. Smiley Face image (2 seconds)
  2. Rainbow pattern (2 seconds)
  3. Heart image (2 seconds)
  4. Fire pattern (2 seconds)
  5. Rainbow Gradient image (2 seconds)
- **Access**: Set mode to "Sequence" (mode 3) with index 0

## Available Pattern Types

The firmware supports 16 different pattern types (0-15):

### Basic Patterns (0-10)
- **0**: Rainbow - Animated rainbow cycling through hues
- **1**: Wave - Sine wave brightness modulation
- **2**: Gradient - Static color gradient
- **3**: Sparkle - Random twinkling sparkles
- **4**: Fire - Realistic fire simulation
- **5**: Comet - Bouncing comet with tail
- **6**: Breathing - Smooth pulsing effect
- **7**: Strobe - Quick flashing
- **8**: Meteor - Falling meteor with sparkly tail
- **9**: Wipe - Progressive color fill/clear
- **10**: Plasma - Organic flowing colors

### Music Reactive Patterns (11-15)
*Requires microphone connected to analog pin A0*

- **11**: VU Meter - Audio level visualization with beat detection
- **12**: Pulse - Whole strip pulses with beat
- **13**: Rainbow - Rainbow colors that shift with beat
- **14**: Center - Audio reactive center-out effect
- **15**: Sparkle - Sparkles triggered by audio peaks

## Using Demo Content

### Via Web Interface

1. Connect to the POI WiFi network: `POV-POI-WiFi` (password: `povpoi123`)
2. Open browser to: `http://192.168.4.1`
3. Select display mode from dropdown:
   - **Idle (Off)**: LEDs off
   - **Image Display**: Shows stored images
   - **Pattern Display**: Shows animated patterns
   - **Sequence**: Plays sequences
   - **Live Mode**: Real-time control from web interface
4. Set the content index (0-15) to select which image/pattern/sequence
5. Adjust brightness and frame rate as needed

### Via Serial Commands

Send commands to the Teensy via Serial1 (115200 baud):

```
Set Mode Command: 0xFF 0x01 0x02 [mode] [index] 0xFE
- mode: 0=idle, 1=image, 2=pattern, 3=sequence, 4=live
- index: 0-15 (which image/pattern/sequence to display)

Example: Display Pattern 1 (Fire)
0xFF 0x01 0x02 0x02 0x01 0xFE
```

## Creating Custom Content

### Custom Images
1. Use the web interface to upload images
2. Images are automatically resized to 31 pixels wide
3. Height is adjusted to maintain aspect ratio (max 64 pixels)
4. Uploaded images replace slot 0

### Custom Patterns
1. Use the web interface pattern controls
2. Select pattern type (0-15)
3. Choose colors using color pickers
4. Adjust speed slider (1-255)
5. Pattern is stored in slot 0

### Custom Sequences
1. Create sequence via serial command (0x04)
2. Specify up to 10 items with durations
3. Items can be images or patterns
4. Enable looping for continuous playback

## Troubleshooting

### No Display
- Check that mode is not set to "Idle" (mode 0)
- Verify brightness is not set to 0
- Ensure the selected index has active content

### Patterns Not Animating
- Check frame rate setting (should be 10-120 FPS)
- Verify pattern speed is not 0
- Some patterns are subtle at low speeds

### Images Look Wrong
- Images may need vertical flip in converter
- Check that image dimensions are correct
- Verify RGB color order matches your LED strip

### Sequence Not Playing
- Ensure sequence has count > 0
- Check that sequence items reference valid content
- Verify durations are reasonable (> 100ms)

## Technical Details

### Memory Usage
- Each image: ~2,883 bytes (31×31×3)
- Each pattern: 9 bytes
- Each sequence: 31 bytes
- Total demo content: ~8.7 KB

### Performance
- Default frame rate: 50 FPS (20ms per frame)
- Pattern animations update every frame
- Image columns advance every frame
- Sequence timing uses millisecond precision

### LED Configuration
- Total LEDs: 32 (APA102)
- Display LEDs: 31 (LEDs 1-31)
- LED 0: Reserved for level shifting (always black)
- Color order: BGR (configurable)

## Next Steps

1. **Test Demo Content**: Power on and watch the demo sequence
2. **Upload Custom Images**: Use the web interface to upload your own images
3. **Create Custom Patterns**: Experiment with different pattern types and colors
4. **Build Sequences**: Combine images and patterns into custom shows
5. **Music Reactive**: Connect a microphone for audio-reactive patterns

For more information, see:
- [README.md](README.md) - Main project documentation
- [QUICKSTART.md](QUICKSTART.md) - Getting started guide
- [docs/API.md](docs/API.md) - Complete API reference
- [examples/README.md](examples/README.md) - Image converter tools
