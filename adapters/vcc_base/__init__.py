"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:31:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
