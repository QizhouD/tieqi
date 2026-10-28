#pragma once

#include <string_view>

namespace tq {

// Piece glyphs use the brush face in every language. Titles, banners, and floaters use brush only for zh-Hant.
// Korean, English, and other languages use the body face, and the caller adds SFML Bold.
enum class TextRole { Body, Title, Banner, Floater, Glyph };
enum class FontFace { Body, Brush };

struct TextStyle {
    FontFace face = FontFace::Body;
    bool bold = false;
};

inline TextStyle textStyle(std::string_view language, TextRole role) {
    if (role == TextRole::Glyph) {
        return {FontFace::Brush, false};
    }
    if (role == TextRole::Body) {
        return {FontFace::Body, false};
    }
    if (language == "zh-Hant") {
        return {FontFace::Brush, false};
    }
    return {FontFace::Body, true};
}

} // namespace tq
