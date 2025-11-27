"""
RESPO Vector Store Base Interface

Abstract base class for pluggable vector store backends.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, Optional


@dataclass
class SearchResult:
    """Result from a vector search."""

    id: str
    content: str
    score: float
    metadata: dict[str, Any]


class VectorStoreBase(ABC):
    """
    Abstract base class for vector store implementations.

    Implementations should provide:
    - add: Add documents with embeddings
    - search: Search for similar documents
    - delete: Delete documents by ID
    - get: Get document by ID
    """

    @abstractmethod
    async def add(
        self,
        ids: list[str],
        embeddings: list[list[float]],
        documents: list[str],
        metadatas: Optional[list[dict[str, Any]]] = None,
    ) -> None:
        """
        Add documents with their embeddings to the store.

        Args:
            ids: Document IDs
            embeddings: Document embeddings
            documents: Document contents
            metadatas: Optional metadata for each document
        """
        pass

    @abstractmethod
    async def search(
        self,
        query_embedding: list[float],
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
    ) -> list[SearchResult]:
        """
        Search for similar documents.

        Args:
            query_embedding: Query vector
            k: Number of results
            filter: Optional metadata filter

        Returns:
            List of search results
        """
        pass

    @abstractmethod
    async def delete(self, ids: list[str]) -> None:
        """
        Delete documents by ID.

        Args:
            ids: Document IDs to delete
        """
        pass

    @abstractmethod
    async def get(self, id: str) -> Optional[SearchResult]:
        """
        Get a document by ID.

        Args:
            id: Document ID

        Returns:
            Document or None if not found
        """
        pass

    @abstractmethod
    async def count(self) -> int:
        """Get total document count."""
        pass

    async def close(self) -> None:
        """Close the connection (optional)."""
        pass


class VectorStoreFactory:
    """Factory for creating vector store instances."""

    _backends: dict[str, type[VectorStoreBase]] = {}

    @classmethod
    def register(cls, name: str, backend_class: type[VectorStoreBase]) -> None:
        """Register a vector store backend."""
        cls._backends[name.lower()] = backend_class

    @classmethod
    def create(cls, name: str, **kwargs: Any) -> VectorStoreBase:
        """
        Create a vector store instance.

        Args:
            name: Backend name (chroma, qdrant, weaviate, themis)
            **kwargs: Backend-specific configuration

        Returns:
            Vector store instance

        Raises:
            ValueError: If backend is not registered
        """
        name = name.lower()
        if name not in cls._backends:
            available = ", ".join(cls._backends.keys())
            raise ValueError(
                f"Unknown vector store backend: {name}. Available: {available}"
            )
        return cls._backends[name](**kwargs)

    @classmethod
    def available_backends(cls) -> list[str]:
        """Get list of registered backends."""
        return list(cls._backends.keys())


# Register built-in backends on import
def _register_backends() -> None:
    """Register available backends."""
    try:
        from respo.vectorstore.chroma import ChromaVectorStore
        VectorStoreFactory.register("chroma", ChromaVectorStore)
    except ImportError:
        pass

    try:
        from respo.vectorstore.qdrant import QdrantVectorStore
        VectorStoreFactory.register("qdrant", QdrantVectorStore)
    except ImportError:
        pass
    
    try:
        from respo.vectorstore.themis import ThemisVectorStore
        VectorStoreFactory.register("themis", ThemisVectorStore)
    except ImportError:
        pass


_register_backends()
