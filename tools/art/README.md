# Art pipeline (V6)

Tools for turning 3D models into the soldier atlas the game reads from `assets/textures/soldiers/soldiers.png`.
They use only the Python standard library (plus Blender for rendering), so no packages need installing.

| File | Runs in | What it does |
|---|---|---|
| `atlas_layout.py` | any Python | The atlas layout, mirrored from `src/app/SoldierAtlas.h`; a test keeps the two in step |
| `pngio.py` | any Python | Minimal PNG read and write, crop, blit, and box downscale |
| `render_atlas.py` | Blender 4.x | Renders one side and kind (8 directions × 9 frames) with Freestyle ink lines into a strip |
| `example_model.py` | Blender 4.x | Builds a simple primitive soldier with every action, to show the model structure and smoke-test the pipeline |
| `assemble_atlas.py` | any Python | Joins strips into a full atlas, filling gaps from another atlas, and checks the result |
| `check_atlas.py` | any Python | Validates an atlas: size, empty cells, drawings that touch a cell edge, feet near the anchor |
| `build_atlas.py` | any Python | Rebuilds, renders, checks and installs all ten finished figures with Blender and the game executable |
| `models/` | Blender 4.5 | Four family entry points and shared geometry, rig and animation helpers |

## Layout

Cells are 32 pixels and span 9 m on the map. Columns are frames: 0 idle, 1–4 move cycle, 5–6 strike, 7 idle
looking aside, 8 fallen. Rows are (side × 5 + kind) × 8 + direction, with sides Ming then Japanese, kinds cavalry,
spear, gun, samurai, general, and directions 0 east, then clockwise on screen in 45° steps (2 faces the viewer).
The last row holds the dust puff and the banner cloths. The atlas is 288 × 2624 pixels. Standing figures have their
feet at (16, 26) in the cell; the fallen frame is centred in the cell.

## Smoke test

Verified with Blender 4.5.10 LTS:

```text
blender --background --factory-startup --python tools/art/example_model.py -- --side jp --out build/art/jp_samurai.blend
blender --background build/art/jp_samurai.blend --python tools/art/render_atlas.py --     --side jp --kind samurai --out build/art/strips/jp_samurai.png
build/dev/tieqi --dump-atlas build/art/placeholder.png
python tools/art/assemble_atlas.py --strips build/art/strips --extras build/art/placeholder.png --out build/art/soldiers.png
```

Rendering one strip takes about 12 seconds. The result passes the checker, and copied to
`assets/textures/soldiers/soldiers.png` it replaces the Japanese samurai in game while every other figure stays the
placeholder.

## Steps

1. Model each side and kind as its own `.blend` file following the requirements at the top of `render_atlas.py`
   (an object named `Figure`, the model facing +X at real scale, actions `idle`, `move`, `strike`, `aside`,
   `fallen`, flat unlit materials in the gongbi or woodblock palette).
2. Render each one:

   ```text
   blender --background art/ming_cavalry.blend --python tools/art/render_atlas.py -- \
       --side ming --kind cavalry --out build/art/strips/ming_cavalry.png
   ```

   `--supersample` (default 4) renders larger and scales down for clean lines; `--line` sets the ink line width in
   final pixels (default 0.5).
3. Write the placeholder atlas as a base and assemble:

   ```text
   build/dev/tieqi --dump-atlas build/art/placeholder.png
   python tools/art/assemble_atlas.py --strips build/art/strips --extras build/art/placeholder.png \
       --out build/art/soldiers.png
   ```

4. Fix anything `check_atlas.py` reports, copy the result to `assets/textures/soldiers/soldiers.png`, and register
   every AI-made input in `assets/AI_ASSETS.md`.

The game loads the PNG at startup when it has the right size; otherwise it falls back to the placeholder and prints
a warning.

## Finished figures

After building the dev executable, rebuild and install all ten figures from the repository root:

```text
python tools/art/build_atlas.py
```

The default Blender path is `C:\Program Files\Blender Foundation\Blender 4.5\blender.exe` (verified with 4.5.10 LTS).
Use `--blender <executable>` and `--game <tieqi executable>` on other machines. `--work` changes the scratch folder
(default `build/codex/atlas`); `--out` changes the installed PNG path. No additional Python packages are needed.
The command rebuilds every model and every strip, dumps the game's placeholder for its extras, assembles the atlas,
and installs it only after the strict checker passes. The dust and runtime-heraldry banner cloths stay from that
placeholder; all 720 figure cells come from Blender.

`models/common.py` supplies the geometry, rigid-part bone parenting and five actions. The four family entry points
are `ming_mounted.py`, `ming_infantry.py`, `jp_mounted.py` and `jp_infantry.py`; each accepts `--kind` and `--out`.
Mounted families build cavalry and generals; infantry families build spear, gun and samurai slots. The Ming samurai
slot is sword-and-shield infantry. Both generals are mounted, following the accepted M4 presentation decision.
The Japanese commander is generic, with a jinbaori and simple helmet ornaments, not a claimed portrait of either
named commander. Small personal sashimono are plain; researched crests remain on the game's separate unit banners.

The shared palette uses vermilion `#a8382a` and gold `#d9b466` for Ming, indigo `#2f3a56` and silver-grey `#aab4c9`
for Japanese figures, with warm brown horses. Flat emission materials and Freestyle contours keep them compatible
with the silk map. Armour plates, rivets, jingasa, helmet neck guards, horse heads and weapons are deliberately
broadened for the final pixel size. The spear shaft and blade span about 3.4 model metres, a visual compression of
the task's 4.5-to-5.5-metre yari reference; the shaft stays readable and inside its cell in all directions.

Render settings: supersampling 4; ink line 0.3 final pixels; figure scales cavalry 1.85, general 1.90, spear 2.00,
gun 2.40, samurai 2.65, identical on both sides. These are presentation scales, not simulation values. Each rig has
idle, four gait samples, two strike samples (aim/fire for guns), a head turn, and a fallen pose. Strike actions run
from frame 1 to 17 so the existing renderer samples frames 1 and 9. Mounted gait moves four horse legs, neck and
tail; fallen rolls horse and rider together. Gun flash scale is keyed in every action to prevent pose leakage.

Intermediate `.blend` files, supersampled cells, per-command logs, and the assembled candidate stay under `--work`.
`contact-sheet-3x.png` rearranges the figure strips into two columns (Ming left, Japanese right), with cavalry,
spear, gun, samurai, general from top to bottom; within each strip, rows are directions and columns are frames.
`atlas-3x.png` is the complete atlas at 3x, including extras, in its original layout.
The atlas is 288 x 2592 pixels (80 figure rows plus one extras row).

The renderer also accepts `--scratch <directory>` to retain cells and avoid private temporary-directory ACL
failures in restricted Windows sessions. The builder uses a separate scratch directory for each strip. Without
this option the existing temporary-directory behaviour is unchanged.
