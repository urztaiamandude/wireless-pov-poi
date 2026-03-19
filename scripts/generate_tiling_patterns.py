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
        for y in range(height):
            # triangle wave on x to create zigzag
            tri = abs((2 * (x % period) / period) - 1.0)  # 0→1→0
            offset = tri * height
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


SHIFT_METHODS: dict[str, Callable] = {
    "hue_rotate_60": lambda c: shift_hue_rotate(c, 60),
    "hue_rotate_120": lambda c: shift_hue_rotate(c, 120),
    "invert": shift_invert,
    "swap": shift_swap,
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

    hues = hues or STARTER_HUES
    schemes = schemes or list(COLOR_SCHEMES)
    patterns = patterns or list(PATTERN_GENERATORS)

    written: list[str] = []

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
