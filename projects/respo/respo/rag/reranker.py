"""
RESPO Reranker

Cross-encoder reranking for improved retrieval precision.
"""

from dataclasses import dataclass
from typing import Optional

import structlog

logger = structlog.get_logger(__name__)


@dataclass
class RerankResult:
    """Result from reranking."""

    id: str
    content: str
    score: float
    original_score: float
    metadata: dict


class CrossEncoderReranker:
    """
    Cross-encoder reranker using sentence-transformers.

    Improves retrieval precision by scoring query-document pairs.
    """

    def __init__(
        self,
        model_name: str = "cross-encoder/ms-marco-MiniLM-L-6-v2",
        device: Optional[str] = None,
    ) -> None:
        """
        Initialize cross-encoder reranker.

        Args:
            model_name: Cross-encoder model name
            device: Device (cuda, cpu, mps)
        """
        from sentence_transformers import CrossEncoder

        self.model_name = model_name

        logger.info("Loading reranker model", model=model_name)
        self._model = CrossEncoder(model_name, device=device)
        logger.info("Reranker model loaded")

    def rerank(
        self,
        query: str,
        documents: list[dict],
        top_k: int = 10,
    ) -> list[RerankResult]:
        """
        Rerank documents based on query relevance.

        Args:
            query: Search query
            documents: List of documents with 'id', 'content', 'score', 'metadata'
            top_k: Number of top results to return

        Returns:
            Reranked results
        """
        if not documents:
            return []

        logger.debug("Reranking documents", count=len(documents), top_k=top_k)

        # Prepare query-document pairs
        pairs = [(query, doc["content"]) for doc in documents]

        # Score pairs
        scores = self._model.predict(pairs)

        # Build results
        results = []
        for doc, score in zip(documents, scores):
            results.append(
                RerankResult(
                    id=doc["id"],
                    content=doc["content"],
                    score=float(score),
                    original_score=doc.get("score", 0.0),
                    metadata=doc.get("metadata", {}),
                )
            )

        # Sort by reranked score
        results.sort(key=lambda x: x.score, reverse=True)

        logger.debug("Reranking complete", top_score=results[0].score if results else 0)
        return results[:top_k]


class NoOpReranker:
    """
    No-operation reranker (passthrough).

    Use when reranking is disabled.
    """

    def rerank(
        self,
        query: str,
        documents: list[dict],
        top_k: int = 10,
    ) -> list[RerankResult]:
        """Pass through documents without reranking."""
        results = []
        for doc in documents[:top_k]:
            results.append(
                RerankResult(
                    id=doc["id"],
                    content=doc["content"],
                    score=doc.get("score", 0.0),
                    original_score=doc.get("score", 0.0),
                    metadata=doc.get("metadata", {}),
                )
            )
        return results
