#pragma once

#include <istream>
#include <string>
#include <vector>

namespace tq {

// 框架脚本：`<秒> down|up|press <键>`、`<秒> shot <文件>`、`<秒> quit`。
// `#` 开头是注释。另外接受 `<秒> resize <宽> <高>`，用来截不同窗口比例。
// 指针坐标是逻辑像素：click、rclick、shiftclick、touchclick、move 是 x y；
// drag、shiftdrag 是 x0 y0 x1 y1。click、rclick、shiftclick 会先在同一点补一次 move，
// 这样脚本截图里的悬停和真鼠标一致。touchclick 不悬停，不补 move。
struct ScriptEvent {
    float time = 0.f;
    std::string action;
    std::string arg;
};

std::vector<ScriptEvent> parseScript(std::istream &in);

} // namespace tq
