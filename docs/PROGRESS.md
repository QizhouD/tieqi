# Progress

| Card | Status | PR | Summary | Open issues and default decisions |
| --- | --- | --- | --- | --- |
| B0 | Done | #1 | Five CMake targets, dev/release/ci presets, Windows and Ubuntu CI | None |
| Naming | Done | #2 | Game title Afterglow: Imjin 1593 (Traditional Chinese 餘暉：壬辰 1593); Byeokjegwan is only the scenario name | The overview said "official title not decided", which conflicted with the naming; it now gives the official title. The codename, namespace, CMake target names, executable name, directory name, and repository name all stay unchanged |
| C1 | Done | #3 | Vec2, angles, oriented rectangles, separating axis, sideOf, terrainAt and terrainSpeed | When the difference is exactly −π, angDiff folds it to +π as the spec says; the prototype keeps −π. turnToward takes a facing angle; the unit wrapper is left to C2. MapDef holds only width, height, paddies, hills, and the main road for now; the road half-width comes from the scenario's half_width. The sideOf acceptance uses an arquebus front of 66 and depth of 22, written in the test. Zero-length road segments use point distance; degenerate ellipses never hit |
| D1 | Done | #4 | Data definitions, JSON loading and validation, Traditional Chinese string table, duel_flat | Group spawn points go into Placement.pos; rally is stored separately. When flow is omitted, enabled is false, which C6 interprets as staying in phase 1 with the withdrawal exit closed. flow, ai, and result are checked only for types and required fields. Map bounds and spawn expansion are both inclusive. A missing string key falls back to zh-Hant, then to the key name, and is added to warnings. The rally text is written as 48 battlefield seconds. num.liang means "兩". duel_flat contains only Li Rusong |
| C2 | Done | #5 | Simulation skeleton, command queue, movement, rout, and state hash | Phase flow, contact, combat resolution, and victory are left empty; phase stays 0 and the first step does not emit PhaseChanged. Three-eyed guns do not call mingVolley; starting a charge still sets volleyDone to false, but does not set it to true when the distance drops below 62. Split and Join are ignored for now. Rng is PCG32 with initseq fixed at 1. The hash goes field by field in bytes: booleans as 0/1, optionals as a flag plus a float, strings length first, engagements count first. The withdrawal y coordinate comes from the scenario JSON. kTickSeconds is defined in Constants.h and included by Types.h |
| C3 | Done | #6 | Contact, impact, three-eyed guns, breakaway, and charge preview | Breaking only changes the state, the morale cap, and the Broken event: no scattering, no chained morale, no momentum. The engagement preview's multiplier is left to C4. Preview hints are only an enum; the remaining rally seconds and the volley target are not stored. When run is 0 the impact speed factor is 1. Pushing on the frame of arrival can move the distance past 5 m. Formula points are 30 m apart and the preview uses engagement when within 45 m, so the preview cases are placed on the same front at 70 m |
| C4 | Done | #7 | Engagement timer, melee, volleys, morale states, and engagement preview | Breaking still does not scatter and does not call shatter; mergeStep is left to C5. primaryContact now uses the full meleePower, and equal power picks the smaller id. Both attack and defense terms of the melee preview are recorded as ModKind::Melee, attacker first, with the defender's value 1/max(eP, 0.1). nearGeneral uses the general's own aura with a strict less-than distance; with several, the smallest id wins. The actual melee ratio is clamped to [0.3, 3]; the preview is not. share uses the full length of the engagement list and skips targets that have left. Volley ties keep the smaller id. Rallying requires a distance strictly below 180 m and routSec strictly above 80. Melee damage is resolved in ascending target id; the prototype uses the insertion order of the first accumulation |
| C5 | Done | #8 | Scattering on break, remnant merging, split and join, display name suffixes | Total troop strength is kept in a private counter; remnants and detachments do not add to it, and the read accessor is left to C6. Units whose origin is kNoUnit never merge. Units created in a step do not take part in the rest of that step, and the merge after a combat resolution excludes remnants that just scattered. displayName returns the suffix kind and a cycling number from 1 to 10. A single group only sets knock and does not emit Shattered. zh-Hant gained suffix.combined |
| C6 | Done | #9 | Phase flow, spawn groups, general assault, reinforcements, victory and grades | Scenario seconds stay in ScenarioDef and are converted to ticks with lround(seconds/0.8) when checked; the spec says to convert at load time, but changing the loader would change seconds fields D1 already accepted. BattleResult gains end, grade, saved, and rate; the value of PhaseChanged is the phase number, the ScoutWarning position is the commander's, the value of BattleEnded is the grade, and an assault that never launched uses kAssaultFailed. After the assault bonus, if that tick also resolves combat, the existing morale cap of 100 truncates again: Kobayakawa's 105 becomes 100, and the wings' 92 is unchanged. Both the danger distance and leaving with the commander are strict less-than; vanguard collapse counts only the ids marked at the start; when flow.enabled is false the first step still enters phase 1. Japanese units are still deploying after spawning; ending the entry is left to C7. The private total strength counter includes later reinforcements, and there is still no public accessor. After the battle ends, step returns immediately |
| C7 | Done | #10 | Japanese AI: target choice, arquebusiers, commander, vanguard general, remnants, and retargeting | The option to disable AI already existed in C2; this card wires it up and it is on by default. Holding time uses tick strictly greater than lround(seconds/0.8), so tick 450 does not count as past 360 seconds. Hold without a facing clears face. Equal scores keep the smaller id. approach returns immediately at distance 0. Follow points are not clipped to the map edge. Entering units with an engagement do not leave the entry state this round. Existing movement and flow tests disable AI. The --no-ai command-line flag is left to H1 |
| H1 | Done | #11 | Headless battles, two policies, CSV summaries, and replay read/write | Replay commands use the scenario id; units without one are written as # plus the UnitId. The build id is the short git hash at configure time. --seed with --runs means consecutive battles starting from that seed. --data-root locates the data directory and by default searches upward for data/units.json. Quantiles use ceil(p*n)-1. The Right Retainers' attack is issued the first time they can fight after tick 300 and executes on the next tick. The withdrawal order once phase 3 reaches 300 ticks executes at phase3At+300. The smoke test returns non-zero on timeout, non-finite numbers, no events, more than 10 seconds, or scripted runs without an impact. The 3000-tick hash test disables AI; event consistency for scripted runs is tested separately |
| P1 | Done | #12 | Melee order and random number use aligned with the prototype; the whole-battle baseline is now the prototype's 100 battles | Floating-point differences are left alone: float32 separating axis pushes overlapping remnants apart by over ten metres; the break timer is 1 tick earlier than the prototype; holding and phase clocks differ by 1–3 ticks. passive Yang Yuan median 2000, prototype 2003. Slow tests are not in the default ctest |
| A0 | Done | #15 | SFML 3 shell: fonts chosen by role, four synthesized sounds, letterboxed scaling, scripted screenshots | The acceptance screen draws all three languages at once; the current language only changes the window title. Switching the whole page is left to L1. F6 plays thud, crack, drum, and rout once each in that order. Settings are written to settings.txt next to the executable. Rules core unchanged |
| A1 | Done | #16 | Silk-scroll base map centred in the battlefield rectangle; the default window is at most 90% of the desktop | Zoom and pan are left to A1b. The window has no maximum size. The map title picks its font through TextStyle::Title. Rules core unchanged |
| A2 | Done | #17 | Formations, counters, facing wedges, strength and morale bars, momentum, timer arcs, flipping, and selection frames, with positions interpolated per tick | Dust and floating text are left to A5. When A4 builds the top bar it changes the battlefield rectangle to the canvas measured on the prototype at 1600×1000: (17, 55), 1216×932. Rules core unchanged |
| A3 | Done | #18 | Click and box selection, orders, hotkeys, pending confirmation; preview arrows and multipliers; flip brokenSec interpolated by frame alpha | The battle starts paused and Space enters phase 1. The confirm-attack button and battlefield canvas change in A4. Rules core unchanged |
| A4 | Done | #19 | Top bar, right-hand cards, preview and attack confirmation, intro and result overlays, banners, and battle log; battlefield canvas (17, 55), 1216×932 | Battle::enlisted is read-only and does not change step. The 100-battle baseline was not rerun. Panel text is drawn in device pixels; a shaper per font, size, and weight stays resident, and wrapping is cached by string, size, and width. Script timing starts at the first frame. Result card height follows its content. release, seed 1, scripted, 2× speed to the result: mean 0.5 ms, p95 0.8 ms, max 81.3 ms. Dust, floating text, hit-stop, and event sounds are left to A5 |
| A5 | Done | #21 | Dust, smoke, muzzle flash, debris, rings, floating text, click markers, screen shake, hit-stop, and event sounds; `--freeze-at` | Rules core unchanged. Rerun on 2026-10-29: full rebuild of the dev preset with zero warnings, ctest all passing, including the 100-battle baseline with the slow label. Toggling reduced_motion on the same replay changes only presentation timing, not the outcome. The presentation RNG is mt19937 with seed 1594 and never touches tq::Rng; effects need not match the prototype pixel for pixel. Debris size follows the prototype in world units 3–7 (the spec says pixels), and the fan uses the facing of the unit after scattering. Floating text rises 18 m per second. Smoke uses the prototype's two parameter sets, 5 puffs for three-eyed guns and 7 for arquebuses, both within the spec envelope. Click markers are not in the event table and were added in the render layer. The heavy particle cap is 1600. Glyph test: when a key is in a language's own table, that language's role font is used; keys that fall back to zh-Hant use the Traditional Chinese font. Noto Serif KR lacks 12 Han characters (U+8D95, U+8173, U+586B, U+57AE, U+4F75, U+5920, U+503C, U+5C1A, U+8DD1, U+5B83, U+78B0, U+64CB); while ko does not own those keys the UI still uses the Korean font. The missing glyphs were left to L1, without changing fonts or the Traditional Chinese wording. release, seed 1, scripted, 2× speed to the result (three runs): mean 0.4 ms, p95 0.5 ms, max 46.5–108.4 ms. GCC 13 on CI reported -Wdangling-reference in tests/app/glyphs.cpp (fontFor took a temporary path, a false positive); the path is now stored in a local first. Seed 1 compared side by side with the prototype at ticks 59, 255, 310, and 1800 |
| A6 | Done | #22 | ImGui debug panel: Sim (tick, phase, state hash, single step, simulation and render timings, save and load replays), Units (list, inspector, rout), Spawn, AI intents and target lines, constants table; F1 toggles, F2 switches tabs. Compiled only with TQ_DEV_TOOLS=ON | Spawning and routing go through two dev commands on submit(), Spawn and Rout, recorded in replays as spawn and rout, with the schema and spec updated; BattleAccess is not used. Battle gains a read-only vanguardAggressive(). Spawned units count toward total strength, draw a reload from tq::Rng once, Hold in place, use the unit type name as labelKey, and emit no event. The app and the headless tool share resolveCommand and recordCommand from tq_data. The constants table is generated from Constants.h at configure time. The panel is in English, shows scenario ids, and does not use the string tables. The ci preset gains the tieqi_without_imgui test; a local check finds no ImGui strings in the release build and 35 in the dev build. src/core changed: the CSVs for seeds 1–100 with both policies, and the per-tick state and event output for seeds 1–5 with both policies, are byte-for-byte identical before and after; the slow baseline passes. Full rebuild of the dev preset with zero warnings. release, seed 1, scripted, 2× speed to the result (three runs): mean 0.4 ms, p95 0.5 ms, max 82.0–103.0 ms |
| L1 | Done | #23 | en and ko string tables completed (285 keys each, all awaiting author review, see the next section); a "設定" button in the top bar and a settings card: language switching (rebuilds the base map place names and the window title) and reduced_motion, saved to settings.txt; word wrapping; layout adapts to text width | Line breaking rules are now: a break is allowed after a space and before or after Han characters and CJK punctuation; Korean and English break by word; closing punctuation never starts a line. Chinese layout changes on a few lines as a result (for example "×4.2" is no longer split). The battle log's key lines used to be measured in regular weight but drawn in bold, which overflowed the panel; they are now measured in bold. The top bar's phase and clock columns, the colour bars on both army cards, the strength and morale bars on the command card, and the sound and settings buttons make room according to text width; Chinese positions are unchanged. Result statistics labels shrink their font size when too wide. en separates the parts of the time-of-day names with `|` (Dragon hour, early); ko uses 진시 초. num.* in en and ko are Arabic numerals. en avoids full-width quotation marks (they are very wide in Noto Serif TC). end.follow in en and ko starts with a space because it follows end.withdraw directly. Battle log entries already written keep their original language after switching. `--script` runs do not write settings.txt. A fallback test in tests/data used to rely on en.json being incomplete; it now uses the tests/data/strings/partial.json fixture with the same assertions. Rules core unchanged. Full rebuild of the dev preset with zero warnings, ctest all passing, slow baseline passing, glyph test passing. release, seed 1, scripted, 2× speed to the result (three runs): mean 0.3–0.4 ms, p95 0.5 ms, max 103.8–115.0 ms |
| R1 | Done | #24 | Windows demo package: release links the MSVC runtime statically; the `package_demo` target produces Afterglow-Imjin-1593-demo-0.1.0-win64.zip (local, not published); version 0.1.0 written into replays; Traditional Chinese and English README.txt; THIRD_PARTY_NOTICES.txt | The version lives in the top-level `project(tieqi VERSION 0.1.0)`; replays gain an optional `version` field that is empty when older replays are read, with the schema and spec updated. The release preset adds `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` and `SFML_USE_STATIC_STD_LIBS=ON` (no effect with GCC). `dumpbin /dependents tieqi.exe`: OPENGL32, GDI32, WINMM, KERNEL32, USER32, ADVAPI32, all system DLLs. The zip is about 21 MB, mostly the three fonts. Versions in the third-party notices: SFML 3.1.0, FreeType VER-2-14-3, HarfBuzz 14.1.0, SheenBidi v3.0.0, libogg v1.3.6, libvorbis v1.3.7, libFLAC 1.5.0, miniaudio 0.11.25, nlohmann/json 3.11.3. The executable is still named tieqi.exe (agreed at naming time). Extracting the zip on this machine to a path with spaces and Chinese characters and starting it from another working directory shows the intro screen correctly. AI_ASSETS.md has no entries yet. Rules core unchanged. Zero warnings with the dev preset, ctest all passing, slow baseline passing. release (static runtime), seed 1, scripted, 2× speed to the result: mean 0.4 ms, p95 0.5 ms, max 98.1–122.2 ms. Not yet tried on a PC without a development environment; left to the author |
| English docs | Done | #25 | Every document in the repository translated to English; the language rule now says the repository is English except player-facing text and the reference material | See the language rule in AGENTS.md. Quoted game text and the zh-Hant column of the review table stay in Chinese; docs/demo/README.txt stays bilingual because it ships to players. The architecture diagram in DESIGN.md, a PNG with Chinese labels, is now an English Mermaid diagram. Old commit messages are not rewritten. dev build and ctest pass; source language check passes |

The source language check (`tools/check_source_language.py`) scans src, tests, tools, cmake, CMakeLists.txt, .github/workflows, and .gitignore. tools/parity/anchors.json holds Chinese snippets used to match the prototype and is excluded; node_modules and tools/parity/work are skipped too.

## Translations awaiting author review (L1)

The table below lists every translation written in L1, all **awaiting author review**: 259 en entries, 259 ko entries, plus 7 new zh-Hant terms for the settings card (marked "(new)"). Proper nouns already in the original table (place names, Li Rusong, Yang Yuan, Tachibana Muneshige, Kobayakawa Takakage, unit type names, counter glyphs) were kept and are not listed. Terminology: the Japanese side is Japanese army / Japanese forces in English (adjective Japanese) and 일본군 in Korean; the Ming army is 명군; in Korean a counter is 기물, to avoid confusion with 말 (horse); derogatory terms such as 왜, 倭, and 오랑캐 are not used, and `tests/app/l10n.cpp` checks that they never appear. After review, edit `data/strings/` only; no code changes are needed.

| Key | zh-Hant | en | ko |
| --- | --- | --- | --- |
| `scn.byeokjegwan` | 碧蹄館之戰 | Battle of Byeokjegwan | 벽제관 전투 |
| `scn.duel_flat` | 平地對決 | Duel on Open Ground | 평지 대결 |
| `fx.pike` | 槍陣 | Pike wall | 창진 |
| `fx.break` | 破陣 | Breakthrough | 진 돌파 |
| `fx.charge` | 衝入 | Charge home | 돌입 |
| `fx.blocked` | 受阻 | Stopped | 저지 |
| `fx.volley` | 三眼銃 | Three-eyed guns | 삼안총 |
| `fx.rally` | 集結 | Rallied | 재집결 |
| `fx.momentum` | 氣勢 | Momentum | 기세 |
| `fx.merge` | 收攏 | Regrouped | 수습 |
| `fx.split` | 分兵 | Split | 분병 |
| `fx.disperse` | 潰散 | Dispersed | 궤산 |
| `fx.withdraw` | 撤出 | Withdrew | 철수 |
| `fx.general_down` | 陣亡 | Fallen | 전사 |
| `fx.assault` | 總攻 | General assault | 총공격 |
| `unit.jiading_left` | 家丁左營 | Left Retainers | 가정 좌영 |
| `unit.jiading_right` | 家丁右營 | Right Retainers | 가정 우영 |
| `unit.liaodong_left` | 遼騎左營 | Liaodong Left | 요동 기병 좌영 |
| `unit.liaodong_center` | 遼騎中營 | Liaodong Center | 요동 기병 중영 |
| `unit.liaodong_right` | 遼騎右營 | Liaodong Right | 요동 기병 우영 |
| `unit.van_yari` | 立花長槍隊 | Tachibana Yari | 다치바나 장창대 |
| `unit.van_teppo_left` | 立花鐵砲左隊 | Tachibana Teppō (Left) | 다치바나 조총 좌대 |
| `unit.van_teppo_right` | 立花鐵砲右隊 | Tachibana Teppō (Right) | 다치바나 조총 우대 |
| `unit.van_samurai` | 立花武士隊 | Tachibana Samurai | 다치바나 사무라이대 |
| `unit.main_teppo_left` | 本隊鐵砲左隊 | Main Army Teppō (Left) | 본대 조총 좌대 |
| `unit.main_teppo_right` | 本隊鐵砲右隊 | Main Army Teppō (Right) | 본대 조총 우대 |
| `unit.main_yari_left` | 本隊長槍左隊 | Main Army Yari (Left) | 본대 장창 좌대 |
| `unit.main_yari_right` | 本隊長槍右隊 | Main Army Yari (Right) | 본대 장창 우대 |
| `unit.main_samurai` | 本隊武士隊 | Main Army Samurai | 본대 사무라이대 |
| `unit.wing_samurai_left` | 左翼武士隊 | Left Wing Samurai | 좌익 사무라이대 |
| `unit.wing_samurai_right` | 右翼武士隊 | Right Wing Samurai | 우익 사무라이대 |
| `terrain.paddy` | 水田 | Paddy | 논 |
| `phase.0` | 未開戰 | Not begun | 개전 전 |
| `phase.1` | 一 · 遭遇 | I · Encounter | 1 · 조우 |
| `phase.2` | 二 · 主力壓上 | II · Main army | 2 · 주력 진격 |
| `phase.3` | 三 · 撤退 | III · Withdrawal | 3 · 철수 |
| `obj.briefing` | 讀完戰前要訣後點擊“開戰”。 | Read the briefing, then click "Begin". | 전투 요령을 읽은 뒤 “개전”을 누르십시오. |
| `obj.phase1` | 擊破立花宗茂的先鋒。衝垮三隊，或支撐到日軍本隊趕到。 | Defeat Tachibana Muneshige's vanguard. Break three of its units, or hold until the Japanese main army arrives. | 다치바나 무네시게의 선봉을 격파하십시오. 세 부대를 무너뜨리거나, 일본군 본대가 도착할 때까지 버티십시오. |
| `obj.warning` | 日軍本隊即將趕到。收攏追擊的騎兵，讓衝過陣的部隊完成重整。 | The Japanese main army is about to arrive. Recall your pursuing cavalry and let units that charged through finish re-forming. | 일본군 본대가 곧 도착합니다. 추격 중인 기병을 거두고, 적진을 뚫고 나간 부대가 정비를 마치게 하십시오. |
| `obj.phase2` | 日軍本隊已越過礪石峴。可隨時經北口撤往坡州：李如松撤出即結束戰鬥，北口附近未接戰的部隊會隨他一同撤出。 | The Japanese main army has crossed Yeoseok Pass. You may withdraw toward Paju through the north exit at any time: the battle ends when Li Rusong leaves, and units near the exit that are not engaged leave with him. | 일본군 본대가 여석현을 넘었습니다. 언제든 북쪽 출구로 파주 방면 철수가 가능합니다. 이여송이 빠져나가면 전투가 끝나며, 출구 근처에서 교전 중이 아닌 부대는 함께 철수합니다. |
| `obj.phase3` | 日軍兩翼武士正繞山包抄北口。盡快脫離接戰，護送李如松撤出。 | Samurai on both Japanese wings are circling the hills toward the north exit. Break off quickly and escort Li Rusong out. | 일본군 양익의 사무라이가 산을 돌아 북쪽 출구로 다가옵니다. 서둘러 교전에서 벗어나 이여송을 호위해 철수하십시오. |
| `banner.phase1` | 一 · 遭遇 | I · Encounter | 1 · 조우 |
| `banner.phase1_sub` | 立花宗茂的先鋒列陣於前 | Tachibana Muneshige's vanguard is drawn up ahead | 다치바나 무네시게의 선봉이 앞에 진을 쳤다 |
| `banner.scout` | 斥候來報 | Scouts report | 척후 보고 |
| `banner.scout_sub` | 小早川隆景本隊即將越過礪石峴 | Kobayakawa Takakage's main army will soon cross Yeoseok Pass | 고바야카와 다카카게의 본대가 곧 여석현을 넘는다 |
| `banner.phase2` | 二 · 主力壓上 | II · Main army | 2 · 주력 진격 |
| `banner.phase2_sub` | 小早川隆景本隊越過礪石峴 | Kobayakawa Takakage's main army crosses Yeoseok Pass | 고바야카와 다카카게의 본대가 여석현을 넘었다 |
| `banner.phase3` | 三 · 撤退 | III · Withdrawal | 3 · 철수 |
| `banner.phase3_sub` | 日軍兩翼包抄北口，護送李如松撤往坡州 | The Japanese wings close on the north exit — escort Li Rusong to Paju | 일본군 양익이 북쪽 출구를 에워싼다. 이여송을 호위해 파주로 철수하라 |
| `banner.reinforcement` | 援軍 · 楊元 | Reinforcements · Yang Yuan | 원군 · 양원 |
| `banner.reinforcement_sub` | 楊元率押後的騎兵自北馳援 | Yang Yuan rides in from the north with the rearguard cavalry | 양원이 후위 기병을 이끌고 북쪽에서 달려온다 |
| `log.impact` | {a} 自{side}衝擊 {b}，衝擊 ×{r} | {a} charges {b} from the {side}, impact ×{r} | {a}, {side}에서 {b} 충격 ×{r} |
| `log.impact_pike` | {a} 自{side}衝擊 {b}，衝擊 ×{r}，撞上槍陣 | {a} charges {b} from the {side}, impact ×{r}, into a pike wall | {a}, {side}에서 {b} 충격 ×{r}, 창진에 부딪힘 |
| `log.breakaway` | {a} 脫離接戰 | {a} breaks off | {a} 교전 이탈 |
| `log.volley` | {a} 向 {b} 齊射 | {a} fires a volley at {b} | {a}, {b}에 일제 사격 |
| `log.broken` | {a} 崩潰 | {a} breaks | {a} 붕괴 |
| `log.broken_scatter` | {a} 崩潰，散作{parts}股 | {a} breaks and scatters into {parts} bands | {a} 붕괴, {parts}갈래로 흩어짐 |
| `log.broken_impact` | {a} 陣腳被衝破，崩潰 | {a} is broken by the charge | {a} 진형이 뚫려 붕괴 |
| `log.broken_impact_scatter` | {a} 陣腳被衝破，崩潰，散作{parts}股 | {a} is broken by the charge and scatters into {parts} bands | {a} 진형이 뚫려 붕괴, {parts}갈래로 흩어짐 |
| `log.rallied` | {a} 重新集結 | {a} rallies | {a} 재집결 |
| `log.merged_frag` | {name} 收攏殘部，兵 {n} | {name} gathers its remnants, {n} men | {name} 잔병 수습, 병력 {n} |
| `log.merged_join` | {name} 合兵，兵 {n} | {name} rejoins, {n} men | {name} 합류, 병력 {n} |
| `log.split` | {name} 分為兩隊，各 {n} 人 | {name} splits into two companies of {n} | {name} 두 부대로 나뉨, 각 {n}명 |
| `log.dispersed` | {a} 潰散 | {a} disperses | {a} 궤산 |
| `log.dispersed_remnant` | {name} 殘部盡散 | The last of {name} disperses | {name} 잔병 모두 흩어짐 |
| `log.fled` | {a} 逃離戰場 | {a} flees the field | {a} 전장 이탈 |
| `log.fled_remnant` | {name} 殘部 逃離戰場 | The remnants of {name} flee the field | {name} 잔병 전장 이탈 |
| `log.withdrawn` | {a} 撤出戰場，兵 {n} | {a} withdraws from the field, {n} men | {a} 전장에서 철수, 병력 {n} |
| `log.general_down` | {a} 陣亡 | {a} has fallen | {a} 전사 |
| `log.phase1` | 明軍前鋒在碧蹄館以南與日軍先鋒遭遇 | The Ming vanguard meets the Japanese vanguard south of Byeokjegwan | 명군 선봉이 벽제관 남쪽에서 일본군 선봉과 조우 |
| `log.scout` | 斥候來報：日軍本隊將至，宜趁早整隊 | Scouts report: the Japanese main army is coming — re-form while you can | 척후 보고: 일본군 본대가 다가온다. 서둘러 대오를 정비하라 |
| `log.phase2` | 日軍本隊自礪石峴壓上，北口撤退路線已開 | The Japanese main army advances from Yeoseok Pass; the north exit is open for withdrawal | 일본군 본대가 여석현에서 밀려오고, 북쪽 출구 철수로가 열렸다 |
| `log.assault` | 小早川隆景下令總攻，日軍士氣大振 | Kobayakawa Takakage orders a general assault; Japanese morale surges | 고바야카와 다카카게가 총공격을 명령, 일본군 사기 고조 |
| `log.assault_failed` | 日軍本隊失去主將，總攻未能發動 | The Japanese main army has lost its general; the general assault never comes | 일본군 본대가 주장을 잃어 총공격이 무산됨 |
| `log.reinforcement` | 楊元率押後的一千騎趕到，可聽調遣 | Yang Yuan arrives with a thousand rearguard horse, ready for orders | 양원이 후위 기병 1,000기를 이끌고 도착, 지휘 가능 |
| `float.pike` | 槍陣 | Pike wall | 창진 |
| `float.break` | 破陣 | Breakthrough | 진 돌파 |
| `float.charge_in` | 衝入 | Charge home | 돌입 |
| `float.blocked` | 受阻 | Stopped | 저지 |
| `float.volley` | 三眼銃 | Three-eyed guns | 삼안총 |
| `float.rally` | 集結 | Rallied | 재집결 |
| `float.merge` | 收攏 | Regrouped | 수습 |
| `float.dead` | 陣亡 | Fallen | 전사 |
| `float.disperse` | 潰散 | Dispersed | 궤산 |
| `float.withdraw` | 撤出 | Withdrew | 철수 |
| `float.assault` | 總攻 | General assault | 총공격 |
| `float.momentum` | 氣勢 | Momentum | 기세 |
| `float.split` | 分兵 | Split | 분병 |
| `state.steady` | 穩定 | Steady | 안정 |
| `state.wavering` | 動搖 | Wavering | 동요 |
| `state.broken` | 崩潰 | Broken | 붕괴 |
| `state.routing` | 潰逃 | Routing | 궤주 |
| `side.front` | 正面 | front | 정면 |
| `side.flank` | 側翼 | flank | 측면 |
| `side.rear` | 背後 | rear | 후방 |
| `side.ming` | 明 | Ming | 명군 |
| `side.jp` | 日 | Japanese | 일본군 |
| `mod.impact` | {name} 衝擊 | {name} impact | {name} 충격 |
| `mod.melee` | {name} 近戰 | {name} melee | {name} 근접전 |
| `mod.defense` | {name} 抗衝 | {name} vs. charge | {name} 충격 저항 |
| `mod.speed` | 衝速不足 | Not enough speed | 돌격 속도 부족 |
| `mod.flank` | 側擊 | Flank attack | 측면 공격 |
| `mod.rear` | 背擊 | Rear attack | 후방 공격 |
| `mod.wavering` | 敵軍動搖 | Enemy wavering | 적 동요 |
| `mod.routing` | 敵軍潰亂 | Enemy in rout | 적 궤주 |
| `mod.reloading` | 敵鐵砲裝填中 | Enemy teppō reloading | 적 조총 장전 중 |
| `mod.unformed` | 敵鐵砲未列陣 | Enemy teppō not deployed | 적 조총 미배치 |
| `mod.paddy` | 水田泥濘 | Muddy paddy | 논 진창 |
| `mod.uphill` | 仰攻 | Attacking uphill | 오르막 공격 |
| `mod.pike` | 正面槍陣 | Pikes to the front | 정면 창진 |
| `mod.pike_wall` | 正面硬撼槍陣 | Head-on into pikes | 창진 정면 돌격 |
| `mod.momentum` | 氣勢 | Momentum | 기세 |
| `mod.own_wavering` | 我軍動搖 | Our troops wavering | 아군 동요 |
| `mod.strength` | 兵力折損 | Losses | 병력 손실 |
| `mod.cavalry_grind` | 騎兵纏鬥乏力 | Cavalry worn down in melee | 기병 난전 약화 |
| `mod.pursuit` | 追擊潰兵 | Pursuing routers | 궤주병 추격 |
| `verdict.pike` | 正面衝槍陣，必受重創 | Head-on into pikes — heavy losses certain | 창진 정면 돌격, 큰 피해 필연 |
| `verdict.pursuit` | 追擊潰兵，可大量殺傷 | Pursuing routers — heavy casualties | 궤주병 추격, 큰 살상 가능 |
| `verdict.breaks` | 衝擊足以破陣 | Enough to break them | 충격으로 적진 돌파 가능 |
| `verdict.favored_charge` | 佔優，可衝入敵陣 | Favorable — will drive in | 우세, 적진 돌입 가능 |
| `verdict.even_charge` | 勢均力敵，將陷入纏鬥 | Even — expect a grinding melee | 호각, 난전에 빠짐 |
| `verdict.unfavored_charge` | 不利，衝擊會被擋住 | Unfavorable — will be stopped | 불리, 충격이 저지됨 |
| `verdict.favored_melee` | 近戰佔優 | Favorable in melee | 근접전 우세 |
| `verdict.even_melee` | 勢均力敵 | Evenly matched | 호각 |
| `verdict.unfavored_melee` | 近戰不利 | Unfavorable in melee | 근접전 불리 |
| `note.recovering` | 重整中，{sec} 秒內無法衝鋒，只能接戰 | Re-forming: no charge for {sec} s, melee only | 정비 중: {sec}초 동안 돌격 불가, 교전만 가능 |
| `note.too_close` | 距離太近，衝不起速度 | Too close to build up speed | 거리가 너무 가까워 속도를 낼 수 없음 |
| `note.volley` | {name} 已裝填，衝鋒途中可能遭齊射 | {name} is loaded; the charge may draw a volley | {name} 장전 완료, 돌격 중 일제 사격을 받을 수 있음 |
| `note.wavering` | 本部士氣動搖，戰力打折 | Our morale is wavering; fighting strength reduced | 아군 사기 동요, 전투력 저하 |
| `tag.charging` | 衝鋒中 | Charging | 돌격 중 |
| `tag.breaking` | 脫離接戰 | Breaking off | 교전 이탈 |
| `tag.contact` | 接戰 | Engaged | 교전 |
| `tag.flanked` | 側背受擊 | Hit in flank or rear | 측후방 피격 |
| `tag.recover` | 重整 {sec} 秒 | Re-forming {sec} s | 정비 {sec}초 |
| `tag.reload` | 裝填 {sec} 秒 | Reloading {sec} s | 장전 {sec}초 |
| `tag.loaded` | 已裝填 | Loaded | 장전 완료 |
| `tag.moving_no_fire` | 行進中，不能齊射 | Moving, cannot fire | 행군 중 사격 불가 |
| `tag.momentum` | 氣勢 ×{n} | Momentum ×{n} | 기세 ×{n} |
| `tag.fragment` | 殘部：與同營殘部會合即合併 | Remnant: merges on meeting its camp's remnants | 잔병: 같은 영의 잔병과 만나면 합쳐짐 |
| `tag.rejoin` | 正在合兵 | Rejoining | 합류 중 |
| `tag.split` | 分隊：同營分隊一起選中按“合兵”收回 | Detachment: select with its sibling and press "Join" | 분대: 같은 영의 분대와 함께 선택해 “합병” |
| `tag.aura` | {name} 鼓舞 | Inspired by {name} | {name} 고무 |
| `tag.paddy` | 身陷水田 | In a paddy | 논에 빠짐 |
| `tag.hill` | 山地 | Hills | 산지 |
| `tag.road` | 大路 | Road | 큰길 |
| `tag.target` | 目標：{name} | Target: {name} | 목표: {name} |
| `tag.moving` | 行進中 | On the move | 행군 중 |
| `tag.hold` | 堅守 | Holding | 고수 |
| `tag.elite` | 精銳 | Elite | 정예 |
| `ui.subtitle` | 萬曆二十一年正月二十七日 · 京畿道 | Wanli 21, 1st month, 27th day · Gyeonggi | 만력 21년 정월 27일 · 경기도 |
| `ui.phase` | 階段 | Phase | 단계 |
| `ui.speed` | 速度 | Speed | 속도 |
| `ui.sound` | 聲 | Sound | 소리 |
| `ui.restart` | 重開 | Restart | 재시작 |
| `ui.start` | 開戰 | Begin | 개전 |
| `ui.pause` | 暫停 | Pause | 일시정지 |
| `ui.resume` | 繼續 | Resume | 계속 |
| `ui.paused` | 暫停中 | Paused | 일시정지 중 |
| `ui.canvas` | 碧蹄館戰場 | Byeokjegwan battlefield | 벽제관 전장 |
| `ui.battle` | 碧蹄館 | Byeokjegwan | 벽제관 |
| `ui.key.space` | 空格 | Space | 스페이스 |
| `ui.intro_eyebrow` | 萬曆二十一年正月 · 明軍東征 | Wanli 21 (1593), 1st month · The Ming army's eastern campaign | 만력 21년 정월 · 명군의 동정 |
| `ui.intro_body` | 平壤收復後，提督李如松率以遼東騎兵為主的前鋒南下漢城，在碧蹄館一帶與日軍遭遇。立花宗茂的先鋒已在前方列陣，小早川隆景的本隊隨後就到。李如松只帶了一半親兵先行，另一半由楊元押後。 | After retaking Pyongyang, Commander Li Rusong led a vanguard of mostly Liaodong cavalry south toward Hanseong and met the Japanese army near Byeokjegwan. Tachibana Muneshige's vanguard is already drawn up ahead, and Kobayakawa Takakage's main army is close behind. Li Rusong rode ahead with only half his guard; Yang Yuan follows with the other half. | 평양을 되찾은 뒤, 제독 이여송은 요동 기병을 주력으로 한 선봉을 이끌고 한성으로 남하하다가 벽제관 일대에서 일본군과 조우했다. 다치바나 무네시게의 선봉이 이미 앞에 진을 쳤고, 고바야카와 다카카게의 본대가 뒤따르고 있다. 이여송은 친병의 절반만 데리고 먼저 나섰고, 나머지 절반은 양원이 후위로 이끌고 온다. |
| `ui.tips_title` | 騎兵要訣 | Cavalry essentials | 기병 요령 |
| `ui.tip.1` | 衝鋒要有助跑，速度越快衝擊越大，接敵前先放一輪三眼銃。 | A charge needs a run-up: the faster you hit, the harder the impact. Fire the three-eyed guns before contact. | 돌격에는 도움닫기가 필요하다. 속도가 빠를수록 충격이 크며, 접적 전에 삼안총을 한 차례 쏜다. |
| `ui.tip.2` | 側擊 ×1.8，背擊 ×2.4，打已動搖的敵人再加成。衝擊夠大即破陣。 | Flank ×1.8, rear ×2.4, and more against a wavering enemy. A big enough impact breaks the line. | 측면 ×1.8, 후방 ×2.4, 동요한 적에게는 더 커진다. 충격이 충분하면 적진을 돌파한다. |
| `ui.tip.3` | 正面衝長槍陣必吃大虧。鐵砲要停下列陣才能齊射，趁它裝填或行進時衝最好。 | Charging pikes head-on is costly. Teppō must halt and deploy to fire; hit them while they reload or march. | 장창 진에 정면으로 돌격하면 크게 당한다. 조총은 멈춰서 진을 쳐야 일제 사격을 하므로, 장전 중이거나 행군 중일 때 치는 것이 가장 좋다. |
| `ui.tip.4` | 衝完要重整約 48 秒，陷入纏鬥越久越吃虧，宜脫離再衝。破陣後騎兵會繼續追擊，記得收攏。 | Re-forming after a charge takes about 48 s, and a long melee wears you down: break off and charge again. Cavalry pursue after a breakthrough, so recall them. | 돌격 후에는 약 48초간 정비해야 하며, 난전이 길어질수록 불리하니 이탈했다가 다시 돌격하라. 적진을 돌파한 기병은 계속 추격하니 잊지 말고 거두어라. |
| `ui.tip.5` | 部隊崩潰會散作兩三股各自逃散，可以分頭追殺；集結後的同營殘部碰到一起會自動合併。 | A broken unit scatters into two or three bands that flee apart, so you can hunt them separately; rallied remnants of the same camp merge when they meet. | 부대가 붕괴하면 두세 갈래로 흩어져 달아나므로 나누어 추격할 수 있다. 재집결한 같은 영의 잔병은 서로 만나면 자동으로 합쳐진다. |
| `ui.tip.6` | 分兵：一隊拆成兩隊各領一半，一隊牽制、一隊包抄。分隊衝擊力和士氣上限都更低，用合兵收回。 | Split: divide a unit into two halves, one to pin and one to flank. Detachments hit softer and cap lower in morale; Join them back together. | 분병: 한 부대를 절반씩 둘로 나눠 하나는 견제하고 하나는 우회한다. 분대는 충격력과 사기 상한이 낮으며, 합병으로 다시 거둔다. |
| `ui.tip.7` | 李如松陣亡即敗。本隊到來後，從北口撤往坡州；危急時楊元的援騎會趕到。 | If Li Rusong falls, the battle is lost. Once the main army arrives, withdraw to Paju through the north exit; in a crisis Yang Yuan's horsemen will come. | 이여송이 전사하면 패배한다. 본대가 도착하면 북쪽 출구로 파주를 향해 철수하라. 위급할 때는 양원의 원군 기병이 달려온다. |
| `ui.help_title` | 操作 | Controls | 조작 |
| `ui.help.pick` | 點選棋子 | Click a piece | 기물 클릭 |
| `ui.help.pick_body` | 選中我軍；Shift 多選，拖動框選，A 全選 | Select your units; Shift to add, drag to box-select, A for all | 아군 선택. Shift로 추가, 드래그로 범위 선택, A로 전체 선택 |
| `ui.help.ground` | 點地面 | Click ground | 지면 클릭 |
| `ui.help.ground_body` | 移動；點敵軍即進攻，距離夠會自動衝鋒 | Move; click an enemy to attack (charges when close enough) | 이동. 적을 클릭하면 공격하며, 거리가 되면 자동으로 돌격 |
| `ui.help.hover` | 懸停敵軍 | Hover an enemy | 적 위에 올리기 |
| `ui.help.hover_body` | 右側顯示衝擊預估和全部修正項 | The right panel shows the charge estimate and every modifier | 오른쪽에 충격 예측과 모든 보정 항목 표시 |
| `ui.help.keys` | 快捷鍵 | Keys | 단축키 |
| `ui.help.keys_body` | 空格 暫停，1 2 3 變速，H 堅守，R 撤退，F 分兵，G 合兵，Esc 取消 | Space pause, 1 2 3 speed, H hold, R retreat, F split, G join, Esc cancel | 스페이스 일시정지, 1 2 3 속도, H 고수, R 철수, F 분병, G 합병, Esc 취소 |
| `ui.help.touch` | 手機 | Touch | 터치 |
| `ui.help.touch_body` | 點選後點地面移動；點敵軍先看預估，再點一次確認 | Tap a unit, then tap ground to move; tap an enemy to see the estimate, then tap again to confirm | 선택 후 지면을 눌러 이동. 적을 누르면 먼저 예측을 보여 주고, 한 번 더 누르면 확정 |
| `ui.intro_fine` | 原型演示：數值為暫定，地形與兵力尚待對照史料核實。暫停時也可以下令。 | Prototype: values are provisional; terrain and troop numbers are still to be checked against sources. You can give orders while paused. | 프로토타입: 수치는 잠정적이며, 지형과 병력은 사료와 대조해 확인해야 한다. 일시정지 중에도 명령할 수 있다. |
| `ui.orders` | 軍令 | Orders | 군령 |
| `ui.roster` | 麾下 | Command | 휘하 |
| `ui.roster_empty` | 點選我軍棋子下令。A 全選可戰部隊。 | Click one of your pieces to give orders. A selects every unit able to fight. | 아군 기물을 클릭해 명령하십시오. A로 싸울 수 있는 부대를 모두 선택합니다. |
| `ui.men` | 兵力 | Men | 병력 |
| `ui.morale` | 士氣 | Morale | 사기 |
| `ui.hold` | 堅守 | Hold | 고수 |
| `ui.retreat` | 撤往北口 | Retreat | 북쪽 철수 |
| `ui.split` | 分兵 | Split | 분병 |
| `ui.join` | 合兵 | Join | 합병 |
| `ui.cancel` | 取消 | Cancel | 취소 |
| `ui.confirm_attack` | 確認進攻 | Confirm attack | 공격 확정 |
| `ui.armies` | 兩軍 | Armies | 양군 |
| `ui.log` | 戰報 | Battle log | 전황 기록 |
| `ui.terrain` | 地形 | Terrain | 지형 |
| `ui.legend.paddy` | 水田：騎兵行進減半，衝擊減半 | Paddy: cavalry at half speed and half impact | 논: 기병 이동 절반, 충격 절반 |
| `ui.legend.hill` | 山地：行進變慢，自下仰攻衝擊 ×0.8 | Hills: slower going; charging uphill ×0.8 | 산지: 이동이 느려지고, 올려 치는 충격 ×0.8 |
| `ui.legend.road` | 大路：行進略快 | Road: slightly faster | 큰길: 이동이 조금 빠름 |
| `ui.legend.rout` | 棋子翻成灰面：該部已崩潰潰逃 | Grey piece: the unit has broken and is routing | 회색 기물: 해당 부대가 붕괴해 궤주 중 |
| `ui.legend.frag` | 小棋子、虛線框：殘部，同營會合可合併 | Small piece, dashed frame: a remnant that merges with its camp | 작은 기물과 점선 테두리: 잔병, 같은 영과 만나면 합쳐짐 |
| `ui.selected` | 已選 {n} 部，兵 {men} | {n} selected, {men} men | {n}개 부대 선택, 병력 {men} |
| `ui.north` | 北 | N | 북 |
| `ui.exit` | 北口 | North exit | 북쪽 출구 |
| `ui.scale` | 百步 | 100 paces | 백 보 |
| `ui.preview_charge` | 衝擊預估 · {side} | Charge estimate · {side} | 충격 예측 · {side} |
| `ui.preview_melee` | 接戰預估 · {side} | Melee estimate · {side} | 교전 예측 · {side} |
| `ui.preview_by` | （按{name}） |  (as {name}) |  ({name} 기준) |
| `ui.army.ming` | 兵 {men} / {max} · 可戰 {n} 部 | Men {men} / {max} · {n} able to fight | 병력 {men} / {max} · 전투 가능 {n}개 부대 |
| `ui.army.ming_exited` | 兵 {men} / {max} · 可戰 {n} 部 · 已撤出 {k} 部 | Men {men} / {max} · {n} able to fight · {k} withdrawn | 병력 {men} / {max} · 전투 가능 {n}개 부대 · 철수 {k}개 부대 |
| `ui.army.jp` | 兵 {men} / {max} · 已崩潰 {n} 隊 | Men {men} / {max} · {n} units broken | 병력 {men} / {max} · 붕괴 {n}개 부대 |
| `ui.army.jp_waiting` | 兵 {men} / {max} · 已崩潰 {n} 隊 · 本隊未到 | Men {men} / {max} · {n} units broken · main army not yet here | 병력 {men} / {max} · 붕괴 {n}개 부대 · 본대 미도착 |
| `ui.list_sep` | 、 | ,  | ,  |
| `ui.shichen_names` | 子丑寅卯辰巳午未申酉戌亥 | Rat\|Ox\|Tiger\|Rabbit\|Dragon\|Snake\|Horse\|Goat\|Monkey\|Rooster\|Dog\|Pig | 자축인묘진사오미신유술해 |
| `ui.shichen_early` | 初 |  hour, early | 시 초 |
| `ui.shichen_late` | 正 |  hour, late | 시 정 |
| `ui.stat.withdrawn` | 撤出兵力 | Withdrawn | 철수 병력 |
| `ui.stat.ming_lost` | 明軍傷亡 | Ming losses | 명군 사상 |
| `ui.stat.jp_lost` | 日軍傷亡 | Japanese losses | 일본군 사상 |
| `ui.stat.jp_broken` | 日軍崩潰 | Units broken | 일본군 붕괴 |
| `ui.stat.best_impact` | 最大衝擊 | Best impact | 최대 충격 |
| `ui.stat.teams` | {n} 隊 | {n} | {n}개 부대 |
| `ui.settings` | 設定 (new) | Settings | 설정 |
| `ui.language` | 語言 (new) | Language | 언어 |
| `ui.reduced_motion` | 減少動態 (new) | Reduce motion | 움직임 줄이기 |
| `ui.on` | 開 (new) | On | 켬 |
| `ui.off` | 關 (new) | Off | 끔 |
| `ui.close` | 關閉 (new) | Close | 닫기 |
| `lang.self` | 繁體中文 (new) | English | 한국어 |
| `grade.decisive` | 大勝 | Decisive victory | 대승 |
| `grade.minor` | 小勝 | Minor victory | 소승 |
| `grade.pyrrhic` | 慘勝 | Pyrrhic victory | 참승 |
| `grade.defeat` | 敗 | Defeat | 패배 |
| `end.report` | 戰報 · 歷時 {time} | Battle report · {time} | 전황 · 경과 {time} |
| `end.events` | 戰況 | Key events | 주요 전황 |
| `end.none` | 無要事記錄 | No key events recorded | 주요 사건 기록 없음 |
| `end.again` | 再戰 | Fight again | 다시 싸우기 |
| `end.sweep` | 日軍盡數潰退，碧蹄館一線肅清。 | The Japanese army has been driven off; the road at Byeokjegwan is clear. | 일본군이 모두 물러나 벽제관 일대가 평정되었다. |
| `end.withdraw` | 李如松撤往坡州。 | Li Rusong withdraws to Paju. | 이여송이 파주로 철수했다. |
| `end.follow` | {names} 隨行撤出。 |  Withdrawing with him: {names}. |  함께 철수한 부대: {names}. |
| `end.dead` | 李如松陣亡，東征前鋒群龍無首。 | Li Rusong has fallen, and the Ming vanguard is leaderless. | 이여송이 전사하여 명군 선봉은 지휘관을 잃었다. |
| `end.fled` | 李如松隨潰兵逃散，前鋒失去節制。 | Li Rusong is swept away with the routers, and the vanguard falls apart. | 이여송이 궤주병에 휩쓸려 선봉이 통제를 잃었다. |
| `end.rout` | 明軍前鋒全軍潰散。 | The whole Ming vanguard has routed. | 명군 선봉이 모두 궤산했다. |
| `end.history` | 史實中，李如松陷入包圍、坐騎被射倒，親兵李有升捨身相護；楊元帶押後的騎兵趕到夾擊，明軍才得以退回坡州，此後轉向議和。各方記載的兵力與傷亡出入很大。 | Historically, Li Rusong was surrounded and his horse shot from under him; his guardsman Li Yousheng gave his life shielding him. Only when Yang Yuan arrived with the rearguard cavalry to strike the attackers could the Ming fall back to Paju, after which they turned toward negotiation. Accounts of the numbers and losses on each side differ widely. | 역사에서 이여송은 포위되어 타고 있던 말이 총에 맞아 쓰러졌고, 친병 이유승이 몸을 던져 그를 지켰다. 양원이 후위 기병을 이끌고 도착해 협공한 뒤에야 명군은 파주로 물러날 수 있었고, 이후 강화로 방향을 돌렸다. 양측의 병력과 사상자에 대한 기록은 서로 크게 다르다. |
| `suffix.remnant` | {name}·殘{n} | {name} · Remnant {n} | {name}·잔{n} |
| `suffix.detachment` | {name}·分{n} | {name} · Det. {n} | {name}·분{n} |
| `suffix.combined` | {name}·合 | {name} (Rejoined) | {name}·합 |
| `num.1` | 一 | 1 | 1 |
| `num.2` | 二 | 2 | 2 |
| `num.3` | 三 | 3 | 3 |
| `num.4` | 四 | 4 | 4 |
| `num.5` | 五 | 5 | 5 |
| `num.6` | 六 | 6 | 6 |
| `num.7` | 七 | 7 | 7 |
| `num.8` | 八 | 8 | 8 |
| `num.9` | 九 | 9 | 9 |
| `num.10` | 十 | 10 | 10 |
| `num.liang` | 兩 | 2 | 2 |
