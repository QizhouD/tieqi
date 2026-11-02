# Fonts

Three fonts are needed, all under the SIL Open Font License 1.1, so they can be distributed with the game. The file names in this folder must match the `fonts` fields in `data/strings/*.json`.

| Use | Font | File in this folder | Download |
|---|---|---|---|
| Traditional Chinese and English body text | Noto Serif TC | `NotoSerifTC-Regular.otf` | The latest Noto Serif CJK release on [noto-cjk Releases](https://github.com/notofonts/noto-cjk/releases), attachment `15_NotoSerifTC.zip` |
| Korean body text | Noto Serif KR | `NotoSerifKR-Regular.otf` | Attachment `13_NotoSerifKR.zip` of the same release |
| Titles, counter glyphs, floating text, banners | LXGW WenKai TC | `LXGWWenKaiTC-Bold.ttf` | Download family on the [Google Fonts](https://fonts.google.com/specimen/LXGW+WenKai+TC) page |

The numbers in front of the attachment names follow the release page.

Put the license texts here too, renamed so they do not overwrite each other: save `LICENSE` from the Noto archive as `LICENSE-NotoSerifCJK.txt`, and `OFL.txt` from the LXGW WenKai archive as `OFL-LXGWWenKaiTC.txt`.

Noto Serif TC / KR can also be downloaded from Google Fonts, but there they come as `.ttf` (use `NotoSerifTC-Regular.ttf` from the `static/` folder of the archive); in that case change the extension in the `fonts` field to `.ttf`.

The font files are all far below GitHub's 100 MB per-file limit and are committed directly.
