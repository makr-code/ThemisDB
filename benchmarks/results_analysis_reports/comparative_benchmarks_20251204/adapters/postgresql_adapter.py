"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgresql_adapter.py                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     384                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
PostgreSQL Adapter for Comparative Benchmarks

This adapter provides a unified interface for PostgreSQL operations
to be used in comparative benchmarks against other databases.
Supports both standard PostgreSQL and pgvector extension for vector operations.
"""

import json
import re
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

# Valid identifier pattern for SQL injection prevention
VALID_IDENTIFIER = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')

def _validate_identifier(name: str) -> str:
    """Validate and return a safe SQL identifier."""
    if not VALID_IDENTIFIER.match(name):
        raise ValueError(f"Invalid identifier: {name}")
    return name



class PostgreSQLAdapter(BaseDatabaseAdapter):
    """PostgreSQL adapter using psycopg2."""
    
    def __init__(self, config: Dict[str, Any]):
        super().__init__(config)
        self.host = config.get('host', 'localhost')
        self.port = config.get('port', 5432)
        self.user = config.get('user', 'benchmark')
        self.password = config.get('password', 'benchmark123')
        self.database = config.get('database', 'benchmark')
        self.use_pgvector = config.get('use_pgvector', False)
        self.conn = None
    
    @property
    def name(self) -> str:
        return "PostgreSQL" + (" (pgvector)" if self.use_pgvector else "")
    
    @retry(stop=stop_after_attempt(5), wait=wait_exponential(multiplier=1, min=1, max=10))
    def connect(self) -> None:
        """Establish connection to PostgreSQL."""
        import psycopg2
        from psycopg2.extras import RealDictCursor
        
        self.conn = psycopg2.connect(
            host=self.host,
            port=self.port,
            user=self.user,
            password=self.password,
            database=self.database,
            cursor_factory=RealDictCursor
        )
        self.conn.autocommit = True
        
        # Enable pgvector if requested
        if self.use_pgvector:
            with self.conn.cursor() as cur:
                cur.execute("CREATE EXTENSION IF NOT EXISTS vector")
        
        self._connected = True
        logger.info("Connected to PostgreSQL", host=self.host, port=self.port)
    
    def disconnect(self) -> None:
        """Close connection to PostgreSQL."""
        if self.conn:
            self.conn.close()
            self.conn = None
        self._connected = False
        logger.info("Disconnected from PostgreSQL")
    
    def clear_data(self) -> None:
        """Clear all benchmark data."""
        if not self.conn:
            return
        with self.conn.cursor() as cur:
            cur.execute("DROP TABLE IF EXISTS benchmark_docs CASCADE")
            cur.execute("DROP TABLE IF EXISTS query_docs CASCADE")
            cur.execute("DROP TABLE IF EXISTS vectors CASCADE")
            cur.execute("DROP TABLE IF EXISTS nodes CASCADE")
            cur.execute("DROP TABLE IF EXISTS edges CASCADE")
    
    def _ensure_table(self, collection: str) -> None:
        """Ensure table exists for the collection."""
        safe_collection = _validate_identifier(collection)
        with self.conn.cursor() as cur:
            if collection == "vectors" and self.use_pgvector:
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS {safe_collection} (
                        id VARCHAR(255) PRIMARY KEY,
                        embedding vector(384),
                        metadata JSONB
                    )
                """)
            else:
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS {safe_collection} (
                        id VARCHAR(255) PRIMARY KEY,
                        data JSONB
                    )
                """)
    
    # ==========================================================================
    # CRUD Operations
    # ==========================================================================
    
    def insert_one(self, collection: str, document: Dict[str, Any]) -> str:
        """Insert a single document."""
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        doc_id = document.get('id') or document.get('_key')
        if not doc_id:
            doc_id = str(uuid.uuid4())
            document['id'] = doc_id
        
        with self.conn.cursor() as cur:
            cur.execute(
                f"INSERT INTO {safe_collection} (id, data) VALUES (%s, %s) ON CONFLICT (id) DO UPDATE SET data = %s",
                (doc_id, json.dumps(document), json.dumps(document))
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
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(f"SELECT data FROM {safe_collection} WHERE id = %s", (doc_id,))
            row = cur.fetchone()
            if row:
                return row['data']
        return None
    
    def update_one(self, collection: str, doc_id: str, updates: Dict[str, Any]) -> bool:
        """Update a single document."""
        safe_collection = _validate_identifier(collection)
        existing = self.find_by_id(collection, doc_id)
        if not existing:
            return False
        existing.update(updates)
        with self.conn.cursor() as cur:
            cur.execute(
                f"UPDATE {safe_collection} SET data = %s WHERE id = %s",
                (json.dumps(existing), doc_id)
            )
        return True
    
    def delete_one(self, collection: str, doc_id: str) -> bool:
        """Delete a single document."""
        safe_collection = _validate_identifier(collection)
        with self.conn.cursor() as cur:
            cur.execute(f"DELETE FROM {safe_collection} WHERE id = %s", (doc_id,))
            return cur.rowcount > 0
    
    # ==========================================================================
    # Query Operations
    # ==========================================================================
    
    def find_by_field(self, collection: str, field: str, value: Any) -> List[Dict[str, Any]]:
        """Find documents by field value."""
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"SELECT data FROM {safe_collection} WHERE data->>%s = %s",
                (field, str(value))
            )
            return [row['data'] for row in cur.fetchall()]
    
    def find_by_range(self, collection: str, field: str, 
                      min_val: Any, max_val: Any) -> List[Dict[str, Any]]:
        """Find documents by field range."""
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"SELECT data FROM {safe_collection} WHERE (data->>%s)::numeric >= %s AND (data->>%s)::numeric <= %s",
                (field, min_val, field, max_val)
            )
            return [row['data'] for row in cur.fetchall()]
    
    def count_by_field(self, collection: str, field: str, value: Any) -> int:
        """Count documents matching field value."""
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"SELECT COUNT(*) as cnt FROM {safe_collection} WHERE data->>%s = %s",
                (field, str(value))
            )
            return cur.fetchone()['cnt']
    
    def aggregate_sum(self, collection: str, field: str, 
                      group_by: Optional[str] = None) -> Any:
        """Aggregate sum of field values."""
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            if group_by:
                cur.execute(
                    f"SELECT data->>%s as grp, SUM((data->>%s)::numeric) as total FROM {safe_collection} GROUP BY data->>%s",
                    (group_by, field, group_by)
                )
                return [{"group": r['grp'], "sum": r['total']} for r in cur.fetchall()]
            else:
                cur.execute(
                    f"SELECT SUM((data->>%s)::numeric) as total FROM {safe_collection}",
                    (field,)
                )
                return cur.fetchone()['total']
    
    # ==========================================================================
    # Vector Operations (pgvector)
    # ==========================================================================
    
    def supports_vector_search(self) -> bool:
        """Return True if database supports vector search."""
        return self.use_pgvector
    
    def insert_vector(self, collection: str, doc_id: str, 
                      vector: List[float], metadata: Dict[str, Any]) -> str:
        """Insert a vector with metadata."""
        if not self.use_pgvector:
            raise NotImplementedError("Vector search requires pgvector extension")
        
        safe_collection = _validate_identifier(collection)
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"INSERT INTO {safe_collection} (id, embedding, metadata) VALUES (%s, %s, %s) ON CONFLICT (id) DO UPDATE SET embedding = %s, metadata = %s",
                (doc_id, vector, json.dumps(metadata), vector, json.dumps(metadata))
            )
        return doc_id
    
    def search_vectors(self, collection: str, query_vector: List[float], 
                       k: int = 10, filter_criteria: Optional[Dict[str, Any]] = None) -> List[Dict[str, Any]]:
        """Search for k nearest vectors."""
        if not self.use_pgvector:
            raise NotImplementedError("Vector search requires pgvector extension")
        
        safe_collection = _validate_identifier(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"SELECT id, metadata, embedding <-> %s as distance FROM {safe_collection} ORDER BY distance LIMIT %s",
                (query_vector, k)
            )
            return [{"id": r['id'], "metadata": r['metadata'], "distance": r['distance']} 
                    for r in cur.fetchall()]
    
    # ==========================================================================
    # Graph Operations (Simulated with tables)
    # ==========================================================================
    
    def supports_graph_operations(self) -> bool:
        """PostgreSQL can simulate basic graph operations."""
        return True
    
    def insert_node(self, node_id: str, labels: List[str], 
                    properties: Dict[str, Any]) -> str:
        """Insert a graph node."""
        with self.conn.cursor() as cur:
            cur.execute("""
                CREATE TABLE IF NOT EXISTS nodes (
                    id VARCHAR(255) PRIMARY KEY,
                    labels TEXT[],
                    properties JSONB
                )
            """)
            cur.execute(
                "INSERT INTO nodes (id, labels, properties) VALUES (%s, %s, %s) ON CONFLICT (id) DO UPDATE SET labels = %s, properties = %s",
                (node_id, labels, json.dumps(properties), labels, json.dumps(properties))
            )
        return node_id
    
    def insert_edge(self, from_id: str, to_id: str, 
                    edge_type: str, properties: Dict[str, Any]) -> str:
        """Insert a graph edge."""
        edge_id = str(uuid.uuid4())
        
        with self.conn.cursor() as cur:
            cur.execute("""
                CREATE TABLE IF NOT EXISTS edges (
                    id VARCHAR(255) PRIMARY KEY,
                    from_id VARCHAR(255),
                    to_id VARCHAR(255),
                    edge_type VARCHAR(100),
                    properties JSONB
                )
            """)
            cur.execute("""
                CREATE INDEX IF NOT EXISTS idx_edges_from ON edges(from_id)
            """)
            cur.execute(
                "INSERT INTO edges (id, from_id, to_id, edge_type, properties) VALUES (%s, %s, %s, %s, %s)",
                (edge_id, from_id, to_id, edge_type, json.dumps(properties))
            )
        return edge_id
    
    def traverse_bfs(self, start_node: str, max_depth: int) -> List[str]:
        """Perform BFS traversal using recursive CTE."""
        with self.conn.cursor() as cur:
            cur.execute("""
                WITH RECURSIVE bfs AS (
                    SELECT from_id, to_id, 1 as depth
                    FROM edges
                    WHERE from_id = %s
                    UNION ALL
                    SELECT e.from_id, e.to_id, b.depth + 1
                    FROM edges e
                    INNER JOIN bfs b ON e.from_id = b.to_id
                    WHERE b.depth < %s
                )
                SELECT DISTINCT to_id FROM bfs
            """, (start_node, max_depth))
            return [start_node] + [r['to_id'] for r in cur.fetchall()]
    
    def shortest_path(self, from_id: str, to_id: str) -> List[str]:
        """Find shortest path (simplified BFS)."""
        # Simplified - returns BFS traversal path
        return self.traverse_bfs(from_id, 10)
    
    # ==========================================================================
    # Full-text Search
    # ==========================================================================
    
    def supports_fulltext_search(self) -> bool:
        """PostgreSQL supports full-text search."""
        return True
    
    def fulltext_search(self, collection: str, query: str, 
                        limit: int = 10) -> List[Dict[str, Any]]:
        """Perform full-text search."""
        self._ensure_table(collection)
        with self.conn.cursor() as cur:
            cur.execute(
                f"SELECT data FROM {collection} WHERE to_tsvector('english', data->>'content') @@ plainto_tsquery('english', %s) LIMIT %s",
                (query, limit)
            )
            return [row['data'] for row in cur.fetchall()]
