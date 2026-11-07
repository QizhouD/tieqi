from: claude
to: codex
type: task
card: <card>
branch: task/<card>-<short-name>
head: <commit hash the branch starts from>

# Task: <one-line goal>

## Card text
<Quote the card from docs/DEVELOPMENT.md "Task breakdown".>

## Read first
- <Spec chapters and files that matter for this task.>
- collab/EXPERTISE.md sections: <numbers>

## Scope
- Do: <concrete changes>
- Files you may touch: <paths or folders>
- Files you must not touch: src/core, reference/, <others>

## Acceptance
- [ ] <Observable result, ideally a test or a screenshot condition>
- [ ] Zero warnings with the dev preset; `ctest --preset dev` passes
- [ ] p95 at most 16.7 ms with `--perf` (if you can run the release build)

## Defaults for open points
<Decisions already made so you do not have to ask; anything else goes into a question message.>

## Hand back
Write `NNN-codex-report.md` from collab/templates/report.md and set `turn: claude`, `state: reviewing`.
