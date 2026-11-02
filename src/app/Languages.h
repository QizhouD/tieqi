#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace tq {

// Languages offered in the settings and shipped in the demo. Korean is on hold: data/strings/ko.json and the Korean
// font stay in the repository but are neither offered nor packaged.
inline constexpr std::array<std::string_view, 2> kShippedLanguages{"zh-Hant", "en"};
inline constexpr std::string_view kDefaultLanguage = "zh-Hant";

// A language that is not shipped (for example "ko" in an old settings.txt) falls back to Traditional Chinese.
inline std::string shippedLanguage(std::string_view requested) {
    const bool shipped =
        std::find(kShippedLanguages.begin(), kShippedLanguages.end(), requested) != kShippedLanguages.end();
    return std::string(shipped ? requested : kDefaultLanguage);
}

} // namespace tq
