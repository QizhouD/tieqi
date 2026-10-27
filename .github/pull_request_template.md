## 卡号

<!-- 例如 [C3] contacts and charge impact，并写 Closes #issue号 -->

## 做了什么

## 怎么验证的

## 遗留问题，或与规格冲突的地方

## 检查清单

- [ ] 构建无警告（GCC/Clang 开 -Wall -Wextra -Wpedantic，MSVC 开 /W4），代码已用 clang-format 格式化
- [ ] ctest 全部通过，新逻辑都有测试
- [ ] 改动规则时：docs/DEVELOPMENT.md 的"规则规格"已同步，原型对照测试仍在允许误差内
- [ ] 改动数据格式时：data/schema/ 和"数据格式"一章已同步
- [ ] 游戏内文字只走 data/strings/，中文为繁体
