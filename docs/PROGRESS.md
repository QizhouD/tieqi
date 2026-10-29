# 进度

| 卡号 | 状态 | PR | 摘要 | 遗留问题与默认决定 |
| --- | --- | --- | --- | --- |
| B0 | 已完成 | #1 | 五个 CMake 目标、dev/release/ci 预设、Windows 与 Ubuntu CI | 无 |
| 定名 | 已完成 | #2 | 游戏名 Afterglow: Imjin 1593（繁中 餘暉：壬辰 1593）；碧蹄馆只作场景名 | 概述里「正式名称未定」与定名冲突，改为写出正式名称。代号、命名空间、CMake 目标名、可执行文件名、目录名、仓库名都不改 |
| C1 | 已完成 | #3 | Vec2、角度、有向矩形、分离轴、sideOf、terrainAt 与 terrainSpeed | angDiff 在差恰好为 −π 时按规格折成 +π，原型保留 −π。turnToward 收朝向角，单位包装留到 C2。MapDef 暂只含宽高和水田、山地、大路；大路半宽取场景里的 half_width。sideOf 验收用铁炮正面 66、纵深 22，写在测试里。零长度路段按点距，退化椭圆不算命中 |
| D1 | 已完成 | #4 | 数据定义、JSON 加载与校验、繁中字符串表、duel_flat | 组出生点写进 Placement.pos，rally 另存。flow 省略时 enabled 为 false，由 C6 解释成停在阶段 1 且撤退口不开。flow、ai、result 只查类型和必填。地图边界和出生点外扩都含边界。字符串缺键回退到 zh-Hant 再退回键名，并写入 warnings。重整文案按战场秒写成 48 秒。num.liang 表示「兩」。duel_flat 只放李如松 |
| C2 | 已完成 | #5 | 模拟骨架、命令队列、移动、溃逃和状态哈希 | 阶段流程、接触、战斗结算和胜负留空，phase 保持 0，第一次 step 不发 PhaseChanged。三眼铳不调用 mingVolley，冲锋开始仍把 volleyDone 置 false，但不因距离小于 62 置 true。Split 和 Join 先忽略。Rng 为 PCG32，initseq 固定为 1。哈希按字段字节：布尔写成 0/1，optional 为标志加浮点，字符串先写长度，接战先写个数。撤退纵坐标取场景 JSON。kTickSeconds 定义在 Constants.h，由 Types.h 引入 |
| C3 | 已完成 | #6 | 接触、冲击、三眼铳、脱离和冲击预估 | 崩溃只改状态、士气上限和 Broken 事件，不散开，也不做连锁士气和气势。接战预估的倍数留到 C4。预估提示只有枚举，重整剩余秒数和齐射目标不另存。run 为 0 时冲击速度系数按 1。到达当帧的推挤可能把距离顶过 5 米。公式点相距 30 米，预估因不超过 45 米走接战，预估用例放在 70 米的同一正面 |
| C4 | 已完成 | #7 | 接战计时、近战、齐射、士气状态与接战预估 | 崩溃仍不散开，不调用 shatter，mergeStep 留到 C5。primaryContact 改用完整 meleePower，同战力取较小 id。近战预估的攻防两项都记成 ModKind::Melee，攻方在前，守方值为 1/max(eP, 0.1)。nearGeneral 用将领自己的 aura，距离严格小于，多个取最小 id。实际近战 ratio 截断到 [0.3, 3]，预估不截断；share 用接战列表全长，跳过已退场目标。齐射同分保留较小 id。集结要求距离严格小于 180 米，且 routSec 严格大于 80。近战伤害按目标 id 升序结算，原型按首次累计的插入顺序 |
| C5 | 已完成 | #8 | 崩溃散开、残部合并、分兵合兵和显示名后缀 | 总兵力记在私有计数里，残部和分队不增加，读取接口留给 C6。origin 为 kNoUnit 的单位不合并。同一步骤里新产生的单位不参加该步骤剩余部分，当次战斗结算的合并不包括刚散开的残部。displayName 返回后缀种类和 1 到 10 的循环序号。单股只设 knock，不发 Shattered。zh-Hant 补了 suffix.combined |
| C6 | 已完成 | #9 | 阶段流程、生成组、总攻、援军和胜负评级 | 场景秒仍留在 ScenarioDef，判定时用 lround(秒/0.8) 换成 tick；规格写加载时换算，改加载器会改掉 D1 已验收的秒数字段。BattleResult 增加 end、grade、saved、rate；PhaseChanged 的 value 是阶段号，ScoutWarning 位置用主帅，BattleEnded 的 value 是评级，总攻未发动用 kAssaultFailed。总攻加成之后，若该 tick 恰好做战斗结算，既有士气上限 100 会再截断，小早川 105 变成 100，两翼 92 不变。危急距离和随主帅撤出都是严格小于；先锋崩溃只数开局标记的 id；flow.enabled 为 false 时第一次 step 仍进入阶段 1。日军生成后仍是 deploying，入场结束留给 C7。总兵力私有计数含后来的援军，仍无对外读取接口。战斗结束后 step 直接返回 |
| C7 | 已完成 | #10 | 日军 AI：选目标、铁炮、主将、先锋大将、残部和换目标 | 关闭 AI 的选项 C2 已有，本卡接上，默认开着。按兵时间用 tick 严格大于 lround(秒/0.8)，第 450 tick 不算超过 360 秒。不带朝向的 Hold 清空 face。得分相同保留较小 id。approach 在距离为 0 时直接返回。跟随点不按地图边裁切。有接战的入场单位本轮不解除入场。既有移动和流程测试关闭 AI。命令行 --no-ai 留给 H1 |
| H1 | 已完成 | #11 | 无头对战、两种策略、CSV 汇总和回放读写 | 回放命令用场景 id，没有场景 id 的单位写成 # 加 UnitId。构建号是配置时的 git 短哈希。--seed 加 --runs 表示从该种子起连续多场。--data-root 用来找数据目录，默认向上查找 data/units.json。分位数用 ceil(p*n)-1。家丁右营的进攻在 tick 300 之后第一次可战时下达，下一 tick 执行。阶段 3 满 300 tick 的撤退命令在 phase3At+300 执行。冒烟在超时、非有限数、没有事件、超过 10 秒，或 scripted 没有冲击时返回非 0。3000 tick 哈希测试关闭 AI，scripted 的事件一致性另测 |
| P1 | 已完成 | #12 | 近战顺序和随机数消耗对齐原型；整场基线改为原型 100 场 | 浮点差不改：分离轴 float32 会把重叠残部推开十几米；崩溃计时比原型早 1 tick；按兵和阶段时钟差 1–3 tick。passive 杨元中位 2000，原型 2003。slow 测试不进默认 ctest |
| A0 | 已完成 | #15 | SFML 3 外壳：字体按角色选择、四个合成音、黑边缩放、脚本截图 | 验收画面同时画三种语言，当前语言只改窗口标题。整页切换留给 L1。F6 按 thud、crack、drum、rout 的顺序各放一次。设置写在可执行文件旁的 settings.txt。规则核心未改 |
| A1 | 已完成 | #16 | 绢本底图在战场矩形里居中；默认窗口不超过桌面 90% | 缩放平移留给 A1b。窗口没有最大尺寸。地图标题按 TextStyle::Title 选字体。规则核心未改 |
| A2 | 已完成 | #17 | 方阵、棋子、朝向楔形、兵力与士气条、气势、计时弧、翻面和选中框，位置按 tick 插值 | 尘土和飘字留到 A5。A4 做顶栏时把战场矩形改到原型 1600×1000 下量到的画布 (17, 55)、1216×932。规则核心未改 |
| A3 | 已完成 | #18 | 点选、框选、下令、快捷键、待确认；预估箭头和倍数；翻面 brokenSec 按帧间 alpha 插值 | 开局暂停，空格才进入阶段 1。确认进攻按钮和战场画布改在 A4。规则核心未改 |
| A4 | 已完成 | #19 | 顶栏、右侧各卡、预估与确认进攻、开局与结算遮罩、横幅和战报；战场画布 (17, 55)、1216×932 | Battle::enlisted 只读，不改 step。100 场基线未重跑。面板文字按设备像素绘制，同一字体、字号、粗细的排版器常驻，换行按字符串、字号和宽度缓存。脚本计时从第一帧起。结算卡片高度随内容。release、种子 1、scripted、2 倍速跑到结算：mean 0.5 ms，p95 0.8 ms，max 81.3 ms。尘土、飘字、顿帧和事件音效留到 A5 |
| A5 | 已完成 | #21 | 尘土、硝烟、火光、碎片、圆环、飘字、点击标记、震屏、顿帧和事件音效；`--freeze-at` | 规则核心未改。2026-10-29 重跑：dev 预设全量重建零告警，ctest 全过，含 slow 标签的 100 场基线。同一回放开关 reduced_motion 只改表现时间，不改战果。表现层随机数是 mt19937，种子 1594，不碰 tq::Rng，特效不要求与原型逐像素相同。碎片尺寸按原型用世界单位 3–7（规格写像素），扇面用散开后单位的 facing。飘字每秒上浮 18 米。硝烟用原型的三眼铳 5 团和铁炮 7 团两套参数，都落在规格包络内。点击标记不在事件表里，按渲染层补上。重粒子上限 1600。字形测试：键在本语言表里就用该语言的角色字体，回退到 zh-Hant 的键用繁中字体。Noto Serif KR 缺 12 个汉字（U+8D95、U+8173、U+586B、U+57AE、U+4F75、U+5920、U+503C、U+5C1A、U+8DD1、U+5B83、U+78B0、U+64CB），ko 未拥有这些键时界面仍用韩文字体，缺字留给 L1，不换字体、不改繁中措辞。release、种子 1、scripted、2 倍速跑到结算（重跑三次）：mean 0.4 ms，p95 0.5 ms，max 46.5–108.4 ms。CI 的 GCC 13 在 tests/app/glyphs.cpp 报 -Wdangling-reference（fontFor 收了临时 path，误报），改为先存进局部变量。种子 1 在 tick 59、255、310、1800 与原型左右对照 |
| A6 | 已完成 | #22 | ImGui 调试面板：Sim（tick、阶段、状态哈希、单步、模拟与渲染耗时、保存与载入回放）、Units（列表、检查器、击溃）、Spawn、AI 意图与目标连线、常数表；F1 开关，F2 换页签。只在 TQ_DEV_TOOLS=ON 时编译 | 生成与击溃走 submit() 的两条开发命令 Spawn、Rout，回放记为 spawn、rout，schema 和规格同步；不用 BattleAccess。Battle 加只读的 vanguardAggressive()。生成的单位计入总兵力，消耗一次 tq::Rng 抽装填，Hold 原地，labelKey 用兵种 name，不发事件。App 和无头工具共用 tq_data 的 resolveCommand、recordCommand。常数表在配置时从 Constants.h 生成。面板英文、显示场景 id，不走字符串表。ci 预设新增 tieqi_without_imgui 测试；release 产物本地检查无 ImGui 字符串，dev 产物检出 35 处。改了 src/core：种子 1–100 两种策略的 CSV、种子 1–5 两种策略的逐 tick 状态与事件输出，改动前后逐字节相同；slow 基线通过。dev 预设全量重建零告警。release、种子 1、scripted、2 倍速跑到结算（三次）：mean 0.4 ms，p95 0.5 ms，max 82.0–103.0 ms |
| L1 | 待合并 | #23 | en、ko 字符串表补全（各 285 键，全部待作者审，见下节）；顶栏「設定」按钮和设定卡：切换语言（重建底图地名和窗口标题）、reduced_motion，写入 settings.txt；按词换行；版面随文字宽度自适应 | 换行规则改为：空格后、汉字与中日文标点前后可断，韩文、英文按词，收尾标点不放行首；中文排版因此在少数行有变化（例如「×4.2」不再被拆开）。战报要事行原先按非粗体测宽、按粗体绘制，会冲出面板，改为按粗体测宽。顶栏阶段列、时钟列，两军卡色条，麾下卡兵力、士气条，声音、设定按钮按文字宽度让位，中文位置不变；结算统计标签超宽时缩小字号。en 的时辰名用 `|` 分隔（Dragon hour, early），ko 用 진시 초。en、ko 的 num.* 用阿拉伯数字。en 不用全角引号（思源宋体 TC 的全角引号很宽）。end.follow 在 en、ko 里以空格开头，因为它直接接在 end.withdraw 后面。已写入的战报条目切换语言后保留原语言。`--script` 运行不写 settings.txt。tests/data 的回退测试原来依赖 en.json 不完整，改用 tests/data/strings/partial.json 夹具，断言不变。规则核心未改。dev 预设全量重建零告警，ctest 全过，slow 基线通过，字形测试通过。release、种子 1、scripted、2 倍速跑到结算（三次）：mean 0.3–0.4 ms，p95 0.5 ms，max 103.8–115.0 ms |

源码检查扫 src、tests、tools、cmake、CMakeLists.txt、.github/workflows、.gitignore。tools/parity/anchors.json 保存对照原型用的中文片段，不在检查范围内；node_modules 和 tools/parity/work 也不查。

## 待作者审的译文（L1）

下表是 L1 新写的全部译文，状态一律为**待作者审**：en 259 条、ko 259 条，另有 zh-Hant 新增的设定卡用语 7 条（标「新增」）。原表里的专有名词（地名、李如松、楊元、立花宗茂、小早川隆景、兵種名、棋子字）沿用，不在表内。用词约定：日方英文写 Japanese army / Japanese forces（形容词 Japanese），韩文写 일본군；明军 명군；韩文里棋子叫 기물，避免和「말」（马）混淆；不用 왜、倭、오랑캐 等贬称，`tests/app/l10n.cpp` 检查这些词不出现。审校后改 `data/strings/` 即可，不必改代码。

| 键 | zh-Hant | en | ko |
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
| `ui.settings` | 設定（新增） | Settings | 설정 |
| `ui.language` | 語言（新增） | Language | 언어 |
| `ui.reduced_motion` | 減少動態（新增） | Reduce motion | 움직임 줄이기 |
| `ui.on` | 開（新增） | On | 켬 |
| `ui.off` | 關（新增） | Off | 끔 |
| `ui.close` | 關閉（新增） | Close | 닫기 |
| `lang.self` | 繁體中文（新增） | English | 한국어 |
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
