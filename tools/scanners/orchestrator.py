"""Canonical wrapper for the v3 unified scanner orchestrator."""

import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TOOLS_DIR = _REPO_ROOT / "tools"
for _path in (str(_REPO_ROOT), str(_TOOLS_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from tools.gs3_orchestrator import main
from tools.scanners import UnifiedGapScanner

__all__ = ["UnifiedGapScanner", "main"]


if __name__ == "__main__":
    raise SystemExit(main())