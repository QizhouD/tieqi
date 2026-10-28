#include "app/TextCache.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/String.hpp>

#include <cmath>
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

std::vector<std::string> wrapOnce(const sf::Font &font, std::string_view value, unsigned size, float maxWidth,
                                  bool bold) {
    std::vector<std::string> lines;
    std::string line;
    float cursor = 0.f;
    char32_t previous = 0;
    bool hasPrevious = false;
    std::size_t index = 0;
    char32_t code = 0;
    std::string bytes;
    while (nextCodepoint(value, index, code, bytes)) {
        if (bytes == "\n") {
            lines.push_back(line);
            line.clear();
            cursor = 0.f;
            hasPrevious = false;
            continue;
        }
        const float added = advanceOf(font, size, bold, previous, hasPrevious, code);
        if (!line.empty() && cursor + added > maxWidth) {
            lines.push_back(line);
            line = bytes;
            cursor = advanceOf(font, size, bold, 0, false, code);
        } else {
            line += bytes;
            cursor += added;
        }
        previous = code;
        hasPrevious = true;
    }
    if (!line.empty() || lines.empty()) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

sf::FloatRect textBounds(const sf::Font &font, unsigned size, bool bold, std::string_view value) {
    return storedText(font, size, bold, value).getLocalBounds();
}

void drawText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
              sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale,
              const sf::RenderStates &states) {
    sf::Text &shape = storedText(font, size, bold, value);
    shape.setScale(scale);
    shape.setRotation(sf::degrees(0.f));
    shape.setOrigin(origin);
    shape.setPosition(position);
    shape.setFillColor(fill);
    target.draw(shape, states);
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
