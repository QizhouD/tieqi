"""Liaodong cavalry and a mounted Ming commander with a trimmed cape."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import main

if __name__ == "__main__":
    main("ming", ("cavalry", "general"))
