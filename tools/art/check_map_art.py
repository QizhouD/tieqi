"""Validate near-view map textures using only the Python standard library.

Usage: python tools/art/check_map_art.py <assets/textures/map>
Exit 0 prints "map art ok"; exit 1 prints one line per asset problem.
"""

import math
from pathlib import Path
import struct
import sys
import zlib

import map_art_layout as layout
import pngio


def check_trees(image):
    errors = []
    if (image.width, image.height) != (layout.TREE_WIDTH, layout.TREE_HEIGHT):
        return [f"size {image.width}x{image.height}, expected "
                f"{layout.TREE_WIDTH}x{layout.TREE_HEIGHT}"]
    size = layout.TREE_CELL_PX
    for index, name in enumerate(layout.TREE_NAMES):
        cell = image.crop(index * size, 0, size, size)
        label = f"cell {index} ({name})"
        visible = [(x, y) for y in range(size) for x in range(size)
                   if cell.get(x, y)[3] > 0]
        solid = [(x, y) for x, y in visible
                 if cell.get(x, y)[3] >= layout.TREE_OPAQUE_ALPHA]
        if not solid:
            errors.append(f"{label}: empty or no opaque trunk")
        if any(min(x, y, size - 1 - x, size - 1 - y) < layout.TREE_PADDING
               for x, y in visible):
            errors.append(f"{label}: drawing touches the 2-pixel cell padding")
        # A reserved 12x12 patch at each corner must be truly alpha-zero. This
        # catches a matte even if its outermost two pixels were cleared.
        corner = 12
        if any(cell.get(x, y)[3] != 0
               for y0 in (0, size - corner) for x0 in (0, size - corner)
               for y in range(y0, y0 + corner) for x in range(x0, x0 + corner)):
            errors.append(f"{label}: background is not transparent")
        if not solid:
            continue
        bottom = max(y for _, y in solid)
        # Contact wash is translucent; the opaque bottom three rows locate
        # the trunk/roots. A pair of saplings is anchored by their joint centre.
        base_x = [x for x, y in solid if y >= bottom - 2]
        centre = (min(base_x) + max(base_x)) / 2.0
        if (abs(bottom - layout.TREE_ANCHOR_Y) > layout.TREE_ANCHOR_TOLERANCE_Y
                or abs(centre - layout.TREE_ANCHOR_X) > layout.TREE_ANCHOR_TOLERANCE_X):
            errors.append(f"{label}: trunk base ({centre:.1f}, {bottom}) outside "
                          f"anchor ({layout.TREE_ANCHOR_X}, {layout.TREE_ANCHOR_Y}) "
                          f"tolerance ({layout.TREE_ANCHOR_TOLERANCE_X}, "
                          f"{layout.TREE_ANCHOR_TOLERANCE_Y})")
    return errors


def tile_metrics(image):
    """Return mean, lower 1% quantile, and (wrap MAD, interior MAD) per axis."""
    width, height = image.width, image.height
    pixels = image.pixels
    luminances = sorted((0.2126 * pixels[i] + 0.7152 * pixels[i + 1]
                         + 0.0722 * pixels[i + 2]) / 255.0
                        for i in range(0, len(pixels), 4))

    def difference(a, b):
        return sum(abs(pixels[a + c] - pixels[b + c]) for c in range(3)) / 3.0

    wrap_x = sum(difference(y * width * 4, (y * width + width - 1) * 4)
                 for y in range(height)) / height
    wrap_y = sum(difference(x * 4, ((height - 1) * width + x) * 4)
                 for x in range(width)) / width
    interior_x = sum(difference((y * width + x) * 4, (y * width + x + 1) * 4)
                     for y in range(height) for x in range(width - 1)) / (height * (width - 1))
    interior_y = sum(difference((y * width + x) * 4, ((y + 1) * width + x) * 4)
                     for y in range(height - 1) for x in range(width)) / (width * (height - 1))
    quantile = max(0, math.ceil(len(luminances) * layout.LUMINANCE_DARK_PERCENTILE) - 1)
    return (sum(luminances) / len(luminances), luminances[quantile],
            (wrap_x, interior_x), (wrap_y, interior_y))


def check_tile(image, size):
    if (image.width, image.height) != (size, size):
        return [f"size {image.width}x{image.height}, expected {size}x{size}"]
    errors = []
    mean, dark, seam_x, seam_y = tile_metrics(image)
    if not layout.LUMINANCE_MEAN_MIN <= mean <= layout.LUMINANCE_MEAN_MAX:
        errors.append(f"mean luminance {mean:.4f} outside "
                      f"[{layout.LUMINANCE_MEAN_MIN}, {layout.LUMINANCE_MEAN_MAX}]")
    if dark < layout.LUMINANCE_DARK_MIN:
        errors.append(f"darkest 1% luminance {dark:.4f} below {layout.LUMINANCE_DARK_MIN}")
    for axis, (wrap, interior) in zip(("x", "y"), (seam_x, seam_y)):
        if wrap > layout.SEAM_RATIO_MAX * interior:
            errors.append(f"{axis} seam MAD {wrap:.4f} exceeds "
                          f"{layout.SEAM_RATIO_MAX:g}x interior MAD {interior:.4f}")
    return errors


def check(directory):
    errors = []
    for name, size in (("trees.png", None), ("ground_detail.png", layout.GROUND_PX),
                       ("paddy_detail.png", layout.PADDY_PX)):
        try:
            image = pngio.read(Path(directory) / name)
            problems = check_trees(image) if size is None else check_tile(image, size)
        except (OSError, ValueError, IndexError, struct.error, zlib.error) as error:
            problems = [f"cannot read PNG: {error}"]
        errors.extend(f"{name}: {problem}" for problem in problems)
    return errors


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip())
        return 2
    errors = check(argv[1])
    for error in errors:
        print(error)
    if errors:
        return 1
    print("map art ok")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
