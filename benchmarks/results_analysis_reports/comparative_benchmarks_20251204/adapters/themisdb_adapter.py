"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_adapter.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     414                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Adapter for Comparative Benchmarks

This adapter provides a unified interface for ThemisDB operations
to be used in comparative benchmarks against other databases.
"""

import json
from typing import Any, Dict, List, Optional
import httpx
from tenacity import retry, stop_after_attempt, wait_exponential
import structlog

# Import base class using relative import
try:
    from ..benchmarks.base_benchmark import BaseDatabaseAdapter
except ImportError:
    # Fallback for direct script execution
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).parent.parent))
    from benchmarks.base_benchmark import BaseDatabaseAdapter

logger = structlog.get_logger()


class ThemisDBAdapter(BaseDatabaseAdapter):
    """ThemisDB adapter using HTTP API."""
    
    def __init__(self, config: Dict[str, Any]):
        super().__init__(config)
        self.host = config.get('host', 'localhost')
        self.port = config.get('port', 8765)
        self.base_url = f"http://{self.host}:{self.port}"
        self.client: Optional[httpx.Client] = None
    
    @property
    def name(self) -> str:
        return "ThemisDB"
    
    @retry(stop=stop_after_attempt(5), wait=wait_exponential(multiplier=1, min=1, max=10))
    def connect(self) -> None:
        """Establish connection to ThemisDB."""
        self.client = httpx.Client(base_url=self.base_url, timeout=30.0)
        # Verify connection
        response = self.client.get("/health")
        response.raise_for_status()
        self._connected = True
        logger.info("Connected to ThemisDB", host=self.host, port=self.port)
    
    def disconnect(self) -> None:
        """Close connection to ThemisDB."""
        if self.client:
            self.client.close()
            self.client = None
        self._connected = False
        logger.info("Disconnected from ThemisDB")
    
    def clear_data(self) -> None:
        """Clear all benchmark data (requires admin endpoint or restart)."""
        # ThemisDB might need a specific admin endpoint for this
        # For now, we'll handle this by clearing specific collections
        logger.warning("ThemisDB clear_data: manual cleanup may be required")
    
    def _ensure_connected(self) -> None:
        if not self.client:
            raise RuntimeError("Not connected to ThemisDB")
    
    # ==========================================================================
    # CRUD Operations
    # ==========================================================================
    
    def insert_one(self, collection: str, document: Dict[str, Any]) -> str:
        """Insert a single document."""
        self._ensure_connected()
        doc_id = document.get('id') or document.get('_key')
        if not doc_id:
            import uuid
            doc_id = str(uuid.uuid4())
            document['id'] = doc_id
        
        key = f"{collection}:{doc_id}"
        payload = {"blob": json.dumps(document)}
        
        response = self.client.put(f"/entities/{key}", json=payload)
        response.raise_for_status()
        return doc_id
    
    def insert_many(self, collection: str, documents: List[Dict[str, Any]]) -> List[str]:
        """Insert multiple documents."""
        # ThemisDB currently doesn't have a batch insert API,
        # so we insert one by one
        ids = []
        for doc in documents:
            doc_id = self.insert_one(collection, doc)
            ids.append(doc_id)
        return ids
    
    def find_by_id(self, collection: str, doc_id: str) -> Optional[Dict[str, Any]]:
        """Find a document by its ID."""
        self._ensure_connected()
        key = f"{collection}:{doc_id}"
        
        response = self.client.get(f"/entities/{key}")
        if response.status_code == 404:
            return None
        response.raise_for_status()
        
        data = response.json()
        if 'blob' in data:
            return json.loads(data['blob'])
        return data
    
    def update_one(self, collection: str, doc_id: str, updates: Dict[str, Any]) -> bool:
        """Update a single document."""
        self._ensure_connected()
        
        # First get the existing document
        existing = self.find_by_id(collection, doc_id)
        if not existing:
            return False
        
        # Merge updates
        existing.update(updates)
        
        # Save back
        key = f"{collection}:{doc_id}"
        payload = {"blob": json.dumps(existing)}
        
        response = self.client.put(f"/entities/{key}", json=payload)
        return response.status_code == 200
    
    def delete_one(self, collection: str, doc_id: str) -> bool:
        """Delete a single document."""
        self._ensure_connected()
        key = f"{collection}:{doc_id}"
        
        response = self.client.delete(f"/entities/{key}")
        return response.status_code == 200
    
    # ==========================================================================
    # Query Operations
    # ==========================================================================
    
    def find_by_field(self, collection: str, field: str, value: Any) -> List[Dict[str, Any]]:
        """Find documents by field value (equality)."""
        self._ensure_connected()
        
        payload = {
            "table": collection,
            "predicates": [{"column": field, "value": str(value)}],
            "return": "entities",
            "optimize": True
        }
        
        response = self.client.post("/query", json=payload)
        response.raise_for_status()
        
        data = response.json()
        entities = data.get('entities', [])
        return [json.loads(e) if isinstance(e, str) else e for e in entities]
    
    def find_by_range(self, collection: str, field: str, 
                      min_val: Any, max_val: Any) -> List[Dict[str, Any]]:
        """Find documents by field range."""
        self._ensure_connected()
        
        payload = {
            "table": collection,
            "range": [{
                "column": field,
                "gte": str(min_val),
                "lte": str(max_val),
                "includeLower": True,
                "includeUpper": True
            }],
            "return": "entities",
            "optimize": True
        }
        
        response = self.client.post("/query", json=payload)
        response.raise_for_status()
        
        data = response.json()
        entities = data.get('entities', [])
        return [json.loads(e) if isinstance(e, str) else e for e in entities]
    
    def count_by_field(self, collection: str, field: str, value: Any) -> int:
        """Count documents matching field value."""
        self._ensure_connected()
        
        payload = {
            "table": collection,
            "predicates": [{"column": field, "value": str(value)}],
            "return": "keys",
            "optimize": True
        }
        
        response = self.client.post("/query", json=payload)
        response.raise_for_status()
        
        data = response.json()
        return data.get('count', 0)
    
    def aggregate_sum(self, collection: str, field: str, 
                      group_by: Optional[str] = None) -> Any:
        """Aggregate sum using AQL."""
        self._ensure_connected()
        
        if group_by:
            aql = f"""
            FOR doc IN {collection}
            COLLECT g = doc.{group_by}
            AGGREGATE total = SUM(doc.{field})
            RETURN {{ group: g, sum: total }}
            """
        else:
            aql = f"""
            FOR doc IN {collection}
            COLLECT AGGREGATE total = SUM(doc.{field})
            RETURN total
            """
        
        payload = {"query": aql}
        response = self.client.post("/query/aql", json=payload)
        response.raise_for_status()
        
        return response.json()
    
    # ==========================================================================
    # Vector Operations
    # ==========================================================================
    
    def supports_vector_search(self) -> bool:
        """ThemisDB supports vector search."""
        return True
    
    def insert_vector(self, collection: str, doc_id: str, 
                      vector: List[float], metadata: Dict[str, Any]) -> str:
        """Insert a vector with metadata."""
        self._ensure_connected()
        
        document = {
            "id": doc_id,
            "embedding": vector,
            **metadata
        }
        
        return self.insert_one(collection, document)
    
    def search_vectors(self, collection: str, query_vector: List[float], 
                       k: int = 10, filter_criteria: Optional[Dict[str, Any]] = None) -> List[Dict[str, Any]]:
        """Search for k nearest vectors."""
        self._ensure_connected()
        
        # Use AQL for vector search (if available)
        # This depends on ThemisDB's specific vector search API
        aql = f"""
        FOR doc IN {collection}
        SORT SIMILARITY(doc.embedding, @query_vector) DESC
        LIMIT {k}
        RETURN doc
        """
        
        payload = {
            "query": aql,
            "bindVars": {"query_vector": query_vector}
        }
        
        response = self.client.post("/query/aql", json=payload)
        response.raise_for_status()
        
        return response.json().get('result', [])
    
    # ==========================================================================
    # Graph Operations
    # ==========================================================================
    
    def supports_graph_operations(self) -> bool:
        """ThemisDB supports graph operations."""
        return True
    
    def insert_node(self, node_id: str, labels: List[str], 
                    properties: Dict[str, Any]) -> str:
        """Insert a graph node."""
        document = {
            "id": node_id,
            "_labels": labels,
            **properties
        }
        return self.insert_one("nodes", document)
    
    def insert_edge(self, from_id: str, to_id: str, 
                    edge_type: str, properties: Dict[str, Any]) -> str:
        """Insert a graph edge."""
        self._ensure_connected()
        
        import uuid
        edge_id = str(uuid.uuid4())
        
        edge_doc = {
            "id": edge_id,
            "_from": from_id,
            "_to": to_id,
            "_type": edge_type,
            **properties
        }
        
        key = f"edge:{edge_id}"
        payload = {"blob": json.dumps(edge_doc)}
        
        response = self.client.put(f"/entities/{key}", json=payload)
        response.raise_for_status()
        return edge_id
    
    def traverse_bfs(self, start_node: str, max_depth: int) -> List[str]:
        """Perform BFS traversal from start node."""
        self._ensure_connected()
        
        payload = {
            "start_vertex": start_node,
            "max_depth": max_depth
        }
        
        response = self.client.post("/graph/traverse", json=payload)
        response.raise_for_status()
        
        data = response.json()
        return data.get('visited', [])
    
    def shortest_path(self, from_id: str, to_id: str) -> List[str]:
        """Find shortest path between two nodes using AQL."""
        self._ensure_connected()
        
        aql = f"""
        FOR v, e, p IN OUTBOUND SHORTEST_PATH '{from_id}' TO '{to_id}' GRAPH 'benchmark'
        RETURN v._key
        """
        
        payload = {"query": aql}
        response = self.client.post("/query/aql", json=payload)
        response.raise_for_status()
        
        return response.json().get('result', [])
    
    # ==========================================================================
    # Full-text Search
    # ==========================================================================
    
    def supports_fulltext_search(self) -> bool:
        """ThemisDB supports full-text search."""
        return True
    
    def fulltext_search(self, collection: str, query: str, 
                        limit: int = 10) -> List[Dict[str, Any]]:
        """Perform full-text search using AQL."""
        self._ensure_connected()
        
        aql = f"""
        FOR doc IN {collection}
        FILTER CONTAINS(LOWER(doc.content), LOWER(@query))
        LIMIT {limit}
        RETURN doc
        """
        
        payload = {
            "query": aql,
            "bindVars": {"query": query}
        }
        
        response = self.client.post("/query/aql", json=payload)
        response.raise_for_status()
        
        return response.json().get('result', [])
    
    # ==========================================================================
    # Index Management (for benchmark setup)
    # ==========================================================================
    
    def create_index(self, collection: str, column: str, 
                     index_type: str = "equality") -> bool:
        """Create an index on a collection column."""
        self._ensure_connected()
        
        payload = {
            "table": collection,
            "column": column,
            "type": index_type
        }
        
        response = self.client.post("/index/create", json=payload)
        return response.status_code == 200
