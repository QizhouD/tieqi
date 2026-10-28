#include "data/Strings.h"

#include <iostream>

namespace tq {

std::string StringTable::text(std::string_view key) const {
    const std::string name(key);
    if (const auto found = entries.find(name); found != entries.end()) {
        return found->second;
    }
    if (warned.insert(name).second) {
        const char *via = fallback != nullptr ? "zh-Hant" : "the key";
        warnings.push_back("missing key " + name + ", fell back to " + via);
        std::cerr << "string: " << warnings.back() << '\n';
    }
    if (fallback != nullptr) {
        return fallback->text(key);
    }
    return name;
}

} // namespace tq
