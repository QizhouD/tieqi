"""Yari and teppo ashigaru in jingasa, and samurai in kabuto with sashimono."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import main

if __name__ == "__main__":
    main("jp", ("spear", "gun", "samurai"))
