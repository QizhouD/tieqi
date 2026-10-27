#pragma once

#include <string_view>

namespace tq {

// 棋子字三种语言都用 brush。标题、横幅、飘字只有 zh-Hant 用 brush；
// ko、en 以及其他语言用 body，再由调用方加上 SFML 的 Bold。
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
