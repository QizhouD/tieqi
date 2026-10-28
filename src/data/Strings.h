#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tq {

// The fallback table must outlive this one. A missing key checks the fallback, then the key itself.
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
