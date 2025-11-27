"""
Qdrant Vector Store Implementation

High-performance vector database backend for RESPO.
"""

from typing import Any, Optional

from respo.vectorstore.base import Document, SearchResult, VectorStoreBase


class QdrantVectorStore(VectorStoreBase):
    """
    Qdrant vector store implementation.

    Requires: pip install qdrant-client
    """

    def __init__(
        self,
        url: str = "http://localhost:6333",
        collection_name: str = "respo_code",
        api_key: Optional[str] = None,
        prefer_grpc: bool = True,
    ):
        """
        Initialize Qdrant vector store.

        Args:
            url: Qdrant server URL
            collection_name: Name of the collection
            api_key: Optional API key for authentication
            prefer_grpc: Use gRPC for better performance
        """
        self.url = url
        self.collection_name = collection_name
        self.api_key = api_key
        self.prefer_grpc = prefer_grpc
        self._client = None
        self._dimension = None

    async def initialize(self, dimension: int = 768) -> None:
        """Initialize the Qdrant client and create collection if needed."""
        try:
            from qdrant_client import QdrantClient
            from qdrant_client.http import models
        except ImportError:
            raise ImportError(
                "qdrant-client is required. Install with: pip install qdrant-client"
            )

        self._dimension = dimension

        # Initialize client
        self._client = QdrantClient(
            url=self.url,
            api_key=self.api_key,
            prefer_grpc=self.prefer_grpc,
        )

        # Check if collection exists
        collections = self._client.get_collections()
        exists = any(c.name == self.collection_name for c in collections.collections)

        if not exists:
            # Create collection with optimized settings for code embeddings
            self._client.create_collection(
                collection_name=self.collection_name,
                vectors_config=models.VectorParams(
                    size=dimension,
                    distance=models.Distance.COSINE,
                    on_disk=True,  # Enable disk storage for large collections
                ),
                optimizers_config=models.OptimizersConfigDiff(
                    indexing_threshold=10000,
                    memmap_threshold=10000,
                ),
                hnsw_config=models.HnswConfigDiff(
                    m=16,
                    ef_construct=100,
                    full_scan_threshold=10000,
                ),
            )

    async def add_documents(
        self,
        documents: list[Document],
        embeddings: list[list[float]],
    ) -> list[str]:
        """Add documents with embeddings to Qdrant."""
        from qdrant_client.http import models

        if not self._client:
            raise RuntimeError("Vector store not initialized. Call initialize() first.")

        if len(documents) != len(embeddings):
            raise ValueError("Documents and embeddings must have the same length")

        points = []
        ids = []

        for doc, embedding in zip(documents, embeddings):
            doc_id = doc.id
            ids.append(doc_id)

            # Prepare payload (metadata)
            payload = {
                "content": doc.content,
                **doc.metadata,
            }

            points.append(
                models.PointStruct(
                    id=doc_id,
                    vector=embedding,
                    payload=payload,
                )
            )

        # Batch upsert
        self._client.upsert(
            collection_name=self.collection_name,
            points=points,
            wait=True,
        )

        return ids

    async def search(
        self,
        query_embedding: list[float],
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
    ) -> list[SearchResult]:
        """Search for similar documents in Qdrant."""
        from qdrant_client.http import models

        if not self._client:
            raise RuntimeError("Vector store not initialized. Call initialize() first.")

        # Build filter conditions
        query_filter = None
        if filter:
            conditions = []
            for key, value in filter.items():
                if isinstance(value, list):
                    conditions.append(
                        models.FieldCondition(
                            key=key,
                            match=models.MatchAny(any=value),
                        )
                    )
                else:
                    conditions.append(
                        models.FieldCondition(
                            key=key,
                            match=models.MatchValue(value=value),
                        )
                    )

            if conditions:
                query_filter = models.Filter(must=conditions)

        # Search
        results = self._client.search(
            collection_name=self.collection_name,
            query_vector=query_embedding,
            limit=k,
            query_filter=query_filter,
            with_payload=True,
        )

        # Convert to SearchResult
        search_results = []
        for hit in results:
            payload = hit.payload or {}
            content = payload.pop("content", "")

            search_results.append(
                SearchResult(
                    id=str(hit.id),
                    content=content,
                    score=hit.score,
                    metadata=payload,
                )
            )

        return search_results

    async def delete(self, ids: list[str]) -> None:
        """Delete documents by ID from Qdrant."""
        from qdrant_client.http import models

        if not self._client:
            raise RuntimeError("Vector store not initialized. Call initialize() first.")

        self._client.delete(
            collection_name=self.collection_name,
            points_selector=models.PointIdsList(points=ids),
            wait=True,
        )

    async def get_stats(self) -> dict[str, Any]:
        """Get collection statistics."""
        if not self._client:
            return {"status": "not_initialized"}

        try:
            info = self._client.get_collection(self.collection_name)
            return {
                "collection": self.collection_name,
                "vectors_count": info.vectors_count,
                "points_count": info.points_count,
                "indexed_vectors_count": info.indexed_vectors_count,
                "status": info.status.name,
                "disk_data_size": info.disk_data_size,
                "ram_data_size": info.ram_data_size,
            }
        except Exception as e:
            return {"status": "error", "message": str(e)}

    async def close(self) -> None:
        """Close the Qdrant client."""
        if self._client:
            self._client.close()
            self._client = None
