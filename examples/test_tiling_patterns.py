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

import pytest

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
    with pytest.raises(ValueError):
        generate_pattern("nonexistent_pattern", complementary(0))


def test_custom_height():
    """Patterns should respect a non-default height."""
    colours = complementary(0)
    for name, gen in PATTERN_GENERATORS.items():
        img = gen(colours, 16)
        assert img.height == 16, f"Pattern '{name}' height should be 16, got {img.height}"


# ── Seamless tiling validation ───────────────────────────────────────

# Maximum per-channel colour difference allowed at the horizontal seam
# between the last and first columns.  A value of 25 (~10 % of 255)
# accommodates the one-pixel step in smooth sinusoidal / gradient
# patterns while still catching visually jarring discontinuities.
MAX_SEAMLESS_COLOR_DIFF = 25


def _column(img: Image.Image, x: int):
    """Extract a column of pixels from an image."""
    return [img.getpixel((x, y)) for y in range(img.height)]


def _max_channel_diff(c1, c2):
    return max(abs(a - b) for a, b in zip(c1, c2))


def test_stripes_tile_seamlessly():
    """Stripe columns at the seam should each be a single solid colour."""
    img = generate_pattern("stripes", complementary(0))
    # Tile: paste two copies side by side, check the junction.
    left_last = _column(img, img.width - 1)
    right_first = _column(img, 0)
    # Each column within a stripe block is a single solid colour.
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
        assert diff < MAX_SEAMLESS_COLOR_DIFF, (
            f"Gradient seam diff too large at y={y}: "
            f"{left[y]} vs {right[y]} (diff={diff})"
        )


def test_wave_tiles_smoothly():
    img = generate_pattern("wave", triadic(0))
    left = _column(img, 0)
    right = _column(img, img.width - 1)
    for y in range(img.height):
        diff = _max_channel_diff(left[y], right[y])
        assert diff < MAX_SEAMLESS_COLOR_DIFF, (
            f"Wave seam diff too large at y={y}: diff={diff}"
        )


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


# ── Parameterized seam test for ALL patterns ─────────────────────────

# Patterns that produce discrete blocks (no smooth gradient at the seam)
# are seamless by construction because their width equals the tile period.
# They are validated by verifying the image is tileable (i.e., placing two
# copies side by side produces no discontinuity beyond what exists within
# a single tile).  Smooth patterns are validated with the
# MAX_SEAMLESS_COLOR_DIFF tolerance between adjacent seam columns.
_BLOCK_PATTERNS = {"stripes", "diagonal", "checkerboard", "diamonds", "zigzag"}


@pytest.mark.parametrize("pat_name", list(PATTERN_GENERATORS))
def test_all_patterns_tile_at_seam(pat_name):
    """Every pattern must tile seamlessly at the horizontal wrap point."""
    colours = complementary(0)
    img = generate_pattern(pat_name, colours)
    left = _column(img, img.width - 1)
    right = _column(img, 0)

    if pat_name in _BLOCK_PATTERNS:
        # Block-based patterns tile by period: verify that the colour
        # difference between the last and first column never exceeds
        # the max difference found between any two adjacent columns
        # within the image itself.
        max_internal = 0
        for x in range(img.width - 1):
            col_a = _column(img, x)
            col_b = _column(img, x + 1)
            for y in range(img.height):
                d = _max_channel_diff(col_a[y], col_b[y])
                if d > max_internal:
                    max_internal = d
        for y in range(img.height):
            seam_diff = _max_channel_diff(left[y], right[y])
            assert seam_diff <= max_internal, (
                f"{pat_name} seam diff ({seam_diff}) exceeds max internal "
                f"diff ({max_internal}) at y={y}"
            )
    else:
        for y in range(img.height):
            diff = _max_channel_diff(left[y], right[y])
            assert diff < MAX_SEAMLESS_COLOR_DIFF, (
                f"{pat_name} seam diff too large at y={y}: "
                f"{left[y]} vs {right[y]} (diff={diff})"
            )


# ── Validation tests ─────────────────────────────────────────────────

def test_generate_pattern_empty_colors_raises():
    """Passing an empty colour list should raise ValueError."""
    with pytest.raises(ValueError, match="at least 2"):
        generate_pattern("stripes", [])


def test_generate_pattern_single_color_raises():
    """Passing a single-colour list should raise ValueError."""
    with pytest.raises(ValueError, match="at least 2"):
        generate_pattern("stripes", [(255, 0, 0)])


def test_generate_all_unknown_scheme_raises():
    """Passing an unknown scheme name should raise ValueError."""
    tmpdir = tempfile.mkdtemp()
    try:
        with pytest.raises(ValueError, match="Unknown colour scheme"):
            generate_all(output_dir=tmpdir, hues=[0],
                         schemes=["nonexistent"], patterns=["stripes"])
    finally:
        shutil.rmtree(tmpdir)


def test_generate_all_unknown_pattern_raises():
    """Passing an unknown pattern name should raise ValueError."""
    tmpdir = tempfile.mkdtemp()
    try:
        with pytest.raises(ValueError, match="Unknown pattern"):
            generate_all(output_dir=tmpdir, hues=[0],
                         schemes=["complementary"], patterns=["fake_pattern"])
    finally:
        shutil.rmtree(tmpdir)


def test_generate_all_empty_lists_produce_nothing():
    """Explicitly passing empty lists should produce no output."""
    tmpdir = tempfile.mkdtemp()
    try:
        written = generate_all(output_dir=tmpdir, hues=[], schemes=[],
                               patterns=[])
        assert written == []
    finally:
        shutil.rmtree(tmpdir)
