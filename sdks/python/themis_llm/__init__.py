"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
