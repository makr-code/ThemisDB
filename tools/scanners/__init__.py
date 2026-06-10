"""ThemisDB Gap Scanner package.

Uniform default scanner: gs3_step00_uniform_full.py
"""

import sys
from pathlib import Path
from typing import Dict, List

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
    from tools.gap_scanner_v3 import UnifiedGapScannerV3 as LegacyUnifiedGapScannerV3
except ImportError:
    LegacyUnifiedGapScannerV3 = None


class UnifiedGapScanner:
    """Backward-compatible wrapper around the uniform scanner pipeline."""

    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def run_complete_scan(self) -> Dict[str, Dict]:
        from tools.scanners.gs3_step00_uniform_full import UniformFullScanner

        registry = ScannerRegistry()
        registry.register(UniformFullScanner())
        pipeline = GapScannerPipeline(registry)

        scan_root = self.repo_root
        if not (scan_root / 'src').exists():
            scan_root = self.repo_root / 'src'

        gaps = pipeline.execute(str(scan_root), verbose=False)
        pipeline.export_json(self.output_dir / 'gap_scan_results.json')
        return self._aggregate_by_module(gaps)

    def _aggregate_by_module(self, gaps: List[Gap]) -> Dict[str, Dict]:
        modules: Dict[str, Dict] = {}
        for gap in gaps:
            module = self._module_from_file(gap.file)
            if module not in modules:
                modules[module] = {
                    'total': 0,
                    'severity_critical': 0,
                    'severity_high': 0,
                    'severity_medium': 0,
                    'gaps_by_file': {},
                }

            modules[module]['total'] += 1
            sev = str(gap.severity).upper()
            if sev == 'CRITICAL':
                modules[module]['severity_critical'] += 1
            elif sev == 'HIGH':
                modules[module]['severity_high'] += 1
            elif sev == 'MEDIUM':
                modules[module]['severity_medium'] += 1

            file_key = gap.file
            if file_key not in modules[module]['gaps_by_file']:
                modules[module]['gaps_by_file'][file_key] = []
            modules[module]['gaps_by_file'][file_key].append(gap.to_dict())

        return modules

    def _module_from_file(self, file_path: str) -> str:
        normalized = str(file_path).replace('\\', '/').lstrip('./')
        parts = Path(normalized).parts
        if not parts:
            return 'unknown'
        if parts[0] == 'src' and len(parts) > 1:
            return parts[1]
        if parts[0] in {'include', 'tests', 'benchmarks', 'internal'}:
            return parts[0]
        return parts[0]

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
    "LegacyUnifiedGapScannerV3",
]