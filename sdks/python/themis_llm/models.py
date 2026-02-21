"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""Data models for ThemisDB LLM SDK."""

from dataclasses import dataclass
from typing import List, Optional, Dict, Any


@dataclass
class InferenceRequest:
    """Request for text inference."""
    prompt: str
    model: Optional[str] = None
    lora: Optional[str] = None
    max_tokens: int = 512
    temperature: float = 0.7
    top_p: float = 0.9


@dataclass
class InferenceResponse:
    """Response from text inference."""
    text: str
    model: str
    tokens_generated: int
    latency_ms: float


@dataclass
class RAGRequest:
    """Request for RAG inference."""
    query: str
    collection: str
    top_k: int = 5
    lora: Optional[str] = None
    similarity_threshold: float = 0.7


@dataclass
class RAGResponse:
    """Response from RAG inference."""
    text: str
    documents: List[Dict[str, Any]]
    model: str
    tokens_generated: int
    latency_ms: float


@dataclass
class ModelInfo:
    """Information about a model."""
    model_id: str
    path: str
    loaded: bool
    vram_usage_mb: Optional[float] = None
    context_length: Optional[int] = None
