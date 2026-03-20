#!/usr/bin/env python3
"""
Generate seamlessly tiling POV pattern images.

Creates pattern images that tile horizontally without visible seams when the
POV display wraps from the last column back to the first. Each pattern uses
complementary color schemes and supports multiple color-shift variants.

The generated PNGs can be uploaded directly to the poi via the web UI or
converted to .pov format for SD card storage with ``examples/image_converter.py``.

Usage:
    python3 scripts/generate_tiling_patterns.py [--output-dir DIR] [--height N]

Output (default):
    pattern_images/stripes_complementary.png
    pattern_images/diagonal_complementary.png
    pattern_images/checkerboard_complementary.png
    ...

Requires: Pillow  (pip install Pillow)
"""

from __future__ import annotations

import argparse
import colorsys
import math
import os
import sys
from typing import Callable

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required.  Install with:  pip install Pillow",
          file=sys.stderr)
    sys.exit(1)


# ── Defaults ─────────────────────────────────────────────────────────
# Matches default g_displayLeds (runtime-configurable via web UI).
DEFAULT_HEIGHT = 31
DEFAULT_OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "pattern_images",
)

# ── Color-scheme helpers ─────────────────────────────────────────────

def _clamp(v: float) -> int:
    return max(0, min(255, int(round(v))))


def hsl_to_rgb(h: float, s: float, l: float) -> tuple[int, int, int]:
    """Convert HSL (h 0-360, s/l 0-1) to RGB 0-255."""
    r, g, b = colorsys.hls_to_rgb(h / 360.0, l, s)
    return (_clamp(r * 255), _clamp(g * 255), _clamp(b * 255))


def complementary(h: float, s: float = 0.9, l: float = 0.5
                  ) -> list[tuple[int, int, int]]:
    """Return two complementary colours (180° apart)."""
    return [hsl_to_rgb(h, s, l), hsl_to_rgb((h + 180) % 360, s, l)]


def triadic(h: float, s: float = 0.9, l: float = 0.5
            ) -> list[tuple[int, int, int]]:
    """Return three triadic colours (120° apart)."""
    return [hsl_to_rgb((h + i * 120) % 360, s, l) for i in range(3)]


def split_complementary(h: float, s: float = 0.9, l: float = 0.5
                        ) -> list[tuple[int, int, int]]:
    """Return three split-complementary colours."""
    return [
        hsl_to_rgb(h, s, l),
        hsl_to_rgb((h + 150) % 360, s, l),
        hsl_to_rgb((h + 210) % 360, s, l),
    ]


def analogous(h: float, s: float = 0.9, l: float = 0.5
              ) -> list[tuple[int, int, int]]:
    """Return three analogous colours (30° apart)."""
    return [hsl_to_rgb((h + i * 30) % 360, s, l) for i in range(3)]


# All available colour-scheme generators keyed by a short name.
COLOR_SCHEMES: dict[str, Callable[..., list[tuple[int, int, int]]]] = {
    "complementary": complementary,
    "triadic": triadic,
    "split_complementary": split_complementary,
    "analogous": analogous,
}

# ── Pixel math helpers ───────────────────────────────────────────────

def _lerp_color(c1: tuple[int, int, int], c2: tuple[int, int, int],
                t: float) -> tuple[int, int, int]:
    """Linearly interpolate between two RGB colours, t in [0, 1]."""
    t = max(0.0, min(1.0, t))
    return (
        _clamp(c1[0] + (c2[0] - c1[0]) * t),
        _clamp(c1[1] + (c2[1] - c1[1]) * t),
        _clamp(c1[2] + (c2[2] - c1[2]) * t),
    )


def _palette_at(colors: list[tuple[int, int, int]],
                t: float) -> tuple[int, int, int]:
    """Sample a smooth palette defined by *colors* at position *t* (0-1)."""
    n = len(colors)
    t = t % 1.0
    idx = t * n
    lo = int(idx) % n
    hi = (lo + 1) % n
    frac = idx - int(idx)
    return _lerp_color(colors[lo], colors[hi], frac)

# ── Pattern generators ───────────────────────────────────────────────
# Each generator returns a PIL Image of size (width, height).
# The image **must** tile seamlessly left↔right.


def gen_stripes(colors: list[tuple[int, int, int]], height: int,
                stripe_width: int = 6) -> Image.Image:
    """Vertical stripes — simple alternating colour bars."""
    period = stripe_width * len(colors)
    img = Image.new("RGB", (period, height))
    for x in range(period):
        ci = (x // stripe_width) % len(colors)
        for y in range(height):
            img.putpixel((x, y), colors[ci])
    return img


def gen_diagonal(colors: list[tuple[int, int, int]], height: int,
                 stripe_width: int = 6) -> Image.Image:
    """Diagonal stripes at ~45°. Tile period = stripe_width * len(colors)."""
    period = stripe_width * len(colors)
    img = Image.new("RGB", (period, height))
    for x in range(period):
        for y in range(height):
            ci = ((x + y) // stripe_width) % len(colors)
            img.putpixel((x, y), colors[ci])
    return img


def gen_checkerboard(colors: list[tuple[int, int, int]], height: int,
                     cell: int = 4) -> Image.Image:
    """Checkerboard of *cell*×*cell* blocks."""
    period = cell * len(colors)
    img = Image.new("RGB", (period, height))
    for x in range(period):
        for y in range(height):
            ci = ((x // cell) + (y // cell)) % len(colors)
            img.putpixel((x, y), colors[ci])
    return img


def gen_wave(colors: list[tuple[int, int, int]], height: int,
             wavelength: int = 48) -> Image.Image:
    """Smooth horizontal sine wave blending through *colors*."""
    width = wavelength  # one full cycle = seamless tile
    img = Image.new("RGB", (width, height))
    for x in range(width):
        t = x / width  # 0→1 across one cycle
        c = _palette_at(colors, t)
        for y in range(height):
            img.putpixel((x, y), c)
    return img


def gen_gradient(colors: list[tuple[int, int, int]], height: int,
                 width: int = 64) -> Image.Image:
    """Smooth horizontal gradient cycling through *colors*, tiling at *width*."""
    img = Image.new("RGB", (width, height))
    for x in range(width):
        t = x / width
        c = _palette_at(colors, t)
        for y in range(height):
            img.putpixel((x, y), c)
    return img


def gen_zigzag(colors: list[tuple[int, int, int]], height: int,
               period: int = 24) -> Image.Image:
    """Chevron / zigzag bands running vertically, tiling at *period*."""
    img = Image.new("RGB", (period, height))
    for x in range(period):
        # triangle wave on x to create zigzag
        tri = abs((2 * (x % period) / period) - 1.0)  # 0→1→0
        offset = tri * height
        for y in range(height):
            t = ((y + offset) % height) / height
            img.putpixel((x, y), _palette_at(colors, t))
    return img


def gen_diamonds(colors: list[tuple[int, int, int]], height: int,
                 cell: int = 8) -> Image.Image:
    """Diamond / argyle tessellation."""
    period = cell * 2
    img = Image.new("RGB", (period, height))
    for x in range(period):
        for y in range(height):
            # Manhattan distance to the nearest diamond centre
            cx = (x % period) - cell
            # Centre the diamond vertically: if the tile period fits inside
            # the image height we tile normally, otherwise offset to the
            # vertical midpoint of each period so the pattern stays centred.
            y_in_period = y % period
            cy = y_in_period - cell if period <= height else y_in_period - period // 2
            d = (abs(cx) + abs(cy)) / cell  # normalised 0→~2
            ci = int(d) % len(colors)
            img.putpixel((x, y), colors[ci])
    return img


def gen_spiral(colors: list[tuple[int, int, int]], height: int,
               turns: int = 2, width: int = 64) -> Image.Image:
    """Helical bands that wrap around the poi; one full revolution = *width*."""
    img = Image.new("RGB", (width, height))
    for x in range(width):
        for y in range(height):
            phase = (x / width + y / height * turns) % 1.0
            img.putpixel((x, y), _palette_at(colors, phase))
    return img


def gen_plasma(colors: list[tuple[int, int, int]], height: int,
               width: int = 64) -> Image.Image:
    """Organic plasma combining two sine components, tiling at *width*."""
    img = Image.new("RGB", (width, height))
    two_pi = 2.0 * math.pi
    for x in range(width):
        for y in range(height):
            v1 = math.sin(x * two_pi / width)
            v2 = math.sin(y * two_pi / height + x * two_pi / width)
            t = (v1 + v2 + 2.0) / 4.0  # normalised 0→1
            img.putpixel((x, y), _palette_at(colors, t))
    return img


# ── POV-optimised pattern generators ────────────────────────────────
# These produce effects especially suited to persistence-of-vision light
# trails where each column is shown for a very brief flash.


def gen_strobe(colors: list[tuple[int, int, int]], height: int,
               duty: int = 2, gap: int = 4) -> Image.Image:
    """Strobe — alternating complementary-colour columns with black gaps.

    *duty* columns of each palette colour followed by *gap* black columns.
    One full cycle = ``(duty + gap) * len(colors)`` columns.
    """
    cycle = (duty + gap) * len(colors)
    img = Image.new("RGB", (cycle, height))
    for x in range(cycle):
        ci = x // (duty + gap)
        phase = x % (duty + gap)
        c = colors[ci % len(colors)] if phase < duty else (0, 0, 0)
        for y in range(height):
            img.putpixel((x, y), c)
    return img


def gen_color_strobe(colors: list[tuple[int, int, int]], height: int,
                     width: int = 64, duty: int = 2,
                     gap: int = 3) -> Image.Image:
    """Colour-shifting strobe — the hue rotates across *width* columns while
    keeping the complementary relationship intact.

    Every ``duty`` columns a flash appears; the remaining ``gap`` columns are
    black.  The flash colour is derived from the palette hue-rotated
    proportionally to the x position so the colour *changes dynamically*
    across the image yet always stays complementary.
    """
    img = Image.new("RGB", (width, height))
    for x in range(width):
        phase = x % (duty + gap)
        if phase < duty:
            # Shift the whole palette proportionally along the width
            t = x / width
            shifted = shift_hue_rotate(colors, t * 360.0)
            c = shifted[0]
        else:
            c = (0, 0, 0)
        for y in range(height):
            img.putpixel((x, y), c)
    return img


def gen_rain(colors: list[tuple[int, int, int]], height: int,
             width: int = 64, density: float = 0.3) -> Image.Image:
    """Falling rain / matrix — vertical streaks with fading tails.

    Deterministic (seeded by column position) so the pattern tiles
    seamlessly at *width*.
    """
    import random as _rng
    img = Image.new("RGB", (width, height))
    for x in range(width):
        # Deterministic seed per column for reproducibility & tiling
        _rng.seed(x * 7919)
        if _rng.random() > density:
            continue  # blank column
        ci = x % len(colors)
        # Choose a starting y and draw a fading streak downward
        start_y = _rng.randint(0, height - 1)
        length = _rng.randint(3, min(12, height))
        for i in range(length):
            y = (start_y + i) % height
            fade = max(0.0, 1.0 - i / length)
            c = tuple(_clamp(v * fade) for v in colors[ci])
            img.putpixel((x, y), c)
    return img


def gen_helix(colors: list[tuple[int, int, int]], height: int,
              width: int = 64, strands: int = 2) -> Image.Image:
    """Double (or multi-) helix — interleaving sinusoidal strands.

    Each strand is offset by ``1/strands`` of the phase, creating a
    DNA-like twist along the poi trail.  Tiles at *width*.
    """
    img = Image.new("RGB", (width, height))
    two_pi = 2.0 * math.pi
    for x in range(width):
        for s in range(strands):
            phase_offset = s / strands
            # Sine maps x-position to y-position
            y_center = (math.sin((x / width + phase_offset) * two_pi) + 1.0) / 2.0
            y_center *= (height - 1)
            ci = s % len(colors)
            # Draw a thick strand (±1 pixel)
            for dy in range(-1, 2):
                y = int(round(y_center)) + dy
                if 0 <= y < height:
                    bright = 1.0 - abs(dy) * 0.35
                    c = tuple(_clamp(v * bright) for v in colors[ci])
                    img.putpixel((x, y), c)
    return img


def gen_starburst(colors: list[tuple[int, int, int]], height: int,
                  width: int = 64) -> Image.Image:
    """Starburst — radial rays emanating from the vertical centre.

    Rays are coloured using the palette and the pattern tiles at *width*
    so the burst repeats smoothly as the poi spins.
    """
    img = Image.new("RGB", (width, height))
    two_pi = 2.0 * math.pi
    cy = height / 2.0
    for x in range(width):
        for y in range(height):
            # Angle from centre
            angle = math.atan2(y - cy, x - width / 2.0)
            # Normalise to 0-1, repeating with number of rays
            num_rays = len(colors) * 4
            ray = (angle / two_pi + 0.5) * num_rays
            ci = int(ray) % len(colors)
            # Radial fade: brighter at centre
            dist = math.sqrt((x - width / 2.0) ** 2 + (y - cy) ** 2)
            max_dist = math.sqrt((width / 2.0) ** 2 + cy ** 2)
            fade = max(0.0, 1.0 - dist / max_dist * 0.5)
            c = tuple(_clamp(v * fade) for v in colors[ci])
            img.putpixel((x, y), c)
    return img


# Registry: name → generator function
PATTERN_GENERATORS: dict[str, Callable[..., Image.Image]] = {
    "stripes": gen_stripes,
    "diagonal": gen_diagonal,
    "checkerboard": gen_checkerboard,
    "wave": gen_wave,
    "gradient": gen_gradient,
    "zigzag": gen_zigzag,
    "diamonds": gen_diamonds,
    "spiral": gen_spiral,
    "plasma": gen_plasma,
    "strobe": gen_strobe,
    "color_strobe": gen_color_strobe,
    "rain": gen_rain,
    "helix": gen_helix,
    "starburst": gen_starburst,
}

# ── Colour-shift helpers ─────────────────────────────────────────────
# These take an existing palette and produce a shifted variant.

def shift_hue_rotate(colors: list[tuple[int, int, int]],
                     degrees: float) -> list[tuple[int, int, int]]:
    """Rotate every colour's hue by *degrees*."""
    out: list[tuple[int, int, int]] = []
    for r, g, b in colors:
        h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
        h = (h + degrees / 360.0) % 1.0
        rr, gg, bb = colorsys.hls_to_rgb(h, l, s)
        out.append((_clamp(rr * 255), _clamp(gg * 255), _clamp(bb * 255)))
    return out


def shift_invert(colors: list[tuple[int, int, int]]
                 ) -> list[tuple[int, int, int]]:
    """Invert every colour."""
    return [(255 - r, 255 - g, 255 - b) for r, g, b in colors]


def shift_swap(colors: list[tuple[int, int, int]]
               ) -> list[tuple[int, int, int]]:
    """Rotate the palette order by one position."""
    if len(colors) < 2:
        return list(colors)
    return colors[1:] + colors[:1]


def shift_complementary_rotate(colors: list[tuple[int, int, int]],
                               degrees: float) -> list[tuple[int, int, int]]:
    """Regenerate a fresh complementary pair from the first colour's hue.

    Unlike ``shift_hue_rotate`` which rotates existing colours (preserving
    *any* original scheme), this function always produces exactly two
    colours that are 180° apart — guaranteeing a complementary
    relationship even if the input palette was not complementary.

    The first colour's hue is rotated by *degrees* and a new
    complementary pair is generated at the target hue.
    """
    r, g, b = colors[0]
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    new_h = ((h * 360.0) + degrees) % 360.0
    return complementary(new_h, s, l)


SHIFT_METHODS: dict[str, Callable] = {
    "hue_rotate_60": lambda c: shift_hue_rotate(c, 60),
    "hue_rotate_120": lambda c: shift_hue_rotate(c, 120),
    "invert": shift_invert,
    "swap": shift_swap,
    "complementary_rotate_60": lambda c: shift_complementary_rotate(c, 60),
    "complementary_rotate_120": lambda c: shift_complementary_rotate(c, 120),
}

# ── Default starter hues ─────────────────────────────────────────────
# Four base hues (red, teal, purple, gold) each rendered with each
# colour-scheme generator to give a broad starting library.
STARTER_HUES = [0, 180, 270, 45]

# ── Public API ───────────────────────────────────────────────────────

def generate_pattern(
    pattern_name: str,
    colors: list[tuple[int, int, int]],
    height: int = DEFAULT_HEIGHT,
) -> Image.Image:
    """Generate a single tiling pattern image.

    Args:
        pattern_name: Key from ``PATTERN_GENERATORS``.
        colors: List of 2+ RGB tuples.
        height: Image height in pixels (should match display LEDs).

    Returns:
        A ``PIL.Image.Image`` that tiles seamlessly along the x axis.
    """
    gen = PATTERN_GENERATORS.get(pattern_name)
    if gen is None:
        raise ValueError(
            f"Unknown pattern '{pattern_name}'. "
            f"Available: {', '.join(PATTERN_GENERATORS)}"
        )
    if colors is None or len(colors) < 2:
        count = 0 if colors is None else len(colors)
        raise ValueError(
            f"'colors' must contain at least 2 RGB tuples; got {count}"
        )
    return gen(colors, height)


def generate_all(
    output_dir: str = DEFAULT_OUTPUT_DIR,
    height: int = DEFAULT_HEIGHT,
    hues: list[float] | None = None,
    schemes: list[str] | None = None,
    patterns: list[str] | None = None,
) -> list[str]:
    """Generate the full starter library of tiling pattern images.

    Returns a list of file paths that were written.
    """
    os.makedirs(output_dir, exist_ok=True)

    if hues is None:
        hues = STARTER_HUES
    if schemes is None:
        schemes = list(COLOR_SCHEMES)
    if patterns is None:
        patterns = list(PATTERN_GENERATORS)

    written: list[str] = []

    unknown_schemes = set(schemes) - set(COLOR_SCHEMES)
    if unknown_schemes:
        raise ValueError(
            f"Unknown colour scheme(s): {', '.join(sorted(unknown_schemes))}. "
            f"Available: {', '.join(COLOR_SCHEMES)}"
        )
    unknown_pats = set(patterns) - set(PATTERN_GENERATORS)
    if unknown_pats:
        raise ValueError(
            f"Unknown pattern(s): {', '.join(sorted(unknown_pats))}. "
            f"Available: {', '.join(PATTERN_GENERATORS)}"
        )

    for hue in hues:
        for scheme_name in schemes:
            colors = COLOR_SCHEMES[scheme_name](hue)
            for pat_name in patterns:
                img = generate_pattern(pat_name, colors, height)
                fname = f"{pat_name}_{scheme_name}_h{int(hue)}.png"
                path = os.path.join(output_dir, fname)
                img.save(path, "PNG")
                written.append(path)
                print(f"  {fname}  ({img.width}x{img.height})")

    return written


# ── CLI ──────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate seamlessly tiling POV pattern images.",
    )
    parser.add_argument(
        "--output-dir", default=DEFAULT_OUTPUT_DIR,
        help="Directory for output PNGs (default: pattern_images/)",
    )
    parser.add_argument(
        "--height", type=int, default=DEFAULT_HEIGHT,
        help="Image height in pixels (default: 31, matching display LEDs)",
    )
    parser.add_argument(
        "--hues", type=float, nargs="+", default=None,
        help="Base hue angles (0-360) to use. Default: 0 180 270 45",
    )
    parser.add_argument(
        "--schemes", nargs="+", default=None,
        choices=list(COLOR_SCHEMES),
        help="Colour schemes to generate. Default: all",
    )
    parser.add_argument(
        "--patterns", nargs="+", default=None,
        choices=list(PATTERN_GENERATORS),
        help="Pattern types to generate. Default: all",
    )
    args = parser.parse_args()

    print(f"Generating tiling patterns  →  {args.output_dir}/")
    print(f"  Height : {args.height}px")
    written = generate_all(
        output_dir=args.output_dir,
        height=args.height,
        hues=args.hues,
        schemes=args.schemes,
        patterns=args.patterns,
    )
    print(f"\nDone — {len(written)} images written.")


if __name__ == "__main__":
    main()
