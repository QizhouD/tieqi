#include "app/TextStyle.h"
#include "data/Load.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/String.hpp>

#include <doctest/doctest.h>

#include <filesystem>
#include <ios>
#include <map>
#include <set>
#include <string>

using namespace tq;

namespace {

TextRole roleFor(std::string_view key) {
    if (key.starts_with("glyph.")) {
        return TextRole::Glyph;
    }
    if (key.starts_with("banner.")) {
        return TextRole::Banner;
    }
    if (key.starts_with("fx.")) {
        return TextRole::Floater;
    }
    if (key == "game.title" || key == "ui.north" || key == "ui.scale" || key.starts_with("place.") ||
        key.starts_with("terrain.")) {
        return TextRole::Title;
    }
    return TextRole::Body;
}

// A missing key is drawn from zh-Hant. Noto Serif KR does not contain every Han character in that table.
const StringTable &ownerOf(const StringTable &table, const std::string &key) {
    if (table.entries.find(key) != table.entries.end() || table.fallback == nullptr) {
        return table;
    }
    return *table.fallback;
}

void collectKeys(const StringTable &table, std::set<std::string> &keys) {
    for (const auto &entry : table.entries) {
        keys.insert(entry.first);
    }
    if (table.fallback != nullptr) {
        collectKeys(*table.fallback, keys);
    }
}

std::string shown(const StringTable &table, const std::string &key) {
    if (const auto found = table.entries.find(key); found != table.entries.end()) {
        return found->second;
    }
    if (table.fallback != nullptr) {
        return shown(*table.fallback, key);
    }
    return key;
}

const sf::Font &fontFor(std::map<std::string, sf::Font> &fonts, const std::filesystem::path &file) {
    const std::string key = file.string();
    if (const auto found = fonts.find(key); found != fonts.end()) {
        return found->second;
    }
    const auto placed = fonts.emplace(key, sf::Font(file));
    return placed.first->second;
}

} // namespace

TEST_CASE("every string has a glyph in its role font") {
    const std::filesystem::path root(TQ_SOURCE_DIR);
    const StringTable zh = loadStrings(root / "data" / "strings" / "zh-Hant.json");
    const StringTable en = loadStrings(root / "data" / "strings" / "en.json", &zh);
    const StringTable ko = loadStrings(root / "data" / "strings" / "ko.json", &zh);
    std::map<std::string, sf::Font> fonts;
    int missing = 0;
    for (const StringTable *table : {&zh, &en, &ko}) {
        std::set<std::string> keys;
        collectKeys(*table, keys);
        for (const std::string &key : keys) {
            const std::string value = shown(*table, key);
            const StringTable &owner = ownerOf(*table, key);
            const TextStyle style = textStyle(owner.language, roleFor(key));
            const std::string &file = style.face == FontFace::Brush ? owner.fontBrush : owner.fontBody;
            const std::filesystem::path fontPath = root / "assets" / "fonts" / file;
            const sf::Font &font = fontFor(fonts, fontPath);
            const sf::String text = sf::String::fromUtf8(value.begin(), value.end());
            for (std::size_t index = 0; index < text.getSize(); ++index) {
                const char32_t code = text[index];
                if (code < 32) {
                    continue;
                }
                if (!font.hasGlyph(code)) {
                    missing += 1;
                    MESSAGE(table->language, " ", key, " U+", std::hex, static_cast<unsigned>(code));
                }
            }
        }
    }
    CHECK(missing == 0);
}
