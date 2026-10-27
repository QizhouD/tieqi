#include "app/Script.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace tq {
namespace {

std::string trim(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.pop_back();
    }
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        begin += 1;
    }
    return text.substr(begin);
}

} // namespace

std::vector<ScriptEvent> parseScript(std::istream &in) {
    std::vector<ScriptEvent> events;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        std::istringstream row(line);
        ScriptEvent event;
        if (!(row >> event.time >> event.action)) {
            continue;
        }
        std::string rest;
        std::getline(row >> std::ws, rest);
        event.arg = trim(rest);
        events.push_back(std::move(event));
    }
    std::stable_sort(events.begin(), events.end(),
                     [](const ScriptEvent &left, const ScriptEvent &right) { return left.time < right.time; });
    return events;
}

} // namespace tq
