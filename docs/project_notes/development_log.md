# Development Log

Chronological record of major changes and milestones for the Wireless POV Poi project.

---

## 2025-02-13 — Project Memory System Initialized

- Created `docs/project_notes/` for architecture decisions and development history
- Created `.claude/skills/` to capture build, test, and development knowledge for AI assistants
- Existing project context lives in `CLAUDE.md` (general) and `AGENTS.md` (cloud-specific)

## Prior Milestones

- **Peer-to-peer synchronization** — multiple poi discover and sync images/patterns/settings
- **BLE support** — Nordic UART Service for cross-platform (Windows, Chrome/Edge) control
- **Music-reactive patterns** — VU meter, pulse, rainbow, center burst, sparkle (types 11-15)
- **SD card storage** — optional microSD for expanded image library
- **Web UI** — React + TypeScript + Tailwind CSS interface served from ESP32 SPIFFS
- **18 animated patterns** — basic (0-10), music-reactive (11-15), advanced (16-17)
