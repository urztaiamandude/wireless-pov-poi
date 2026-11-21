# CLAUDE.md

Follow [AGENTS.md](AGENTS.md). That file is the ruleset for every coding agent. The root [README.md](README.md) is the hardware description. This file only keeps notes that are easy to miss while editing and that AGENTS.md does not already spell out.

## While editing firmware

- LED loops use the runtime range `g_displayLedStart` and `g_displayLeds`. `NUM_LEDS` stays 32. `DEFAULT_SACRIFICIAL_LEDS` is 0.
- The Teensy display loop stays non-blocking.
- Image columns run along the strip: the LED at `g_displayLedStart` is the bottom of the image. See [docs/POV_DISPLAY_ORIENTATION_GUIDE.md](docs/POV_DISPLAY_ORIENTATION_GUIDE.md).
- Names: `camelCase` for functions and locals, `PascalCase` for types, `UPPER_CASE` for constants.
- The SD card is on the Teensy. The ESP32 reaches it only over the UART in [AGENTS.md](AGENTS.md). Do not add an SPI SD driver on the ESP32.

## Project memory

Bug logs and decision records live in `docs/project_notes/`. The procedure for updating them is [.claude/skillz/SKILL.md](.claude/skillz/SKILL.md). When a note disagrees with [AGENTS.md](AGENTS.md) or [README.md](README.md), those two win. Update the note. Do not paste a second copy of the agent rules into this file.
