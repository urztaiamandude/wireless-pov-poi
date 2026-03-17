#!/usr/bin/env python3
"""
Generate static pattern preview images for all 18 firmware LED patterns.

Each pattern is rendered as a horizontal strip showing multiple animation frames
side by side (time flows left→right, LEDs are vertical columns within each frame).
This allows visual verification that each pattern behaves as expected before
building and uploading firmware.

Usage:
    python3 scripts/generate_pattern_previews.py

Output:
    pattern_previews/00_rainbow.png
    pattern_previews/01_wave.png
    ...
    pattern_previews/17_theater_chase.png

Requires: Pillow  (pip install Pillow)
"""

import math
import os
import random
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required. Install with:  pip install Pillow", file=sys.stderr)
    sys.exit(1)

# ── Configuration ────────────────────────────────────────────────────
# Matches default g_displayLeds (runtime-configurable via web UI Advanced Settings).
# Adjust if your device uses a different display LED count.
NUM_LEDS = 31
FRAMES_PER_PREVIEW = 120 # Number of animation frames to capture
LED_PIXEL_W = 2          # Width of each LED in pixels per frame column
LED_PIXEL_H = 6          # Height of each LED in pixels
LED_GAP = 1              # Gap between LEDs vertically
FRAME_GAP = 0            # Gap between frame columns
DEFAULT_SPEED = 50
DEFAULT_COLOR1 = (255, 0, 0)
DEFAULT_COLOR2 = (0, 0, 255)
BG_COLOR = (15, 23, 42)  # slate-900 matching the web UI

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                          "pattern_previews")

# ── Color helpers (match FastLED behavior) ───────────────────────────

def hsv_to_rgb(h: int, s: int, v: int) -> tuple[int, int, int]:
    """FastLED-compatible HSV→RGB (h,s,v all 0-255)."""
    h = h % 256
    hf = (h / 256) * 6
    hi = int(hf)
    f = hf - hi
    p = v * (1 - s / 255)
    q = v * (1 - (s / 255) * f)
    t = v * (1 - (s / 255) * (1 - f))
    r, g, b = 0.0, 0.0, 0.0
    sector = hi % 6
    if sector == 0:
        r, g, b = v, t, p
    elif sector == 1:
        r, g, b = q, v, p
    elif sector == 2:
        r, g, b = p, v, t
    elif sector == 3:
        r, g, b = p, q, v
    elif sector == 4:
        r, g, b = t, p, v
    elif sector == 5:
        r, g, b = v, p, q
    return (int(round(r)), int(round(g)), int(round(b)))


def sin8(x: int) -> int:
    """FastLED sin8: 0-255 in → 0-255 out."""
    return int(round((math.sin(((x & 0xFF) / 256) * 2 * math.pi - math.pi / 2) + 1) * 127.5))


def beatsin8(bpm: int, lo: int, hi: int, time_ms: float) -> int:
    """FastLED beatsin8 approximation."""
    beat = (time_ms / 1000) * bpm / 60
    val = (math.sin(beat * 2 * math.pi) + 1) / 2
    return int(round(lo + val * (hi - lo)))


def blend_color(c1: tuple, c2: tuple, amount: int) -> tuple[int, int, int]:
    """Blend two RGB tuples by amount (0-255)."""
    a = amount / 255
    return (
        int(round(c1[0] + (c2[0] - c1[0]) * a)),
        int(round(c1[1] + (c2[1] - c1[1]) * a)),
        int(round(c1[2] + (c2[2] - c1[2]) * a)),
    )


def heat_color(heat: int) -> tuple[int, int, int]:
    """FastLED HeatColor approximation."""
    heat = max(0, min(255, heat))
    if heat < 85:
        return (heat * 3, 0, 0)
    if heat < 170:
        return (255, (heat - 85) * 3, 0)
    return (255, 255, (heat - 170) * 3)


def scale_color(c: tuple, s: int) -> tuple[int, int, int]:
    """Scale RGB by 0-255 factor."""
    return (int(c[0] * s // 255), int(c[1] * s // 255), int(c[2] * s // 255))


def fade_to_black(c: tuple, amount: int) -> tuple[int, int, int]:
    """Reduce each channel proportionally."""
    sc = max(0.0, 1 - amount / 255)
    return (int(round(c[0] * sc)), int(round(c[1] * sc)), int(round(c[2] * sc)))


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


# ── Pattern definitions ──────────────────────────────────────────────

PATTERNS = [
    {"id": 0,  "label": "Rainbow",       "group": "basic",    "file": "00_rainbow"},
    {"id": 1,  "label": "Wave",          "group": "basic",    "file": "01_wave"},
    {"id": 2,  "label": "Gradient",      "group": "basic",    "file": "02_gradient"},
    {"id": 3,  "label": "Sparkle",       "group": "basic",    "file": "03_sparkle"},
    {"id": 4,  "label": "Fire",          "group": "basic",    "file": "04_fire"},
    {"id": 5,  "label": "Comet",         "group": "basic",    "file": "05_comet"},
    {"id": 6,  "label": "Breathing",     "group": "basic",    "file": "06_breathing"},
    {"id": 7,  "label": "Strobe",        "group": "basic",    "file": "07_strobe"},
    {"id": 8,  "label": "Meteor",        "group": "basic",    "file": "08_meteor"},
    {"id": 9,  "label": "Wipe",          "group": "basic",    "file": "09_wipe"},
    {"id": 10, "label": "Plasma",        "group": "basic",    "file": "10_plasma"},
    {"id": 11, "label": "VU Meter",      "group": "audio",    "file": "11_vu_meter"},
    {"id": 12, "label": "Pulse",         "group": "audio",    "file": "12_pulse"},
    {"id": 13, "label": "Audio Rainbow", "group": "audio",    "file": "13_audio_rainbow"},
    {"id": 14, "label": "Center Burst",  "group": "audio",    "file": "14_center_burst"},
    {"id": 15, "label": "Audio Sparkle", "group": "audio",    "file": "15_audio_sparkle"},
    {"id": 16, "label": "Split Spin",    "group": "advanced", "file": "16_split_spin"},
    {"id": 17, "label": "Theater Chase", "group": "advanced", "file": "17_theater_chase"},
]


# ── Simulated audio (matches useSimulatedAudio in PatternPreview.tsx) ─

SIM_BASS_FREQ = 1.8
SIM_BASS_AMP = 0.7
SIM_MID_FREQ = 3.2
SIM_MID_AMP = 0.4
SIM_MID_PHASE = 1.3
SIM_HI_FREQ = 7.5
SIM_HI_AMP = 0.2
SIM_HI_PHASE = 0.7
SIM_ENV_FREQ = 0.15
SIM_NOISE_AMP = 0.08


def simulated_audio_level(t_sec: float) -> int:
    """Return 0-255 simulated audio level at time t (seconds)."""
    bass = max(0, math.sin(t_sec * 2 * math.pi * SIM_BASS_FREQ) * SIM_BASS_AMP)
    mid = max(0, math.sin(t_sec * 2 * math.pi * SIM_MID_FREQ + SIM_MID_PHASE) * SIM_MID_AMP)
    hi = max(0, math.sin(t_sec * 2 * math.pi * SIM_HI_FREQ + SIM_HI_PHASE) * SIM_HI_AMP)
    envelope = 0.5 + 0.5 * math.sin(t_sec * 2 * math.pi * SIM_ENV_FREQ)
    noise = random.random() * SIM_NOISE_AMP
    raw = (bass + mid + hi + noise) * envelope
    return int(round(clamp(raw * 255, 0, 255)))


# ── Pattern state ────────────────────────────────────────────────────

class PatternState:
    """Persistent state across frames for a single pattern."""

    def __init__(self, n: int):
        self.heat = [0.0] * n
        self.comet_pos = 0
        self.comet_dir = 1
        self.meteor_pos = n - 1
        self.wipe_pos = 0
        self.wipe_filling = True
        self.strobe_on = False
        self.last_strobe_ms = 0.0
        self.peak_level = 0
        self.peak_decay = 0
        self.beat_hue = 0
        self.pulse_val = 0.0
        self.last_audio_level = 0
        self.rainbow_offset = 0.0
        self.sparkle_buf: list[tuple[int, int, int]] = [(0, 0, 0)] * n


# ── Pattern rendering (matches PatternPreview.tsx exactly) ───────────

def render_pattern(
    pattern_id: int,
    frame_idx: int,
    state: PatternState,
    speed: int = DEFAULT_SPEED,
    color1: tuple = DEFAULT_COLOR1,
    color2: tuple = DEFAULT_COLOR2,
    audio_level: int = 0,
) -> list[tuple[int, int, int]]:
    """Compute one frame of LED colors for the given pattern.

    Returns a list of NUM_LEDS RGB tuples.
    """
    n = NUM_LEDS
    c1, c2 = color1, color2
    st = state
    pattern_time = frame_idx  # ~60fps frame counter
    time_ms = frame_idx * 16.0  # approximate ms elapsed
    leds: list[tuple[int, int, int]] = [(0, 0, 0)] * n

    if pattern_id == 0:
        # ── Rainbow ──────────────────────────────────
        for i in range(n):
            hue = int(pattern_time * speed / 10 + i * 255 / n) % 256
            leds[i] = hsv_to_rgb(hue, 255, 255)

    elif pattern_id == 1:
        # ── Wave ─────────────────────────────────────
        for i in range(n):
            brightness = sin8(int(pattern_time * speed / 10 + i * 255 / n))
            leds[i] = scale_color(c1, brightness)

    elif pattern_id == 2:
        # ── Gradient ─────────────────────────────────
        time_offset = int((time_ms / 500) * speed) & 0xFF
        for i in range(n):
            phase = (int(i * 255 / n) + time_offset) & 0xFF
            leds[i] = blend_color(c1, c2, sin8(phase))

    elif pattern_id == 3:
        # ── Sparkle ──────────────────────────────────
        buf = list(st.sparkle_buf)
        for i in range(n):
            buf[i] = fade_to_black(buf[i], 20)
        if random.random() * 255 < speed:
            pos = random.randint(0, n - 1)
            buf[pos] = c1
        st.sparkle_buf = buf
        leds = list(buf)

    elif pattern_id == 4:
        # ── Fire ─────────────────────────────────────
        heat = list(st.heat)
        for i in range(n):
            heat[i] = max(0, heat[i] - random.random() * (((55 * 10) / n) + 2))
        for i in range(n - 1, 1, -1):
            heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3
        if random.random() * 255 < speed:
            y = random.randint(0, min(2, n - 1))
            heat[y] = min(255, heat[y] + 160 + random.random() * 95)
        st.heat = heat
        for i in range(n):
            leds[i] = heat_color(int(round(heat[i])))

    elif pattern_id == 5:
        # ── Comet ────────────────────────────────────
        buf = list(st.sparkle_buf)
        for i in range(n):
            buf[i] = fade_to_black(buf[i], 60)
        st.comet_pos += st.comet_dir
        if st.comet_pos >= n - 1 or st.comet_pos <= 0:
            st.comet_dir = -st.comet_dir
        st.comet_pos = clamp(st.comet_pos, 0, n - 1)
        buf[st.comet_pos] = c1
        tail = st.comet_pos - st.comet_dir
        if 0 <= tail < n:
            buf[tail] = scale_color(c1, 128)
        st.sparkle_buf = buf
        leds = list(buf)

    elif pattern_id == 6:
        # ── Breathing ────────────────────────────────
        breath = beatsin8(speed // 4, 20, 255, time_ms)
        for i in range(n):
            leds[i] = scale_color(c1, breath)

    elif pattern_id == 7:
        # ── Strobe ───────────────────────────────────
        strobe_delay = 500 - ((speed / 255) * 490)
        if time_ms - st.last_strobe_ms >= strobe_delay:
            st.strobe_on = not st.strobe_on
            st.last_strobe_ms = time_ms
        for i in range(n):
            leds[i] = c1 if st.strobe_on else (0, 0, 0)

    elif pattern_id == 8:
        # ── Meteor ───────────────────────────────────
        buf = list(st.sparkle_buf)
        for i in range(n):
            if random.random() < 80 / 255:
                buf[i] = fade_to_black(buf[i], 64)
        for j in range(4):
            pos = st.meteor_pos - j
            if 0 <= pos < n:
                buf[pos] = scale_color(c1, 255 - j * 60)
        st.meteor_pos -= 1
        if st.meteor_pos < 0:
            st.meteor_pos = n - 1
        st.sparkle_buf = buf
        leds = list(buf)

    elif pattern_id == 9:
        # ── Wipe ─────────────────────────────────────
        for i in range(n):
            if st.wipe_filling:
                leds[i] = c1 if i <= st.wipe_pos else (0, 0, 0)
            else:
                leds[i] = (0, 0, 0) if i <= st.wipe_pos else c1
        st.wipe_pos += 1
        if st.wipe_pos >= n:
            st.wipe_pos = 0
            st.wipe_filling = not st.wipe_filling

    elif pattern_id == 10:
        # ── Plasma ───────────────────────────────────
        for i in range(n):
            hue = (sin8(int(i * 10 + pattern_time * speed / 20)) +
                   sin8(int(i * 15 - pattern_time * speed / 15)) +
                   sin8(int(pattern_time * speed / 10))) & 0xFF
            leds[i] = hsv_to_rgb(hue, 255, 255)

    elif pattern_id == 11:
        # ── VU Meter (audio-reactive) ────────────────
        al = audio_level
        if al > st.peak_level + 30:
            st.beat_hue = (st.beat_hue + 32) & 0xFF
        if al > st.peak_level:
            st.peak_level = al
            st.peak_decay = 0
        else:
            st.peak_decay += 1
            if st.peak_decay > 5:
                st.peak_level = max(0, st.peak_level - 3)
        leds_to_light = round(al / 255 * n)
        for i in range(n):
            if i < leds_to_light:
                if i < n // 3:
                    hue = 96   # Green
                elif i < 2 * n // 3:
                    hue = 64   # Yellow
                else:
                    hue = 0    # Red
                hue = (hue + st.beat_hue) & 0xFF
                leds[i] = hsv_to_rgb(hue, 255, 255)
            else:
                leds[i] = fade_to_black(leds[i], 50)
        peak_pos = round(st.peak_level / 255 * (n - 1))
        if 0 <= peak_pos < n:
            leds[peak_pos] = (255, 255, 255)

    elif pattern_id == 12:
        # ── Music Pulse (audio-reactive) ─────────────
        al = audio_level
        if al > st.last_audio_level + 20 and al > 100:
            st.pulse_val = 255
        st.last_audio_level = al
        for i in range(n):
            leds[i] = scale_color(c1, int(round(st.pulse_val)))
        st.pulse_val = st.pulse_val * (220 / 256)

    elif pattern_id == 13:
        # ── Audio Rainbow (audio-reactive) ───────────
        al = audio_level
        speed_add = round(1 + (al / 255) * 19)
        st.rainbow_offset += speed_add
        for i in range(n):
            hue = (int(st.rainbow_offset / 4) + int(i * 255 / n)) & 0xFF
            brightness = clamp(al + 50, 50, 255)
            leds[i] = hsv_to_rgb(hue, 255, brightness)

    elif pattern_id == 14:
        # ── Center Burst (audio-reactive) ────────────
        al = audio_level
        expansion = round(al / 255 * (n / 2))
        center = n // 2
        buf = list(st.sparkle_buf)
        for i in range(n):
            buf[i] = fade_to_black(buf[i], 80)
        for i in range(expansion + 1):
            hue = (int(pattern_time * speed / 20) + i * 10) & 0xFF
            rgb = hsv_to_rgb(hue, 255, 255)
            if center + i < n:
                buf[center + i] = rgb
            if center - i >= 0:
                buf[center - i] = rgb
        st.sparkle_buf = buf
        leds = list(buf)

    elif pattern_id == 15:
        # ── Audio Sparkle (audio-reactive) ───────────
        al = audio_level
        buf = list(st.sparkle_buf)
        for i in range(n):
            buf[i] = fade_to_black(buf[i], 40)
        num_sparkles = round(al / 255 * 8)
        for _ in range(num_sparkles):
            pos = random.randint(0, n - 1)
            hue = (pattern_time * 2 + random.randint(0, 63)) & 0xFF
            buf[pos] = hsv_to_rgb(hue, 255, 255)
        st.sparkle_buf = buf
        leds = list(buf)

    elif pattern_id == 16:
        # ── Split Spin ───────────────────────────────
        offset = int(pattern_time * speed / 20) % n
        split_point = n // 2
        for i in range(n):
            pos = (i + offset) % n
            leds[i] = c1 if pos < split_point else c2

    elif pattern_id == 17:
        # ── Theater Chase ────────────────────────────
        chase_offset = int(pattern_time * speed / 20) % 3
        for i in range(n):
            phase = (i + chase_offset) % 3
            leds[i] = c1 if phase == 0 else c2

    return leds


# ── Image generation ─────────────────────────────────────────────────

def generate_preview(
    pattern_id: int,
    label: str,
    group: str,
    filename: str,
    output_dir: str,
) -> str:
    """Generate a PNG preview strip for one pattern.

    The image shows FRAMES_PER_PREVIEW columns (one per animation frame).
    Each column is NUM_LEDS pixels tall.  Audio-reactive patterns get
    simulated audio input.

    Returns the path to the written PNG file.
    """
    is_audio = group == "audio"
    n = NUM_LEDS
    num_frames = FRAMES_PER_PREVIEW

    # Fixed seed per pattern ensures deterministic output across runs.
    # This prevents spurious Git diffs when regenerating committed PNGs.
    random.seed(42 + pattern_id)

    state = PatternState(n)

    # Collect all frames
    all_frames: list[list[tuple[int, int, int]]] = []
    for f in range(num_frames):
        t_sec = f * 16.0 / 1000.0  # 60fps
        al = simulated_audio_level(t_sec) if is_audio else 0
        frame = render_pattern(pattern_id, f, state, audio_level=al)
        all_frames.append(frame)

    # Build image: each frame is a column, LEDs are stacked vertically
    img_w = num_frames * (LED_PIXEL_W + FRAME_GAP) - FRAME_GAP
    img_h = n * (LED_PIXEL_H + LED_GAP) - LED_GAP
    # Add label bar at top
    label_h = 24
    total_h = label_h + img_h
    img = Image.new("RGB", (img_w, total_h), BG_COLOR)
    draw = ImageDraw.Draw(img)

    # Label text
    tag = f"#{pattern_id:02d} {label}"
    if is_audio:
        tag += " (audio)"
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 13)
    except (IOError, OSError):
        font = ImageFont.load_default()

    # Group color badge
    group_colors = {
        "basic": (99, 102, 241),     # indigo
        "audio": (236, 72, 153),     # pink
        "advanced": (139, 92, 246),  # violet
    }
    badge_color = group_colors.get(group, (100, 100, 100))
    draw.rectangle([(0, 0), (img_w - 1, label_h - 1)], fill=(30, 41, 59))
    draw.text((6, 4), tag, fill=(255, 255, 255), font=font)
    # Small group badge on the right
    badge_text = group.upper()
    badge_w = len(badge_text) * 8 + 10
    draw.rectangle(
        [(img_w - badge_w - 4, 4), (img_w - 4, label_h - 4)],
        fill=badge_color,
    )
    draw.text((img_w - badge_w, 5), badge_text, fill=(255, 255, 255), font=font)

    # Render LED frames
    for f_idx, frame in enumerate(all_frames):
        x_base = f_idx * (LED_PIXEL_W + FRAME_GAP)
        for led_idx, (r, g, b) in enumerate(frame):
            y_base = label_h + led_idx * (LED_PIXEL_H + LED_GAP)
            r = clamp(r, 0, 255)
            g = clamp(g, 0, 255)
            b = clamp(b, 0, 255)
            draw.rectangle(
                [(x_base, y_base), (x_base + LED_PIXEL_W - 1, y_base + LED_PIXEL_H - 1)],
                fill=(r, g, b),
            )

    out_path = os.path.join(output_dir, f"{filename}.png")
    img.save(out_path, "PNG")
    return out_path


# ── Main ─────────────────────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print(f"Generating pattern previews in {OUTPUT_DIR}/")
    print(f"  LEDs: {NUM_LEDS}  |  Frames: {FRAMES_PER_PREVIEW}  |  Speed: {DEFAULT_SPEED}")
    print(f"  Color1: {DEFAULT_COLOR1}  Color2: {DEFAULT_COLOR2}")
    print()

    for pat in PATTERNS:
        path = generate_preview(
            pattern_id=pat["id"],
            label=pat["label"],
            group=pat["group"],
            filename=pat["file"],
            output_dir=OUTPUT_DIR,
        )
        marker = "♪" if pat["group"] == "audio" else "✓"
        print(f"  {marker} {pat['id']:2d}. {pat['label']:<16s} → {os.path.relpath(path)}")

    # Generate index / README
    readme_path = os.path.join(OUTPUT_DIR, "README.md")
    with open(readme_path, "w") as f:
        f.write("# Pattern Previews\n\n")
        f.write("Static renders of all 18 firmware LED patterns.\n")
        f.write("Each image shows the pattern animating over time (left to right).\n")
        f.write("Vertical axis = LED position on the strip (31 display LEDs).\n\n")
        f.write("**Regenerate:** `python3 scripts/generate_pattern_previews.py`\n\n")

        f.write("## Basic Patterns (0-10)\n\n")
        f.write("| # | Pattern | Preview |\n")
        f.write("|---|---------|---------|\n")
        for p in PATTERNS:
            if p["group"] == "basic":
                f.write(f"| {p['id']} | {p['label']} | ![{p['label']}]({p['file']}.png) |\n")

        f.write("\n## Audio-Reactive Patterns (11-15)\n\n")
        f.write("These patterns respond to audio input. Previews use simulated audio.\n\n")
        f.write("| # | Pattern | Preview |\n")
        f.write("|---|---------|---------|\n")
        for p in PATTERNS:
            if p["group"] == "audio":
                f.write(f"| {p['id']} | {p['label']} | ![{p['label']}]({p['file']}.png) |\n")

        f.write("\n## Advanced Patterns (16-17)\n\n")
        f.write("| # | Pattern | Preview |\n")
        f.write("|---|---------|---------|\n")
        for p in PATTERNS:
            if p["group"] == "advanced":
                f.write(f"| {p['id']} | {p['label']} | ![{p['label']}]({p['file']}.png) |\n")

    print(f"\n  📄 {os.path.relpath(readme_path)}")
    print(f"\nDone — {len(PATTERNS)} pattern previews generated.")


if __name__ == "__main__":
    main()
