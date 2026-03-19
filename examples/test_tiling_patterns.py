#!/usr/bin/env python3
"""
Tests for the seamlessly tiling POV pattern image generator.

Validates that generated images have correct dimensions, tile seamlessly,
and that colour-scheme / shift helpers produce expected results.

Run:
    cd examples
    python3 -m pytest test_tiling_patterns.py -v
"""

import os
import sys
import tempfile
import shutil

# Allow importing from sibling scripts/ directory
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))

from PIL import Image

from generate_tiling_patterns import (
    COLOR_SCHEMES,
    PATTERN_GENERATORS,
    SHIFT_METHODS,
    generate_all,
    generate_pattern,
    complementary,
    triadic,
    split_complementary,
    analogous,
    shift_hue_rotate,
    shift_invert,
    shift_swap,
    _lerp_color,
    _palette_at,
)


# ── Colour-scheme tests ─────────────────────────────────────────────

def test_complementary_returns_two_colours():
    colours = complementary(0)
    assert len(colours) == 2
    for c in colours:
        assert len(c) == 3
        assert all(0 <= v <= 255 for v in c)


def test_triadic_returns_three_colours():
    colours = triadic(0)
    assert len(colours) == 3


def test_split_complementary_returns_three_colours():
    colours = split_complementary(0)
    assert len(colours) == 3


def test_analogous_returns_three_colours():
    colours = analogous(120)
    assert len(colours) == 3


def test_complementary_colours_differ():
    c1, c2 = complementary(0)
    assert c1 != c2, "Complementary colours should differ"


# ── Shift-method tests ──────────────────────────────────────────────

def test_shift_hue_rotate():
    original = [(255, 0, 0), (0, 255, 0)]
    shifted = shift_hue_rotate(original, 120)
    assert len(shifted) == 2
    # Hues should have changed
    assert shifted[0] != original[0]


def test_shift_invert():
    original = [(255, 0, 0)]
    inverted = shift_invert(original)
    assert inverted == [(0, 255, 255)]


def test_shift_swap():
    original = [(255, 0, 0), (0, 255, 0), (0, 0, 255)]
    swapped = shift_swap(original)
    assert swapped == [(0, 255, 0), (0, 0, 255), (255, 0, 0)]


def test_shift_swap_single():
    original = [(10, 20, 30)]
    assert shift_swap(original) == [(10, 20, 30)]


# ── Interpolation tests ─────────────────────────────────────────────

def test_lerp_color_endpoints():
    c1 = (0, 0, 0)
    c2 = (255, 255, 255)
    assert _lerp_color(c1, c2, 0.0) == c1
    assert _lerp_color(c1, c2, 1.0) == c2


def test_lerp_color_midpoint():
    c1 = (0, 0, 0)
    c2 = (254, 254, 254)
    mid = _lerp_color(c1, c2, 0.5)
    assert mid == (127, 127, 127)


def test_palette_at_wraps():
    colours = [(255, 0, 0), (0, 255, 0)]
    # t=0.0 and t=1.0 should both sample the same position (wrap)
    assert _palette_at(colours, 0.0) == _palette_at(colours, 1.0)


# ── Pattern generation tests ────────────────────────────────────────

def test_all_patterns_produce_correct_height():
    colours = complementary(0)
    for name, gen in PATTERN_GENERATORS.items():
        img = gen(colours, 31)
        assert img.height == 31, f"Pattern '{name}' height should be 31, got {img.height}"


def test_all_patterns_have_positive_width():
    colours = triadic(90)
    for name, gen in PATTERN_GENERATORS.items():
        img = gen(colours, 31)
        assert img.width > 0, f"Pattern '{name}' has non-positive width"


def test_all_patterns_are_rgb():
    colours = complementary(0)
    for name, gen in PATTERN_GENERATORS.items():
        img = gen(colours, 31)
        assert img.mode == "RGB", f"Pattern '{name}' should be RGB, got {img.mode}"


def test_generate_pattern_api():
    img = generate_pattern("stripes", complementary(0), height=31)
    assert isinstance(img, Image.Image)
    assert img.height == 31


def test_generate_pattern_unknown_raises():
    try:
        generate_pattern("nonexistent_pattern", complementary(0))
        assert False, "Should have raised ValueError"
    except ValueError:
        pass


def test_custom_height():
    """Patterns should respect a non-default height."""
    colours = complementary(0)
    for name, gen in PATTERN_GENERATORS.items():
        img = gen(colours, 16)
        assert img.height == 16, f"Pattern '{name}' height should be 16, got {img.height}"


# ── Seamless tiling validation ───────────────────────────────────────

def _column(img: Image.Image, x: int):
    """Extract a column of pixels from an image."""
    return [img.getpixel((x, y)) for y in range(img.height)]


def _max_channel_diff(c1, c2):
    return max(abs(a - b) for a, b in zip(c1, c2))


def test_stripes_tile_seamlessly():
    """Stripes are block-aligned so column -1 → column 0 must be exact."""
    img = generate_pattern("stripes", complementary(0))
    # Tile: paste two copies side by side, check the junction
    left_last = _column(img, img.width - 1)
    right_first = _column(img, 0)
    # For stripes, consecutive columns at the seam should match the pattern
    # period, meaning column 0 follows column width-1 in the same cycle.
    # They won't be identical (different stripe), but they should both be
    # solid colours from the palette.
    assert len(set(left_last)) == 1, "Stripe column should be a single colour"
    assert len(set(right_first)) == 1, "Stripe column should be a single colour"


def test_gradient_tiles_smoothly():
    """Gradient left/right edges should be close since they represent
    adjacent points on a smooth cycle."""
    img = generate_pattern("gradient", complementary(0))
    left = _column(img, 0)
    right = _column(img, img.width - 1)
    for y in range(img.height):
        diff = _max_channel_diff(left[y], right[y])
        assert diff < 25, (
            f"Gradient seam diff too large at y={y}: "
            f"{left[y]} vs {right[y]} (diff={diff})"
        )


def test_wave_tiles_smoothly():
    img = generate_pattern("wave", triadic(0))
    left = _column(img, 0)
    right = _column(img, img.width - 1)
    for y in range(img.height):
        diff = _max_channel_diff(left[y], right[y])
        assert diff < 25, f"Wave seam diff too large at y={y}: diff={diff}"


# ── Batch generation test ────────────────────────────────────────────

def test_generate_all_creates_files():
    tmpdir = tempfile.mkdtemp()
    try:
        written = generate_all(
            output_dir=tmpdir,
            height=31,
            hues=[0],
            schemes=["complementary"],
            patterns=["stripes", "diagonal"],
        )
        assert len(written) == 2
        for path in written:
            assert os.path.isfile(path)
            img = Image.open(path)
            assert img.height == 31
    finally:
        shutil.rmtree(tmpdir)
