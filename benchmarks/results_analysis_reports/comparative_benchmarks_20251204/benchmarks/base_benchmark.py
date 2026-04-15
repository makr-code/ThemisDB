"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            base_benchmark.py                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     365                                            ║
    • Open Issues:     TODOs: 0, Stubs: 7                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Comparative Benchmark - Base Benchmark Class

This module provides the abstract base class for all benchmark implementations.
Each database adapter must implement these interfaces for fair comparison.
"""

import abc
import time
import statistics
import json
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Callable
from enum import Enum
import structlog

logger = structlog.get_logger()


class BenchmarkCategory(Enum):
    """Categories of benchmarks for classification."""
    CRUD = "crud"
    QUERY = "query"
    VECTOR = "vector"
    GRAPH = "graph"
    FULLTEXT = "fulltext"
    MIXED = "mixed"


@dataclass
class BenchmarkResult:
    """Results from a single benchmark run."""
    name: str
    category: BenchmarkCategory
    database: str
    iterations: int
    total_time_ms: float
    times_ms: List[float] = field(default_factory=list)
    
    # Computed metrics
    @property
    def mean_ms(self) -> float:
        return statistics.mean(self.times_ms) if self.times_ms else 0.0
    
    @property
    def median_ms(self) -> float:
        return statistics.median(self.times_ms) if self.times_ms else 0.0
    
    @property
    def std_dev_ms(self) -> float:
        return statistics.stdev(self.times_ms) if len(self.times_ms) > 1 else 0.0
    
    @property
    def p50_ms(self) -> float:
        return self._percentile(50)
    
    @property
    def p95_ms(self) -> float:
        return self._percentile(95)
    
    @property
    def p99_ms(self) -> float:
        return self._percentile(99)
    
    @property
    def min_ms(self) -> float:
        return min(self.times_ms) if self.times_ms else 0.0
    
    @property
    def max_ms(self) -> float:
        return max(self.times_ms) if self.times_ms else 0.0
    
    @property
    def ops_per_second(self) -> float:
        if self.mean_ms > 0:
            return 1000.0 / self.mean_ms
        return 0.0
    
    def _percentile(self, p: int) -> float:
        if not self.times_ms:
            return 0.0
        sorted_times = sorted(self.times_ms)
        idx = int(len(sorted_times) * p / 100)
        return sorted_times[min(idx, len(sorted_times) - 1)]
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert result to dictionary for JSON serialization."""
        return {
            "name": self.name,
            "category": self.category.value,
            "database": self.database,
            "iterations": self.iterations,
            "total_time_ms": self.total_time_ms,
            "mean_ms": self.mean_ms,
            "median_ms": self.median_ms,
            "std_dev_ms": self.std_dev_ms,
            "p50_ms": self.p50_ms,
            "p95_ms": self.p95_ms,
            "p99_ms": self.p99_ms,
            "min_ms": self.min_ms,
            "max_ms": self.max_ms,
            "ops_per_second": self.ops_per_second,
        }


@dataclass
class BenchmarkConfig:
    """Configuration for benchmark execution."""
    warmup_iterations: int = 100
    iterations: int = 1000
    dataset_size: int = 10000
    batch_size: int = 100
    concurrent_threads: int = 4
    vector_dimensions: int = 384
    graph_depth: int = 3
    k_nearest: int = 10


class BaseDatabaseAdapter(abc.ABC):
    """Abstract base class for database adapters."""
    
    def __init__(self, config: Dict[str, Any]):
        """Initialize adapter with database connection configuration."""
        self.config = config
        self._connected = False
    
    @property
    @abc.abstractmethod
    def name(self) -> str:
        """Return the name of the database."""
        pass
    
    @abc.abstractmethod
    def connect(self) -> None:
        """Establish connection to the database."""
        pass
    
    @abc.abstractmethod
    def disconnect(self) -> None:
        """Close connection to the database."""
        pass
    
    @abc.abstractmethod
    def clear_data(self) -> None:
        """Clear all benchmark data from the database."""
        pass
    
    @property
    def is_connected(self) -> bool:
        return self._connected
    
    # CRUD Operations
    @abc.abstractmethod
    def insert_one(self, collection: str, document: Dict[str, Any]) -> str:
        """Insert a single document. Returns the document ID."""
        pass
    
    @abc.abstractmethod
    def insert_many(self, collection: str, documents: List[Dict[str, Any]]) -> List[str]:
        """Insert multiple documents. Returns list of document IDs."""
        pass
    
    @abc.abstractmethod
    def find_by_id(self, collection: str, doc_id: str) -> Optional[Dict[str, Any]]:
        """Find a document by its ID."""
        pass
    
    @abc.abstractmethod
    def update_one(self, collection: str, doc_id: str, updates: Dict[str, Any]) -> bool:
        """Update a single document. Returns success status."""
        pass
    
    @abc.abstractmethod
    def delete_one(self, collection: str, doc_id: str) -> bool:
        """Delete a single document. Returns success status."""
        pass
    
    # Query Operations
    @abc.abstractmethod
    def find_by_field(self, collection: str, field: str, value: Any) -> List[Dict[str, Any]]:
        """Find documents by field value (equality)."""
        pass
    
    @abc.abstractmethod
    def find_by_range(self, collection: str, field: str, 
                      min_val: Any, max_val: Any) -> List[Dict[str, Any]]:
        """Find documents by field range."""
        pass
    
    @abc.abstractmethod
    def count_by_field(self, collection: str, field: str, value: Any) -> int:
        """Count documents matching field value."""
        pass
    
    @abc.abstractmethod
    def aggregate_sum(self, collection: str, field: str, 
                      group_by: Optional[str] = None) -> Any:
        """Aggregate sum of field values, optionally grouped."""
        pass
    
    # Vector Operations (optional - not all DBs support)
    def supports_vector_search(self) -> bool:
        """Return True if database supports vector search."""
        return False
    
    def insert_vector(self, collection: str, doc_id: str, 
                      vector: List[float], metadata: Dict[str, Any]) -> str:
        """Insert a vector with metadata."""
        raise NotImplementedError("Vector search not supported")
    
    def search_vectors(self, collection: str, query_vector: List[float], 
                       k: int = 10, filter_criteria: Optional[Dict[str, Any]] = None) -> List[Dict[str, Any]]:
        """Search for k nearest vectors, optionally with filters."""
        raise NotImplementedError("Vector search not supported")
    
    # Graph Operations (optional - not all DBs support)
    def supports_graph_operations(self) -> bool:
        """Return True if database supports graph operations."""
        return False
    
    def insert_node(self, node_id: str, labels: List[str], 
                    properties: Dict[str, Any]) -> str:
        """Insert a graph node."""
        raise NotImplementedError("Graph operations not supported")
    
    def insert_edge(self, from_id: str, to_id: str, 
                    edge_type: str, properties: Dict[str, Any]) -> str:
        """Insert a graph edge."""
        raise NotImplementedError("Graph operations not supported")
    
    def traverse_bfs(self, start_node: str, max_depth: int) -> List[str]:
        """Perform BFS traversal from start node."""
        raise NotImplementedError("Graph operations not supported")
    
    def shortest_path(self, from_id: str, to_id: str) -> List[str]:
        """Find shortest path between two nodes."""
        raise NotImplementedError("Graph operations not supported")
    
    # Full-text Search (optional)
    def supports_fulltext_search(self) -> bool:
        """Return True if database supports full-text search."""
        return False
    
    def fulltext_search(self, collection: str, query: str, 
                        limit: int = 10) -> List[Dict[str, Any]]:
        """Perform full-text search."""
        raise NotImplementedError("Full-text search not supported")


class BenchmarkRunner:
    """Runs benchmarks and collects results."""
    
    def __init__(self, config: BenchmarkConfig):
        self.config = config
        self.results: List[BenchmarkResult] = []
        self.logger = structlog.get_logger()
    
    def run_benchmark(self, 
                      name: str,
                      category: BenchmarkCategory,
                      database: str,
                      func: Callable[[], Any],
                      iterations: Optional[int] = None,
                      warmup: Optional[int] = None) -> BenchmarkResult:
        """
        Run a benchmark function and collect timing results.
        
        Args:
            name: Name of the benchmark
            category: Category of the benchmark
            database: Name of the database being tested
            func: Function to benchmark (should take no arguments)
            iterations: Number of iterations (uses config default if None)
            warmup: Number of warmup iterations (uses config default if None)
        
        Returns:
            BenchmarkResult with collected metrics
        """
        iterations = iterations or self.config.iterations
        warmup = warmup or self.config.warmup_iterations
        
        self.logger.info(f"Running benchmark", 
                        name=name, 
                        database=database,
                        warmup=warmup,
                        iterations=iterations)
        
        # Warmup phase
        for _ in range(warmup):
            func()
        
        # Measurement phase
        times_ms = []
        total_start = time.perf_counter()
        
        for _ in range(iterations):
            start = time.perf_counter()
            func()
            end = time.perf_counter()
            times_ms.append((end - start) * 1000)  # Convert to milliseconds
        
        total_end = time.perf_counter()
        total_time_ms = (total_end - total_start) * 1000
        
        result = BenchmarkResult(
            name=name,
            category=category,
            database=database,
            iterations=iterations,
            total_time_ms=total_time_ms,
            times_ms=times_ms
        )
        
        self.results.append(result)
        
        self.logger.info(f"Benchmark complete",
                        name=name,
                        mean_ms=f"{result.mean_ms:.3f}",
                        p95_ms=f"{result.p95_ms:.3f}",
                        ops_per_sec=f"{result.ops_per_second:.1f}")
        
        return result
    
    def save_results(self, filepath: str) -> None:
        """Save all results to a JSON file."""
        results_dict = {
            "config": {
                "warmup_iterations": self.config.warmup_iterations,
                "iterations": self.config.iterations,
                "dataset_size": self.config.dataset_size,
                "batch_size": self.config.batch_size,
            },
            "results": [r.to_dict() for r in self.results]
        }
        
        with open(filepath, 'w') as f:
            json.dump(results_dict, f, indent=2)
        
        self.logger.info(f"Results saved", filepath=filepath, count=len(self.results))
    
    def clear_results(self) -> None:
        """Clear all collected results."""
        self.results = []
