"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_database_adapters.py                          ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  ⚫ DRAFT                                        ║
    • Quality Score:   0.0/100                                        ║
    • Total Lines:     1170                                           ║
    • Open Issues:     TODOs: 0, Stubs: 19                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8964bef5bc  2026-02-28  feat(chimera): implement CapabilityMatrix for adapter cap... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📝 Draft / Stub                                              ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Test suite for CHIMERA database adapters - Multi-model coverage.

Tests cover:
- Relational CRUD/transactions
- Graph traversal/path queries
- Vector search (kNN/ANN, filters, metrics)
- Cross-model hybrid workloads
- Adapter capability detection
- Error handling

This test suite validates the core database adapter functionality
required for the CHIMERA benchmark suite.
"""

import pytest
import numpy as np
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from enum import Enum


# ============================================================================
# Mock Adapter Infrastructure (for testing without real databases)
# ============================================================================

class ErrorCode(Enum):
    """Error codes for database operations"""
    SUCCESS = 0
    NOT_IMPLEMENTED = 1
    INVALID_ARGUMENT = 2
    NOT_FOUND = 3
    ALREADY_EXISTS = 4
    CONNECTION_ERROR = 5
    TIMEOUT = 6
    TRANSACTION_ABORTED = 7


@dataclass
class Result:
    """Generic result type for operations"""
    success: bool
    value: Any = None
    error_code: ErrorCode = ErrorCode.SUCCESS
    error_message: str = ""
    
    @staticmethod
    def ok(value: Any = None):
        return Result(success=True, value=value, error_code=ErrorCode.SUCCESS)
    
    @staticmethod
    def err(code: ErrorCode, message: str):
        return Result(success=False, error_code=code, error_message=message)


class Capability(Enum):
    """Database capabilities"""
    RELATIONAL_QUERIES = "relational_queries"
    VECTOR_SEARCH = "vector_search"
    GRAPH_TRAVERSAL = "graph_traversal"
    DOCUMENT_STORE = "document_store"
    TRANSACTIONS = "transactions"
    FULL_TEXT_SEARCH = "full_text_search"


@dataclass
class Vector:
    """Vector/embedding representation"""
    data: List[float]
    metadata: Dict[str, Any] = None
    
    def __post_init__(self):
        if self.metadata is None:
            self.metadata = {}
    
    @property
    def dimensions(self) -> int:
        return len(self.data)


@dataclass
class GraphNode:
    """Graph node representation"""
    id: str
    label: str
    properties: Dict[str, Any]


@dataclass
class GraphEdge:
    """Graph edge representation"""
    id: str
    source_id: str
    target_id: str
    label: str
    properties: Dict[str, Any]
    weight: Optional[float] = None


@dataclass
class GraphPath:
    """Graph path representation"""
    nodes: List[GraphNode]
    edges: List[GraphEdge]
    total_weight: float = 0.0


@dataclass
class RelationalRow:
    """Relational row representation"""
    columns: Dict[str, Any]


@dataclass
class RelationalTable:
    """Relational table representation"""
    column_names: List[str]
    rows: List[RelationalRow]


@dataclass
class TransactionOptions:
    """Transaction configuration"""
    isolation_level: str = "READ_COMMITTED"
    timeout_ms: int = 5000
    read_only: bool = False


# ============================================================================
# Abstract Database Adapter Interfaces
# ============================================================================

class IRelationalAdapter:
    """Interface for relational database operations"""
    
    def execute_query(self, query: str, params: List[Any] = None) -> Result:
        """Execute SQL query"""
        raise NotImplementedError
    
    def insert_row(self, table_name: str, row: RelationalRow) -> Result:
        """Insert single row"""
        raise NotImplementedError
    
    def batch_insert(self, table_name: str, rows: List[RelationalRow]) -> Result:
        """Batch insert rows"""
        raise NotImplementedError


class IVectorAdapter:
    """Interface for vector similarity search"""
    
    def insert_vector(self, collection: str, vector: Vector) -> Result:
        """Insert vector"""
        raise NotImplementedError
    
    def batch_insert_vectors(self, collection: str, vectors: List[Vector]) -> Result:
        """Batch insert vectors"""
        raise NotImplementedError
    
    def search_vectors(self, collection: str, query_vector: Vector, k: int, 
                      filters: Dict[str, Any] = None, metric: str = "cosine") -> Result:
        """Search for similar vectors"""
        raise NotImplementedError
    
    def create_index(self, collection: str, dimensions: int, 
                    index_params: Dict[str, Any] = None) -> Result:
        """Create vector index"""
        raise NotImplementedError


class IGraphAdapter:
    """Interface for graph database operations"""
    
    def insert_node(self, node: GraphNode) -> Result:
        """Insert graph node"""
        raise NotImplementedError
    
    def insert_edge(self, edge: GraphEdge) -> Result:
        """Insert graph edge"""
        raise NotImplementedError
    
    def shortest_path(self, source_id: str, target_id: str, max_depth: int = 10) -> Result:
        """Find shortest path between nodes"""
        raise NotImplementedError
    
    def traverse(self, start_id: str, max_depth: int, edge_labels: List[str] = None) -> Result:
        """Traverse graph from starting node"""
        raise NotImplementedError


class ITransactionAdapter:
    """Interface for transaction management"""
    
    def begin_transaction(self, options: TransactionOptions = None) -> Result:
        """Begin transaction"""
        raise NotImplementedError
    
    def commit_transaction(self, txn_id: str) -> Result:
        """Commit transaction"""
        raise NotImplementedError
    
    def rollback_transaction(self, txn_id: str) -> Result:
        """Rollback transaction"""
        raise NotImplementedError


class IDatabaseAdapter(IRelationalAdapter, IVectorAdapter, IGraphAdapter, ITransactionAdapter):
    """Complete database adapter interface"""
    
    def connect(self, connection_string: str, options: Dict[str, str] = None) -> Result:
        """Connect to database"""
        raise NotImplementedError
    
    def disconnect(self) -> Result:
        """Disconnect from database"""
        raise NotImplementedError
    
    def is_connected(self) -> bool:
        """Check connection status"""
        raise NotImplementedError
    
    def has_capability(self, cap: Capability) -> bool:
        """Check if capability is supported"""
        raise NotImplementedError
    
    def get_capabilities(self) -> List[Capability]:
        """Get all supported capabilities"""
        raise NotImplementedError


# ============================================================================
# Mock Adapter Implementation (for testing)
# ============================================================================

class MockDatabaseAdapter(IDatabaseAdapter):
    """Mock adapter for testing"""
    
    def __init__(self):
        self.connected = False
        self.vectors = {}
        self.nodes = {}
        self.edges = {}
        self.tables = {}
        self.transactions = {}
        self._capabilities = [
            Capability.RELATIONAL_QUERIES,
            Capability.VECTOR_SEARCH,
            Capability.GRAPH_TRAVERSAL,
            Capability.TRANSACTIONS
        ]
    
    def connect(self, connection_string: str, options: Dict[str, str] = None) -> Result:
        self.connected = True
        return Result.ok(True)
    
    def disconnect(self) -> Result:
        self.connected = False
        return Result.ok(True)
    
    def is_connected(self) -> bool:
        return self.connected
    
    def has_capability(self, cap: Capability) -> bool:
        return cap in self._capabilities
    
    def get_capabilities(self) -> List[Capability]:
        return self._capabilities.copy()
    
    # Relational operations
    def execute_query(self, query: str, params: List[Any] = None) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        # Mock query execution
        table = RelationalTable(column_names=["id", "name"], rows=[])
        return Result.ok(table)
    
    def insert_row(self, table_name: str, row: RelationalRow) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if table_name not in self.tables:
            self.tables[table_name] = []
        self.tables[table_name].append(row)
        return Result.ok(1)
    
    def batch_insert(self, table_name: str, rows: List[RelationalRow]) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if table_name not in self.tables:
            self.tables[table_name] = []
        self.tables[table_name].extend(rows)
        return Result.ok(len(rows))
    
    # Vector operations
    def insert_vector(self, collection: str, vector: Vector) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if collection not in self.vectors:
            self.vectors[collection] = []
        vec_id = f"vec_{len(self.vectors[collection])}"
        self.vectors[collection].append((vec_id, vector))
        return Result.ok(vec_id)
    
    def batch_insert_vectors(self, collection: str, vectors: List[Vector]) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        count = 0
        for vec in vectors:
            result = self.insert_vector(collection, vec)
            if result.success:
                count += 1
        return Result.ok(count)
    
    def search_vectors(self, collection: str, query_vector: Vector, k: int,
                      filters: Dict[str, Any] = None, metric: str = "cosine") -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if collection not in self.vectors:
            return Result.ok([])
        
        # Simple cosine similarity search
        results = []
        for vec_id, stored_vec in self.vectors[collection]:
            # Check filters
            if filters:
                match = all(
                    stored_vec.metadata.get(key) == value 
                    for key, value in filters.items()
                )
                if not match:
                    continue
            
            # Calculate similarity
            sim = self._cosine_similarity(query_vector.data, stored_vec.data)
            results.append((stored_vec, sim))
        
        # Sort by similarity and return top k
        results.sort(key=lambda x: x[1], reverse=True)
        return Result.ok(results[:k])
    
    def create_index(self, collection: str, dimensions: int,
                    index_params: Dict[str, Any] = None) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if collection not in self.vectors:
            self.vectors[collection] = []
        return Result.ok(True)
    
    # Graph operations
    def insert_node(self, node: GraphNode) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if node.id in self.nodes:
            return Result.err(ErrorCode.ALREADY_EXISTS, f"Node {node.id} already exists")
        self.nodes[node.id] = node
        return Result.ok(node.id)
    
    def insert_edge(self, edge: GraphEdge) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if edge.source_id not in self.nodes:
            return Result.err(ErrorCode.NOT_FOUND, f"Source node {edge.source_id} not found")
        if edge.target_id not in self.nodes:
            return Result.err(ErrorCode.NOT_FOUND, f"Target node {edge.target_id} not found")
        self.edges[edge.id] = edge
        return Result.ok(edge.id)
    
    def shortest_path(self, source_id: str, target_id: str, max_depth: int = 10) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if source_id not in self.nodes:
            return Result.err(ErrorCode.NOT_FOUND, f"Source node {source_id} not found")
        if target_id not in self.nodes:
            return Result.err(ErrorCode.NOT_FOUND, f"Target node {target_id} not found")
        
        # Simple BFS for shortest path
        path = self._bfs_shortest_path(source_id, target_id, max_depth)
        if path:
            return Result.ok(path)
        else:
            return Result.err(ErrorCode.NOT_FOUND, "No path found")
    
    def traverse(self, start_id: str, max_depth: int, edge_labels: List[str] = None) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if start_id not in self.nodes:
            return Result.err(ErrorCode.NOT_FOUND, f"Start node {start_id} not found")
        
        visited_nodes = self._bfs_traverse(start_id, max_depth, edge_labels)
        return Result.ok(visited_nodes)
    
    # Transaction operations
    def begin_transaction(self, options: TransactionOptions = None) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        txn_id = f"txn_{len(self.transactions)}"
        self.transactions[txn_id] = {"status": "active", "options": options}
        return Result.ok(txn_id)
    
    def commit_transaction(self, txn_id: str) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if txn_id not in self.transactions:
            return Result.err(ErrorCode.NOT_FOUND, f"Transaction {txn_id} not found")
        self.transactions[txn_id]["status"] = "committed"
        return Result.ok(True)
    
    def rollback_transaction(self, txn_id: str) -> Result:
        if not self.connected:
            return Result.err(ErrorCode.CONNECTION_ERROR, "Not connected")
        if txn_id not in self.transactions:
            return Result.err(ErrorCode.NOT_FOUND, f"Transaction {txn_id} not found")
        self.transactions[txn_id]["status"] = "rolled_back"
        return Result.ok(True)
    
    # Helper methods
    def _cosine_similarity(self, vec1: List[float], vec2: List[float]) -> float:
        """Calculate cosine similarity between two vectors"""
        if len(vec1) != len(vec2):
            return 0.0
        dot_product = sum(a * b for a, b in zip(vec1, vec2))
        norm1 = sum(a * a for a in vec1) ** 0.5
        norm2 = sum(b * b for b in vec2) ** 0.5
        if norm1 == 0 or norm2 == 0:
            return 0.0
        return dot_product / (norm1 * norm2)
    
    def _bfs_shortest_path(self, source_id: str, target_id: str, max_depth: int) -> Optional[GraphPath]:
        """Find shortest path using BFS"""
        from collections import deque
        
        queue = deque([(source_id, [self.nodes[source_id]], [])])
        visited = {source_id}
        
        while queue:
            current_id, node_path, edge_path = queue.popleft()
            
            if len(node_path) > max_depth:
                continue
            
            if current_id == target_id:
                total_weight = sum(e.weight or 1.0 for e in edge_path)
                return GraphPath(nodes=node_path, edges=edge_path, total_weight=total_weight)
            
            # Find outgoing edges
            for edge in self.edges.values():
                if edge.source_id == current_id and edge.target_id not in visited:
                    visited.add(edge.target_id)
                    new_node_path = node_path + [self.nodes[edge.target_id]]
                    new_edge_path = edge_path + [edge]
                    queue.append((edge.target_id, new_node_path, new_edge_path))
        
        return None
    
    def _bfs_traverse(self, start_id: str, max_depth: int, edge_labels: List[str] = None) -> List[GraphNode]:
        """Traverse graph using BFS"""
        from collections import deque
        
        visited_nodes = [self.nodes[start_id]]
        visited = {start_id}
        queue = deque([(start_id, 0)])
        
        while queue:
            current_id, depth = queue.popleft()
            
            if depth >= max_depth:
                continue
            
            for edge in self.edges.values():
                if edge.source_id == current_id:
                    # Check edge label filter
                    if edge_labels and edge.label not in edge_labels:
                        continue
                    
                    if edge.target_id not in visited:
                        visited.add(edge.target_id)
                        visited_nodes.append(self.nodes[edge.target_id])
                        queue.append((edge.target_id, depth + 1))
        
        return visited_nodes


# ============================================================================
# Test Cases
# ============================================================================

class TestRelationalOperations:
    """Test relational database operations"""
    
    @pytest.fixture
    def adapter(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        return adapter
    
    def test_connect_disconnect(self):
        adapter = MockDatabaseAdapter()
        assert not adapter.is_connected()
        
        result = adapter.connect("mock://localhost")
        assert result.success
        assert adapter.is_connected()
        
        result = adapter.disconnect()
        assert result.success
        assert not adapter.is_connected()
    
    def test_insert_single_row(self, adapter):
        row = RelationalRow(columns={"id": 1, "name": "Alice", "age": 30})
        result = adapter.insert_row("users", row)
        
        assert result.success
        assert result.value == 1
        assert "users" in adapter.tables
        assert len(adapter.tables["users"]) == 1
    
    def test_batch_insert(self, adapter):
        rows = [
            RelationalRow(columns={"id": i, "name": f"User{i}", "age": 20 + i})
            for i in range(100)
        ]
        
        result = adapter.batch_insert("users", rows)
        
        assert result.success
        assert result.value == 100
        assert len(adapter.tables["users"]) == 100
    
    def test_execute_query(self, adapter):
        result = adapter.execute_query("SELECT * FROM users WHERE age > ?", [25])
        
        assert result.success
        assert isinstance(result.value, RelationalTable)
        assert "id" in result.value.column_names
        assert "name" in result.value.column_names


class TestVectorOperations:
    """Test vector similarity search operations"""
    
    @pytest.fixture
    def adapter(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        return adapter
    
    def test_create_vector_index(self, adapter):
        result = adapter.create_index("embeddings", dimensions=128)
        
        assert result.success
        assert "embeddings" in adapter.vectors
    
    def test_insert_vector(self, adapter):
        vector = Vector(data=[0.1, 0.2, 0.3, 0.4], metadata={"category": "test"})
        result = adapter.insert_vector("embeddings", vector)
        
        assert result.success
        assert result.value.startswith("vec_")
        assert len(adapter.vectors["embeddings"]) == 1
    
    def test_batch_insert_vectors(self, adapter):
        vectors = [
            Vector(data=[float(i), float(i+1), float(i+2)], metadata={"index": i})
            for i in range(1000)
        ]
        
        result = adapter.batch_insert_vectors("embeddings", vectors)
        
        assert result.success
        assert result.value == 1000
        assert len(adapter.vectors["embeddings"]) == 1000
    
    def test_vector_search_knn(self, adapter):
        # Insert test vectors
        vectors = [
            Vector(data=[1.0, 0.0, 0.0], metadata={"label": "A"}),
            Vector(data=[0.0, 1.0, 0.0], metadata={"label": "B"}),
            Vector(data=[0.0, 0.0, 1.0], metadata={"label": "C"}),
            Vector(data=[0.9, 0.1, 0.0], metadata={"label": "D"}),  # Similar to A
        ]
        adapter.batch_insert_vectors("test_collection", vectors)
        
        # Search for vectors similar to [1.0, 0.0, 0.0]
        query = Vector(data=[1.0, 0.0, 0.0])
        result = adapter.search_vectors("test_collection", query, k=2)
        
        assert result.success
        results = result.value
        assert len(results) == 2
        # First result should be most similar
        assert results[0][1] > results[1][1]  # Higher similarity score
    
    def test_vector_search_with_filters(self, adapter):
        # Insert vectors with metadata
        vectors = [
            Vector(data=[1.0, 0.0], metadata={"category": "A", "active": True}),
            Vector(data=[0.9, 0.1], metadata={"category": "A", "active": False}),
            Vector(data=[0.8, 0.2], metadata={"category": "B", "active": True}),
        ]
        adapter.batch_insert_vectors("filtered_collection", vectors)
        
        # Search with filter
        query = Vector(data=[1.0, 0.0])
        result = adapter.search_vectors(
            "filtered_collection", 
            query, 
            k=5, 
            filters={"category": "A", "active": True}
        )
        
        assert result.success
        results = result.value
        assert len(results) == 1  # Only one vector matches filter
        assert results[0][0].metadata["category"] == "A"
        assert results[0][0].metadata["active"] is True
    
    def test_vector_distance_metrics(self, adapter):
        """Test different distance metrics (cosine, euclidean, etc.)"""
        vec1 = Vector(data=[1.0, 0.0, 0.0])
        vec2 = Vector(data=[0.0, 1.0, 0.0])
        
        # Cosine similarity should be 0 (orthogonal vectors)
        sim = adapter._cosine_similarity(vec1.data, vec2.data)
        assert abs(sim - 0.0) < 0.01


class TestGraphOperations:
    """Test graph traversal and path queries"""
    
    @pytest.fixture
    def adapter(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        return adapter
    
    @pytest.fixture
    def simple_graph(self, adapter):
        """Create a simple graph: A -> B -> C -> D"""
        nodes = [
            GraphNode(id="A", label="Person", properties={"name": "Alice"}),
            GraphNode(id="B", label="Person", properties={"name": "Bob"}),
            GraphNode(id="C", label="Person", properties={"name": "Carol"}),
            GraphNode(id="D", label="Person", properties={"name": "Dave"}),
        ]
        
        for node in nodes:
            adapter.insert_node(node)
        
        edges = [
            GraphEdge(id="e1", source_id="A", target_id="B", label="KNOWS", properties={}, weight=1.0),
            GraphEdge(id="e2", source_id="B", target_id="C", label="KNOWS", properties={}, weight=1.0),
            GraphEdge(id="e3", source_id="C", target_id="D", label="KNOWS", properties={}, weight=1.0),
        ]
        
        for edge in edges:
            adapter.insert_edge(edge)
        
        return adapter
    
    def test_insert_node(self, adapter):
        node = GraphNode(id="node1", label="Person", properties={"name": "Alice", "age": 30})
        result = adapter.insert_node(node)
        
        assert result.success
        assert result.value == "node1"
        assert "node1" in adapter.nodes
    
    def test_insert_duplicate_node(self, adapter):
        node = GraphNode(id="node1", label="Person", properties={"name": "Alice"})
        adapter.insert_node(node)
        
        result = adapter.insert_node(node)
        
        assert not result.success
        assert result.error_code == ErrorCode.ALREADY_EXISTS
    
    def test_insert_edge(self, adapter):
        node1 = GraphNode(id="n1", label="Person", properties={"name": "Alice"})
        node2 = GraphNode(id="n2", label="Person", properties={"name": "Bob"})
        adapter.insert_node(node1)
        adapter.insert_node(node2)
        
        edge = GraphEdge(
            id="e1",
            source_id="n1",
            target_id="n2",
            label="KNOWS",
            properties={"since": 2020},
            weight=1.0
        )
        result = adapter.insert_edge(edge)
        
        assert result.success
        assert result.value == "e1"
        assert "e1" in adapter.edges
    
    def test_shortest_path(self, simple_graph):
        result = simple_graph.shortest_path("A", "D", max_depth=10)
        
        assert result.success
        path = result.value
        assert isinstance(path, GraphPath)
        assert len(path.nodes) == 4
        assert len(path.edges) == 3
        assert path.nodes[0].id == "A"
        assert path.nodes[-1].id == "D"
        assert path.total_weight == 3.0
    
    def test_shortest_path_no_connection(self, simple_graph):
        # Add disconnected node
        simple_graph.insert_node(GraphNode(id="Z", label="Person", properties={"name": "Zoe"}))
        
        result = simple_graph.shortest_path("A", "Z", max_depth=10)
        
        assert not result.success
        assert result.error_code == ErrorCode.NOT_FOUND
    
    def test_graph_traversal(self, simple_graph):
        result = simple_graph.traverse("A", max_depth=2)
        
        assert result.success
        nodes = result.value
        assert len(nodes) >= 1  # At least starting node
        assert nodes[0].id == "A"
        # Should reach B and C within depth 2
        node_ids = [n.id for n in nodes]
        assert "B" in node_ids
        assert "C" in node_ids
    
    def test_graph_traversal_with_edge_filter(self, simple_graph):
        # Add edge with different label
        simple_graph.insert_edge(GraphEdge(
            id="e4",
            source_id="A",
            target_id="D",
            label="LIKES",
            properties={},
            weight=1.0
        ))
        
        # Traverse only through "KNOWS" edges
        result = simple_graph.traverse("A", max_depth=10, edge_labels=["KNOWS"])
        
        assert result.success
        nodes = result.value
        # Should not include direct path to D via "LIKES" edge


class TestTransactionOperations:
    """Test transaction management (ACID)"""
    
    @pytest.fixture
    def adapter(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        return adapter
    
    def test_begin_transaction(self, adapter):
        result = adapter.begin_transaction()
        
        assert result.success
        txn_id = result.value
        assert txn_id.startswith("txn_")
        assert txn_id in adapter.transactions
    
    def test_commit_transaction(self, adapter):
        txn_result = adapter.begin_transaction()
        txn_id = txn_result.value
        
        result = adapter.commit_transaction(txn_id)
        
        assert result.success
        assert adapter.transactions[txn_id]["status"] == "committed"
    
    def test_rollback_transaction(self, adapter):
        txn_result = adapter.begin_transaction()
        txn_id = txn_result.value
        
        result = adapter.rollback_transaction(txn_id)
        
        assert result.success
        assert adapter.transactions[txn_id]["status"] == "rolled_back"
    
    def test_transaction_with_options(self, adapter):
        options = TransactionOptions(
            isolation_level="SERIALIZABLE",
            timeout_ms=10000,
            read_only=True
        )
        
        result = adapter.begin_transaction(options)
        
        assert result.success
        txn_id = result.value
        assert adapter.transactions[txn_id]["options"] == options


class TestHybridWorkloads:
    """Test cross-model hybrid workloads"""
    
    @pytest.fixture
    def adapter(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        return adapter
    
    def test_hybrid_relational_and_vector(self, adapter):
        """Test combining relational and vector operations"""
        # Insert relational data
        row = RelationalRow(columns={"user_id": 1, "name": "Alice"})
        adapter.insert_row("users", row)
        
        # Insert corresponding vector embedding
        vector = Vector(data=[0.1, 0.2, 0.3], metadata={"user_id": 1})
        vec_result = adapter.insert_vector("user_embeddings", vector)
        
        assert vec_result.success
        
        # Search for similar users by vector
        query_vec = Vector(data=[0.15, 0.25, 0.35])
        search_result = adapter.search_vectors("user_embeddings", query_vec, k=5)
        
        assert search_result.success
        assert len(search_result.value) > 0
    
    def test_hybrid_graph_and_vector(self, adapter):
        """Test combining graph and vector operations"""
        # Create graph nodes with embeddings
        node = GraphNode(id="user1", label="User", properties={"name": "Alice"})
        adapter.insert_node(node)
        
        # Store embedding for the node
        vector = Vector(data=[0.1, 0.2, 0.3], metadata={"node_id": "user1"})
        adapter.insert_vector("node_embeddings", vector)
        
        # Find similar nodes by embedding
        query_vec = Vector(data=[0.12, 0.22, 0.32])
        result = adapter.search_vectors(
            "node_embeddings", 
            query_vec, 
            k=5,
            filters={"node_id": "user1"}
        )
        
        assert result.success
    
    def test_hybrid_transaction_across_models(self, adapter):
        """Test transaction spanning multiple data models"""
        # Begin transaction
        txn_result = adapter.begin_transaction()
        txn_id = txn_result.value
        
        # Insert relational data
        row = RelationalRow(columns={"id": 1, "value": "test"})
        rel_result = adapter.insert_row("data", row)
        
        # Insert vector data
        vector = Vector(data=[1.0, 2.0, 3.0])
        vec_result = adapter.insert_vector("vectors", vector)
        
        # Insert graph data
        node = GraphNode(id="node1", label="Data", properties={"ref_id": 1})
        graph_result = adapter.insert_node(node)
        
        # All operations should succeed
        assert rel_result.success
        assert vec_result.success
        assert graph_result.success
        
        # Commit transaction
        commit_result = adapter.commit_transaction(txn_id)
        assert commit_result.success


class TestCapabilityDiscovery:
    """Test capability detection and feature support"""
    
    def test_capability_detection(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        
        assert adapter.has_capability(Capability.RELATIONAL_QUERIES)
        assert adapter.has_capability(Capability.VECTOR_SEARCH)
        assert adapter.has_capability(Capability.GRAPH_TRAVERSAL)
        assert adapter.has_capability(Capability.TRANSACTIONS)
    
    def test_get_all_capabilities(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        
        capabilities = adapter.get_capabilities()
        
        assert Capability.RELATIONAL_QUERIES in capabilities
        assert Capability.VECTOR_SEARCH in capabilities
        assert Capability.GRAPH_TRAVERSAL in capabilities
        assert Capability.TRANSACTIONS in capabilities


class TestErrorHandling:
    """Test error handling and edge cases"""
    
    def test_operation_without_connection(self):
        adapter = MockDatabaseAdapter()
        
        result = adapter.insert_row("users", RelationalRow(columns={"id": 1}))
        
        assert not result.success
        assert result.error_code == ErrorCode.CONNECTION_ERROR
    
    def test_invalid_graph_edge(self):
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        
        # Try to insert edge without nodes
        edge = GraphEdge(
            id="e1",
            source_id="nonexistent1",
            target_id="nonexistent2",
            label="KNOWS",
            properties={}
        )
        result = adapter.insert_edge(edge)
        
        assert not result.success
        assert result.error_code == ErrorCode.NOT_FOUND


from .capability_matrix import CapabilityMatrix, CapableAdapter


class TestCapabilityMatrix:
    """Tests for CapabilityMatrix (ROADMAP Issue #2376)."""

    def _make_adapter(self, caps):
        """Return a minimal object that satisfies CapableAdapter with *caps*."""
        class _Stub:
            def get_capabilities(self):
                return caps
        return _Stub()

    # ------------------------------------------------------------------
    # Registration
    # ------------------------------------------------------------------

    def test_register_adapter(self):
        matrix = CapabilityMatrix()
        adapter = self._make_adapter([Capability.RELATIONAL_QUERIES, Capability.VECTOR_SEARCH])
        matrix.register("Alpha", adapter)
        assert "Alpha" in matrix.registered_systems()

    def test_register_capabilities_direct(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("Beta", [Capability.VECTOR_SEARCH, Capability.GRAPH_TRAVERSAL])
        assert Capability.VECTOR_SEARCH in matrix.capabilities_of("Beta")

    def test_register_empty_name_raises(self):
        matrix = CapabilityMatrix()
        with pytest.raises(ValueError):
            matrix.register_capabilities("", [Capability.RELATIONAL_QUERIES])

    def test_register_non_capable_adapter_raises(self):
        matrix = CapabilityMatrix()
        with pytest.raises(TypeError):
            matrix.register("Bad", object())  # type: ignore[arg-type]

    def test_register_overwrites_existing(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        matrix.register_capabilities("A", [Capability.VECTOR_SEARCH])
        assert matrix.capabilities_of("A") == [Capability.VECTOR_SEARCH]

    # ------------------------------------------------------------------
    # Query helpers
    # ------------------------------------------------------------------

    def test_supports_true(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        assert matrix.supports("A", Capability.RELATIONAL_QUERIES)

    def test_supports_false(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        assert not matrix.supports("A", Capability.VECTOR_SEARCH)

    def test_supports_unknown_system(self):
        matrix = CapabilityMatrix()
        assert not matrix.supports("NonExistent", Capability.RELATIONAL_QUERIES)

    def test_capabilities_of_known_system(self):
        matrix = CapabilityMatrix()
        caps = [Capability.RELATIONAL_QUERIES, Capability.TRANSACTIONS]
        matrix.register_capabilities("DB", caps)
        assert set(matrix.capabilities_of("DB")) == set(caps)

    def test_capabilities_of_unknown_system(self):
        matrix = CapabilityMatrix()
        assert matrix.capabilities_of("Unknown") == []

    def test_systems_supporting_sorted(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("Zebra", [Capability.VECTOR_SEARCH])
        matrix.register_capabilities("Alpha", [Capability.VECTOR_SEARCH, Capability.GRAPH_TRAVERSAL])
        matrix.register_capabilities("Mango", [Capability.RELATIONAL_QUERIES])
        result = matrix.systems_supporting(Capability.VECTOR_SEARCH)
        assert result == ["Alpha", "Zebra"]

    def test_systems_supporting_empty_when_none(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        assert matrix.systems_supporting(Capability.FULL_TEXT_SEARCH) == []

    def test_registered_systems_order(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("First", [])
        matrix.register_capabilities("Second", [])
        assert matrix.registered_systems() == ["First", "Second"]

    def test_all_capabilities_deduped_sorted(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.VECTOR_SEARCH, Capability.RELATIONAL_QUERIES])
        matrix.register_capabilities("B", [Capability.RELATIONAL_QUERIES, Capability.GRAPH_TRAVERSAL])
        all_caps = matrix.all_capabilities()
        assert len(all_caps) == 3
        assert Capability.RELATIONAL_QUERIES in all_caps
        assert Capability.VECTOR_SEARCH in all_caps
        assert Capability.GRAPH_TRAVERSAL in all_caps

    # ------------------------------------------------------------------
    # to_dict
    # ------------------------------------------------------------------

    def test_to_dict_structure(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES, Capability.VECTOR_SEARCH])
        matrix.register_capabilities("B", [Capability.VECTOR_SEARCH])
        data = matrix.to_dict()
        assert "systems" in data
        assert "capabilities" in data
        assert "matrix" in data
        assert "coverage" in data

    def test_to_dict_systems(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("X", [Capability.RELATIONAL_QUERIES])
        matrix.register_capabilities("Y", [Capability.VECTOR_SEARCH])
        data = matrix.to_dict()
        assert data["systems"] == ["X", "Y"]

    def test_to_dict_matrix_values(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        matrix.register_capabilities("B", [Capability.VECTOR_SEARCH])
        data = matrix.to_dict()
        assert data["matrix"]["A"][Capability.RELATIONAL_QUERIES.value] is True
        assert data["matrix"]["A"][Capability.VECTOR_SEARCH.value] is False
        assert data["matrix"]["B"][Capability.VECTOR_SEARCH.value] is True

    def test_to_dict_coverage(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES, Capability.VECTOR_SEARCH])
        matrix.register_capabilities("B", [Capability.VECTOR_SEARCH])
        data = matrix.to_dict()
        assert data["coverage"][Capability.VECTOR_SEARCH.value] == 2
        assert data["coverage"][Capability.RELATIONAL_QUERIES.value] == 1

    def test_to_dict_json_serializable(self):
        import json
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        json.dumps(matrix.to_dict())  # must not raise

    def test_to_dict_empty_matrix(self):
        matrix = CapabilityMatrix()
        data = matrix.to_dict()
        assert data["systems"] == []
        assert data["capabilities"] == []
        assert data["matrix"] == {}
        assert data["coverage"] == {}

    # ------------------------------------------------------------------
    # to_text_table
    # ------------------------------------------------------------------

    def test_to_text_table_contains_system_name(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("ThemisDB", [Capability.RELATIONAL_QUERIES])
        table = matrix.to_text_table()
        assert "ThemisDB" in table

    def test_to_text_table_contains_capability(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.VECTOR_SEARCH])
        table = matrix.to_text_table()
        assert "vector_search" in table

    def test_to_text_table_marks(self):
        matrix = CapabilityMatrix()
        matrix.register_capabilities("A", [Capability.RELATIONAL_QUERIES])
        matrix.register_capabilities("B", [])
        table = matrix.to_text_table(supported_mark="Y", unsupported_mark="N")
        assert "Y" in table
        assert "N" in table

    def test_to_text_table_empty(self):
        matrix = CapabilityMatrix()
        assert "empty" in matrix.to_text_table()

    # ------------------------------------------------------------------
    # Integration: use MockDatabaseAdapter
    # ------------------------------------------------------------------

    def test_register_mock_adapter(self):
        matrix = CapabilityMatrix()
        adapter = MockDatabaseAdapter()
        adapter.connect("mock://localhost")
        matrix.register("Mock", adapter)
        assert matrix.supports("Mock", Capability.RELATIONAL_QUERIES)
        assert matrix.supports("Mock", Capability.VECTOR_SEARCH)
        assert matrix.supports("Mock", Capability.GRAPH_TRAVERSAL)
        assert matrix.supports("Mock", Capability.TRANSACTIONS)

    def test_multi_system_matrix(self):
        """Full integration: register two adapters, verify matrix correctness."""
        matrix = CapabilityMatrix()

        full_adapter = MockDatabaseAdapter()
        full_adapter.connect("mock://full")
        matrix.register("FullDB", full_adapter)

        matrix.register_capabilities("VectorOnly", [Capability.VECTOR_SEARCH])

        assert matrix.supports("FullDB", Capability.RELATIONAL_QUERIES)
        assert not matrix.supports("VectorOnly", Capability.RELATIONAL_QUERIES)
        assert matrix.supports("VectorOnly", Capability.VECTOR_SEARCH)

        systems = matrix.systems_supporting(Capability.VECTOR_SEARCH)
        assert "FullDB" in systems
        assert "VectorOnly" in systems

        data = matrix.to_dict()
        assert len(data["systems"]) == 2
        assert data["coverage"][Capability.VECTOR_SEARCH.value] == 2


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
