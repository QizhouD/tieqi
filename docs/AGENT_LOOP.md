# 连续开发的流程和停止条件

一、收尾 A5（PR #21，分支 task/A5-fx）
1. 用 REST API 查 #21 的 CI 结果（凭据用 Git 已保存的，只碰 QizhouD/tieqi，不要打印出来，也不要写进任何文件）。在 Linux 上用 GCC 13 编译时，tests/app/glyphs.cpp 第 89 行报 -Wdangling-reference，ci 预设下警告当错误，编译失败。这是误报：给 fontFor 传了临时 path。改法是先把路径存进局部变量再传。
2. 不要沿用之前记录的数字，自己重新跑：dev 预设零警告；ctest 全过，包括 slow 标签的 100 场基线；release 预设下用 tieqi --perf 跑完种子 1 的 scripted 回放，要求 p95 ≤ 16.7 毫秒。把新数字更新到 PR 描述和 PROGRESS。
3. CI 全绿后用 squash 合并 #21，删除分支。

二、然后按顺序做 A6 → L1 → R1，每张卡一个 PR
流程：从 main 拉分支 task/<卡号>-<英文短名>；读“给 agent 的工作约定”、这张卡，以及“表现层规格”“窗口与布局”“本地化与字体”“测试与验收”；实现，写测试；dev 预设零警告、ctest 全过；更新 PROGRESS；开 PR；等 CI。CI 全绿、下面的自检全部通过后，自己用 squash 合并，然后做下一张。
- A6：调试工具只在 TQ_DEV_TOOLS=ON 时编译，不能进 release。
- L1：起草 en、ko 字符串，在 PROGRESS 里把所有新译文列为“待作者审”。用词保持中立：日本一方英文写 Japanese army 或 Japanese forces，韩文写 일본군；明军写 명군，朝鲜写 조선。不用 왜、倭、오랑캐 之类贬称，任何一方都不丑化。人名、地名的写法和现有 en.json 一致。设置里可以切换语言；切换后重建地图文字；ko、en 各截一张整页图，检查有没有文字溢出。
- R1：做 Windows 试玩包。release 静态链接 MSVC 运行库（CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded，SFML_USE_STATIC_STD_LIBS=ON），用 dumpbin /dependents 确认只依赖系统 DLL。包里放可执行文件、data、assets、字体和各自的许可证、THIRD_PARTY_NOTICES（SFML、FreeType、HarfBuzz、SheenBidi、ogg、vorbis、FLAC、miniaudio、nlohmann/json）、AI_ASSETS.md、繁中加英文的试玩说明。版本号写进回放文件。压缩包命名为 Afterglow-Imjin-1593-demo-<版本>-win64.zip，留在本地，不要发布 GitHub Release。

三、每张卡合并前的自检
1. 性能：同一-2 的 --perf 实测，达标才能合并。
2. 字体：tests/app/glyphs.cpp 的缺字检查必须通过。
3. 规则核心：只要改了 src/core，100 场基线和逐 tick 对照的结果必须和改之前完全一样。
4. 截图：用 --script 和 --freeze-at 截出能证明这张卡验收的画面，放进 PR。
5. 源代码（src、tests、tools、cmake、CMakeLists.txt、.github/workflows）里不出现中文；玩家看到的文字只放 data/strings/。
6. 提交前用 GCC 的告警标准想一遍：CI 的 Ubuntu 任务用 GCC 13 且把警告当错误，Windows 本地编译通过不代表 CI 能过。

四、停止条件（遇到任何一条就停）
- 规格不清楚或和原型冲突，并且会影响玩法、数值或外观；小事按文档已有的默认做法定，写进 PR 和 PROGRESS。
- 性能不达标，或者改了 src/core 后基线、对照结果变了。
- 同一张卡 CI 修了 3 轮还是红的。
- 需要新的第三方依赖，或者要做对外发布这类不可逆的操作。
- 一律禁止：直接推送到 main、force push、合并 CI 没有全绿的 PR、跳过或删除测试、改 reference/、操作别的仓库。
停下来时，把原因和你建议的做法写进 docs/PROGRESS.md 的“停在这里”一节，推送到当前分支（不要合并），再告诉我。

五、R1 合并后停下来，列出需要我亲自做的事：完整打三局（M2 手动验收）、试听音效、审阅英文和韩文译文、在一台没装开发环境的 Windows 电脑上解压试玩。每合并一个 PR 只回我一行，例如“A6 已合并 #22，p95 9.8 毫秒”。
