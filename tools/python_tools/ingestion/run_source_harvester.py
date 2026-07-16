from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent / "source_harvester"
sys.path.insert(0, str(ROOT.parent))

from source_harvester.cli import main

if __name__ == "__main__":
    raise SystemExit(main())
