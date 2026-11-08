"""Padded Ming infantry: spear, matchlock, and the sword-and-shield atlas slot."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import main

if __name__ == "__main__":
    main("ming", ("spear", "gun", "samurai"))
