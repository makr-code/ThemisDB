"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
CHIMERA Suite: Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment

"Benchmark the Unbenchmarkable"

A scientifically rigorous, vendor-neutral framework for benchmark reporting and 
visualization that complies with IEEE/ACM standards. Designed specifically for 
evaluating hybrid multi-model databases with native AI/LLM integration.

Key Features:
- Multi-model workload evaluation (Graph, Vector, Relational, Document)
- AI/LLM benchmark support (inference, LoRA, RAG)
- Color-blind friendly visualization palettes (Okabe-Ito, Paul Tol)
- Statistical validation (t-tests, Mann-Whitney-U, Cohen's d, confidence intervals)
- Vendor-neutral sorting and presentation
- IEEE-compliant citations and methodology
- Comprehensive outlier detection and removal
- Transparent methodology disclosure

Usage:
    from chimera import ChimeraReporter
    
    reporter = ChimeraReporter()
    reporter.add_system_results("SystemA", "Query Throughput", "ops/s", results_a)
    reporter.add_system_results("SystemB", "Query Throughput", "ops/s", results_b)
    reporter.generate_html_report("CHIMERA_report.html")
"""

__version__ = "1.0.0"
__author__ = "CHIMERA Development Team"
__license__ = "MIT"

from .reporter import ChimeraReporter
from .statistics import StatisticalAnalyzer
from .colors import ColorBlindPalette
from .citations import CitationManager

__all__ = [
    "ChimeraReporter",
    "StatisticalAnalyzer", 
    "ColorBlindPalette",
    "CitationManager"
]
