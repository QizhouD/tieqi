# Tieqi（碧蹄馆 demo）

C++20 + SFML 3 的可暂停实时制战棋。完整规格见 docs/DEVELOPMENT.md，
规则参考实现见 reference/prototype/byeokjegwan.html（浏览器直接打开）。

## 构建与测试
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

## 硬性规则
- src/core 不得引用 SFML、ImGui 或系统头文件
- 随机数只用 tq::Rng；遍历单位按 UnitId 升序
- 兵种数值只放 data/*.json；规则常数只放 src/core/rules/Constants.h
- 改规则必须同步 docs/DEVELOPMENT.md 的"规则规格"
- 每个任务对应"任务拆分"里的一张卡，分支名 task/<卡号>-<短名>
- 不确定时在 PR 里提问，不要猜
- 游戏内中文一律繁体（zh-Hant），文案以 data/strings/zh-Hant.json 为准

## 目录
src/core 规则核心 · src/data 数据加载 · src/app 表现层 · tools/headless 无头对战
data 兵种与场景 · tests 测试 · reference 原型与参考资料
