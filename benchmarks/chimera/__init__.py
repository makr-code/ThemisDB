"""
CHIMERA Suite: Vendor-Neutral Reporting and Plotting Framework

This module provides a scientifically rigorous, vendor-neutral framework for
benchmark reporting and visualization that complies with IEEE/ACM standards.

Key Features:
- Color-blind friendly visualization palettes
- Statistical validation (t-tests, Mann-Whitney-U, Cohen's d)
- Vendor-neutral sorting and presentation
- IEEE-compliant citations
- Comprehensive outlier detection and removal
- Transparent methodology disclosure

Usage:
    from chimera import ChimeraReporter
    
    reporter = ChimeraReporter()
    reporter.add_system_results("SystemA", results_a)
    reporter.add_system_results("SystemB", results_b)
    reporter.generate_html_report("report.html")
"""

__version__ = "1.0.0"
__author__ = "CHIMERA Suite Contributors"
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
