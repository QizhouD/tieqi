"""Build all ten M5 figures, render strips, validate and install the soldier atlas.

Requires Blender 4.5 and a built tieqi executable. Uses only the Python standard
library. Intermediate blend files, cells, logs and contact sheets stay in build/.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

import atlas_layout as layout
import check_atlas
import pngio

ROOT = Path(__file__).resolve().parents[2]
SCALES = {"cavalry": 1.85, "general": 1.90, "spear": 2.0, "gun": 2.4, "samurai": 2.65}


def run(command, log, env):
    print("Running:", " ".join(str(arg) for arg in command), flush=True)
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run([str(arg) for arg in command], cwd=ROOT, env=env,
                                stdout=output, stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        raise SystemExit(f"Command failed ({result.returncode}); see {log}")


def contact_sheet(atlas, path):
    """Ten 3x strips in two columns: Ming left, Japanese right; kinds top to bottom."""
    width = len(layout.FRAMES) * layout.CELL_PX
    height = layout.DIRECTIONS * layout.CELL_PX
    sheet = pngio.Image(width * 2 * 3, height * len(layout.KINDS) * 3)
    for side_index, side in enumerate(layout.SIDES):
        for kind_index, kind in enumerate(layout.KINDS):
            strip = atlas.crop(0, layout.row(side, kind, 0) * layout.CELL_PX, width, height)
            for y in range(height):
                for x in range(width):
                    r, g, b, a = strip.get(x, y)
                    bg = (220, 211, 189) if (x // 32 + y // 32) % 2 == 0 else (204, 196, 177)
                    pixel = tuple((v * a + c * (255 - a)) // 255 for v, c in zip((r, g, b), bg)) + (255,)
                    for dy in range(3):
                        for dx in range(3):
                            sheet.put((side_index * width + x) * 3 + dx,
                                      (kind_index * height + y) * 3 + dy, pixel)
    pngio.write(str(path), sheet)
    # Preserve the actual complete layout, including the extras and unused cells.
    full = pngio.Image(atlas.width * 3, atlas.height * 3)
    for y in range(atlas.height):
        row = b"".join(bytes(atlas.get(x, y)) * 3 for x in range(atlas.width))
        for dy in range(3):
            start = (y * 3 + dy) * full.width * 4
            full.pixels[start:start + len(row)] = row
    pngio.write(str(path.with_name("atlas-3x.png")), full)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--blender", type=Path, default=Path(r"C:\Program Files\Blender Foundation\Blender 4.5\blender.exe"))
    parser.add_argument("--game", type=Path, default=ROOT / "build/dev/tieqi.exe")
    parser.add_argument("--work", type=Path, default=ROOT / "build/codex/atlas")
    parser.add_argument("--out", type=Path, default=ROOT / "assets/textures/soldiers/soldiers.png")
    args = parser.parse_args()
    blender = shutil.which(str(args.blender))
    if blender is None:
        parser.error(f"Blender executable not found: {args.blender}; use --blender")
    args.blender = Path(blender).resolve()
    if not args.game.is_file():
        parser.error(f"Game executable not found: {args.game}; build it first or use --game")
    work = args.work.resolve()
    for folder in ("models", "strips", "logs", "cells", "tmp"):
        (work / folder).mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TEMP=str(work / "tmp"), TMP=str(work / "tmp"))
    for side in layout.SIDES:
        for kind in layout.KINDS:
            name = f"{side}_{kind}"
            family = "mounted" if kind in ("cavalry", "general") else "infantry"
            model = work / "models" / f"{name}.blend"
            run([args.blender, "--background", "--factory-startup", "--python-exit-code", "1",
                 "--python", ROOT / "tools/art/models" / f"{side}_{family}.py", "--", "--kind", kind,
                 "--out", model], work / "logs" / f"{name}-model.log", env)
            run([args.blender, "--background", model, "--python-exit-code", "1", "--python",
                 ROOT / "tools/art/render_atlas.py", "--", "--side", side, "--kind", kind,
                 "--figure-scale", SCALES[kind], "--line", "0.3", "--scratch", work / "cells" / name,
                 "--out", work / "strips" / f"{name}.png"], work / "logs" / f"{name}-render.log", env)
    extras = work / "placeholder.png"
    run([args.game.resolve(), "--dump-atlas", extras], work / "logs/dump.log", env)
    candidate = work / "soldiers.png"
    run([sys.executable, ROOT / "tools/art/assemble_atlas.py", "--strips", work / "strips",
         "--extras", extras, "--out", candidate], work / "logs/assemble.log", env)
    errors, warnings = check_atlas.check(str(candidate), strict=True)
    if errors or warnings:
        raise SystemExit("\n".join(errors + warnings))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(candidate.read_bytes())
    contact_sheet(pngio.read(str(candidate)), work / "contact-sheet-3x.png")
    print(f"Installed {args.out}: 720 rendered cells, 0 errors, 0 warnings")


if __name__ == "__main__":
    main()
