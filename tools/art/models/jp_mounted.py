"""Mounted samurai and a generic daimyo in kabuto and jinbaori."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import main

if __name__ == "__main__":
    main("jp", ("cavalry", "general"))
