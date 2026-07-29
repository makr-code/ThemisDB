"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     86                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • edfcc1a70a  2026-02-28  feat(chimera): implement BenchmarkDashboard for result ag... ║
    • 8964bef5bc  2026-02-28  feat(chimera): implement CapabilityMatrix for adapter cap... ║
    • c39051d13c  2026-02-27  feat(chimera): implement unified benchmark harness ║
    • f2104380d2  2026-02-26  feat(chimera): add benchmark result normalization and sco... ║
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
from .scoring import BenchmarkScorer, MetricConfig, MetricDirection, NormalizationMethod, NormalizedScore, CompositeScore
from .benchmark_harness import BenchmarkHarness, WorkloadDefinition, HarnessConfig, WorkloadResult
from .capability_matrix import CapabilityMatrix, CapableAdapter
from .dashboard import BenchmarkDashboard

__all__ = [
    "ChimeraReporter",
    "StatisticalAnalyzer",
    "ColorBlindPalette",
    "CitationManager",
    "BenchmarkScorer",
    "MetricConfig",
    "MetricDirection",
    "NormalizationMethod",
    "NormalizedScore",
    "CompositeScore",
    "BenchmarkHarness",
    "WorkloadDefinition",
    "HarnessConfig",
    "WorkloadResult",
    "CapabilityMatrix",
    "CapableAdapter",
    "BenchmarkDashboard",
]
