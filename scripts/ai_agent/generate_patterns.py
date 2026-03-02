#!/usr/bin/env python3
"""
generate_patterns.py - LED pattern generation stub for wireless-pov-poi.

In CI this script runs the full code analysis (analyze_code + detect_errors)
and additionally outputs a template/suggestions for new LED patterns based
on the existing firmware.

Usage:
    python3 scripts/ai_agent/generate_patterns.py
    python3 scripts/ai_agent/generate_patterns.py '{"name":"my_run","num_patterns":4}'
"""

from __future__ import annotations

import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
OUTPUT_DIR = SCRIPT_DIR / "output"

# ---------------------------------------------------------------------------
# Pattern templates for the Teensy APA102 firmware
# ---------------------------------------------------------------------------

_PATTERN_TEMPLATES = [
    {
        "id": 18,
        "name": "Dual Color Chase",
        "description": "Two colors chasing each other around the LED strip.",
        "code": """\
case 18: {  // Dual Color Chase
    uint8_t pos = (millis() / (256 - patternSpeed)) % NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t dist = abs(i - (int)pos) % NUM_LEDS;
        leds[i] = (dist < 4) ? CRGB::Cyan : CRGB::DarkBlue;
    }
    break;
}""",
    },
    {
        "id": 19,
        "name": "Rainbow Sparkle",
        "description": "Rainbow gradient with random bright sparkle pixels.",
        "code": """\
case 19: {  // Rainbow Sparkle
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hue + i * (256 / NUM_LEDS), 200, 180);
    }
    if (random8() < 40) {
        leds[random8(NUM_LEDS)] = CRGB::White;
    }
    break;
}""",
    },
    {
        "id": 20,
        "name": "Gradient Wipe",
        "description": "Solid color that fills from one end to the other and back.",
        "code": """\
case 20: {  // Gradient Wipe
    static uint8_t fillPos = 0;
    static int8_t dir = 1;
    leds[fillPos] = ColorFromPalette(RainbowColors_p, hue, 255, LINEARBLEND);
    fillPos += dir;
    if (fillPos >= NUM_LEDS || fillPos == 0) dir = -dir;
    break;
}""",
    },
    {
        "id": 21,
        "name": "Sine Wave",
        "description": "Sinusoidal brightness wave scrolling along the strip.",
        "code": """\
case 21: {  // Sine Wave
    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t brightness = sin8(hue + i * (256 / NUM_LEDS));
        leds[i] = CHSV(160, 255, brightness);
    }
    break;
}""",
    },
]


def _save_outputs(patterns: List[Dict], run_name: str) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    generated_dir = OUTPUT_DIR / "generated_patterns"
    generated_dir.mkdir(parents=True, exist_ok=True)

    ts = datetime.now(timezone.utc).isoformat()

    # Save machine-readable JSON
    data = {
        "timestamp": ts,
        "run_name": run_name,
        "patterns": patterns,
    }
    json_path = OUTPUT_DIR / "findings.json"
    with open(json_path, "w", encoding="utf-8") as fh:
        json.dump(
            {"timestamp": ts, "run_name": run_name,
             "summary": {"errors": 0, "warnings": 0, "info": len(patterns)},
             "findings": [
                 {"id": f"pattern-{p['id']:04d}", "source": "generate_patterns",
                  "severity": "info", "description": f"Pattern {p['id']}: {p['name']} — {p['description']}"}
                 for p in patterns
             ]},
            fh, indent=2,
        )

    # Save each pattern as a standalone .cpp snippet
    for p in patterns:
        snippet_path = generated_dir / f"pattern_{p['id']:02d}_{p['name'].lower().replace(' ', '_')}.cpp"
        with open(snippet_path, "w", encoding="utf-8") as fh:
            fh.write(f"// Pattern {p['id']}: {p['name']}\n")
            fh.write(f"// {p['description']}\n")
            fh.write("// Add this case to the displayPattern() switch in teensy_firmware.ino\n\n")
            fh.write(p["code"])
            fh.write("\n")
        print(f"  Saved pattern snippet: {snippet_path}")

    # Save summary Markdown report
    md_path = OUTPUT_DIR / "report.md"
    with open(md_path, "w", encoding="utf-8") as fh:
        fh.write("# AI Agent — Generated LED Pattern Suggestions\n\n")
        fh.write(f"**Run:** `{run_name}`  \n**Timestamp:** {ts}  \n\n")
        fh.write(f"Generated {len(patterns)} pattern suggestion(s) for the Teensy APA102 firmware.\n\n")
        for p in patterns:
            fh.write(f"## Pattern {p['id']}: {p['name']}\n\n")
            fh.write(f"{p['description']}\n\n")
            fh.write(f"```cpp\n{p['code']}\n```\n\n")
            fh.write(
                f"**Integration:** Add the case above to `displayPattern()` in "
                f"`teensy_firmware/teensy_firmware.ino` and update `MAX_PATTERNS`.\n\n"
            )
    print(f"  Saved report: {md_path}")


def main() -> int:
    run_name = "generate_patterns"
    num_patterns = 4
    if len(sys.argv) > 1:
        try:
            args = json.loads(sys.argv[1])
            run_name = args.get("name", run_name)
            num_patterns = int(args.get("num_patterns", num_patterns))
        except (json.JSONDecodeError, AttributeError, TypeError, ValueError):
            pass

    print(f"[generate_patterns] Generating {num_patterns} pattern(s) (run: {run_name})")
    patterns = _PATTERN_TEMPLATES[:num_patterns]
    _save_outputs(patterns, run_name)
    print(f"[generate_patterns] ✅ Done — {len(patterns)} pattern(s) saved to output/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
