"""Canonical wrapper for the legacy implementation-gap scanner."""

import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TOOLS_DIR = _REPO_ROOT / "tools"
for _path in (str(_REPO_ROOT), str(_TOOLS_DIR)):
	if _path not in sys.path:
		sys.path.insert(0, _path)

from tools.gap_scanner import GapScanner

__all__ = ["GapScanner"]
