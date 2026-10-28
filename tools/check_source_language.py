import pathlib
import re
import sys

CHARACTERS = re.compile(
    "["
    "\u3400-\u4dbf"
    "\u4e00-\u9fff"
    "\uf900-\ufaff"
    "\u3040-\u30ff"
    "\uac00-\ud7af"
    "\u3000-\u303f"
    "\uff01-\uff60"
    "\uffe0-\uffee"
    "]"
)

ROOTS = ["src", "tests", "tools", "cmake", ".github/workflows"]
FILES = ["CMakeLists.txt", ".gitignore"]
SKIP_PARTS = {"node_modules", "work"}
SKIP_FILES = {pathlib.Path("tools/parity/anchors.json")}
TEXT_SUFFIXES = {
    ".h",
    ".hpp",
    ".cpp",
    ".cc",
    ".c",
    ".cmake",
    ".txt",
    ".yml",
    ".yaml",
    ".js",
    ".mjs",
    ".json",
    ".md",
    "",
}


def skipped(path):
    if path in SKIP_FILES:
        return True
    return any(part in SKIP_PARTS for part in path.parts)


def scan(path, failures):
    if skipped(path) or not path.is_file():
        return
    if path.suffix.lower() not in TEXT_SUFFIXES and path.name not in FILES:
        return
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        failures.append(f"{path}: not utf-8")
        return
    for number, line in enumerate(text.splitlines(), 1):
        if CHARACTERS.search(line):
            failures.append(f"{path}:{number}: {line.strip()}")


def main():
    failures = []
    for root_name in ROOTS:
        root = pathlib.Path(root_name)
        if not root.exists():
            continue
        for path in root.rglob("*"):
            scan(path, failures)
    for name in FILES:
        scan(pathlib.Path(name), failures)
    if failures:
        print("\n".join(failures))
        print(f"{len(failures)} source line(s) contain CJK text")
        return 1
    print("source language ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
