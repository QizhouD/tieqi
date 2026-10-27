#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tq {

// fallback 指向的表必须比本表活得更久。缺键时先查 fallback，再退回键名本身。
struct StringTable {
    std::string language;
    std::string fontBody;
    std::string fontBrush;
    std::map<std::string, std::string> entries;
    const StringTable *fallback = nullptr;
    mutable std::vector<std::string> warnings;
    mutable std::set<std::string> warned;

    std::string text(std::string_view key) const;
};

} // namespace tq
