# Roles and permissions

## The author

Owns the repository and every decision about gameplay, art direction, historical presentation, spending, new
dependencies, and anything public. Accepts milestones by playing the game. Can write into any thread as `author`.

## Claude Code: lead, reviewer, integrator

- Picks the next card in the order the author gave, splits it into tasks, and writes the task messages.
- Owns everything that leaves the machine: pushing, pull requests, CI, merging, and the one-line report to the
  author after each merge. Uses the credentials Git already stores, touches only `QizhouD/tieqi`, and never prints
  credentials or writes them into any file.
- Reviews every change from Codex before it is committed to a pushed branch: reads the diff, builds with the dev
  preset (zero warnings), runs `ctest --preset dev`, runs the `--perf` replay with the release preset, captures
  screenshots, and checks the result against GCC 13 warnings before CI does.
- Keeps `docs/PROGRESS.md` and the PR description up to date.
- May implement directly when a change is small or when handing it over would cost more than doing it; says so in
  the thread.

## Codex: implementer

- Works only inside this repository, only on the branch named in `STATUS.md`, and only while `turn: codex`.
- Writes code, tests, tools, and the doc changes the task asks for.
- May build and run tests locally and may commit on the task branch. Never pushes, never opens or merges pull
  requests, never calls the GitHub API, and never changes branches other than the task branch.
- Stays inside the scope of the task message. Anything outside it (a bug found elsewhere, a spec question, an idea)
  goes into the report under "Out of scope" instead of into the code.
- Reports exactly what it ran and what it could not run. A check that was not run is listed as not run, never
  described as passing.

## Forbidden for both agents

From `docs/AGENT_LOOP.md` and the repository hard rules; there are no exceptions:

- Pushing directly to `main`, force pushing, merging a PR whose CI is not fully green.
- Skipping, disabling, or deleting tests, or loosening a test so that it passes.
- Changing anything under `reference/`.
- Touching any other repository or any folder outside this repository.
- Changing the rules core (`src/core`) during M4 and M5 presentation cards; if a card seems to need it, stop and set
  `turn: author`.
- Using `tq::Rng` from presentation code, or iterating units in any order other than ascending `UnitId`.
- Bringing in data, scripts, text, or assets from Sengoku Jidai or Field of Glory.
- Adding a third-party dependency without the author's approval.
- Writing non-English text anywhere except the places listed in `AGENTS.md`.
- Committing AI-generated assets without a row in `assets/AI_ASSETS.md` in the same commit.

## Handover etiquette

- Leave the working tree in a state the other agent can build: no half-applied edits, no stray files outside the
  task. Scratch output goes under `build/` or `out/`, which Git ignores.
- If you stop in the middle of your turn (out of time, blocked), still write a message saying where you stopped,
  and hand the turn back.
