"""
RESPO ChromaDB Vector Store Backend

Default local vector store implementation using ChromaDB.
"""

import os
from typing import Any, Optional

import chromadb
from chromadb.config import Settings

from respo.vectorstore.base import SearchResult, VectorStoreBase


class ChromaVectorStore(VectorStoreBase):
    """
    ChromaDB vector store implementation.

    This is the default backend - no external server required.
    """

    def __init__(
        self,
        persist_directory: Optional[str] = None,
        collection_name: str = "respo_code",
        **kwargs: Any,
    ) -> None:
        """
        Initialize ChromaDB vector store.

        Args:
            persist_directory: Directory for persistence (default: in-memory)
            collection_name: Collection name
            **kwargs: Additional ChromaDB settings
        """
        persist_dir = persist_directory or os.environ.get(
            "CHROMA_PERSIST_DIR", "./data/chroma"
        )

        settings = Settings(
            persist_directory=persist_dir,
            anonymized_telemetry=False,
        )

        self._client = chromadb.Client(settings)
        self._collection = self._client.get_or_create_collection(
            name=collection_name,
            metadata={"hnsw:space": "cosine"},
        )

    async def add(
        self,
        ids: list[str],
        embeddings: list[list[float]],
        documents: list[str],
        metadatas: Optional[list[dict[str, Any]]] = None,
    ) -> None:
        """Add documents with their embeddings."""
        self._collection.add(
            ids=ids,
            embeddings=embeddings,
            documents=documents,
            metadatas=metadatas,
        )

    async def search(
        self,
        query_embedding: list[float],
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
    ) -> list[SearchResult]:
        """Search for similar documents."""
        results = self._collection.query(
            query_embeddings=[query_embedding],
            n_results=k,
            where=filter,
            include=["documents", "metadatas", "distances"],
        )

        search_results = []
        if results["ids"] and results["ids"][0]:
            for i, doc_id in enumerate(results["ids"][0]):
                # ChromaDB returns distance, convert to similarity score
                distance = results["distances"][0][i] if results["distances"] else 0
                score = 1 - distance  # cosine distance to similarity

                search_results.append(
                    SearchResult(
                        id=doc_id,
                        content=results["documents"][0][i] if results["documents"] else "",
                        score=score,
                        metadata=results["metadatas"][0][i] if results["metadatas"] else {},
                    )
                )

        return search_results

    async def delete(self, ids: list[str]) -> None:
        """Delete documents by ID."""
        self._collection.delete(ids=ids)

    async def get(self, id: str) -> Optional[SearchResult]:
        """Get a document by ID."""
        result = self._collection.get(
            ids=[id],
            include=["documents", "metadatas"],
        )

        if result["ids"] and result["ids"]:
            return SearchResult(
                id=result["ids"][0],
                content=result["documents"][0] if result["documents"] else "",
                score=1.0,
                metadata=result["metadatas"][0] if result["metadatas"] else {},
            )

        return None

    async def count(self) -> int:
        """Get total document count."""
        return self._collection.count()

    async def close(self) -> None:
        """Close the connection."""
        pass  # ChromaDB handles cleanup automatically
