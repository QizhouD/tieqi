"""Checks a soldier atlas PNG against the layout in atlas_layout.py.

Usage: python tools/art/check_atlas.py <atlas.png> [--strict]

Errors (exit code 1): wrong size, an empty figure cell, an empty extras cell, a cell whose content touches its
neighbour (bleeding across cells). Warnings: a figure whose lowest opaque pixel is far from the anchor row, or a cell
whose corners are not transparent. --strict turns warnings into errors.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import atlas_layout as layout  # noqa: E402
import pngio  # noqa: E402

# A pixel counts as opaque at or above this alpha.
OPAQUE = 32
# How far the lowest opaque pixel of a standing figure may sit from the anchor row.
ANCHOR_SLACK = 6


def cell_pixels(image, row, column):
    return image.crop(column * layout.CELL_PX, row * layout.CELL_PX, layout.CELL_PX, layout.CELL_PX)


def opaque_rows(cell):
    rows = []
    for y in range(cell.height):
        if any(cell.get(x, y)[3] >= OPAQUE for x in range(cell.width)):
            rows.append(y)
    return rows


def touches_edge(cell):
    size = cell.width
    edges = [(x, 0) for x in range(size)] + [(x, size - 1) for x in range(size)]
    edges += [(0, y) for y in range(size)] + [(size - 1, y) for y in range(size)]
    return any(cell.get(x, y)[3] >= OPAQUE for x, y in edges)


def check(path, strict=False):
    errors = []
    warnings = []
    image = pngio.read(path)
    if (image.width, image.height) != (layout.WIDTH, layout.HEIGHT):
        errors.append(f"size is {image.width}x{image.height}, expected {layout.WIDTH}x{layout.HEIGHT}")
        return errors, warnings
    for side in layout.SIDES:
        for kind in layout.KINDS:
            for direction in range(layout.DIRECTIONS):
                row = layout.row(side, kind, direction)
                for column, frame in enumerate(layout.FRAMES):
                    cell = cell_pixels(image, row, column)
                    where = f"{side}/{kind} dir {direction} {frame} (row {row}, column {column})"
                    rows = opaque_rows(cell)
                    if not rows:
                        errors.append(f"{where}: empty")
                        continue
                    if touches_edge(cell):
                        errors.append(f"{where}: drawing touches the cell edge and may bleed into a neighbour")
                    if frame != "fallen" and abs(rows[-1] - layout.ANCHOR_Y) > ANCHOR_SLACK:
                        warnings.append(f"{where}: feet at y={rows[-1]}, anchor is y={layout.ANCHOR_Y}")
                    corners = [cell.get(0, 0), cell.get(layout.CELL_PX - 1, 0)]
                    if any(pixel[3] >= OPAQUE for pixel in corners):
                        warnings.append(f"{where}: background is not transparent")
    for name, column in layout.EXTRA_COLUMNS.items():
        if not opaque_rows(cell_pixels(image, layout.EXTRA_ROW, column)):
            errors.append(f"extras {name} (row {layout.EXTRA_ROW}, column {column}): empty")
    if strict:
        errors += warnings
        warnings = []
    return errors, warnings


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    errors, warnings = check(argv[1], strict="--strict" in argv[2:])
    for message in warnings:
        print("warning:", message)
    for message in errors:
        print("error:", message)
    if errors:
        print(f"{len(errors)} error(s)")
        return 1
    print(f"atlas ok ({len(warnings)} warning(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
