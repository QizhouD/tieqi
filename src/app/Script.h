#pragma once

#include <istream>
#include <string>
#include <vector>

namespace tq {

// 框架脚本：`<秒> down|up|press <键>`、`<秒> shot <文件>`、`<秒> quit`。
// `#` 开头是注释。另外接受 `<秒> resize <宽> <高>`，用来截不同窗口比例。
struct ScriptEvent {
    float time = 0.f;
    std::string action;
    std::string arg;
};

std::vector<ScriptEvent> parseScript(std::istream &in);

} // namespace tq
