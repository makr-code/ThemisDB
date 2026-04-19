# Graph Advanced Features

> **Status:** 2026-04-19 – Architekturtext gegen realen Sourcecode verifizieren; Abweichungen mit `<!-- TODO -->` markiert.

This directory contains advanced graph analytics and query features for ThemisDB. These modules extend the basic graph query capabilities with sophisticated algorithms for path analysis.

## Implementation Status ✅

**PathConstraints Module:** Fully Implemented  
**GraphQueryOptimizer Integration:** Fully Implemented  
**Implementation Date:** February 2026

## Important Note on Existing Implementations

**Centrality and Community Detection algorithms are already fully implemented** in the `GraphAnalytics` class:
- **Location:** `include/index/graph_analytics.h` and `src/index/graph_analytics.cpp`
<!-- TODO: verify feature exists in include/graph/ -->
- **Implemented Algorithms:**
  - ✅ Degree Centrality
  - ✅ PageRank
  - ✅ Betweenness Centrality (Brandes' algorithm)
  - ✅ Closeness Centrality
  - ✅ Louvain Community Detection
  - ✅ Label Propagation Community Detection
  - ✅ K-Shortest Paths (Yen's algorithm)

**Use the existing `GraphAnalytics` class for these features instead of waiting for future implementations.**

## New Module: Path Constraints ✅

### Path Constraints (`path_constraints.h/cpp`)

Advanced path finding with complex constraint specifications. **Now fully implemented with BFS traversal algorithm.**

**Features:**
- ✅ Length constraints (min/max)
- ✅ Node requirements and prohibitions
- ✅ Edge requirements and prohibitions
- ✅ Uniqueness constraints (nodes/edges)
- ✅ Acyclic path requirements
- ✅ Custom validation predicates
- ✅ GraphIndexManager integration
- ✅ GraphQueryOptimizer integration

**Example Usage:**
```cpp
#include "graph/path_constraints.h"
#include "index/graph_index.h"
<!-- TODO: verify feature exists in include/graph/ -->

using namespace themis::graph;

// Create graph manager
GraphIndexManager graph_mgr(storage);

// Create constraints with graph manager
PathConstraints constraints(&graph_mgr);
constraints.addMinLength(3);
constraints.addMaxLength(10);
constraints.addForbiddenNode("blocked_node");
constraints.requireAcyclic();

// Find paths that satisfy constraints
auto result = constraints.findConstrainedPaths("start", "end", 5);
if (result.has_value()) {
    for (const auto& path : *result) {
        std::cout << "Path cost: " << path.cost << std::endl;
        std::cout << "Nodes: ";
        for (const auto& node : path.nodes) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
}
```

**Integration with Query Optimizer:**
```cpp
#include "graph/graph_query_optimizer.h"
#include "graph/path_constraints.h"

GraphQueryOptimizer optimizer(graph_mgr);
PathConstraints constraints(&graph_mgr);
constraints.addMinLength(2);
constraints.addMaxLength(5);
constraints.requireUniqueNodes();

// Get optimization plan
auto plan = optimizer.optimizeConstrainedPath("A", "D", constraints);
if (plan.has_value()) {
    std::cout << "Algorithm: " << (plan->algorithm == TraversalAlgorithm::BFS ? "BFS" : "DFS") << std::endl;
    std::cout << "Estimated cost: " << plan->estimated_cost << std::endl;
    std::cout << "Estimated time: " << plan->estimated_time_ms << "ms" << std::endl;
}
```

### Supported Constraint Types

PathConstraints supports 14 constraint types for flexible path finding:

1. **MIN_LENGTH** - Minimum number of nodes in path
   ```cpp
   constraints.addMinLength(3); // Path must have at least 3 nodes
   ```

2. **MAX_LENGTH** - Maximum number of nodes in path
   ```cpp
   constraints.addMaxLength(10); // Path cannot exceed 10 nodes
   ```

3. **FORBIDDEN_NODE** - Nodes that cannot appear in the path
   ```cpp
   constraints.addForbiddenNode("blocked_user");
   ```

4. **REQUIRED_NODE** - Nodes that must appear in the path
   ```cpp
   constraints.addRequiredNode("checkpoint");
   ```

5. **FORBIDDEN_EDGE** - Edges that cannot be used in the path
   ```cpp
   constraints.addForbiddenEdge("edge_id_123");
   ```

6. **REQUIRED_EDGE** - Edges that must be used in the path
   ```cpp
   constraints.addRequiredEdge("edge_id_456");
   ```

7. **NO_CYCLES** - Path must be acyclic (no node appears twice)
   ```cpp
   constraints.requireAcyclic();
   ```

8. **UNIQUE_NODES** - All nodes in path must be unique
   ```cpp
   constraints.requireUniqueNodes();
   ```

9. **UNIQUE_EDGES** - All edges in path must be unique
   ```cpp
   constraints.requireUniqueEdges();
   ```

10. **CUSTOM_PREDICATE** - Custom validation function
    ```cpp
    constraints.addCustomPredicate([](const std::vector<std::string>& nodes) {
        return nodes.size() % 2 == 0; // Only even-length paths
    });
    ```

11. **NODE_PROPERTY** - Node must have specific properties
    ```cpp
    // ✅ Implemented (v1.7.0)
    // addNodePropertyConstraint(field_name, expected_value) prunes next-nodes
    // during BFS traversal and validates on complete paths.
    constraints.addNodePropertyConstraint("country", "USA");
    ```

12. **EDGE_PROPERTY** - Edge must have specific properties
    ```cpp
    // ✅ Implemented (v1.7.0)
    // addEdgePropertyConstraint(field_name, expected_value) prunes edges
    // during BFS and validates on complete paths.
    constraints.addEdgePropertyConstraint("type", "follows");
    ```

13. **MAX_WEIGHT** - Total path weight must not exceed threshold
    ```cpp
    // ✅ Implemented (v1.7.0)
    // BFS prunes states whose accumulated edge-weight cost exceeds the threshold.
    constraints.addMaxWeight(100.0);
    ```

14. **MIN_WEIGHT** - Total path weight must meet minimum threshold
    ```cpp
    // ✅ Implemented (v1.7.0)
    // Completed paths whose total weight is below the threshold are rejected.
    constraints.addMinWeight(10.0);
    ```

### Algorithm Details

**Traversal Strategy:**
- Uses BFS (Breadth-First Search) by default for optimal short-path discovery
- DFS can be selected by optimizer for deep exploration scenarios
- Early termination when MAX_LENGTH is reached
- Constraint validation during traversal for efficiency

**Performance Characteristics:**
- Time Complexity: O(V + E) × f(constraints)
- Space Complexity: O(V) for visited tracking + O(k × L) for storing k paths of length L
- Optimized for graphs with 100K+ nodes
- Constraint checking overhead: ~5-15% per constraint

**Use Cases:**
- Supply chain routing with restrictions
- Social network path analysis with privacy constraints
- Network security path enumeration
- Knowledge graph reasoning with constraints

## Using Existing Graph Analytics

For centrality and community detection, use the fully-implemented `GraphAnalytics` class:
<!-- TODO: verify feature exists in include/graph/ -->

**Example: Centrality Analysis**
```cpp
#include "index/graph_analytics.h"

GraphAnalytics analytics(graph_manager);

// Get all node IDs
std::vector<std::string> node_ids = {"user1", "user2", "user3"};

// PageRank
auto [status, ranks] = analytics.pageRank(node_ids, 0.85, 100, 1e-6);
if (status.ok) {
    for (const auto& [node, score] : ranks) {
        std::cout << node << ": " << score << std::endl;
    }
}

// Betweenness Centrality
auto [st2, betweenness] = analytics.betweennessCentrality(node_ids);

// Degree Centrality
auto [st3, degrees] = analytics.degreeCentrality(node_ids);
```

**Example: Community Detection**
```cpp
#include "index/graph_analytics.h"

GraphAnalytics analytics(graph_manager);
std::vector<std::string> node_ids = {"user1", "user2", "user3"};

// Louvain Method
auto [status, communities] = analytics.louvainCommunities(node_ids);
if (status.ok) {
    for (const auto& [node, community_id] : communities) {
        std::cout << node << " -> Community " << community_id << std::endl;
    }
}

// Label Propagation
auto [st2, communities2] = analytics.labelPropagationCommunities(node_ids, 100);
```

**Example: K-Shortest Paths**
```cpp
#include "index/graph_analytics.h"

GraphAnalytics analytics(graph_manager);

// Find 5 shortest paths from A to B
auto [status, paths] = analytics.kShortestPaths("A", "B", 5);
if (status.ok) {
    for (size_t i = 0; i < paths.size(); ++i) {
        std::cout << "Path " << i+1 << ": ";
        for (const auto& vertex : paths[i].vertices) {
            std::cout << vertex << " ";
        }
        std::cout << "(length: " << paths[i].length << ")" << std::endl;
    }
}
```

## Integration with Existing Systems

### Graph Query Optimizer Integration

Path constraints can enhance query optimization:

```cpp
#include "graph/graph_query_optimizer.h"
#include "graph/path_constraints.h"

// Path constraints can enhance query optimization
PathConstraints constraints;
constraints.addMaxLength(5);
constraints.requireUniqueNodes();

GraphQueryOptimizer::QueryConstraints query_constraints;
query_constraints.max_depth = 5;
query_constraints.unique_vertices = true;

auto plan = optimizer.optimizeShortestPath("A", "B", query_constraints);
```

### Graph Index Manager Integration

```cpp
#include "index/graph_index.h"
#include "index/graph_analytics.h"

GraphIndexManager graph_manager(db);

// Use GraphAnalytics for centrality and community detection
GraphAnalytics analytics(graph_manager);
```

## Implementation Notes

### Path Constraints Implementation
- ✅ Constraint validation (all 14 types: MIN_LENGTH, MAX_LENGTH, FORBIDDEN_NODE, REQUIRED_NODE, FORBIDDEN_EDGE, REQUIRED_EDGE, NO_CYCLES, UNIQUE_NODES, UNIQUE_EDGES, CUSTOM_PREDICATE, NODE_PROPERTY, EDGE_PROPERTY, MAX_WEIGHT, MIN_WEIGHT)
- ✅ EDGE_PROPERTY: `addEdgePropertyConstraint(key, value)` – early pruning in BFS + `validatePath`
- ✅ NODE_PROPERTY: `addNodePropertyConstraint(key, value)` – early pruning in BFS + `validatePath`
- ✅ MAX_WEIGHT: BFS prunes states whose accumulated cost exceeds threshold
- ✅ MIN_WEIGHT: completed paths below the threshold are rejected at acceptance
- ✅ Constrained BFS traversal with early termination
- ✅ Integration with GraphQueryOptimizer
- ✅ Integration tests and validation

## Algorithm Complexity

### Path Constraints
| Operation | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| Validation | O(n) | O(n) |
| Constrained BFS | O(V + E) × f(constraints) | O(V) |
| Constrained DFS | O(V + E) × f(constraints) | O(V) |

### Existing GraphAnalytics (Already Implemented)
| Algorithm | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| Degree | O(V + E) | O(V) |
| Betweenness | O(V × E) | O(V + E) |
| Closeness | O(V²) or O(V × E) | O(V) |
| PageRank | O(iterations × E) | O(V) |
| Louvain | O(E × log V) | O(V + E) |
| Label Propagation | O(E) | O(V) |
| K-Shortest Paths | O(K × E × log V) | O(V + E) |

## Performance Considerations

### Path Constraints (Current Implementation)
1. **Index Utilization**: Uses GraphIndexManager for optimized neighbor access
2. **Caching**: Leverages adjacency cache when available
3. **Early Termination**: Stops exploration when constraints can't be satisfied
4. **Visited Tracking**: Efficient unordered_set for cycle detection and uniqueness
5. **Constraint Filtering**: Validates constraints during traversal, not just at completion
6. **Parallel BFS**: Level-parallel frontier expansion via `enable_parallel`/`num_threads` (v1.7.0)

### GraphAnalytics (Current Implementation)
- Optimized batch lookups (10-100× faster for large graphs)
- Pre-allocated maps to avoid rehashing
- Efficient adjacency structure building
- See `src/index/graph_analytics.cpp` for implementation details

## Error Handling

All methods return `Result<T>` for consistent error handling:

```cpp
// Example: PathConstraints
// First, create and configure constraints
PathConstraints constraints(&graph_mgr);
constraints.addMinLength(2);
constraints.addMaxLength(10);
constraints.requireUniqueNodes();

auto result = constraints.findConstrainedPaths("start", "end", 10);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return;
}

const auto& paths = result.value();
for (const auto& path : paths) {
    // Process path...
}

// Example: GraphAnalytics
auto analytics_result = analytics.pageRank(nodes, 0.85, 100, 1e-6);
if (!analytics_result.first.ok) {
    std::cerr << "Error: " << analytics_result.first.message << std::endl;
    return;
}
```

Common error codes for PathConstraints:
- `ErrorRegistry::ErrorCode::INVALID_STATE`: GraphIndexManager not set
<!-- TODO: verify feature exists in include/graph/ -->
- `ErrorRegistry::ErrorCode::VALIDATION_FAILED`: Contradictory or unsatisfiable constraints
- `ErrorRegistry::ErrorCode::NOT_FOUND`: No paths found satisfying constraints

Graph-specific codes (v1.7.0+):
- `errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX` (6400): Vertex not found during traversal
<!-- TODO: verify feature exists in include/graph/ -->
- `errors::ErrorCode::ERR_GRAPH_CONSTRAINT_CONFLICT` (6402): Contradictory constraints
- `errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND` (6403): No satisfying path
- `errors::ErrorCode::ERR_QUERY_TIMEOUT` (6103): SLO budget exceeded

## Observability

### Query Metrics (`GraphQueryMetrics`)

`GraphQueryOptimizer::getQueryMetrics()` returns a cumulative snapshot of all
queries executed since the optimizer was constructed:

| Metric | Type | Description |
|--------|------|-------------|
| `total_queries` | `std::atomic<uint64_t>` | Total traversal executions |
| `failed_queries` | `std::atomic<uint64_t>` | Executions that returned no result |
| `timed_out_queries` | `std::atomic<uint64_t>` | Aborted by `timeout_ms` SLO |
| `total_execution_time_ms` | `std::atomic<uint64_t>` | Sum of execution durations (ms) |
| `max_execution_time_ms` | `std::atomic<uint64_t>` | Peak single-query duration (ms) |
| `total_nodes_explored` | `std::atomic<uint64_t>` | Cumulative nodes visited |
| `total_edges_traversed` | `std::atomic<uint64_t>` | Cumulative edges traversed |
| `plan_cache_hits` | `std::atomic<uint64_t>` | Plan-cache hits |
| `plan_cache_misses` | `std::atomic<uint64_t>` | Plan-cache misses |
| `latency_histogram` | `LatencyHistogram` | 10-bucket fixed-width histogram (ms: 1,5,10,25,50,100,250,500,1000,+Inf) |

Computed helpers: `avgExecutionTimeMs()`, `errorRate()`.

**Latency percentiles:**
```cpp
const auto& hist = optimizer.getQueryMetrics().latency_histogram;
double p50 = hist.percentileMs(0.50); // approximate median
double p99 = hist.percentileMs(0.99); // approximate p99
```

### Prometheus Scrape Endpoint (`GET /api/v1/graph/metrics/prometheus`)

Returns metrics in **Prometheus text exposition format** (`text/plain; version=0.0.4`) for
direct Prometheus scraping without a custom exporter:

```
themis_graph_queries_total 42
themis_graph_query_errors_total 1
themis_graph_latency_ms_bucket{le="1"} 5
...
themis_graph_latency_ms_bucket{le="+Inf"} 42
themis_graph_latency_p99_ms 87.500000
```

### Query Rate Limiter

`setMaxQueriesPerSecond(uint32_t)` limits the maximum number of graph queries
per second across all five execute methods. Excess queries return
`ERR_GRAPH_RATE_LIMIT_EXCEEDED` (6406) immediately:

```cpp
optimizer.setMaxQueriesPerSecond(200); // 200 QPS limit
optimizer.setMaxQueriesPerSecond(0);   // disable (default)
```

### Admin API Endpoint (`GET /api/v1/graph/metrics`)

The HTTP server exposes the `GraphQueryMetrics` snapshot as JSON for operational
dashboards and alerting rules:

```http
GET /api/v1/graph/metrics HTTP/1.1
```

```json
{
  "total_queries": 42,
  "failed_queries": 1,
  "timed_out_queries": 0,
  "total_execution_time_ms": 350,
  "max_execution_time_ms": 12,
  "avg_execution_time_ms": 8.35,
  "total_nodes_explored": 1234,
  "total_edges_traversed": 5678,
  "plan_cache_hits": 30,
  "plan_cache_misses": 12,
  "error_rate": 0.0238
}
```

Prometheus/OTel metric name mappings:

| JSON key | Prometheus name |
|----------|----------------|
| `total_queries` | `themis_graph_queries_total` |
| `failed_queries` | `themis_graph_query_errors_total` |
| `timed_out_queries` | `themis_graph_query_timeouts_total` |
| `total_execution_time_ms` | `themis_graph_query_duration_ms_sum` |
| `max_execution_time_ms` | `themis_graph_query_duration_ms_max` |
| `total_nodes_explored` | `themis_graph_nodes_explored_total` |
| `total_edges_traversed` | `themis_graph_edges_traversed_total` |
| `plan_cache_hits` | `themis_graph_plan_cache_hits_total` |
| `plan_cache_misses` | `themis_graph_plan_cache_misses_total` |

## Testing

### Current Tests
Comprehensive test coverage includes:
- ✅ Constructor/destructor behavior
- ✅ Constraint addition and validation
- ✅ Path finding with various constraint combinations
- ✅ Error handling for invalid inputs
- ✅ Integration with GraphIndexManager
- ✅ Integration with GraphQueryOptimizer via `optimizeConstrainedPath()`
- ✅ Correctness tests with known graphs
- ✅ Edge case handling (empty paths, contradictory constraints, etc.)
- ✅ Query timeout / SLO enforcement (BFS/DFS/Dijkstra)
- ✅ Aggregate metrics and plan cache hit/miss counters
- ✅ Graph error codes 6400–6406
- ✅ `explainConstrainedPath()` dry-run (no query counter increment)
- ✅ Parallel BFS / Parallel Dijkstra (Δ-Stepping) correctness
- ✅ Adaptive cost model: EMA update, export/import roundtrip
- ✅ EDGE_PROPERTY / NODE_PROPERTY / weight constraint API and pruning
- ✅ Latency histogram percentiles
- ✅ Rate limiter (set/get, high-limit allows, exceeded returns 6406)
- ✅ `GET /api/v1/graph/metrics` JSON endpoint
- ✅ `GET /api/v1/graph/metrics/prometheus` Prometheus endpoint

### Test Location
- Optimizer + metrics + constraint tests: `tests/test_graph_query_optimizer.cpp`
- Integration tests: `test_path_constraints_optimizer_integration.cpp`
- See `docs/ARCHIVED/implementation-summaries/INTEGRATION_TEST_REPORT.md` for details

### Future Test Enhancements
- Performance benchmarks on large graphs (>1M nodes)
- Scalability tests with increasing constraint complexity
- Comparison with alternative path-finding libraries

## References

### Academic Papers
1. **Betweenness Centrality**: Brandes, U. (2001). "A faster algorithm for betweenness centrality"
2. **PageRank**: Page, L., et al. (1999). "The PageRank Citation Ranking"
3. **Louvain**: Blondel, V. D., et al. (2008). "Fast unfolding of communities in large networks"
4. **Leiden**: Traag, V. A., et al. (2019). "From Louvain to Leiden"
5. **Girvan-Newman**: Girvan, M., & Newman, M. E. (2002). "Community structure in social and biological networks"

### Similar Systems
- **Neo4j Graph Data Science**: Comprehensive graph algorithms library
- **NetworkX**: Python library for graph analytics
- **igraph**: Fast graph library for R and Python
- **GraphX**: Apache Spark's graph processing framework

## Contributing

When implementing these algorithms:

1. **Follow existing patterns**: Use `Result<T>`, integrate with `GraphIndexManager`
2. **Maintain API compatibility**: Keep interface signatures stable
3. **Add comprehensive tests**: Cover correctness, performance, edge cases
4. **Document complexity**: Include time/space complexity analysis
5. **Consider scalability**: Design for large graphs from the start
6. **Benchmark**: Compare with established implementations

## License

Part of ThemisDB - Multi-Model Database System

## Related Documentation

- [Graph Analytics](../../include/index/graph_analytics.h) - **Existing centrality and community detection implementations**
- [Graph Query Optimizer](./README.md) - Existing graph query optimization
- [Graph Index](../../include/index/graph_index.h) - Core graph storage
- [Error Handling](../../include/utils/expected.h) - Result<T> pattern
- [GAP Analysis](../../docs/Audit/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md) - Implementation gaps

## Future Enhancements

### Path Constraints Enhancements
- ✅ Constraint validation algorithms (DONE)
- ✅ Constrained BFS traversal (DONE)
- ✅ Integration with query optimizer (DONE)
- ✅ EDGE_PROPERTY constraint validation (DONE – v1.7.0)
- ✅ Parallel BFS (`enable_parallel`, `num_threads`) (DONE – v1.7.0)
- ✅ NODE_PROPERTY constraint validation (DONE – v1.7.0, backed by `getNodeField`)
- ✅ Weight constraints (MAX_WEIGHT / MIN_WEIGHT) (DONE – v1.7.0)
- ⏳ Performance optimization for massive graphs (>10M nodes)
- ⏳ DFS alternative for deep path scenarios

### Additional Graph Features
- **Temporal Path Analysis**: Time-aware path constraints
- ✅ **Weighted Constraints**: `addMaxWeight()` / `addMinWeight()` (DONE – v1.7.0)
- **Approximate Algorithms**: Fast approximations for massive graphs
- **Streaming Constraints**: Real-time constraint evaluation
- **A* with Constraints**: Heuristic-guided constrained path finding

### Integration Features
- **AQL Integration**: Native query language support for constraints
- **Visualization**: Path visualization with constraint highlighting
- **Export**: JSON/CSV export of constrained path results
- **Monitoring**: Real-time metrics for constraint evaluation
- **Query Plan Caching**: Cache optimized plans for repeated constraint patterns
