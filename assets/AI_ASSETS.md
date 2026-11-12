# AI-generated assets

AI-generated assets are disclosed and registered here. Every asset generated or modified with AI
tools gets one row below when it is committed, in the same commit. An asset replaced later (for example by an
artist's work) keeps its row, marked as replaced, so the history stays complete.

## How to register an asset

| Column | What to write |
|---|---|
| File | Path from the repository root, for example `assets/textures/portraits/jiading.png`. One row per file; a set made in one session (such as an atlas strip and its source renders) may share a row if every path is listed |
| Use | Where the game shows it: soldier atlas, portrait, banner, map texture, effect, interface |
| Tool and model | Product and model with version, for example "Blender 4.2 render of an AI-generated mesh (Meshy 4)" or "Image model X, version Y". Name every AI step; plain tools such as Blender count only when they render AI-made input |
| Generated on | Date in YYYY-MM-DD |
| Prompt or notes | The prompt, or a short summary with the reference images used (for example the Wanli-era paintings named in DESIGN.md), and any seed or settings needed to redo it |
| Human edits | What a person changed afterwards: repainting, cropping, recolouring, retopology, none |
| Status | in use, replaced (with the replacing file or artist and date), or removed |

Code-drawn placeholders (the soldier atlas from `tieqi --dump-atlas`, banner cloths, crests) are not AI-generated
and are not listed.

## Register

| File | Use | Tool and model | Generated on | Prompt or notes | Human edits | Status |
|---|---|---|---|---|---|---|
| `assets/textures/map/trees.png` | Near-view map trees and contact shadows | Built-in `image_gen.imagegen`; model name/version not reported by tool; Python processing with pngio and Blender 4.5 bundled numpy 1.26.4 | 2026-11-07 | Prompt: transparent 4-column by 2-row source sheet of tall pine, crooked pine, bare deciduous, broad bare deciduous, bare winter willow, low winter shrub, young pine, bare sapling pair; isolated side views, clear margins and ground bases. Ming blue-green brush painting, ink #2a221b, mineral green #467860, azurite #325e7e, ochre #a87e48; dry-brush bark, simple canopy masses, contact wash, readable at 30-80 px. Late winter, only pines evergreen; no text, grid, seals, signature, scenery, gloss or photography. No reference image. Full prompt retained locally in build/codex/v7-prompts.json. | none; processed by script: remove alpha haze and disconnected neighbouring fragments, split sheet, premultiplied-alpha area resize, anchor at (96,184), add soft ink contact wash, pack eight 192 px cells into 1536 x 192 RGBA | in use |
| `assets/textures/map/ground_detail.png` | Near-view repeating ground multiply layer | Built-in `image_gen.imagegen`; model name/version not reported by tool; Python processing with pngio and Blender 4.5 bundled numpy 1.26.4 | 2026-11-07 | Prompt: seamless square flat ground-detail multiply texture; near-white mean luminance about 0.90, fine woven silk threads/fibres, faint dry-brush strokes and mottling, scattered tiny dry grass ticks and pebbles. Ming landscape material as neutral multiply detail; late-winter dry earth. Consistent density to every edge, no border, vignette, gradient, large focal/repeated motifs, horizon, text, seal, signature, photography or gloss. No reference image. Full prompt retained locally in build/codex/v7-prompts.json. | none; processed by script: area resize to 512 x 512, neutral grayscale conversion, normalize mean to 0.90, blend opposite 16 px edge bands with quadratic falloff for seamless wrapping, write opaque RGBA | in use |
| `assets/textures/map/paddy_detail.png` | Near-view repeating paddy stubble multiply layer | Built-in `image_gen.imagegen`; model name/version not reported by tool; Python processing with pngio and Blender 4.5 bundled numpy 1.26.4 | 2026-11-07 | Prompt: seamless square flat winter rice-paddy multiply texture, no perspective; short harvested rice-stubble ink ticks in about 51 horizontal rows, slightly irregular spacing and sparse bare-earth gaps, faint frost/water sheen, silk fibres and dry-brush marks. Near-white neutral grayscale mean about 0.90, minimum 0.45; Ming ink-wash character. No green crops, standing rice, snow blanket, big blobs, border, vignette, horizon, text, seal or signature. Downsample to 256 px with a row roughly every 5 px. No reference image. Full prompt retained locally in build/codex/v7-prompts.json. | none; processed by script: area resize to 256 x 256, neutral grayscale conversion, normalize mean to 0.90, blend opposite 16 px edge bands with quadratic falloff for seamless wrapping, write opaque RGBA | in use |
| `assets/textures/soldiers/soldiers.png`; `tools/art/models/common.py`; `tools/art/models/ming_mounted.py`; `tools/art/models/ming_infantry.py`; `tools/art/models/jp_mounted.py`; `tools/art/models/jp_infantry.py` | Soldier atlas: ten figure kinds, eight directions, nine frames | OpenAI Codex (GPT-6; backend revision not exposed), writing procedural Blender Python models and rigs; rendered with Blender 4.5.10 LTS | 2026-11-07 | M5 task brief and DESIGN.md equipment/style guidance; original primitive geometry, no external meshes or reference-image inputs. Vermilion/gold Ming brigandine and bowl helmets; indigo/silver Japanese armour, jingasa and kabuto; mounted commanders. Rebuild with `python tools/art/build_atlas.py`; scales and deliberate equipment simplifications in tools/art/README.md. Existing code-drawn dust and banner cloth cells retained. Historical and style choices awaiting author review. | None | in use |
