# Expertise: what a specialist in each field brings to this project

Each section names the skills a specialist in that field would apply here, the tools in this repository that
support them, and the checks that prove the work. A task message lists the sections that apply; the agent doing the
work follows them, and the report shows the checks were run.

Exact commands assume the repository root. On Windows, run the CMake commands from a Visual Studio developer
environment (vcvars64); the presets pin `VSLANG=1033` so Ninja tracks header dependencies.

## 1. Deterministic simulation (rules core)

Skills
- Fixed-step simulation: state changes only inside logic ticks (1 tick = 0.8 battlefield seconds); the
  presentation reads snapshots and sends commands only through `Battle::submit()`.
- Reproducibility: one `tq::Rng` per battle, no `std::rand`, `std::random_device`, clocks, or unordered iteration;
  units always in ascending `UnitId`.
- Parity with the reference prototype `reference/prototype/byeokjegwan.html`, which is read-only.

Tools: `tools/headless` (headless battles and the 100-battle baseline), `tools/parity` (tick-by-tick comparison with
the prototype), `src/core/rules/Constants.h`.

Checks
- `ctest --preset dev`, plus the baseline tests labelled `slow`, which that preset skips:
  `ctest --test-dir build/dev -L slow`.
- During M4 and M5, `git diff --stat main -- src/core` is empty.

## 2. Real-time 2D rendering (SFML 3.1)

Skills
- Batching: build `sf::VertexArray` geometry per layer and draw each texture once; no per-object draw calls and no
  per-frame heap allocation in hot paths (reuse buffers, `clear()` keeps capacity).
- Texture atlases: cell padding or inset texture coordinates so neighbours never bleed; smoothing and mipmaps
  chosen per texture for the zoom range.
- Camera and zoom: `sf::View`, world-to-screen transforms, pixel snapping where sprites shimmer, culling against the
  view rectangle before building geometry.
- Level of detail: what the far view shows (counters, banners) versus the near view at 3x to 4x (soldiers, ground
  detail, effects), with cross-fades rather than pops.
- Draw order and layering: ground, ground detail, shadows, bodies, figures, effects, banners, interface.

Tools: `src/app` (views, `SoldierView`, `Formation`, `SoldierAtlas`), `tieqi --script <file>` and `--freeze-at
<tick>` for reproducible screenshots, `--dump-atlas <file>`.

Checks
- Before and after screenshots of the same script and tick, at far and near zoom, attached to the PR.
- No visible seams, bleeding, or shimmer when panning and zooming.

## 3. Performance

Skills
- Measure, do not guess: frame-time distribution (mean, p95, max) from release builds only.
- Budgeting by layer; worst case is the most soldiers on screen at near zoom.
- Bounded work per frame: caps on particles, decals, and instances; spatial hashing; work scaled by zoom.

Tools: `cmake --preset release`, `cmake --build --preset release`, then
`build/release/tieqi --perf` (starts the battle at 2x, runs it to the result, and prints mean, p95, and max frame
time in ms); repeat with `--time-scale 4`.

Checks
- p95 at most 16.7 ms in both runs, measured on a fresh release build, numbers in the report and the PR.
- Report the numbers before and after the change, from the same machine.

## 4. Technical art: procedural ground and landscape (V7)

Skills
- Procedural textures: value or gradient noise, fractal sums, domain warping, tileable textures, colour ramps; all
  presentation randomness from its own seeded generator, never `tq::Rng`.
- Style target: the silk-scroll base map already in the game (blue-green landscape painting on aged silk). Muted
  mineral greens and ochres, ink outlines, paper grain; readable rather than photographic.
- Terrain features: paddy fields with ridges (narrow raised banks in regular, slightly irregular plots), scattered
  trees and groves as batched sprites with simple shadows, roads and streams kept from the existing map data.
- Generating detail once (into textures or cached vertex arrays) rather than every frame.
- Holding up at 3x to 4x zoom without changing the far view.

Tools: existing map rendering in `src/app`, scenario map data under `data/`, `tools/art` for offline images.

Checks
- Screenshots at 1x, 2x, 3x, 4x of the same spot; the far view is unchanged pixel for pixel or the difference is
  explained.
- Perf as in section 3 at 4x zoom over the busiest area.

## 5. Visual effects (V8)

Skills
- Particle systems with pooled storage, fixed upper bounds, and lifetimes; emitters driven by rules events read from
  snapshots.
- Zoom-aware level of detail: size and count of dust, smoke, and debris scale with zoom; far view stays light.
- Blending and layering: alpha versus additive, sorting only when needed, soft edges from small textures.
- Presentation randomness from a separate generator seeded per effect.

Checks: screenshots at near and far zoom during a melee and a volley; particle count at the busiest moment; perf as
in section 3.

## 6. Art pipeline and assets

Skills
- The atlas contract in `src/app/SoldierAtlas.h` and `tools/art/atlas_layout.py`: 32 px cells of 9 m, 9 frames,
  anchor (16, 26), fallen frame centred.
- Blender 4.5 LTS headless rendering with `tools/art/render_atlas.py`; readable 32 px figures (thin lines,
  silhouettes first).
- Provenance: every AI-generated or AI-modified asset gets a row in `assets/AI_ASSETS.md` in the same commit.

Tools: `tools/art/README.md`, `render_atlas.py`, `assemble_atlas.py`, `check_atlas.py`, `example_model.py`.

Checks: `python tools/art/check_atlas.py <atlas.png>` reports 0 errors; the atlas layout test in
`tests/app/soldiers.cpp` passes.

## 7. History and cultural presentation

Skills
- The setting: the battle of Byeokjegwan, early 1593, Ming army and Joseon allies against the Japanese army.
- Neutral naming: "Japanese army" or "Japanese forces", never derogatory terms for any side.
- Heraldry and equipment drawn from period sources; when a source is uncertain, ask the author.

Checks: anything that shows a person, a crest, a banner, or a historical claim is listed in the report under
"Needs author review".

## 8. Localization and fonts

Skills: Traditional Chinese (zh-Hant) as the source text in `data/strings/zh-Hant.json`; en and ko translations
reviewed by the author; no player-visible text in code.

Checks: the missing-glyph test in `tests/app/glyphs.cpp`; `python tools/check_source_language.py` passes.

## 9. C++ quality, tests, and CI

Skills
- C++20, warnings as errors on GCC 13 in CI and `/W4` on MSVC locally. Common GCC-only failures: unused functions
  and variables (`-Wunused-function`), `-Wdangling-reference` on references bound to temporaries, sign and shadow
  warnings, missing includes that MSVC tolerates.
- Tests first with doctest 2.4.12; each acceptance item of the card has a test where it can be tested.
- `clang-format` on touched files.

Checks
- `cmake --preset dev && cmake --build --preset dev` with zero warnings.
- `ctest --preset dev` all passing.
- `python tools/check_source_language.py` passes.
- A read of the diff against the GCC pitfalls above.

## Card to sections

| Card | Sections |
|---|---|
| V7 Near-view map | 2, 3, 4, 7, 9 (and 1: no core change) |
| V8 Near-view effects | 2, 3, 5, 9 (and 1: no core change) |
| Art from AI models | 6, 7 |
| Any rules change | 1, 9, and the "Rule spec" in `docs/DEVELOPMENT.md` |
