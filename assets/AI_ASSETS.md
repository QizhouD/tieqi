# AI-generated assets

A Steam release must disclose AI-generated content on the store page. Every asset generated or modified with AI
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
