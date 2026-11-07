# Agent collaboration: Claude Code and Codex

Two coding agents work on this repository: Claude Code (the lead) and Codex (the implementer). They talk only
through Markdown files in this folder, one turn at a time. The author (the human owner of the repository) can read
every message and step in at any point.

Read in this order before your first turn:

1. `AGENTS.md` (Codex) or `CLAUDE.md` (Claude Code): the hard rules of the repository.
2. `docs/DEVELOPMENT.md`, chapter "Working agreement for agents", and the task card you are working on.
3. `docs/AGENT_LOOP.md`: process and stop conditions.
4. `collab/ROLES.md`: who does what, and what each agent may and may not do.
5. `collab/EXPERTISE.md`: the skills, tools, and checks expected in each field of this project.
6. This file: how the conversation works.

## Layout

```
collab/
  README.md          protocol (this file)
  ROLES.md           roles and permissions
  EXPERTISE.md       field-by-field skills, tools, and checks
  templates/         message templates: task, report, review, question
  threads/           live conversations, one folder per card; not committed (see .gitignore)
    <card>-<short-name>/
      STATUS.md      whose turn it is, current branch and state
      001-claude-task.md
      002-codex-report.md
      003-claude-review.md
      ...
```

Threads stay on this machine and out of Git, so switching branches never loses them and PR diffs stay clean. What
matters from a thread ends up in the PR description and `docs/PROGRESS.md`.

## Turn-taking

`STATUS.md` decides whose turn it is. Its format is fixed:

```
turn: codex            # codex | claude | author
state: implementing    # open | implementing | reviewing | changes-requested | blocked | done
card: V7
branch: task/V7-near-map
base: <commit hash the branch started from>
updated: 2026-11-07 10:30 by claude
```

Rules:

1. Act on a thread only when `turn:` names you. Otherwise do nothing in that thread.
2. A turn ends with exactly one new message file and an updated `STATUS.md` that hands the turn over.
3. Messages are append-only. Never edit or delete an earlier message, yours or the other agent's; correct
   yourself in a new message.
4. Name messages `NNN-<from>-<type>.md`: a three-digit sequence number one higher than the last file, the sender
   (`claude`, `codex`, `author`), and the type (`task`, `report`, `review`, `question`, `answer`, `note`).
5. Only one agent touches the working tree at a time: the one whose turn it is.
6. `turn: author` means both agents stop and wait. Use it for the stop conditions in `docs/AGENT_LOOP.md` and for
   anything that is the author's decision (art direction, historical choices, new dependencies, spending).

## Message format

Every message starts with this header and then follows its template in `templates/`:

```
from: codex
to: claude
type: report
card: V7
branch: task/V7-near-map
head: <commit hash or "uncommitted">
```

Write messages in English, like everything else in the repository. Be specific: name files with
`path:line`, quote exact commands and their exact output (trim long logs to the relevant lines), and give numbers
rather than adjectives ("p95 0.6 ms", not "fast enough"). Say plainly what you did not do or could not verify.

## The loop for one card

1. **Task** (Claude): creates the thread, the branch, and `001-claude-task.md` from `templates/task.md`, with the
   card text, scope, files to touch, files not to touch, acceptance checks, and the expertise sections that apply.
   Sets `turn: codex`.
2. **Report** (Codex): implements on the branch, runs the checks it can, writes a report from
   `templates/report.md`, sets `turn: claude`, `state: reviewing`.
3. **Review** (Claude): reads the diff, builds, runs the tests, perf, and screenshots, and writes a review from
   `templates/review.md`. Either `changes-requested` and `turn: codex` (back to step 2), or approved: Claude
   commits, pushes, opens the PR, waits for CI, and merges as in `docs/AGENT_LOOP.md`, then sets `state: done`.
4. **Questions**: either agent may hand the turn over with a `question` instead; the other answers with an `answer`.
   If neither can decide, set `turn: author`.

After three review rounds on the same card without approval, Claude sets `turn: author` and summarises the
disagreement.

## Starting a turn

- Codex is started by the author in the Codex app, or by Claude with
  `codex exec -C <repo> "Read collab/README.md, then take your turn in collab/threads/<thread>."`.
- Claude is started by the author in Claude Code with the same kind of instruction.

On every turn, first read `STATUS.md`, then every message in the thread you have not read, then act.
