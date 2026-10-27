# 进度

| 卡号 | 状态 | PR | 摘要 | 遗留问题与默认决定 |
| --- | --- | --- | --- | --- |
| B0 | 已完成 | #1 | 五个 CMake 目标、dev/release/ci 预设、Windows 与 Ubuntu CI | 无 |
| 定名 | 已完成 | #2 | 游戏名 Afterglow: Imjin 1593（繁中 餘暉：壬辰 1593）；碧蹄馆只作场景名 | 概述里「正式名称未定」与定名冲突，改为写出正式名称。代号、命名空间、CMake 目标名、可执行文件名、目录名、仓库名都不改 |
| C1 | 已完成 | #3 | Vec2、角度、有向矩形、分离轴、sideOf、terrainAt 与 terrainSpeed | angDiff 在差恰好为 −π 时按规格折成 +π，原型保留 −π。turnToward 收朝向角，单位包装留到 C2。MapDef 暂只含宽高和水田、山地、大路；大路半宽取场景里的 half_width。sideOf 验收用铁炮正面 66、纵深 22，写在测试里。零长度路段按点距，退化椭圆不算命中 |
| D1 | 已完成 |  | 数据定义、JSON 加载与校验、繁中字符串表、duel_flat | 组出生点写进 Placement.pos，rally 另存。flow 省略时 enabled 为 false，由 C6 解释成停在阶段 1 且撤退口不开。flow、ai、result 只查类型和必填。地图边界和出生点外扩都含边界。字符串缺键回退到 zh-Hant 再退回键名，并写入 warnings。重整文案按战场秒写成 48 秒。num.liang 表示「兩」。duel_flat 只放李如松 |

## 停在这里
