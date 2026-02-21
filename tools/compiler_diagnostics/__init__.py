"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:00:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Compiler Diagnostics Package

This package provides tools for analyzing compiler errors, linker issues,
and cross-platform compatibility problems in ThemisDB.

Modules:
    diagnostic_scanner: Parse and categorize compiler error logs
    source_audit: Analyze source files for common issues
    symbol_checker: Validate symbol visibility and exports
    issue_tracker: Integrate with CI/CD for automated tracking
"""

__version__ = "1.0.0"
__author__ = "ThemisDB Team"

from pathlib import Path

# Package root directory
PACKAGE_ROOT = Path(__file__).parent
THEMIS_ROOT = PACKAGE_ROOT.parent.parent

# Error categorization
ERROR_CATEGORIES = {
    "SYMBOL_VISIBILITY": "Symbol visibility and export issues",
    "LINKER": "Linker errors and undefined references",
    "ABI": "ABI compatibility issues",
    "PLATFORM_SPECIFIC": "Platform-specific code problems",
    "TEMPLATE": "Template instantiation issues",
    "INTRINSICS": "Compiler intrinsics without fallbacks",
    "STANDARD_LIBRARY": "Standard library compatibility",
    "WARNING": "Compiler warnings",
    "OTHER": "Uncategorized errors"
}

# Supported platforms
PLATFORMS = {
    "windows": ["msvc", "clang-cl"],
    "linux": ["gcc", "clang"],
    "macos": ["clang", "apple-clang"],
    "arm": ["gcc", "clang"]
}

# Export main utilities
__all__ = [
    "ERROR_CATEGORIES",
    "PLATFORMS",
    "PACKAGE_ROOT",
    "THEMIS_ROOT"
]
