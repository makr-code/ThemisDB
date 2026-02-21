"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
VCC Base Adapter Library

Common functionality shared across all VCC (Virtual Compliance Center) adapters
for connecting to ThemisDB backend.

This library provides:
- ThemisDB HTTP client wrapper
- Common data transformation utilities
- Shared configuration management
- Error handling and logging utilities

Direct HTTP connections to ThemisDB - no external framework dependencies required.
"""

from .themis_client import ThemisVCCClient
from .config import VCCAdapterConfig
from .processors import BaseProcessor, TextProcessor
from .utils import setup_logging, validate_themis_connection

__version__ = "0.1.0"
__all__ = [
    "ThemisVCCClient",
    "VCCAdapterConfig", 
    "BaseProcessor",
    "TextProcessor",
    "setup_logging",
    "validate_themis_connection",
]
