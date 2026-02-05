# POV Display Orientation Guide

## LED Strip Layout Diagrams
- **Diagram**: ASCII representation of the 32 LED strip, showcasing how LED 0 is reserved for level shifting, LED 1 (bottom) displays row 0, and LED 31 (top) displays row 30.

```
    LED 31 (Top)
    +-------------+
    |             |
    |    LED 30   |
    |             |
    |    ...      |
    |             |
    |    LED 1    | <---- Row 0 of the image
    |             |
    +-------------+
    LED 0 (Level Shifting)
```

## Image Data Structure Explanation
- Describe how an image is structured for POV display, including pixel mappings to LEDs.

## POV Spinning Effect Visualization
- Explain how the POV effect creates 2D images when spinning, possibly using diagrams to illustrate the concept.

## Coordinate System Mapping
- **Mapping**: Image row 0 maps to LED 1, continuing up to LED 31 corresponding to image row 30.

## Test Pattern Verification Instructions
- Offer instructions for verifying static and spinning displays using specific patterns.

## Troubleshooting for Common Orientation Issues
- List common issues like upside-down images, wrong colors, LED 0 showing data, and stretched images with their fixes.

## Technical Specifications Table
- Provide a table summarizing technical specifications relevant to the display.

## Quick Reference Commands
- Include a section with important commands for quick reference.