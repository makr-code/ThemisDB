"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
