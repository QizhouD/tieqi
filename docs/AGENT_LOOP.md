# Continuous development: process and stop conditions

## 1. Finish A5 (PR #21, branch task/A5-fx)

1. Check the CI results of #21 with the REST API (use the credentials Git has already stored, touch only QizhouD/tieqi, never print them, and never write them into any file). When compiling with GCC 13 on Linux, tests/app/glyphs.cpp line 89 reports -Wdangling-reference, and the ci preset treats warnings as errors, so the build fails. It is a false positive: fontFor was given a temporary path. The fix is to store the path in a local variable first and pass that.
2. Do not reuse previously recorded numbers; rerun everything yourself: zero warnings with the dev preset; ctest all passing, including the 100-battle baseline labelled slow; with the release preset, `tieqi --perf` runs the scripted replay of seed 1 to the end, with p95 ≤ 16.7 ms. Update the PR description and PROGRESS with the new numbers.
3. Once CI is fully green, squash-merge #21 and delete the branch.

## 2. Then do A6 → L1 → R1 in order, one PR per card

Process: branch from main as task/<card>-<english-short-name>; read "Working agreement for agents", the card itself, and "Presentation spec", "Window and layout", "Localization and fonts", and "Tests and acceptance"; implement and write tests; zero warnings with the dev preset and ctest all passing; update PROGRESS; open the PR; wait for CI. When CI is fully green and every self-check below passes, squash-merge it yourself and move on to the next card.

- A6: the debug tools are compiled only with TQ_DEV_TOOLS=ON and must never end up in release.
- L1: draft the en and ko strings and list every new translation in PROGRESS as "awaiting author review". Keep the wording neutral: the Japanese side is Japanese army or Japanese forces in English and 일본군 in Korean; the Ming army is 명군 and Joseon is 조선. Do not use derogatory terms such as 왜, 倭, or 오랑캐, and do not demean any side. Spell personal and place names as in the existing en.json. The language can be switched in the settings; switching rebuilds the map text; take one full-page screenshot each in ko and en and check for overflowing text.
- R1: build the Windows demo package. The release build links the MSVC runtime statically (CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded, SFML_USE_STATIC_STD_LIBS=ON); check with dumpbin /dependents that it depends only on system DLLs. The package contains the executable, data, assets, the fonts and each of their licenses, THIRD_PARTY_NOTICES (SFML, FreeType, HarfBuzz, SheenBidi, ogg, vorbis, FLAC, miniaudio, nlohmann/json), AI_ASSETS.md, and player notes in Traditional Chinese and English. Write the version into replay files. Name the zip Afterglow-Imjin-1593-demo-<version>-win64.zip, keep it local, and do not publish a GitHub Release.

## 3. Self-checks before merging each card

1. Performance: measured with the same `--perf` run as in 1.2; merge only if it meets the target.
2. Fonts: the missing-glyph check in tests/app/glyphs.cpp must pass.
3. Rules core: whenever src/core changes, the 100-battle baseline and the tick-by-tick comparison must give exactly the same results as before the change.
4. Screenshots: use --script and --freeze-at to capture screens that prove the card's acceptance, and put them in the PR.
5. No Chinese in the source code (src, tests, tools, cmake, CMakeLists.txt, .github/workflows); player-visible text lives only in data/strings/.
6. Before committing, think it through against GCC's warning standards: the Ubuntu CI job uses GCC 13 with warnings as errors, and a clean local Windows build does not mean CI will pass.

## 4. Stop conditions (stop on any one of these)

- The spec is unclear or conflicts with the prototype, in a way that affects gameplay, values, or appearance; small matters follow the defaults already in the docs and are recorded in the PR and PROGRESS.
- Performance misses the target, or the baseline or comparison results change after changing src/core.
- CI for the same card is still red after 3 rounds of fixes.
- A new third-party dependency is needed, or an irreversible action such as a public release is required.
- Always forbidden: pushing directly to main, force pushing, merging a PR whose CI is not fully green, skipping or deleting tests, changing reference/, touching other repositories.

When stopping, write the reason and your suggested course of action in the "Stopped here" section of docs/PROGRESS.md, push to the current branch (do not merge), and then report to the author.

## 5. Stop after R1 is merged

List what the author has to do personally: play three full battles (M2 manual acceptance), listen to the sound effects, review the English and Korean translations, and unzip and play the demo on a Windows PC without a development environment. After each merged PR, report only one line, for example "A6 merged as #22, p95 9.8 ms".

## Repository language

Everything committed to the repository is written in English: code, comments, logs, tests, docs, PR descriptions, and commit messages. The exceptions are listed in the hard rules of AGENTS.md.
