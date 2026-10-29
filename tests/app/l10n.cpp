#include "app/Report.h"
#include "app/TextCache.h"
#include "data/Load.h"

#include <SFML/Graphics/Font.hpp>

#include <doctest/doctest.h>

#include <filesystem>
#include <regex>
#include <set>
#include <string>
#include <vector>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

StringTable zhTable() { return loadStrings(sourceRoot() / "data" / "strings" / "zh-Hant.json"); }

std::set<std::string> placeholders(const std::string &text) {
    std::set<std::string> found;
    static const std::regex pattern(R"(\{[a-z]+\})");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), pattern); it != std::sregex_iterator(); ++it) {
        found.insert(it->str());
    }
    return found;
}

bool containsWord(const std::string &text, const std::string &word) {
    const std::regex pattern("\\b" + word + "\\b", std::regex::icase);
    return std::regex_search(text, pattern);
}

} // namespace

TEST_CASE("en and ko own every zh-Hant key with the same placeholders") {
    const StringTable zh = zhTable();
    for (const char *language : {"en", "ko"}) {
        CAPTURE(language);
        const StringTable table =
            loadStrings(sourceRoot() / "data" / "strings" / (std::string(language) + ".json"), &zh);
        CHECK(table.language == language);
        for (const auto &[key, source] : zh.entries) {
            CAPTURE(key);
            const auto found = table.entries.find(key);
            REQUIRE(found != table.entries.end());
            CHECK_FALSE(found->second.empty());
            CHECK(placeholders(found->second) == placeholders(source));
        }
        for (const auto &entry : table.entries) {
            CHECK(zh.entries.count(entry.first) == 1u);
            table.text(entry.first);
        }
        CHECK(table.warnings.empty());
    }
}

TEST_CASE("no table uses a slur for either army") {
    const StringTable zh = zhTable();
    const StringTable en = loadStrings(sourceRoot() / "data" / "strings" / "en.json", &zh);
    const StringTable ko = loadStrings(sourceRoot() / "data" / "strings" / "ko.json", &zh);
    for (const StringTable *table : {&zh, &en, &ko}) {
        for (const auto &[key, value] : table->entries) {
            CAPTURE(key);
            CHECK(value.find("\u502d") == std::string::npos);
        }
    }
    for (const auto &[key, value] : ko.entries) {
        CAPTURE(key);
        for (const char *slur : {"\uc65c\uad70", "\uc65c\uc801", "\uc65c\ubcd1", "\uc65c\uad6c", "\uc65c\ub188",
                                 "\uc624\ub791\uce90"}) {
            CHECK(value.find(slur) == std::string::npos);
        }
    }
    // The word check itself must work, or the loop below would pass on anything.
    CHECK(containsWord("the Japanese army", "army"));
    CHECK_FALSE(containsWord("Japanese", "Japs?"));
    for (const auto &[key, value] : en.entries) {
        CAPTURE(key);
        for (const char *slur : {"Japs?", "Nips?", "barbarians?", "dwarf", "pirates?", "bandits?"}) {
            CHECK_FALSE(containsWord(value, slur));
        }
    }
}

TEST_CASE("the double-hour reads in each language") {
    const StringTable zh = zhTable();
    const StringTable en = loadStrings(sourceRoot() / "data" / "strings" / "en.json", &zh);
    const StringTable ko = loadStrings(sourceRoot() / "data" / "strings" / "ko.json", &zh);
    // 07:00 is the start of the dragon hour; one double-hour later is the start of the snake hour.
    CHECK(formatShichen(zh, "07:00", 0.f) == "\u8fb0\u521d");
    CHECK(formatShichen(en, "07:00", 0.f) == "Dragon hour, early");
    CHECK(formatShichen(en, "07:00", 300.f) == "Dragon hour, late");
    CHECK(formatShichen(en, "07:00", 600.f) == "Snake hour, early");
    CHECK(formatShichen(ko, "07:00", 0.f) == "\uc9c4\uc2dc \ucd08");
}

TEST_CASE("wrapping keeps English and Korean words whole and Chinese text intact") {
    const std::filesystem::path fonts = sourceRoot() / "assets" / "fonts";
    const sf::Font latin(fonts / "NotoSerifTC-Regular.otf");
    const sf::Font hangul(fonts / "NotoSerifKR-Regular.otf");
    const StringTable zh = zhTable();
    const StringTable en = loadStrings(sourceRoot() / "data" / "strings" / "en.json", &zh);
    const StringTable ko = loadStrings(sourceRoot() / "data" / "strings" / "ko.json", &zh);

    const auto words = [](const std::string &text) {
        std::vector<std::string> out;
        std::string word;
        for (char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    out.push_back(word);
                }
                word.clear();
            } else {
                word += c;
            }
        }
        if (!word.empty()) {
            out.push_back(word);
        }
        return out;
    };
    for (const auto &[table, font] : {std::pair{&en, &latin}, std::pair{&ko, &hangul}}) {
        for (const char *key : {"ui.intro_body", "ui.tip.4", "end.history", "obj.phase2"}) {
            CAPTURE(key);
            const std::string text = table->text(key);
            const std::vector<std::string> lines = wrapText(*font, text, 14, 300.f, false);
            CHECK(lines.size() > 1);
            std::vector<std::string> rejoined;
            for (const std::string &line : lines) {
                CHECK_FALSE(line.empty());
                CHECK(line.front() != ' ');
                CHECK(line.back() != ' ');
                for (const std::string &word : words(line)) {
                    rejoined.push_back(word);
                }
            }
            CHECK(rejoined == words(text));
        }
    }

    const sf::Font han(fonts / "NotoSerifTC-Regular.otf");
    for (const char *key : {"ui.intro_body", "end.history", "ui.tip.6"}) {
        CAPTURE(key);
        const std::string text = zh.text(key);
        const std::vector<std::string> lines = wrapText(han, text, 14, 300.f, false);
        CHECK(lines.size() > 1);
        std::string joined;
        for (const std::string &line : lines) {
            joined += line;
            for (const char *mark : {"\uff0c", "\u3002", "\u3001", "\uff1b", "\uff1a"}) {
                CHECK_FALSE(line.starts_with(mark));
            }
        }
        CHECK(joined == text);
    }
}
