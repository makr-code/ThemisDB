"""
RESPO RAG Module

Complete RAG pipeline for code assistance.
"""

from respo.rag.pipeline import RAGPipeline, RAGResponse
from respo.rag.prompts import build_chat_prompt, format_context, get_system_prompt
from respo.rag.reranker import CrossEncoderReranker, NoOpReranker, RerankResult
from respo.rag.retriever import HybridRetriever, RetrievalResult, SimpleRetriever

__all__ = [
    "RAGPipeline",
    "RAGResponse",
    "HybridRetriever",
    "SimpleRetriever",
    "RetrievalResult",
    "CrossEncoderReranker",
    "NoOpReranker",
    "RerankResult",
    "get_system_prompt",
    "format_context",
    "build_chat_prompt",
]
