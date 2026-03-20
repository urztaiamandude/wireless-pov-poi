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
    shift_complementary_rotate,
    _lerp_color,
    _palette_at,
    hsl_to_rgb,
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
_BLOCK_PATTERNS = {"stripes", "diagonal", "checkerboard", "diamonds", "zigzag",
                   "strobe", "color_strobe", "rain", "helix", "starburst"}


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


# ── Complementary colour integrity tests ─────────────────────────────

def _rgb_to_hue(r: int, g: int, b: int) -> float:
    """Return hue in degrees (0-360) for an RGB colour."""
    import colorsys
    h, _, _ = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
    return h * 360.0


def _hue_distance(h1: float, h2: float) -> float:
    """Shortest angular distance between two hue angles (0-360)."""
    d = abs(h1 - h2) % 360.0
    return min(d, 360.0 - d)


def test_complementary_is_180_degrees_apart():
    """The complementary scheme must produce hues exactly 180° apart."""
    for hue in [0, 45, 90, 135, 180, 270]:
        c1, c2 = complementary(hue)
        h1 = _rgb_to_hue(*c1)
        h2 = _rgb_to_hue(*c2)
        assert abs(_hue_distance(h1, h2) - 180.0) < 2.0, (
            f"Complementary({hue}): hues {h1:.1f}° and {h2:.1f}° are not ~180° apart"
        )


def test_shift_hue_rotate_preserves_complementary():
    """shift_hue_rotate should maintain the 180° gap for complementary pairs."""
    for degrees in [30, 60, 90, 120, 180, 270]:
        original = complementary(0)
        shifted = shift_hue_rotate(original, degrees)
        h1 = _rgb_to_hue(*shifted[0])
        h2 = _rgb_to_hue(*shifted[1])
        assert abs(_hue_distance(h1, h2) - 180.0) < 2.0, (
            f"shift_hue_rotate({degrees}): hues {h1:.1f}° and {h2:.1f}° "
            f"lost complementary relationship"
        )


def test_shift_complementary_rotate_always_complementary():
    """shift_complementary_rotate must always produce a 180° pair."""
    for degrees in [0, 30, 60, 90, 120, 180, 270]:
        original = complementary(0)
        shifted = shift_complementary_rotate(original, degrees)
        assert len(shifted) == 2
        h1 = _rgb_to_hue(*shifted[0])
        h2 = _rgb_to_hue(*shifted[1])
        assert abs(_hue_distance(h1, h2) - 180.0) < 2.0, (
            f"shift_complementary_rotate({degrees}): hues {h1:.1f}° and "
            f"{h2:.1f}° are not ~180° apart"
        )


def test_shift_complementary_rotate_from_non_complementary():
    """Even starting from triadic colours, the output must be complementary."""
    tri = triadic(0)
    shifted = shift_complementary_rotate(tri, 45)
    assert len(shifted) == 2
    h1 = _rgb_to_hue(*shifted[0])
    h2 = _rgb_to_hue(*shifted[1])
    assert abs(_hue_distance(h1, h2) - 180.0) < 2.0


# ── New pattern-type tests ───────────────────────────────────────────

def test_strobe_has_black_gaps():
    """Strobe pattern must contain black columns between flashes."""
    img = generate_pattern("strobe", complementary(0))
    has_black = False
    for x in range(img.width):
        col = _column(img, x)
        if all(px == (0, 0, 0) for px in col):
            has_black = True
            break
    assert has_black, "Strobe should contain black (gap) columns"


def test_strobe_has_colour_columns():
    """Strobe must also have non-black flash columns."""
    img = generate_pattern("strobe", complementary(0))
    has_colour = False
    for x in range(img.width):
        col = _column(img, x)
        if any(px != (0, 0, 0) for px in col):
            has_colour = True
            break
    assert has_colour, "Strobe should contain coloured flash columns"


def test_color_strobe_hue_changes_across_width():
    """Colour strobe flash columns should shift hue across the image."""
    img = generate_pattern("color_strobe", complementary(0))
    first_flash = None
    last_flash = None
    for x in range(img.width):
        px = img.getpixel((x, img.height // 2))
        if px != (0, 0, 0):
            if first_flash is None:
                first_flash = px
            last_flash = px
    assert first_flash is not None and last_flash is not None
    # The first and last flash colours should differ (hue rotated)
    assert first_flash != last_flash, (
        "Colour strobe flash colours should change across the image width"
    )


def test_rain_has_non_blank_and_blank_columns():
    """Rain pattern should have a mix of filled and empty columns."""
    img = generate_pattern("rain", complementary(0))
    filled = 0
    blank = 0
    for x in range(img.width):
        col = _column(img, x)
        if all(px == (0, 0, 0) for px in col):
            blank += 1
        else:
            filled += 1
    assert filled > 0, "Rain should have some filled columns"
    assert blank > 0, "Rain should have some blank columns"


def test_helix_draws_in_middle_rows():
    """Helix strands should appear in the middle rows of the image."""
    img = generate_pattern("helix", complementary(0))
    mid = img.height // 2
    has_pixel_near_mid = False
    for x in range(img.width):
        for dy in range(-3, 4):
            y = mid + dy
            if 0 <= y < img.height:
                if img.getpixel((x, y)) != (0, 0, 0):
                    has_pixel_near_mid = True
                    break
        if has_pixel_near_mid:
            break
    assert has_pixel_near_mid, "Helix should draw strands near the vertical centre"


def test_starburst_has_central_colours():
    """Starburst should have bright colours near the image centre."""
    img = generate_pattern("starburst", complementary(0))
    cx, cy = img.width // 2, img.height // 2
    px = img.getpixel((cx, cy))
    assert any(v > 50 for v in px), (
        f"Starburst centre pixel should be bright, got {px}"
    )


@pytest.mark.parametrize("pat_name", ["strobe", "color_strobe", "rain",
                                       "helix", "starburst"])
def test_new_patterns_dimensions(pat_name):
    """All new patterns must produce the correct height and positive width."""
    img = generate_pattern(pat_name, complementary(0), height=31)
    assert img.height == 31
    assert img.width > 0
    assert img.mode == "RGB"
