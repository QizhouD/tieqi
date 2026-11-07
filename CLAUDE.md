# Tieqi (Afterglow: Imjin 1593)

A pausable real-time tactics game in C++20 and SFML 3. The full spec is in docs/DEVELOPMENT.md,
and the reference implementation of the rules is reference/prototype/byeokjegwan.html (open it in a browser).

## Build and test
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

## Hard rules
- src/core must not include SFML, ImGui, or system headers
- Randomness in the simulation comes only from the Battle's own tq::Rng; the presentation layer (particles, sound) uses its own generators and never touches the simulation's; iterate units in ascending UnitId
- Unit values live only in data/*.json; rule constants only in src/core/rules/Constants.h
- Rule changes must update the "Rule spec" in docs/DEVELOPMENT.md
- Each task is one card in the "Task breakdown"; branch name task/<card>-<short-name>
- When unsure, ask in the PR instead of guessing
- Chinese in the game is always Traditional (zh-Hant); data/strings/zh-Hant.json is the source text
- The repository is written in English: source code (including comments, logs, and tests), documentation, PR descriptions, and commit messages. Player-visible text lives only in data/strings/. Other languages appear only in data/strings/, the player notes docs/demo/README.txt, game text quoted in the docs, tools/parity/anchors.json (snippets matched against the prototype), and reference/

## Layout
src/core rules core · src/data data loading · src/app presentation · tools/headless headless battles
data unit types and scenarios · tests tests · reference prototype and reference material

Process and stop conditions for continuous development are in docs/AGENT_LOOP.md
Claude Code and Codex work together through Markdown messages in collab/; read collab/README.md before taking a turn there
