#include "app/TextCache.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/String.hpp>

#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <utility>

namespace tq {
namespace {

struct TextKey {
    const sf::Font *font = nullptr;
    unsigned size = 0;
    bool bold = false;
    std::string text;

    bool operator==(const TextKey &other) const {
        return font == other.font && size == other.size && bold == other.bold && text == other.text;
    }
};

struct TextKeyHash {
    std::size_t operator()(const TextKey &key) const {
        std::size_t hash = std::hash<const void *>{}(key.font);
        const auto mix = [&](std::size_t value) { hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2); };
        mix(std::hash<unsigned>{}(key.size));
        mix(std::hash<bool>{}(key.bold));
        mix(std::hash<std::string>{}(key.text));
        return hash;
    }
};

struct StyleKey {
    const sf::Font *font = nullptr;
    unsigned size = 0;
    bool bold = false;

    bool operator==(const StyleKey &other) const {
        return font == other.font && size == other.size && bold == other.bold;
    }
};

struct StyleKeyHash {
    std::size_t operator()(const StyleKey &key) const {
        std::size_t hash = std::hash<const void *>{}(key.font);
        hash ^= std::hash<unsigned>{}(key.size) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        hash ^= std::hash<bool>{}(key.bold) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        return hash;
    }
};

struct WrapKey {
    const sf::Font *font = nullptr;
    unsigned size = 0;
    bool bold = false;
    int width = 0;
    std::string text;

    bool operator==(const WrapKey &other) const {
        return font == other.font && size == other.size && bold == other.bold && width == other.width &&
               text == other.text;
    }
};

struct WrapKeyHash {
    std::size_t operator()(const WrapKey &key) const {
        std::size_t hash = std::hash<const void *>{}(key.font);
        const auto mix = [&](std::size_t value) { hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2); };
        mix(std::hash<unsigned>{}(key.size));
        mix(std::hash<bool>{}(key.bold));
        mix(std::hash<int>{}(key.width));
        mix(std::hash<std::string>{}(key.text));
        return hash;
    }
};

struct Cache {
    std::unordered_map<StyleKey, sf::Text, StyleKeyHash> anchors;
    std::unordered_map<TextKey, sf::Text, TextKeyHash> texts;
    std::unordered_map<WrapKey, std::vector<std::string>, WrapKeyHash> wraps;
};

Cache &cache() {
    static Cache stored;
    return stored;
}

sf::String utf8(std::string_view text) { return sf::String::fromUtf8(text.begin(), text.end()); }

void retain(const sf::Font &font, unsigned size, bool bold) {
    const StyleKey key{&font, size, bold};
    if (cache().anchors.find(key) != cache().anchors.end()) {
        return;
    }
    sf::Text shape(font, sf::String("0"), size);
    if (bold) {
        shape.setStyle(sf::Text::Bold);
    }
    const sf::FloatRect bounds = shape.getLocalBounds();
    shape.setOrigin({bounds.position.x, bounds.position.y});
    cache().anchors.emplace(key, std::move(shape));
}

sf::Text &storedText(const sf::Font &font, unsigned size, bool bold, std::string_view value) {
    retain(font, size, bold);
    TextKey key{&font, size, bold, std::string(value)};
    Cache &stored = cache();
    if (const auto found = stored.texts.find(key); found != stored.texts.end()) {
        return found->second;
    }
    sf::Text shape(font, utf8(value), size);
    if (bold) {
        shape.setStyle(sf::Text::Bold);
    }
    const sf::FloatRect bounds = shape.getLocalBounds();
    shape.setOrigin({bounds.position.x, bounds.position.y});
    const auto placed = stored.texts.emplace(std::move(key), std::move(shape));
    return placed.first->second;
}

bool nextCodepoint(std::string_view text, std::size_t &index, char32_t &code, std::string &bytes) {
    if (index >= text.size()) {
        return false;
    }
    const auto lead = static_cast<unsigned char>(text[index]);
    std::size_t length = 1;
    if ((lead & 0xE0) == 0xC0) {
        length = 2;
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3;
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4;
    }
    if (index + length > text.size()) {
        length = 1;
    }
    bytes.assign(text.substr(index, length));
    if (length == 1) {
        code = lead;
    } else if (length == 2) {
        code = (static_cast<char32_t>(lead & 0x1Fu) << 6) | (static_cast<unsigned char>(text[index + 1]) & 0x3Fu);
    } else if (length == 3) {
        code = (static_cast<char32_t>(lead & 0x0Fu) << 12) |
               (static_cast<char32_t>(static_cast<unsigned char>(text[index + 1]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(text[index + 2]) & 0x3Fu);
    } else {
        code = (static_cast<char32_t>(lead & 0x07u) << 18) |
               (static_cast<char32_t>(static_cast<unsigned char>(text[index + 1]) & 0x3Fu) << 12) |
               (static_cast<char32_t>(static_cast<unsigned char>(text[index + 2]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(text[index + 3]) & 0x3Fu);
    }
    index += length;
    return true;
}

// SFML adds 0.8px to a non-zero advance when the text style is bold.
float advanceOf(const sf::Font &font, unsigned size, bool bold, char32_t previous, bool hasPrevious, char32_t code) {
    float width = font.getGlyph(code, size, bold).advance;
    if (bold && width != 0.f) {
        width += width > 0.f ? 0.8f : -0.8f;
    }
    if (hasPrevious) {
        width += font.getKerning(previous, code, size, bold);
    }
    return width;
}

// Han, kana, and CJK punctuation may break on either side. Hangul and Latin break only at spaces, so Korean and
// English wrap by word.
bool breaksAround(char32_t code) {
    return (code >= 0x2E80 && code <= 0x303F) || (code >= 0x3040 && code <= 0x30FF) || (code >= 0x3400 && code <= 0x4DBF) ||
           (code >= 0x4E00 && code <= 0x9FFF) || (code >= 0xF900 && code <= 0xFAFF) || (code >= 0xFF00 && code <= 0xFFEF);
}

// Closing punctuation never starts a line.
bool closing(char32_t code) {
    switch (code) {
    case U'\u3001':
    case U'\u3002':
    case U'\uFF0C':
    case U'\uFF0E':
    case U'\uFF1A':
    case U'\uFF1B':
    case U'\uFF01':
    case U'\uFF1F':
    case U'\uFF09':
    case U'\u300D':
    case U'\u300F':
    case U'\u3009':
    case U'\u300B':
    case U'\u3011':
    case U'\u201D':
    case U'\u2019':
    case U',':
    case U'.':
    case U';':
    case U':':
    case U'!':
    case U'?':
    case U')':
        return true;
    default:
        return false;
    }
}

struct Glyph {
    char32_t code = 0;
    std::string bytes;
};

std::string joinGlyphs(const std::vector<Glyph> &glyphs, std::size_t from, std::size_t to) {
    std::string out;
    for (std::size_t index = from; index < to; ++index) {
        out += glyphs[index].bytes;
    }
    return out;
}

std::vector<std::string> wrapOnce(const sf::Font &font, std::string_view value, unsigned size, float maxWidth,
                                  bool bold) {
    std::vector<std::string> lines;
    std::vector<Glyph> line;
    float cursor = 0.f;
    // A break may fall before line[breakAt]; 0 means no break yet.
    std::size_t breakAt = 0;
    const auto measureLine = [&]() {
        float width = 0.f;
        for (std::size_t index = 0; index < line.size(); ++index) {
            width += advanceOf(font, size, bold, index == 0 ? 0 : line[index - 1].code, index > 0, line[index].code);
        }
        return width;
    };
    const auto flush = [&](std::size_t cut) {
        std::size_t head = cut;
        while (head > 0 && line[head - 1].code == U' ') {
            head -= 1;
        }
        lines.push_back(joinGlyphs(line, 0, head));
        std::size_t tail = cut;
        while (tail < line.size() && line[tail].code == U' ') {
            tail += 1;
        }
        line.erase(line.begin(), line.begin() + static_cast<std::ptrdiff_t>(tail));
        breakAt = 0;
        cursor = measureLine();
    };
    std::size_t index = 0;
    Glyph next;
    while (nextCodepoint(value, index, next.code, next.bytes)) {
        if (next.bytes == "\n") {
            lines.push_back(joinGlyphs(line, 0, line.size()));
            line.clear();
            cursor = 0.f;
            breakAt = 0;
            continue;
        }
        if (line.empty() && next.code == U' ' && !lines.empty()) {
            continue;
        }
        if (!line.empty()) {
            const char32_t previous = line.back().code;
            const bool afterSpace = previous == U' ' && next.code != U' ';
            const bool cjk = (breaksAround(previous) || breaksAround(next.code)) && !closing(next.code);
            if (afterSpace || cjk) {
                breakAt = line.size();
            }
        }
        const float added = advanceOf(font, size, bold, line.empty() ? 0 : line.back().code, !line.empty(), next.code);
        if (!line.empty() && cursor + added > maxWidth) {
            if (breakAt > 0) {
                flush(breakAt);
            } else {
                flush(line.size());
            }
            if (line.empty() && next.code == U' ') {
                continue;
            }
            cursor += advanceOf(font, size, bold, line.empty() ? 0 : line.back().code, !line.empty(), next.code);
            line.push_back(next);
            continue;
        }
        cursor += added;
        line.push_back(next);
    }
    if (!line.empty() || lines.empty()) {
        lines.push_back(joinGlyphs(line, 0, line.size()));
    }
    return lines;
}

} // namespace

sf::FloatRect textBounds(const sf::Font &font, unsigned size, bool bold, std::string_view value) {
    return storedText(font, size, bold, value).getLocalBounds();
}

static void paintText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
                      sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale, sf::Color outline,
                      float thickness, const sf::RenderStates &states) {
    sf::Text &shape = storedText(font, size, bold, value);
    shape.setScale(scale);
    shape.setRotation(sf::degrees(0.f));
    shape.setOrigin(origin);
    shape.setPosition(position);
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(thickness);
    target.draw(shape, states);
}

void drawText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
              sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale,
              const sf::RenderStates &states) {
    paintText(target, font, size, bold, value, fill, position, origin, scale, sf::Color::Transparent, 0.f, states);
}

void drawOutlinedText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
                      sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale, sf::Color outline,
                      float thickness) {
    paintText(target, font, size, bold, value, fill, position, origin, scale, outline, thickness,
              sf::RenderStates::Default);
}

std::vector<std::string> wrapText(const sf::Font &font, std::string_view value, unsigned size, float maxWidth,
                                  bool bold) {
    WrapKey key{&font, size, bold, static_cast<int>(std::lround(maxWidth)), std::string(value)};
    Cache &stored = cache();
    if (const auto found = stored.wraps.find(key); found != stored.wraps.end()) {
        return found->second;
    }
    std::vector<std::string> lines = wrapOnce(font, value, size, maxWidth, bold);
    const auto placed = stored.wraps.emplace(std::move(key), std::move(lines));
    return placed.first->second;
}

void clearTextCache() { cache() = {}; }

TextCacheGuard::~TextCacheGuard() { clearTextCache(); }

} // namespace tq
