"""Joins rendered strips into the full soldier atlas and checks it.

Usage: python tools/art/assemble_atlas.py --strips build/art/strips --extras <atlas.png> --out <soldiers.png>

--strips holds one PNG per side and kind, named <side>_<kind>.png (for example ming_cavalry.png), each 8 rows by
9 columns of cells as written by render_atlas.py. A missing strip is taken from the --extras atlas, so a partial set
of renders can be tried in game. --extras is any complete atlas, usually the placeholder written by
`tieqi --dump-atlas`; its extras row (dust and banner cloths) is copied as is. The result is checked with
check_atlas.py; copy it to assets/textures/soldiers/soldiers.png to use it in the game.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import atlas_layout as layout  # noqa: E402
import check_atlas  # noqa: E402
import pngio  # noqa: E402


def parse(argv):
    options = {}
    i = 1
    while i < len(argv):
        options[argv[i].lstrip("-")] = argv[i + 1]
        i += 2
    for required in ("strips", "extras", "out"):
        if required not in options:
            raise SystemExit(__doc__)
    return options


def main(argv):
    options = parse(argv)
    base = pngio.read(options["extras"])
    if (base.width, base.height) != (layout.WIDTH, layout.HEIGHT):
        raise SystemExit(f"--extras must be a full atlas of {layout.WIDTH}x{layout.HEIGHT}")
    atlas = pngio.Image(layout.WIDTH, layout.HEIGHT)
    atlas.blit(base, 0, 0)
    strip_w = layout.CELL_PX * len(layout.FRAMES)
    strip_h = layout.CELL_PX * layout.DIRECTIONS
    for side in layout.SIDES:
        for kind in layout.KINDS:
            path = os.path.join(options["strips"], f"{side}_{kind}.png")
            if not os.path.exists(path):
                print(f"{side}_{kind}: no strip, keeping the --extras figures")
                continue
            strip = pngio.read(path)
            if (strip.width, strip.height) != (strip_w, strip_h):
                raise SystemExit(f"{path}: expected {strip_w}x{strip_h}, got {strip.width}x{strip.height}")
            atlas.blit(strip, 0, layout.row(side, kind, 0) * layout.CELL_PX)
            print(f"{side}_{kind}: placed")
    pngio.write(options["out"], atlas)
    errors, warnings = check_atlas.check(options["out"])
    for message in warnings:
        print("warning:", message)
    for message in errors:
        print("error:", message)
    print(f"wrote {options['out']}: {len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
