"""
ThemisDB Vector Store Backend for RESPO

Leverages ThemisDB's unique capabilities:
- HNSW vector search
- Graph traversals for code dependencies
- Hybrid search (vector + keyword + graph)
- Multi-model queries
"""

from dataclasses import dataclass, field
from typing import Any, Optional
from urllib.parse import urljoin

import httpx
import structlog

from respo.vectorstore.base import SearchResult, VectorStoreBase

logger = structlog.get_logger(__name__)


@dataclass
class ThemisConfig:
    """Configuration for ThemisDB connection."""
    
    url: str = "http://localhost:8765"
    collection: str = "respo_code"
    timeout: float = 30.0
    
    # Vector index settings
    vector_dimension: int = 768
    hnsw_m: int = 16
    hnsw_ef_construction: int = 200
    hnsw_ef_search: int = 100
    
    # Graph settings
    enable_graph: bool = True
    graph_namespace: str = "code_graph"
    
    # Hybrid search weights
    vector_weight: float = 0.6
    keyword_weight: float = 0.2
    graph_weight: float = 0.2


@dataclass
class GraphEdge:
    """Represents a graph edge (relationship between code entities)."""
    
    source_id: str
    target_id: str
    edge_type: str  # imports, calls, inherits, implements, uses
    properties: dict[str, Any] = field(default_factory=dict)


@dataclass
class GraphNode:
    """Represents a graph node (code entity)."""
    
    id: str
    node_type: str  # function, class, module, method, variable
    name: str
    properties: dict[str, Any] = field(default_factory=dict)


class ThemisVectorStore(VectorStoreBase):
    """
    ThemisDB backend with Graph and Hybrid Search support.
    
    Features:
    - HNSW vector search for code embeddings
    - Graph storage for code relationships (imports, calls, inherits)
    - Hybrid search combining vector + keyword + graph scores
    - Traversal queries for dependency analysis
    """
    
    def __init__(self, config: Optional[ThemisConfig] = None) -> None:
        """
        Initialize ThemisDB connection.
        
        Args:
            config: ThemisDB configuration
        """
        self.config = config or ThemisConfig()
        self._client: Optional[httpx.AsyncClient] = None
        self._initialized = False
    
    async def _ensure_client(self) -> httpx.AsyncClient:
        """Get or create HTTP client."""
        if self._client is None:
            self._client = httpx.AsyncClient(
                base_url=self.config.url,
                timeout=self.config.timeout,
            )
        return self._client
    
    async def _ensure_initialized(self) -> None:
        """Ensure collection and graph namespace are created."""
        if self._initialized:
            return
        
        client = await self._ensure_client()
        
        # Create vector collection
        try:
            await client.post(
                "/api/v1/collections",
                json={
                    "name": self.config.collection,
                    "config": {
                        "vector": {
                            "dimension": self.config.vector_dimension,
                            "index": {
                                "type": "hnsw",
                                "m": self.config.hnsw_m,
                                "ef_construction": self.config.hnsw_ef_construction,
                            },
                        },
                        "fulltext": {
                            "enabled": True,
                            "analyzer": "code",  # Code-aware tokenizer
                        },
                    },
                },
            )
            logger.info("Created collection", collection=self.config.collection)
        except httpx.HTTPStatusError as e:
            if e.response.status_code != 409:  # Already exists
                raise
        
        # Create graph namespace
        if self.config.enable_graph:
            try:
                await client.post(
                    "/api/v1/graphs",
                    json={
                        "namespace": self.config.graph_namespace,
                        "config": {
                            "node_types": [
                                "function", "class", "module", 
                                "method", "variable", "file"
                            ],
                            "edge_types": [
                                "imports", "calls", "inherits",
                                "implements", "uses", "defines",
                                "contains", "depends_on"
                            ],
                        },
                    },
                )
                logger.info("Created graph namespace", namespace=self.config.graph_namespace)
            except httpx.HTTPStatusError as e:
                if e.response.status_code != 409:  # Already exists
                    raise
        
        self._initialized = True
    
    async def add(
        self,
        ids: list[str],
        embeddings: list[list[float]],
        documents: list[str],
        metadatas: Optional[list[dict[str, Any]]] = None,
    ) -> None:
        """
        Add documents with embeddings and optional graph edges.
        
        Args:
            ids: Document IDs
            embeddings: Document embeddings
            documents: Document contents
            metadatas: Optional metadata (can include 'graph_edges')
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        # Prepare documents for indexing
        docs = []
        graph_operations = []
        
        for i, (doc_id, embedding, content) in enumerate(zip(ids, embeddings, documents)):
            meta = metadatas[i] if metadatas else {}
            
            # Extract graph edges from metadata
            edges = meta.pop("graph_edges", [])
            node_type = meta.get("chunk_type", "code")
            node_name = meta.get("name", doc_id)
            
            docs.append({
                "id": doc_id,
                "vector": embedding,
                "content": content,
                "metadata": meta,
            })
            
            # Add graph node
            if self.config.enable_graph and edges:
                graph_operations.append({
                    "operation": "add_node",
                    "node": {
                        "id": doc_id,
                        "type": node_type,
                        "name": node_name,
                        "properties": {
                            "language": meta.get("language", "unknown"),
                            "path": meta.get("path", ""),
                        },
                    },
                })
                
                # Add edges
                for edge in edges:
                    graph_operations.append({
                        "operation": "add_edge",
                        "edge": {
                            "source": edge["source"],
                            "target": edge["target"],
                            "type": edge["type"],
                            "properties": edge.get("properties", {}),
                        },
                    })
        
        # Batch insert documents
        response = await client.post(
            f"/api/v1/collections/{self.config.collection}/documents/batch",
            json={"documents": docs},
        )
        response.raise_for_status()
        
        # Batch insert graph operations
        if graph_operations:
            await client.post(
                f"/api/v1/graphs/{self.config.graph_namespace}/batch",
                json={"operations": graph_operations},
            )
        
        logger.debug(
            "Added documents",
            count=len(ids),
            graph_ops=len(graph_operations),
        )
    
    async def search(
        self,
        query_embedding: list[float],
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
    ) -> list[SearchResult]:
        """
        Search using hybrid strategy (vector + keyword + graph).
        
        Args:
            query_embedding: Query vector
            k: Number of results
            filter: Optional metadata filter
            
        Returns:
            Combined and ranked results
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        # Hybrid search request
        response = await client.post(
            f"/api/v1/collections/{self.config.collection}/search",
            json={
                "vector": query_embedding,
                "k": k * 2,  # Get more for reranking
                "filter": filter,
                "search_type": "hybrid",
                "params": {
                    "ef_search": self.config.hnsw_ef_search,
                    "vector_weight": self.config.vector_weight,
                    "keyword_weight": self.config.keyword_weight,
                },
            },
        )
        response.raise_for_status()
        
        data = response.json()
        results = []
        
        for hit in data.get("results", [])[:k]:
            results.append(
                SearchResult(
                    id=hit["id"],
                    content=hit.get("content", ""),
                    score=hit.get("score", 0.0),
                    metadata=hit.get("metadata", {}),
                )
            )
        
        return results
    
    async def hybrid_search(
        self,
        query_embedding: list[float],
        query_text: str,
        k: int = 10,
        filter: Optional[dict[str, Any]] = None,
        expand_graph: bool = True,
        graph_depth: int = 2,
    ) -> list[SearchResult]:
        """
        Enhanced hybrid search with graph expansion.
        
        This leverages ThemisDB's unique multi-model capabilities:
        1. Vector similarity search
        2. Full-text keyword search
        3. Graph traversal to find related code
        4. Score fusion for final ranking
        
        Args:
            query_embedding: Query vector
            query_text: Query text for keyword search
            k: Number of results
            filter: Optional metadata filter
            expand_graph: Whether to expand results via graph
            graph_depth: Depth of graph traversal
            
        Returns:
            Ranked search results
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        # Combined multi-model search
        response = await client.post(
            f"/api/v1/search/multi",
            json={
                "collection": self.config.collection,
                "graph_namespace": self.config.graph_namespace if expand_graph else None,
                "queries": [
                    {
                        "type": "vector",
                        "vector": query_embedding,
                        "weight": self.config.vector_weight,
                    },
                    {
                        "type": "fulltext",
                        "text": query_text,
                        "weight": self.config.keyword_weight,
                    },
                ],
                "k": k,
                "filter": filter,
                "graph_expansion": {
                    "enabled": expand_graph,
                    "depth": graph_depth,
                    "weight": self.config.graph_weight,
                    "edge_types": ["imports", "calls", "uses", "depends_on"],
                } if expand_graph else None,
                "fusion": "rrf",  # Reciprocal Rank Fusion
            },
        )
        response.raise_for_status()
        
        data = response.json()
        results = []
        
        for hit in data.get("results", []):
            results.append(
                SearchResult(
                    id=hit["id"],
                    content=hit.get("content", ""),
                    score=hit.get("fused_score", hit.get("score", 0.0)),
                    metadata={
                        **hit.get("metadata", {}),
                        "_vector_score": hit.get("vector_score"),
                        "_keyword_score": hit.get("keyword_score"),
                        "_graph_score": hit.get("graph_score"),
                    },
                )
            )
        
        return results
    
    async def graph_traverse(
        self,
        start_id: str,
        edge_types: Optional[list[str]] = None,
        direction: str = "outgoing",  # outgoing, incoming, both
        depth: int = 2,
        limit: int = 100,
    ) -> list[dict[str, Any]]:
        """
        Traverse the code graph from a starting node.
        
        Useful for:
        - Finding all functions that call a specific function
        - Finding all imports of a module
        - Tracing dependency chains
        
        Args:
            start_id: Starting node ID
            edge_types: Edge types to traverse (None = all)
            direction: Traversal direction
            depth: Maximum traversal depth
            limit: Maximum results
            
        Returns:
            List of nodes with path information
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        response = await client.post(
            f"/api/v1/graphs/{self.config.graph_namespace}/traverse",
            json={
                "start": start_id,
                "edge_types": edge_types or ["imports", "calls", "inherits", "uses"],
                "direction": direction,
                "depth": depth,
                "limit": limit,
            },
        )
        response.raise_for_status()
        
        return response.json().get("nodes", [])
    
    async def find_dependencies(
        self,
        code_id: str,
        include_transitive: bool = True,
    ) -> dict[str, list[str]]:
        """
        Find all dependencies of a code entity.
        
        Args:
            code_id: Code entity ID
            include_transitive: Include transitive dependencies
            
        Returns:
            Dictionary of dependency types to IDs
        """
        depth = 3 if include_transitive else 1
        
        nodes = await self.graph_traverse(
            start_id=code_id,
            edge_types=["imports", "calls", "uses", "depends_on"],
            direction="outgoing",
            depth=depth,
        )
        
        dependencies: dict[str, list[str]] = {
            "imports": [],
            "calls": [],
            "uses": [],
            "depends_on": [],
        }
        
        for node in nodes:
            edge_type = node.get("edge_type", "uses")
            if edge_type in dependencies:
                dependencies[edge_type].append(node["id"])
        
        return dependencies
    
    async def find_usages(
        self,
        code_id: str,
    ) -> list[dict[str, Any]]:
        """
        Find all usages of a code entity (reverse dependencies).
        
        Args:
            code_id: Code entity ID
            
        Returns:
            List of code entities that use this one
        """
        return await self.graph_traverse(
            start_id=code_id,
            edge_types=["imports", "calls", "uses"],
            direction="incoming",
            depth=2,
        )
    
    async def get_call_graph(
        self,
        function_id: str,
        depth: int = 3,
    ) -> dict[str, Any]:
        """
        Get the call graph for a function.
        
        Args:
            function_id: Function ID
            depth: Maximum call depth
            
        Returns:
            Call graph structure
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        response = await client.post(
            f"/api/v1/graphs/{self.config.graph_namespace}/subgraph",
            json={
                "start": function_id,
                "edge_types": ["calls"],
                "depth": depth,
                "include_properties": True,
            },
        )
        response.raise_for_status()
        
        return response.json()
    
    async def get_import_tree(
        self,
        module_id: str,
    ) -> dict[str, Any]:
        """
        Get the import tree for a module.
        
        Args:
            module_id: Module ID
            
        Returns:
            Import tree structure
        """
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        response = await client.post(
            f"/api/v1/graphs/{self.config.graph_namespace}/subgraph",
            json={
                "start": module_id,
                "edge_types": ["imports"],
                "depth": 5,
                "direction": "outgoing",
            },
        )
        response.raise_for_status()
        
        return response.json()
    
    async def delete(self, ids: list[str]) -> None:
        """Delete documents and their graph nodes."""
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        # Delete from vector collection
        await client.post(
            f"/api/v1/collections/{self.config.collection}/delete",
            json={"ids": ids},
        )
        
        # Delete from graph
        if self.config.enable_graph:
            for doc_id in ids:
                await client.delete(
                    f"/api/v1/graphs/{self.config.graph_namespace}/nodes/{doc_id}",
                )
    
    async def get(self, id: str) -> Optional[SearchResult]:
        """Get document by ID."""
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        try:
            response = await client.get(
                f"/api/v1/collections/{self.config.collection}/documents/{id}",
            )
            response.raise_for_status()
            
            data = response.json()
            return SearchResult(
                id=data["id"],
                content=data.get("content", ""),
                score=1.0,
                metadata=data.get("metadata", {}),
            )
        except httpx.HTTPStatusError as e:
            if e.response.status_code == 404:
                return None
            raise
    
    async def count(self) -> int:
        """Get document count."""
        await self._ensure_initialized()
        client = await self._ensure_client()
        
        response = await client.get(
            f"/api/v1/collections/{self.config.collection}/stats",
        )
        response.raise_for_status()
        
        return response.json().get("document_count", 0)
    
    async def close(self) -> None:
        """Close HTTP client."""
        if self._client:
            await self._client.aclose()
            self._client = None


# Register with factory
try:
    from respo.vectorstore.base import VectorStoreFactory
    VectorStoreFactory.register("themis", ThemisVectorStore)
except ImportError:
    pass
