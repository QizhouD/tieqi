#pragma once

#include <istream>
#include <string>
#include <vector>

namespace tq {

// Script lines: `<seconds> down|up|press <key>`, `<seconds> shot <file>`, `<seconds> quit`.
// A line starting with `#` is a comment. `<seconds> resize <width> <height>` captures another window size.
// Pointer coordinates are logical pixels. click, rclick, shiftclick, touchclick, and move take x y.
// drag and shiftdrag take x0 y0 x1 y1. click, rclick, and shiftclick first move to that point,
// so a scripted shot hovers the way a real mouse does. touchclick does not hover and does not move first.
struct ScriptEvent {
    float time = 0.f;
    std::string action;
    std::string arg;
};

std::vector<ScriptEvent> parseScript(std::istream &in);

} // namespace tq
