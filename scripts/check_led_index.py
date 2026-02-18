"""Pre-edit hook: detect LED index 0 usage in display code.

Reads a JSON tool-use event from stdin. If the edit introduces a loop
starting at LED index 0 in firmware files, exits non-zero to block.
"""
import json
import re
import sys

def main():
    event = json.load(sys.stdin)
    tool_name = event.get("tool_name", "")
    tool_input = event.get("tool_input", {})

    # Only check Edit and Write on firmware files
    if tool_name not in ("Edit", "Write"):
        sys.exit(0)

    file_path = tool_input.get("file_path", "")
    is_firmware = any(
        ext in file_path
        for ext in (".ino", ".cpp", ".h", ".hpp")
    )
    if not is_firmware:
        sys.exit(0)

    # Get the new content being written/edited
    content = tool_input.get("new_string", "") or tool_input.get("content", "")
    if not content:
        sys.exit(0)

    # Patterns that indicate LED 0 being used in display loops
    # Match: for (int i = 0  or  for (int y = 0  or  for(int i=0
    # These are bugs when iterating over display LEDs
    led_zero_patterns = [
        r'for\s*\(\s*int\s+[iy]\s*=\s*0\s*;.*NUM_LEDS',
        r'for\s*\(\s*int\s+[iy]\s*=\s*0\s*;.*num_leds',
        r'for\s*\(\s*int\s+led\s*=\s*0\s*;',
        r'leds\s*\[\s*0\s*\]\s*=(?!=)',  # leds[0] = (assignment, not ==)
    ]

    for pattern in led_zero_patterns:
        match = re.search(pattern, content, re.IGNORECASE)
        if match:
            line = match.group(0)
            print(f"BLOCKED: Potential LED index 0 bug detected in firmware edit.", file=sys.stderr)
            print(f"  Pattern: {line}", file=sys.stderr)
            print(f"  File: {file_path}", file=sys.stderr)
            print(f"", file=sys.stderr)
            print(f"  LED 0 is the level-shift LED, NOT a display pixel.", file=sys.stderr)
            print(f"  Display loops MUST start at index 1:", file=sys.stderr)
            print(f"    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++)", file=sys.stderr)
            print(f"", file=sys.stderr)
            print(f"  If this is intentional (e.g., FastLED.clear()), add a comment:", file=sys.stderr)
            print(f"    // LED 0 intentional: <reason>", file=sys.stderr)
            sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    main()
