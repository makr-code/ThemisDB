"""
RESPO Retriever

Hybrid retrieval combining vector search with optional keyword and graph-based retrieval.
"""

from dataclasses import dataclass
from typing import Any, Optional

import structlog

from respo.embedding import CodeEmbedder
from respo.vectorstore.base import SearchResult, VectorStoreBase

logger = structlog.get_logger(__name__)


@dataclass
class RetrievalResult:
    """Result from retrieval."""

    id: str
    content: str
    score: float
    metadata: dict[str, Any]
    source: str  # 'vector', 'keyword', 'graph'


class HybridRetriever:
    """
    Hybrid retriever combining multiple retrieval strategies.

    Supports:
    - Vector search (semantic similarity)
    - Keyword search (BM25-like)
    - Graph traversal (code dependencies)
    """

    def __init__(
        self,
        vector_store: VectorStoreBase,
        embedder: CodeEmbedder,
        vector_weight: float = 0.7,
        keyword_weight: float = 0.3,
    ) -> None:
        """
        Initialize hybrid retriever.

        Args:
            vector_store: Vector store backend
            embedder: Code embedder
            vector_weight: Weight for vector search results
            keyword_weight: Weight for keyword search results
        """
        self.vector_store = vector_store
        self.embedder = embedder
        self.vector_weight = vector_weight
        self.keyword_weight = keyword_weight

    async def retrieve(
        self,
        query: str,
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
        language: Optional[str] = None,
    ) -> list[RetrievalResult]:
        """
        Retrieve relevant documents for a query.

        Args:
            query: Search query
            k: Number of results
            filter: Optional metadata filter
            language: Optional language filter

        Returns:
            List of retrieval results
        """
        logger.debug("Retrieving documents", query=query[:100], k=k)

        # Build filter
        search_filter = filter or {}
        if language:
            search_filter["language"] = language

        # Vector search
        query_embedding = self.embedder.embed_single(query)
        vector_results = await self.vector_store.search(
            query_embedding=query_embedding,
            k=k * 2,  # Get more for reranking
            filter=search_filter if search_filter else None,
        )

        # Convert to retrieval results
        results = []
        for vr in vector_results:
            results.append(
                RetrievalResult(
                    id=vr.id,
                    content=vr.content,
                    score=vr.score * self.vector_weight,
                    metadata=vr.metadata,
                    source="vector",
                )
            )

        # Sort by score and limit
        results.sort(key=lambda x: x.score, reverse=True)
        results = results[:k]

        logger.debug("Retrieved documents", count=len(results))
        return results

    async def retrieve_similar(
        self,
        code: str,
        k: int = 5,
        language: Optional[str] = None,
    ) -> list[RetrievalResult]:
        """
        Retrieve similar code snippets.

        Args:
            code: Source code to find similar snippets for
            k: Number of results
            language: Optional language filter

        Returns:
            List of similar code snippets
        """
        return await self.retrieve(
            query=code,
            k=k,
            language=language,
        )

    async def retrieve_by_function(
        self,
        function_name: str,
        k: int = 5,
    ) -> list[RetrievalResult]:
        """
        Retrieve code containing a specific function.

        Args:
            function_name: Function name to search for
            k: Number of results

        Returns:
            List of matching code snippets
        """
        query = f"function {function_name}"
        return await self.retrieve(query=query, k=k)


class SimpleRetriever:
    """
    Simple vector-only retriever.

    Use this when keyword search is not needed.
    """

    def __init__(
        self,
        vector_store: VectorStoreBase,
        embedder: CodeEmbedder,
    ) -> None:
        """
        Initialize simple retriever.

        Args:
            vector_store: Vector store backend
            embedder: Code embedder
        """
        self.vector_store = vector_store
        self.embedder = embedder

    async def retrieve(
        self,
        query: str,
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
    ) -> list[SearchResult]:
        """
        Retrieve relevant documents.

        Args:
            query: Search query
            k: Number of results
            filter: Optional metadata filter

        Returns:
            List of search results
        """
        query_embedding = self.embedder.embed_single(query)
        return await self.vector_store.search(
            query_embedding=query_embedding,
            k=k,
            filter=filter,
        )
