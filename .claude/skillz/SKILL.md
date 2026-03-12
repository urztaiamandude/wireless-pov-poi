---
name: project-memory
description: Set up and maintain a structured project memory system in docs/project_notes/
  that tracks bugs with solutions, architectural decisions, key project facts, and work
  history for the wireless-pov-poi (Nebula Poi) project. Use this skill when asked to
  "set up project memory", "track our decisions", "log a bug fix", "update project memory",
  "initialize memory system", "what did we decide about X", "have we seen this bug before",
  or "add to key facts".
---

# Project Memory Skill — wireless-pov-poi

## Overview

This skill manages a structured knowledge base in `docs/project_notes/` for the Nebula POV Poi project (Teensy 4.1 + ESP32-S3 + APA102 + FastLED). It prevents re-solving known problems and enforces consistency with established architectural decisions.

## When to Use This Skill

Invoke this skill when:

- Starting a new development session (always check memory files first)
- Encountering an error or unexpected behavior ("have we seen this before?")
- Before proposing any architectural change (check decisions.md first)
- After fixing a bug (log it immediately)
- After any hardware/firmware decision is made
- When looking up pins, constants, URLs, or firmware paths
- The user asks "which library do we use for X?" or "what's the image height?"

## Memory File Structure

```
docs/
└── project_notes/
    ├── bugs.md        — Known bugs with root causes, solutions, and prevention
    ├── decisions.md   — Architectural Decision Records (ADRs)
    ├── key_facts.md   — Pins, constants, URLs, firmware paths, hardware specs
    └── issues.md      — Work log: completed, in-progress, backlog, open issues
```

## Core Protocols

### Before Starting Any Work

1. Read `docs/project_notes/key_facts.md` — know the current hardware config, pin assignments, and firmware locations
2. Read `docs/project_notes/decisions.md` — do not propose anything that contradicts an active ADR without flagging it
3. Check `docs/project_notes/bugs.md` — if the issue feels familiar, it's probably already documented

### When Encountering a Bug or Error

1. Search `bugs.md` for the error message or symptom keywords
2. If found: apply the documented solution; do not re-investigate
3. If not found: investigate, then document a new entry using this format:

```markdown
### YYYY-MM-DD — BUG-NNN: Short descriptive title

**Issue**: What happened / what the user observed
**Root Cause**: Why it happened
**Solution**: Exact steps to fix it
**Prevention**: How to avoid it in future code/decisions
```

### When Proposing Architecture or Library Changes

1. Read all active ADRs in `decisions.md`
2. Check if the change contradicts any ADR — if so, flag it explicitly
3. If proceeding with a new decision, add an ADR entry:

```markdown
### ADR-NNN: Short title

**Date**: YYYY-MM-DD  
**Status**: Active

**Context**: Why the decision was needed
**Decision**: What was decided
**Alternatives**: What else was considered
**Consequences**: What this means going forward
```

### When Adding Key Facts

Add to `key_facts.md` only:
- ✅ Pin numbers, GPIO assignments
- ✅ Firmware `#define` constants
- ✅ Hardware model numbers and specs
- ✅ WiFi SSID, IP addresses, port numbers
- ✅ File paths to canonical firmware versions
- ✅ Tool commands and converter usage
- ❌ NEVER: passwords, API keys, tokens, secrets — those go in `.env` (gitignored)

### When Completing Work

Update `issues.md` with a log entry:

```markdown
- ✅ FEATURE/FIX-NNN: Brief description — YYYY-MM-DD
```

Move items from Backlog → Active → Completed as work progresses.

## Critical Project Facts (Quick Reference)

These are the most-frequently-needed facts. Always verify against key_facts.md.

- **Display height**: 31 pixels (fixed — one per display LED; LED 32 is level-shift only)
- **Image width**: `round(src_width × (31 / src_height))` — never hardcode
- **LED library**: FastLED — do NOT introduce competing LED libraries (ADR-005)
- **Large buffers — Teensy**: `EXTMEM` keyword (ADR-009)
- **Large buffers — ESP32-S3**: `ps_malloc()` — never `malloc()` for large allocs (ADR-009)
- **Canonical Teensy firmware**: `teensy_firmware/teensy_firmware.ino` (ADR-001)
- **WiFi IP**: `192.168.4.1` — mDNS unreliable on Windows (BUG-006)
- **BLE profile**: Nordic UART Service (NUS) (ADR-010)
- **UART baud**: 115200, Teensy Serial1 ↔ ESP32 Serial2 (ADR-008)
- **Multi-POI**: Peer-to-peer, no master/slave (ADR-006)

## CLAUDE.md Memory-Aware Protocols

When updating CLAUDE.md, ensure it contains this section:

```markdown
## Project Memory System

### Auto-Check Protocols

**Before proposing any architectural change:**
- Check `docs/project_notes/decisions.md` for active ADRs
- Do not contradict an active ADR without explicit flagging

**When encountering any error or unexpected behavior:**
- Search `docs/project_notes/bugs.md` first
- If solution found: apply it; do not re-investigate
- If not found: investigate, fix, then document a new BUG entry

**When writing any firmware code:**
- Verify pin assignments against `docs/project_notes/key_facts.md`
- Verify EXTMEM/ps_malloc usage for any buffer > ~50 KB
- Default to Arduino IDE firmware (`teensy_firmware/`) not PlatformIO

**When completing any task:**
- Update `docs/project_notes/issues.md` with a ✅ entry
```

## Setup Instructions (First Run)

If `docs/project_notes/` does not exist in the repo:

1. Create the directory: `mkdir -p docs/project_notes`
2. Copy the four template files from this skill's bundled templates (or create from formats above)
3. Populate `key_facts.md` with current hardware config from README
4. Add any known bugs from TROUBLESHOOTING.md into `bugs.md`
5. Add any known decisions from ARCHITECTURE.md into `decisions.md`
6. Update CLAUDE.md with the memory-aware protocols section above
7. Commit: `git add docs/project_notes/ && git commit -m "chore: initialize project memory system"`

## Success Criteria

After this skill runs successfully:

- [ ] `docs/project_notes/bugs.md` exists and has at least one entry
- [ ] `docs/project_notes/decisions.md` exists with active ADRs
- [ ] `docs/project_notes/key_facts.md` has pins, constants, and firmware paths
- [ ] `docs/project_notes/issues.md` has work log entries
- [ ] CLAUDE.md references the project_notes/ files and includes memory-aware protocols
- [ ] No secrets are stored in any project_notes/ file
