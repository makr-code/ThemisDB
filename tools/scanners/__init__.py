"""
ThemisDB Gap Scanner V3 — Unified Pipeline Architecture (Flat Structure)

File naming convention (flat structure in tools/scanners/):
  gs3_step00_<focus>.py    (Tier 0: Baseline, ultra-fast keyword matching)
  gs3_step01_<focus>.py    (Tier 1: Basic code quality, Phase 1-4)
  gs3_step02_<focus>.py    (Tier 2: Specialized patterns)
  gs3_step03_<focus>.py    (Tier 3: Hardening & Security, Phase 11)
  gs3_step04_<focus>.py    (Tier 4: Semantic & FP Filters, Wave 5-6)

Legacy imports (backwards compatibility):
  LegacyGapScanner: Original gap_scanner.py
  ContextualGapScanner: gap_scanner_v2.py with context
  UnifiedGapScanner: gap_scanner_v3.py orchestrator (broken, deprecated)
"""

import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TOOLS_DIR = _REPO_ROOT / "tools"
for _path in (str(_REPO_ROOT), str(_TOOLS_DIR)):
    if _path not in sys.path:
        sys.path.insert(0, _path)

# New OOP base (always import)
from tools.gs3_base_scanner import (
    BaseGapScanner,
    Gap,
    ScannerPriority,
    ScannerRegistry,
    GapScannerPipeline,
    FPFilter,
)

# Legacy imports (for backwards compatibility)
try:
    from tools.gap_scanner import GapScanner as LegacyGapScanner
except ImportError:
    LegacyGapScanner = None

try:
    from tools.gap_scanner_v2 import EnhancedGapScanner as ContextualGapScanner
except ImportError:
    ContextualGapScanner = None

try:
    from tools.gap_scanner_v3 import UnifiedGapScannerV3 as UnifiedGapScanner
except ImportError:
    UnifiedGapScanner = None

__all__ = [
    # New OOP architecture
    "BaseGapScanner",
    "Gap",
    "ScannerPriority",
    "ScannerRegistry",
    "GapScannerPipeline",
    "FPFilter",
    # Legacy (deprecated)
    "LegacyGapScanner",
    "ContextualGapScanner",
    "UnifiedGapScanner",
]