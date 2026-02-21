"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""ThemisDB LLM Python SDK - Client library for LLM operations."""

__version__ = '1.3.0'

from .client import ThemisLLMClient
from .models import (
    InferenceRequest,
    InferenceResponse,
    RAGRequest,
    RAGResponse,
    ModelInfo
)
from .exceptions import (
    ThemisError,
    ThemisAuthError,
    ThemisAPIError,
    ThemisConnectionError
)

__all__ = [
    'ThemisLLMClient',
    'InferenceRequest',
    'InferenceResponse',
    'RAGRequest',
    'RAGResponse',
    'ModelInfo',
    'ThemisError',
    'ThemisAuthError',
    'ThemisAPIError',
    'ThemisConnectionError',
]
