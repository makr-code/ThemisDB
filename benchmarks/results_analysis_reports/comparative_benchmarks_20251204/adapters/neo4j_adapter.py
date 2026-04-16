"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            neo4j_adapter.py                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     355                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Neo4j Adapter for Comparative Benchmarks

This adapter provides a unified interface for Neo4j operations
to be used in comparative benchmarks against other databases.
Neo4j excels at graph operations and relationship traversals.
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

# Valid identifier pattern for Cypher injection prevention
VALID_IDENTIFIER = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')

def _validate_label(name: str) -> str:
    """Validate and return a safe Cypher label/type name."""
    if not VALID_IDENTIFIER.match(name):
        raise ValueError(f"Invalid identifier: {name}")
    return name


class Neo4jAdapter(BaseDatabaseAdapter):
    """Neo4j adapter using official neo4j driver."""
    
    def __init__(self, config: Dict[str, Any]):
        super().__init__(config)
        self.host = config.get('host', 'localhost')
        self.bolt_port = config.get('bolt_port', 7687)
        self.user = config.get('user', 'neo4j')
        self.password = config.get('password', 'benchmark123')
        self.uri = f"bolt://{self.host}:{self.bolt_port}"
        self.driver = None
    
    @property
    def name(self) -> str:
        return "Neo4j"
    
    @retry(stop=stop_after_attempt(5), wait=wait_exponential(multiplier=1, min=1, max=10))
    def connect(self) -> None:
        """Establish connection to Neo4j."""
        from neo4j import GraphDatabase
        
        self.driver = GraphDatabase.driver(
            self.uri,
            auth=(self.user, self.password)
        )
        # Verify connection
        with self.driver.session() as session:
            session.run("RETURN 1")
        
        self._connected = True
        logger.info("Connected to Neo4j", host=self.host, port=self.bolt_port)
    
    def disconnect(self) -> None:
        """Close connection to Neo4j."""
        if self.driver:
            self.driver.close()
            self.driver = None
        self._connected = False
        logger.info("Disconnected from Neo4j")
    
    def clear_data(self) -> None:
        """Clear all benchmark data."""
        if not self.driver:
            return
        with self.driver.session() as session:
            session.run("MATCH (n) DETACH DELETE n")
    
    def _ensure_connected(self) -> None:
        if not self.driver:
            raise RuntimeError("Not connected to Neo4j")
    
    # ==========================================================================
    # CRUD Operations (Document-like using node properties)
    # ==========================================================================
    
    def insert_one(self, collection: str, document: Dict[str, Any]) -> str:
        """Insert a single document as a node."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        doc_id = document.get('id') or document.get('_key')
        if not doc_id:
            doc_id = str(uuid.uuid4())
            document['id'] = doc_id
        
        # Store document as node with label = collection
        with self.driver.session() as session:
            session.run(
                f"MERGE (n:{safe_collection} {{id: $id}}) SET n += $props",
                id=doc_id,
                props=document
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
        safe_collection = _validate_label(collection)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection} {{id: $id}}) RETURN properties(n) as props",
                id=doc_id
            )
            record = result.single()
            if record:
                return dict(record['props'])
        return None
    
    def update_one(self, collection: str, doc_id: str, updates: Dict[str, Any]) -> bool:
        """Update a single document."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection} {{id: $id}}) SET n += $updates RETURN n",
                id=doc_id,
                updates=updates
            )
            return result.single() is not None
    
    def delete_one(self, collection: str, doc_id: str) -> bool:
        """Delete a single document."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection} {{id: $id}}) DELETE n RETURN count(n) as cnt",
                id=doc_id
            )
            record = result.single()
            return record and record['cnt'] > 0
    
    # ==========================================================================
    # Query Operations
    # ==========================================================================
    
    def find_by_field(self, collection: str, field: str, value: Any) -> List[Dict[str, Any]]:
        """Find documents by field value."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        safe_field = _validate_label(field)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection}) WHERE n.{safe_field} = $value RETURN properties(n) as props",
                value=value
            )
            return [dict(r['props']) for r in result]
    
    def find_by_range(self, collection: str, field: str, 
                      min_val: Any, max_val: Any) -> List[Dict[str, Any]]:
        """Find documents by field range."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        safe_field = _validate_label(field)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection}) WHERE n.{safe_field} >= $min AND n.{safe_field} <= $max RETURN properties(n) as props",
                min=min_val,
                max=max_val
            )
            return [dict(r['props']) for r in result]
    
    def count_by_field(self, collection: str, field: str, value: Any) -> int:
        """Count documents matching field value."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        safe_field = _validate_label(field)
        with self.driver.session() as session:
            result = session.run(
                f"MATCH (n:{safe_collection}) WHERE n.{safe_field} = $value RETURN count(n) as cnt",
                value=value
            )
            record = result.single()
            return record['cnt'] if record else 0
    
    def aggregate_sum(self, collection: str, field: str, 
                      group_by: Optional[str] = None) -> Any:
        """Aggregate sum of field values."""
        self._ensure_connected()
        safe_collection = _validate_label(collection)
        safe_field = _validate_label(field)
        with self.driver.session() as session:
            if group_by:
                safe_group_by = _validate_label(group_by)
                result = session.run(
                    f"MATCH (n:{safe_collection}) RETURN n.{safe_group_by} as grp, sum(n.{safe_field}) as total",
                )
                return [{"group": r['grp'], "sum": r['total']} for r in result]
            else:
                result = session.run(
                    f"MATCH (n:{safe_collection}) RETURN sum(n.{safe_field}) as total",
                )
                record = result.single()
                return record['total'] if record else 0
    
    # ==========================================================================
    # Graph Operations (Native Neo4j strength)
    # ==========================================================================
    
    def supports_graph_operations(self) -> bool:
        """Neo4j is a native graph database."""
        return True
    
    def insert_node(self, node_id: str, labels: List[str], 
                    properties: Dict[str, Any]) -> str:
        """Insert a graph node."""
        self._ensure_connected()
        # Validate all labels
        safe_labels = [_validate_label(label) for label in labels] if labels else ['Node']
        labels_str = ':'.join(safe_labels)
        properties['id'] = node_id
        
        with self.driver.session() as session:
            session.run(
                f"MERGE (n:{labels_str} {{id: $id}}) SET n += $props",
                id=node_id,
                props=properties
            )
        return node_id
    
    def insert_edge(self, from_id: str, to_id: str, 
                    edge_type: str, properties: Dict[str, Any]) -> str:
        """Insert a graph edge (relationship)."""
        self._ensure_connected()
        safe_edge_type = _validate_label(edge_type)
        edge_id = str(uuid.uuid4())
        properties['id'] = edge_id
        
        with self.driver.session() as session:
            session.run(
                f"""
                MATCH (a {{id: $from_id}})
                MATCH (b {{id: $to_id}})
                MERGE (a)-[r:{safe_edge_type} {{id: $edge_id}}]->(b)
                SET r += $props
                """,
                from_id=from_id,
                to_id=to_id,
                edge_id=edge_id,
                props=properties
            )
        return edge_id
    
    def traverse_bfs(self, start_node: str, max_depth: int) -> List[str]:
        """Perform BFS traversal from start node."""
        self._ensure_connected()
        # Validate max_depth is a reasonable positive integer
        if not isinstance(max_depth, int) or max_depth < 1 or max_depth > 100:
            raise ValueError("max_depth must be an integer between 1 and 100")
        with self.driver.session() as session:
            result = session.run(
                f"""
                MATCH (start {{id: $start_id}})
                MATCH path = (start)-[*1..{max_depth}]->(connected)
                RETURN DISTINCT connected.id as node_id
                """,
                start_id=start_node
            )
            return [start_node] + [r['node_id'] for r in result if r['node_id']]
    
    def shortest_path(self, from_id: str, to_id: str) -> List[str]:
        """Find shortest path between two nodes."""
        self._ensure_connected()
        with self.driver.session() as session:
            result = session.run(
                """
                MATCH (start {id: $from_id}), (end {id: $to_id})
                MATCH path = shortestPath((start)-[*..15]-(end))
                RETURN [n IN nodes(path) | n.id] as path_ids
                """,
                from_id=from_id,
                to_id=to_id
            )
            record = result.single()
            return record['path_ids'] if record else []
    
    # ==========================================================================
    # Vector Operations (Not natively supported)
    # ==========================================================================
    
    def supports_vector_search(self) -> bool:
        """Neo4j doesn't natively support vector search (requires GDS plugin)."""
        return False
    
    # ==========================================================================
    # Full-text Search
    # ==========================================================================
    
    def supports_fulltext_search(self) -> bool:
        """Neo4j supports full-text search with indexes."""
        return True
    
    def fulltext_search(self, collection: str, query: str, 
                        limit: int = 10) -> List[Dict[str, Any]]:
        """Perform full-text search (requires fulltext index)."""
        self._ensure_connected()
        # Note: Requires fulltext index to be created first
        with self.driver.session() as session:
            # Fallback to CONTAINS for simple search without index
            result = session.run(
                f"""
                MATCH (n:{collection})
                WHERE toLower(n.content) CONTAINS toLower($query)
                RETURN properties(n) as props
                LIMIT $limit
                """,
                query=query,
                limit=limit
            )
            return [dict(r['props']) for r in result]
