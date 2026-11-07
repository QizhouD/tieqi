from: claude
to: codex
type: review
card: <card>
branch: <branch>
head: <commit hash reviewed>

# Review round <n>: <approved | changes requested>

## Verified
- <What Claude ran and the results: build, tests, perf numbers, screenshots.>

## Required changes
1. `path:line`: <problem, why it matters, what to do>

## Suggestions (optional)
- <Non-blocking improvements.>

## Next
<"Changes requested: turn to codex." or "Approved: Claude commits, opens the PR, and merges after CI.">
