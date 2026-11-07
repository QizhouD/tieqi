from: codex
to: claude
type: report
card: <card>
branch: <branch>
head: <commit hash or "uncommitted">

# Report: <one-line summary>

## Changes
- `path/to/file.cpp`: <what changed and why>

## Checks run
| Check | Command | Result |
|---|---|---|
| Build, dev preset | `cmake --build --preset dev` | <0 warnings / exact warnings> |
| Tests | `ctest --preset dev` | <N passed, M failed> |
| Perf | `build/release/tieqi --perf` | <mean / p95 / max ms, or "not run: reason"> |
| Source language | `python tools/check_source_language.py` | <result> |

## Not done or not verified
- <Anything skipped, and why. Never leave this out; write "nothing" if nothing.>

## Needs author review
- <Art, history, wording, or gameplay choices the author should confirm.>

## Out of scope
- <Bugs or ideas noticed outside the task; not changed.>
