# Art pipeline (V6)

Tools for turning 3D models into the soldier atlas the game reads from `assets/textures/soldiers/soldiers.png`.
They use only the Python standard library (plus Blender for rendering), so no packages need installing.

| File | Runs in | What it does |
|---|---|---|
| `atlas_layout.py` | any Python | The atlas layout, mirrored from `src/app/SoldierAtlas.h`; a test keeps the two in step |
| `pngio.py` | any Python | Minimal PNG read and write, crop, blit, and box downscale |
| `render_atlas.py` | Blender 4.x | Renders one side and kind (8 directions × 9 frames) with Freestyle ink lines into a strip |
| `assemble_atlas.py` | any Python | Joins strips into a full atlas, filling gaps from another atlas, and checks the result |
| `check_atlas.py` | any Python | Validates an atlas: size, empty cells, drawings that touch a cell edge, feet near the anchor |

## Layout

Cells are 32 pixels and span 9 m on the map. Columns are frames: 0 idle, 1–4 move cycle, 5–6 strike, 7 idle
looking aside, 8 fallen. Rows are (side × 5 + kind) × 8 + direction, with sides Ming then Japanese, kinds cavalry,
spear, gun, samurai, general, and directions 0 east, then clockwise on screen in 45° steps (2 faces the viewer).
The last row holds the dust puff and the banner cloths. The atlas is 288 × 2624 pixels. Standing figures have their
feet at (16, 26) in the cell; the fallen frame is centred on that point.

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
   final pixels (default 1).
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
