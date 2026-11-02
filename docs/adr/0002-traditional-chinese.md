# 0002 Chinese in the game is always Traditional

- Status: accepted
- Date: 2026-10-27

## Context

The HTML prototype originally used Simplified characters and the Ma Shan Zheng font, which covers only Simplified characters.

## Decision

- Chinese in the game (interface, battle log, counter glyphs, map labels) is always Traditional, language code `zh-Hant`, with glyph forms following the Taiwan standard; when transcribing from Simplified, only characters change, never words. 铁炮 is written 鐵砲.
- Counter glyphs are Traditional Chinese characters in all three languages: Chinese, English, and Korean.
- Fonts: body text uses Noto Serif TC / KR; titles, counter glyphs, floating text, and banners use LXGW WenKai TC. All three are under the SIL OFL.
- Documentation and code comments were originally written in Simplified Chinese. Superseded on 2026-11-01: the repository, including documentation and comments, is now written in English.

## Consequences

- `data/strings/zh-Hant.json` is the source language, and other languages fall back to it for missing keys.
- The prototype has been converted to Traditional, so interface text can be carried over from it directly.
