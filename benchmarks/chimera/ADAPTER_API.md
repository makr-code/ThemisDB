# CHIMERA Suite: Adapter API Documentation

**Version:** 1.0.0  
**Last Updated:** April 2026  
**Status:** ✅ STABLE

---

## Table of Contents

1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Adapter Interface](#adapter-interface)
4. [Implementation Guide](#implementation-guide)
5. [Data Formats](#data-formats)
6. [Error Handling](#error-handling)
7. [Testing](#testing)
8. [Examples](#examples)
9. [Best Practices](#best-practices)

---

## 1. Introduction

The CHIMERA Adapter API enables integration of any database system with the CHIMERA benchmark suite. This document provides the complete specification for implementing a vendor-neutral adapter.

### 1.1 Purpose

The Adapter API:
- Provides standard interface for database integration
- Ensures vendor-neutral implementation
- Enables fair, reproducible benchmarks
- Supports multiple data models

### 1.2 Design Principles

- **Simplicity**: Minimal required methods
- **Neutrality**: No vendor-specific features
- **Extensibility**: Support for custom operations
- **Testability**: Easy to validate implementations

---

## 2. Architecture Overview

### 2.1 System Architecture

```
┌────────────────────────────────────────────────┐
│         CHIMERA Benchmark Engine               │
│  (Workload execution, statistics, reporting)   │
└────────────────────┬───────────────────────────┘
                     │
          ┌──────────▼──────────┐
          │  Adapter Interface  │
          │   (This Spec)       │
          └──────────┬──────────┘
                     │
     ┌───────────────┼───────────────┐
     │               │               │
┌────▼────┐    ┌────▼────┐    ┌────▼────┐
│System A │    │System B │    │System C │
│Adapter  │    │Adapter  │    │Adapter  │
└────┬────┘    └────┬────┘    └────┬────┘
     │              │              │
┌────▼────┐    ┌────▼────┐    ┌────▼────┐
│Database │    │Database │    │Database │
│System A │    │System B │    │System C │
└─────────┘    └─────────┘    └─────────┘
```

### 2.2 Adapter Responsibilities

The adapter is responsible for:
- Connection management
- Query translation
- Result format conversion
- Error handling
- Resource cleanup

The adapter is **NOT** responsible for:
- Workload generation
- Performance measurement
- Statistical analysis
- Report generation

---

## 3. Adapter Interface

### 3.1 Core Interface (Python)

```python
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from abc import ABC, abstractmethod


@dataclass
class ConnectionConfig:
    """Configuration for database connection"""
    host: str
    port: int
    database: str
    username: Optional[str] = None
    password: Optional[str] = None
    options: Dict[str, Any] = None


@dataclass
class QueryResult:
    """Result from a database query"""
    rows: List[Dict[str, Any]]
    execution_time_ms: float
    rows_affected: int = 0
    error: Optional[str] = None


class DatabaseAdapter(ABC):
    """
    Abstract base class for CHIMERA database adapters.
    
    All adapters must implement this interface to ensure
    vendor-neutral benchmark execution.
    """
    
    @abstractmethod
    def connect(self, config: ConnectionConfig) -> bool:
        """
        Establish connection to the database.
        
        Args:
            config: Connection configuration
            
        Returns:
            True if connection successful, False otherwise
        """
        pass
    
    @abstractmethod
    def disconnect(self) -> bool:
        """
        Close database connection and cleanup resources.
        
        Returns:
            True if disconnection successful, False otherwise
        """
        pass
    
    @abstractmethod
    def execute_query(self, query: str, params: Optional[Dict] = None) -> QueryResult:
        """
        Execute a query and return results.
        
        Args:
            query: Query string (adapter should translate if needed)
            params: Optional query parameters
            
        Returns:
            QueryResult with rows and execution time
        """
        pass
    
    @abstractmethod
    def insert(self, collection: str, document: Dict[str, Any]) -> bool:
        """
        Insert a document/record.
        
        Args:
            collection: Collection/table name
            document: Document/record to insert
            
        Returns:
            True if insert successful, False otherwise
        """
        pass
    
    @abstractmethod
    def batch_insert(self, collection: str, documents: List[Dict[str, Any]]) -> int:
        """
        Insert multiple documents/records.
        
        Args:
            collection: Collection/table name
            documents: List of documents/records
            
        Returns:
            Number of documents successfully inserted
        """
        pass
    
    @abstractmethod
    def update(self, collection: str, query: Dict, update: Dict) -> int:
        """
        Update documents/records matching query.
        
        Args:
            collection: Collection/table name
            query: Query to match documents
            update: Update operations
            
        Returns:
            Number of documents updated
        """
        pass
    
    @abstractmethod
    def delete(self, collection: str, query: Dict) -> int:
        """
        Delete documents/records matching query.
        
        Args:
            collection: Collection/table name
            query: Query to match documents
            
        Returns:
            Number of documents deleted
        """
        pass
    
    @abstractmethod
    def create_index(self, collection: str, fields: List[str], 
                     index_type: str = "btree") -> bool:
        """
        Create an index on specified fields.
        
        Args:
            collection: Collection/table name
            fields: Fields to index
            index_type: Type of index (btree, hash, vector, etc.)
            
        Returns:
            True if index created successfully
        """
        pass
    
    @abstractmethod
    def get_stats(self) -> Dict[str, Any]:
        """
        Get database statistics.
        
        Returns:
            Dictionary with statistics (connections, ops, cache, etc.)
        """
        pass
    
    @abstractmethod
    def reset(self) -> bool:
        """
        Reset database to initial state (clear data, drop indexes).
        
        Returns:
            True if reset successful
        """
        pass
    
    @property
    @abstractmethod
    def adapter_name(self) -> str:
        """
        Get adapter name (must be vendor-neutral).
        
        Examples: "PostgreSQLAdapter", "MongoDBAdapter", "NativeAdapter"
        """
        pass
    
    @property
    @abstractmethod
    def supported_features(self) -> List[str]:
        """
        Get list of supported features.
        
        Returns:
            List of feature strings:
            - "transactions"
            - "vector_search"
            - "graph_queries"
            - "full_text_search"
            - "geospatial"
        """
        pass
```

### 3.2 Optional Extensions

```python
class VectorSearchAdapter(DatabaseAdapter):
    """Optional extension for vector search capabilities"""
    
    @abstractmethod
    def insert_vector(self, collection: str, vector: List[float], 
                      metadata: Dict[str, Any]) -> bool:
        """Insert a vector with metadata"""
        pass
    
    @abstractmethod
    def vector_search(self, collection: str, query_vector: List[float],
                      k: int = 10, distance_metric: str = "cosine") -> QueryResult:
        """Search for k nearest neighbors"""
        pass


class GraphAdapter(DatabaseAdapter):
    """Optional extension for graph database capabilities"""
    
    @abstractmethod
    def add_vertex(self, label: str, properties: Dict[str, Any]) -> str:
        """Add a vertex/node, return vertex ID"""
        pass
    
    @abstractmethod
    def add_edge(self, from_vertex: str, to_vertex: str, 
                 label: str, properties: Dict[str, Any]) -> str:
        """Add an edge, return edge ID"""
        pass
    
    @abstractmethod
    def traverse(self, start_vertex: str, direction: str = "out",
                 max_depth: int = 3) -> QueryResult:
        """Traverse graph from starting vertex"""
        pass
```

---

## 4. Implementation Guide

### 4.1 Basic Adapter Template

```python
from chimera.adapters import DatabaseAdapter, ConnectionConfig, QueryResult
from typing import Dict, List, Any, Optional
import time


class MySystemAdapter(DatabaseAdapter):
    """
    Vendor-neutral adapter for [System Description].
    
    Implements the CHIMERA DatabaseAdapter interface.
    """
    
    def __init__(self):
        self.connection = None
        self.client = None
    
    def connect(self, config: ConnectionConfig) -> bool:
        """Establish connection"""
        try:
            # Import your database client
            import your_database_client
            
            self.client = your_database_client.Client(
                host=config.host,
                port=config.port,
                database=config.database,
                username=config.username,
                password=config.password,
                **config.options or {}
            )
            
            self.connection = self.client.connect()
            return self.connection is not None
            
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self) -> bool:
        """Close connection"""
        if self.connection:
            try:
                self.connection.close()
                return True
            except Exception as e:
                print(f"Disconnect failed: {e}")
                return False
        return True
    
    def execute_query(self, query: str, params: Optional[Dict] = None) -> QueryResult:
        """Execute query"""
        start_time = time.time()
        
        try:
            cursor = self.connection.execute(query, params or {})
            rows = [dict(row) for row in cursor.fetchall()]
            
            execution_time = (time.time() - start_time) * 1000  # Convert to ms
            
            return QueryResult(
                rows=rows,
                execution_time_ms=execution_time,
                rows_affected=cursor.rowcount
            )
            
        except Exception as e:
            return QueryResult(
                rows=[],
                execution_time_ms=0,
                error=str(e)
            )
    
    def insert(self, collection: str, document: Dict[str, Any]) -> bool:
        """Insert single document"""
        try:
            self.connection.collection(collection).insert_one(document)
            return True
        except Exception as e:
            print(f"Insert failed: {e}")
            return False
    
    def batch_insert(self, collection: str, documents: List[Dict[str, Any]]) -> int:
        """Insert multiple documents"""
        try:
            result = self.connection.collection(collection).insert_many(documents)
            return len(result.inserted_ids)
        except Exception as e:
            print(f"Batch insert failed: {e}")
            return 0
    
    def update(self, collection: str, query: Dict, update: Dict) -> int:
        """Update documents"""
        try:
            result = self.connection.collection(collection).update_many(query, update)
            return result.modified_count
        except Exception:
            return 0
    
    def delete(self, collection: str, query: Dict) -> int:
        """Delete documents"""
        try:
            result = self.connection.collection(collection).delete_many(query)
            return result.deleted_count
        except Exception:
            return 0
    
    def create_index(self, collection: str, fields: List[str], 
                     index_type: str = "btree") -> bool:
        """Create index"""
        try:
            index_spec = [(field, 1) for field in fields]
            self.connection.collection(collection).create_index(index_spec)
            return True
        except Exception:
            return False
    
    def get_stats(self) -> Dict[str, Any]:
        """Get database statistics"""
        try:
            return self.connection.command("serverStatus")
        except Exception:
            return {}
    
    def reset(self) -> bool:
        """Reset database"""
        try:
            for collection in self.connection.list_collection_names():
                self.connection.drop_collection(collection)
            return True
        except Exception:
            return False
    
    @property
    def adapter_name(self) -> str:
        return "NativeAdapter"  # Use generic or protocol-based name
    
    @property
    def supported_features(self) -> List[str]:
        return ["transactions", "indexes", "aggregation"]
```

### 4.2 Registration

```python
# In your adapter module
from chimera.adapters import register_adapter

# Register your adapter
register_adapter("native", MySystemAdapter)
register_adapter("system_a", MySystemAdapter)
```

### 4.3 Usage in Benchmarks

```yaml
# benchmark_config.yaml
systems:
  - system_id: "system_multimodel_a"
    adapter:
      type: "NativeAdapter"
      connection:
        host: "localhost"
        port: 8080
        database: "benchmark_db"
```

---

## 5. Data Formats

### 5.1 Document Format

All documents/records use Python dictionaries:

```python
document = {
    "id": "user_123",
    "name": "John Doe",
    "age": 30,
    "tags": ["developer", "python"],
    "metadata": {
        "created_at": "2026-01-20T00:00:00Z",
        "updated_at": "2026-01-20T00:00:00Z"
    }
}
```

### 5.2 Query Format

Generic query format (adapter translates to native):

```python
query = {
    "field_name": {"$eq": "value"},
    "age": {"$gte": 18, "$lte": 65},
    "tags": {"$in": ["developer", "engineer"]}
}
```

### 5.3 Update Format

Update operations:

```python
update = {
    "$set": {"status": "active"},
    "$inc": {"views": 1},
    "$push": {"tags": "verified"}
}
```

---

## 6. Error Handling

### 6.1 Error Response

Errors should be captured and returned, not raised:

```python
try:
    # Operation
    result = perform_operation()
except Exception as e:
    return QueryResult(
        rows=[],
        execution_time_ms=0,
        error=str(e)
    )
```

### 6.2 Connection Errors

Handle connection failures gracefully:

```python
def connect(self, config: ConnectionConfig) -> bool:
    try:
        # Connection logic
        return True
    except ConnectionError as e:
        print(f"Connection failed: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False
```

---

## 7. Testing

### 7.1 Test Template

```python
import pytest
from your_adapter import MySystemAdapter
from chimera.adapters import ConnectionConfig


@pytest.fixture
def adapter():
    adapter = MySystemAdapter()
    config = ConnectionConfig(
        host="localhost",
        port=8080,
        database="test_db"
    )
    assert adapter.connect(config), "Connection failed"
    yield adapter
    adapter.disconnect()


def test_insert(adapter):
    """Test single insert"""
    doc = {"name": "Test", "value": 123}
    assert adapter.insert("test_collection", doc)


def test_batch_insert(adapter):
    """Test batch insert"""
    docs = [{"id": i, "value": i * 10} for i in range(100)]
    count = adapter.batch_insert("test_collection", docs)
    assert count == 100


def test_query(adapter):
    """Test query execution"""
    result = adapter.execute_query("SELECT * FROM test_collection")
    assert result.error is None
    assert len(result.rows) >= 0


def test_update(adapter):
    """Test update operation"""
    count = adapter.update(
        "test_collection",
        {"name": "Test"},
        {"$set": {"value": 999}}
    )
    assert count >= 0


def test_delete(adapter):
    """Test delete operation"""
    count = adapter.delete("test_collection", {"value": 999})
    assert count >= 0
```

### 7.2 Running Tests

```bash
pytest test_my_adapter.py -v
```

---

## 8. Examples

### 8.1 PostgreSQL Adapter

```python
class PostgreSQLAdapter(DatabaseAdapter):
    """Adapter for PostgreSQL wire protocol"""
    
    def __init__(self):
        self.connection = None
    
    def connect(self, config: ConnectionConfig) -> bool:
        import psycopg2
        try:
            self.connection = psycopg2.connect(
                host=config.host,
                port=config.port,
                database=config.database,
                user=config.username,
                password=config.password
            )
            return True
        except Exception:
            return False
    
    @property
    def adapter_name(self) -> str:
        return "PostgreSQLAdapter"
    
    @property
    def supported_features(self) -> List[str]:
        return ["transactions", "indexes", "full_text_search", "json"]
```

### 8.2 MongoDB Adapter

```python
class MongoDBAdapter(DatabaseAdapter):
    """Adapter for MongoDB wire protocol"""
    
    def __init__(self):
        self.client = None
        self.db = None
    
    def connect(self, config: ConnectionConfig) -> bool:
        from pymongo import MongoClient
        try:
            self.client = MongoClient(
                host=config.host,
                port=config.port,
                username=config.username,
                password=config.password
            )
            self.db = self.client[config.database]
            return True
        except Exception:
            return False
    
    @property
    def adapter_name(self) -> str:
        return "MongoDBAdapter"
    
    @property
    def supported_features(self) -> List[str]:
        return ["transactions", "indexes", "aggregation", "geospatial"]
```

---

## 9. Best Practices

### 9.1 Performance

- Use connection pooling
- Batch operations when possible
- Minimize network round-trips
- Close resources properly

### 9.2 Neutrality

- Use protocol or standard names (not vendor names)
- No vendor-specific optimizations
- Document all assumptions
- Support standard operations

### 9.3 Testing

- Test all required methods
- Include error cases
- Test with realistic data
- Validate performance

### 9.4 Documentation

- Document any limitations
- Provide usage examples
- List supported features
- Note version compatibility

---

## Appendix A: Feature Flags

Standard feature flags:

- `transactions` - ACID transaction support
- `indexes` - Secondary index support
- `full_text_search` - Full-text search
- `vector_search` - Vector similarity search
- `graph_queries` - Graph traversal
- `geospatial` - Geospatial queries
- `aggregation` - Aggregation pipelines
- `json` - Native JSON support
- `time_series` - Time-series optimization

---

## Appendix B: Query Translation

When translating generic queries to native syntax:

**Generic:**
```python
query = {"age": {"$gte": 18}}
```

**SQL:**
```sql
WHERE age >= 18
```

**MongoDB:**
```javascript
{age: {$gte: 18}}
```

---

**Version:** 1.0.0  
**Status:** ✅ STABLE  
**License:** MIT  
**Maintainer:** CHIMERA Development Team
