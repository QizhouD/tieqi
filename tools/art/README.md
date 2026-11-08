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

## Near-view map art

The V7 assets live in `assets/textures/map/`. Their contract is in `map_art_layout.py`, with dimensions and
world spans for the C++ loader and its layout test:

- `trees.png`: 1536 x 192 RGBA, eight 192-pixel cells in one row. Each cell spans 24 m, with its trunk/root
  anchor at (96, 184), tolerances 8 pixels horizontally and 4 vertically. Cell order: tall pine, crooked pine,
  bare deciduous, broad bare deciduous, bare willow, low shrub clump, young pine, bare sapling pair. The background
  is transparent, with two clear pixels at each edge. Draw with ordinary alpha blending; the soft ink contact
  wash is part of the sprite. Large silhouettes are 120-170 pixels high; the shrub is 50-80 pixels high.
- `ground_detail.png`: 512 x 512, one repeating 64 m square. Silk weave, fibres, dry-brush marks, faint grass
  ticks and pebbles. Draw with multiply blending over the existing base map: white leaves the map unchanged.
- `paddy_detail.png`: 256 x 256, one repeating 32 m square. Harvested winter stubble in horizontal rows roughly
  five pixels apart, with bare earth and pale frost/water sheen. Also multiply blended, restricted to paddies.

Both tiles use 8 pixels per metre. Their alpha is ignored. Mean normalized RGB luminance must be 0.86-0.94 and
the lower 1% quantile at least 0.45. This is Rec. 709 weighting of stored channel values, without a linear-light
conversion. The near-view renderer controls fading; these files do not bake in that fade.

Run the stdlib-only checker from the repository root:

```text
python tools/art/check_map_art.py assets/textures/map
```

Success prints exactly `map art ok` and exits 0. A failure prints one line per problem and exits 1. It checks
sizes, nonempty trees, alpha-zero two-pixel padding and 12-pixel corner patches, and trunk anchors. Opaque means
alpha at least 128: the lowest three opaque rows locate the trunk/root centre, excluding translucent contact
wash. A sapling pair uses the joint centre. Tile checks compare RGB mean absolute differences across each wrap
seam against all adjacent interior pixels on that axis, failing above 2x, and check the luminance limits.
Visual review is still needed for silhouette identity, season, interior matte islands, halos and repeated motifs.

The initial art was generated on 2026-11-07 with the built-in `image_gen.imagegen` tool; the tool did not expose
a model name or version. Three separate prompts requested a transparent 4-by-2 tree source sheet, a near-white
silk/earth tile, and a near-white harvested paddy tile. All requested Ming landscape brushwork, no text or seals,
and late-winter vegetation. Prompt summaries and processing provenance are registered in `assets/AI_ASSETS.md`.
No external reference image was supplied.

Offline processing used `pngio.py` and numpy 1.26.4 already bundled with Blender 4.5; no package was installed.
Trees were separated, residual near-transparent haze and disconnected neighbouring fragments removed, resized
with a premultiplied-alpha area filter, placed on the anchors, and given a soft neutral ink contact wash.
Tiles were area-filtered to size, converted to neutral grayscale, normalized to a 0.90 mean, and opposite edge
bands blended toward each other with a quadratic falloff over 16 pixels. This preserves the drawn interior and
makes both wrap seams continuous. Human edits: none; processed by script.

Local scratch files (ignored by Git) are `build/codex/process_v7.py`, `build/codex/v7-prompts.json`, the three
`build/codex/v7-*-source.png` images, and `build/codex/test_map_art_checker.py`. The latter exercises the checker
with valid and deliberately broken fixtures. `build/codex/v7-contact.png` shows the atlas on silk and each tile
repeated 3 x 3 at native pixel size, multiplied over silk. `build/codex/v7-trees-preview.png` shows the trees at
half atlas size for readability review. These local files are review aids, not runtime dependencies.

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
