"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chromadb_adapter.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     343                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ChromaDB Adapter for Comparative Benchmarks

This adapter provides a unified interface for ChromaDB operations
to be used in comparative benchmarks against other databases.
ChromaDB is optimized for vector/embedding storage and similarity search.
"""

import hashlib
import json
import uuid
from typing import Any, Dict, List, Optional
import structlog
from tenacity import retry, stop_after_attempt, wait_exponential

# Import base class using relative import
try:
    from ..benchmarks.base_benchmark import BaseDatabaseAdapter
except ImportError:
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).parent.parent))
    from benchmarks.base_benchmark import BaseDatabaseAdapter

logger = structlog.get_logger()


class ChromaDBAdapter(BaseDatabaseAdapter):
    """ChromaDB adapter using chromadb client."""
    
    def __init__(self, config: Dict[str, Any]):
        super().__init__(config)
        self.host = config.get('host', 'localhost')
        self.port = config.get('port', 8000)
        self.client = None
        self._collections: Dict[str, Any] = {}
    
    @property
    def name(self) -> str:
        return "ChromaDB"
    
    @retry(stop=stop_after_attempt(5), wait=wait_exponential(multiplier=1, min=1, max=10))
    def connect(self) -> None:
        """Establish connection to ChromaDB."""
        import chromadb
        from chromadb.config import Settings
        
        self.client = chromadb.HttpClient(
            host=self.host,
            port=self.port,
            settings=Settings(anonymized_telemetry=False)
        )
        # Verify connection
        self.client.heartbeat()
        
        self._connected = True
        logger.info("Connected to ChromaDB", host=self.host, port=self.port)
    
    def disconnect(self) -> None:
        """Close connection to ChromaDB."""
        self.client = None
        self._collections = {}
        self._connected = False
        logger.info("Disconnected from ChromaDB")
    
    def clear_data(self) -> None:
        """Clear all benchmark data."""
        if not self.client:
            return
        # Delete all collections
        for name in list(self._collections.keys()):
            try:
                self.client.delete_collection(name)
            except Exception:
                pass
        self._collections = {}
    
    def _ensure_connected(self) -> None:
        if not self.client:
            raise RuntimeError("Not connected to ChromaDB")
    
    def _get_collection(self, name: str):
        """Get or create a collection."""
        if name not in self._collections:
            self._collections[name] = self.client.get_or_create_collection(
                name=name,
                metadata={"hnsw:space": "l2"}
            )
        return self._collections[name]
    
    # ==========================================================================
    # CRUD Operations (Document-style with embeddings)
    # ==========================================================================
    
    def insert_one(self, collection: str, document: Dict[str, Any]) -> str:
        """Insert a single document (stores as metadata)."""
        self._ensure_connected()
        doc_id = document.get('id') or document.get('_key')
        if not doc_id:
            doc_id = str(uuid.uuid4())
            document['id'] = doc_id
        
        col = self._get_collection(collection)
        
        # Generate a dummy embedding if not provided
        embedding = document.pop('embedding', None)
        if embedding is None:
            # Create deterministic pseudo-embedding from document
            hash_val = hashlib.sha256(json.dumps(document, sort_keys=True).encode()).hexdigest()
            embedding = [float(int(hash_val[i:i+2], 16)) / 255.0 for i in range(0, 64, 2)]
            # Pad to 384 dimensions
            embedding = (embedding * 12)[:384]
        
        # Convert document to string metadata (ChromaDB requires simple types)
        metadata = {}
        for k, v in document.items():
            if isinstance(v, (str, int, float, bool)):
                metadata[k] = v
            else:
                metadata[k] = json.dumps(v)
        
        col.upsert(
            ids=[doc_id],
            embeddings=[embedding],
            metadatas=[metadata]
        )
        return doc_id
    
    def insert_many(self, collection: str, documents: List[Dict[str, Any]]) -> List[str]:
        """Insert multiple documents."""
        ids = []
        for doc in documents:
            doc_id = self.insert_one(collection, doc)
            ids.append(doc_id)
        return ids
    
    def find_by_id(self, collection: str, doc_id: str) -> Optional[Dict[str, Any]]:
        """Find a document by its ID."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        result = col.get(ids=[doc_id], include=["metadatas"])
        if result['ids'] and result['metadatas']:
            return result['metadatas'][0]
        return None
    
    def update_one(self, collection: str, doc_id: str, updates: Dict[str, Any]) -> bool:
        """Update a single document."""
        self._ensure_connected()
        existing = self.find_by_id(collection, doc_id)
        if not existing:
            return False
        
        existing.update(updates)
        col = self._get_collection(collection)
        
        # Convert to simple types
        metadata = {}
        for k, v in existing.items():
            if isinstance(v, (str, int, float, bool)):
                metadata[k] = v
            else:
                metadata[k] = json.dumps(v)
        
        col.update(ids=[doc_id], metadatas=[metadata])
        return True
    
    def delete_one(self, collection: str, doc_id: str) -> bool:
        """Delete a single document."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        try:
            col.delete(ids=[doc_id])
            return True
        except Exception:
            return False
    
    # ==========================================================================
    # Query Operations
    # ==========================================================================
    
    def find_by_field(self, collection: str, field: str, value: Any) -> List[Dict[str, Any]]:
        """Find documents by field value using where filter."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        result = col.get(
            where={field: str(value) if not isinstance(value, (int, float, bool)) else value},
            include=["metadatas"]
        )
        return result.get('metadatas', [])
    
    def find_by_range(self, collection: str, field: str, 
                      min_val: Any, max_val: Any) -> List[Dict[str, Any]]:
        """Find documents by field range."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        result = col.get(
            where={
                "$and": [
                    {field: {"$gte": min_val}},
                    {field: {"$lte": max_val}}
                ]
            },
            include=["metadatas"]
        )
        return result.get('metadatas', [])
    
    def count_by_field(self, collection: str, field: str, value: Any) -> int:
        """Count documents matching field value."""
        results = self.find_by_field(collection, field, value)
        return len(results)
    
    def aggregate_sum(self, collection: str, field: str, 
                      group_by: Optional[str] = None) -> Any:
        """Aggregate sum (limited support - fetches all and computes)."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        result = col.get(include=["metadatas"])
        metadatas = result.get('metadatas', [])
        
        if group_by:
            groups = {}
            for m in metadatas:
                grp = m.get(group_by, 'unknown')
                val = float(m.get(field, 0))
                groups[grp] = groups.get(grp, 0) + val
            return [{"group": k, "sum": v} for k, v in groups.items()]
        else:
            return sum(float(m.get(field, 0)) for m in metadatas)
    
    # ==========================================================================
    # Vector Operations (ChromaDB's primary strength)
    # ==========================================================================
    
    def supports_vector_search(self) -> bool:
        """ChromaDB is built for vector search."""
        return True
    
    def insert_vector(self, collection: str, doc_id: str, 
                      vector: List[float], metadata: Dict[str, Any]) -> str:
        """Insert a vector with metadata."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        # Convert metadata to simple types
        clean_metadata = {}
        for k, v in metadata.items():
            if isinstance(v, (str, int, float, bool)):
                clean_metadata[k] = v
            else:
                clean_metadata[k] = json.dumps(v)
        clean_metadata['id'] = doc_id
        
        col.upsert(
            ids=[doc_id],
            embeddings=[vector],
            metadatas=[clean_metadata]
        )
        return doc_id
    
    def search_vectors(self, collection: str, query_vector: List[float], 
                       k: int = 10, filter_criteria: Optional[Dict[str, Any]] = None) -> List[Dict[str, Any]]:
        """Search for k nearest vectors."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        query_params = {
            "query_embeddings": [query_vector],
            "n_results": k,
            "include": ["metadatas", "distances"]
        }
        
        if filter_criteria:
            query_params["where"] = filter_criteria
        
        result = col.query(**query_params)
        
        results = []
        if result['ids'] and result['ids'][0]:
            for i, doc_id in enumerate(result['ids'][0]):
                results.append({
                    "id": doc_id,
                    "metadata": result['metadatas'][0][i] if result['metadatas'] else {},
                    "distance": result['distances'][0][i] if result['distances'] else None
                })
        return results
    
    # ==========================================================================
    # Graph Operations (Not supported)
    # ==========================================================================
    
    def supports_graph_operations(self) -> bool:
        """ChromaDB doesn't support graph operations."""
        return False
    
    # ==========================================================================
    # Full-text Search
    # ==========================================================================
    
    def supports_fulltext_search(self) -> bool:
        """ChromaDB has basic document search via metadata."""
        return True
    
    def fulltext_search(self, collection: str, query: str, 
                        limit: int = 10) -> List[Dict[str, Any]]:
        """Perform text search using document field filtering."""
        self._ensure_connected()
        col = self._get_collection(collection)
        
        # ChromaDB doesn't have native fulltext search
        # Use where_document for substring matching
        result = col.get(
            where_document={"$contains": query},
            include=["metadatas"],
            limit=limit
        )
        return result.get('metadatas', [])
