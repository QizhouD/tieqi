# 字体

需要三款字体，都是 SIL Open Font License 1.1，可以随游戏分发。放进本目录的文件名要和 `data/strings/*.json` 里的 `fonts` 字段一致。

| 用途 | 字体 | 放进本目录的文件 | 下载 |
|---|---|---|---|
| 繁体中文、英文正文 | 思源宋体 TC（Noto Serif TC） | `NotoSerifTC-Regular.otf` | [noto-cjk Releases](https://github.com/notofonts/noto-cjk/releases) 里最新的 Noto Serif CJK 版本，附件 `15_NotoSerifTC.zip` |
| 韩文正文 | 思源宋体 KR（Noto Serif KR） | `NotoSerifKR-Regular.otf` | 同一版本的附件 `13_NotoSerifKR.zip` |
| 标题、棋子字、飘字、横幅 | 霞鹜文楷 TC（LXGW WenKai TC） | `LXGWWenKaiTC-Bold.ttf` | [Google Fonts](https://fonts.google.com/specimen/LXGW+WenKai+TC) 页面的 Download family |

附件名前的编号以发布页为准。

许可证文本一并放进来，改名以免互相覆盖：Noto 压缩包里的 `LICENSE` 存为 `LICENSE-NotoSerifCJK.txt`，霞鹜文楷压缩包里的 `OFL.txt` 存为 `OFL-LXGWWenKaiTC.txt`。

Noto Serif TC / KR 也可以从 Google Fonts 下载，但那里给的是 `.ttf`（用压缩包 `static/` 目录里的 `NotoSerifTC-Regular.ttf`），这时要把 `fonts` 字段里的扩展名改成 `.ttf`。

字体文件都远小于 GitHub 单文件 100 MB 的上限，直接提交即可。
