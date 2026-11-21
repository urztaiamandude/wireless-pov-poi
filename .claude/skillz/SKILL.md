---
name: project-memory
description: Set up and maintain a structured project memory system in docs/project_notes/
  that tracks bugs with solutions, architectural decisions, key project facts, and work
  history for the wireless-pov-poi (Nebula Poi) project. Use this skill when asked to
  "set up project memory", "track our decisions", "log a bug fix", "update project memory",
  "initialize memory system", "what did we decide about X", "have we seen this bug before",
  or "add to key facts".
---

# Project memory skill

This skill maintains `docs/project_notes/` for Nebula Poi. [AGENTS.md](../../AGENTS.md) is the ruleset. [README.md](../../README.md) is the hardware description. A note in `docs/project_notes/` that disagrees with either of those is stale. Update the note. Do not follow it, and do not copy the agent rules into `CLAUDE.md`.

`docs/project_notes/` already exists. Do not recreate it and do not add a parallel memory directory.

## When to use it

- At the start of a session, after reading AGENTS.md and the README.
- When an error looks familiar.
- Before changing an architectural decision that already has an ADR.
- After fixing a bug, or after a hardware decision. A hardware decision also updates the README, the BOM, and the wiring and power docs in the same PR (see AGENTS.md).

## Files

```
docs/project_notes/
├── bugs.md        — bugs, root cause, fix, prevention
├── decisions.md   — ADRs
├── key_facts.md   — pins, constants, paths
└── issues.md      — work log
```

## Before starting

1. Read [AGENTS.md](../../AGENTS.md) and [README.md](../../README.md).
2. Read `docs/project_notes/key_facts.md`, `decisions.md`, and `bugs.md`.
3. Where a note disagrees with AGENTS.md or the README, treat the note as the thing to fix.

## Logging a bug

Search `bugs.md` first. If the entry exists, apply it. If it does not, add:

```markdown
### YYYY-MM-DD — BUG-NNN: Short title

**Issue**: What happened
**Root Cause**: Why
**Solution**: How it was fixed
**Prevention**: What to do next time
```

## Logging a decision

Read the active ADRs first. If the new decision contradicts one, say so in the PR. Then add:

```markdown
### ADR-NNN: Short title

**Date**: YYYY-MM-DD
**Status**: Active

**Context**: Why this came up
**Decision**: What was decided
**Alternatives**: What else was considered
**Consequences**: What changes because of this
```

Mark the old ADR superseded. Do not leave two active ADRs that describe different hardware.

## Key facts

Add pins, `#define` values, part numbers, and firmware paths to `key_facts.md` only when they match the README and AGENTS.md. Do not store passwords, tokens, or API keys there.

Facts that are current (verify against AGENTS.md before copying them elsewhere):

- Firmware trees: `firmware/teensy_firmware/` and `firmware/esp32_firmware/`. There is no `firmware/teensy41/` tree.
- Preferred build: PlatformIO, commands in AGENTS.md. Arduino IDE is the other flashing path.
- UART: Teensy `Serial1` pins 0/1 ↔ ESP32 `Serial2` GPIO16/17, 115200.
- 32 APA102 LEDs. Sacrificial count default is 0 and stays configurable. There is no LED 32.
- Level shift is two SN74AHCT1G125 buffers.
- PSRAM on the Teensy is required. Large Teensy buffers in the sketch use `EXTMEM`.
- The ESP32 sources do not call `ps_malloc()`. Do not rewrite allocations just to match the old skill line.
- LED library is FastLED. Do not add another LED library.
- WiFi AP address is `192.168.4.1`. mDNS hostname is `povpoi-` plus a short device id, not `povpoi.local`.
- BLE is the Nordic UART Service. Multi-poi sync is peer-to-peer.

## After the work

Add a line to `issues.md`:

```markdown
- YYYY-MM-DD: FEATURE or FIX-NNN — one sentence
```

The PR description still has to name the files touched and include the handoff from AGENTS.md. A line in `issues.md` does not replace that.
