# Graph Query Engine Optimization

This directory contains the implementation of the Graph Query Engine Optimization system for ThemisDB. The system provides cost-based optimization for complex graph traversals, supporting multiple algorithms and optimization strategies.

## Overview

The Graph Query Optimizer is designed to efficiently execute graph queries by:
- Selecting the optimal traversal algorithm based on query patterns
- Estimating costs using graph statistics
- Applying performance optimizations (early termination, caching, parallel execution)
- Tracking execution metrics for adaptive optimization

## Components

### GraphQueryOptimizer Class

Main optimizer class that provides:
- **Query Planning**: Generates and selects optimal execution plans
- **Algorithm Selection**: Chooses between BFS, DFS, Dijkstra, A*, or Bidirectional search
- **Cost Estimation**: Estimates query cost based on graph statistics
- **Execution**: Executes optimized traversals with performance tracking

### Supported Algorithms

1. **BFS (Breadth-First Search)**
   - Time: O(V + E) or O(b^d) where b is branching factor, d is depth
   - Best for: Shortest unweighted path, level-order exploration
   - Use case: K-hop queries, shortest path in unweighted graphs

2. **DFS (Depth-First Search)**
   - Time: O(V + E)
   - Best for: Deep exploration, pattern matching
   - Use case: Subgraph isomorphism, path enumeration

3. **Dijkstra's Algorithm**
   - Time: O((V + E) log V)
   - Best for: Weighted shortest path
   - Use case: Shortest path with edge weights

4. **A* Search**
   - Time: O((V + E) log V) with good heuristic
   - Best for: Heuristic-guided exploration
   - Use case: Shortest path when spatial/domain heuristics available

5. **Bidirectional Search**
   - Time: O(2 * b^(d/2)) = O(b^(d/2))
   - Best for: Long-distance paths
   - Use case: Paths in large graphs with high depth

## Query Patterns

The optimizer recognizes and optimizes for these query patterns:

- **SHORTEST_PATH**: Finding the shortest path between two vertices
- **K_HOP_NEIGHBORS**: Finding all vertices within k hops
- **PATTERN_MATCH**: Subgraph pattern matching
- **REACHABILITY**: Checking if one vertex is reachable from another
- **ALL_PATHS**: Enumerating all paths between vertices
- **CONNECTED_COMPONENT**: Finding connected components

## Optimization Features

### Cost-Based Selection
- Evaluates multiple algorithms and selects the one with lowest estimated cost
- Considers graph statistics (vertex count, edge count, branching factor)
- Accounts for available indices and caches

### Performance Optimizations
- **Early Termination**: Stops traversal when goal is reached
- **Path Deduplication**: Uses visited sets to avoid revisiting nodes
- **Cycle Detection**: Prevents infinite loops in cyclic graphs
- **Index Utilization**: Leverages edge indices for faster neighbor lookups
- **Cache Utilization**: Uses adjacency cache for repeated queries
- **Parallel Hints**: Suggests parallel execution for large traversals

### Plan Caching
- Caches query plans for repeated queries
- Cache key based on query pattern, vertices, and constraints
- Reduces planning overhead for common queries

### Execution Monitoring
- Tracks nodes explored, edges traversed, execution time
- Records execution history (bounded at 1000 entries)
- Provides statistics for adaptive optimization

## Usage Example

```cpp
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"

// Initialize
themis::GraphIndexManager graph_mgr(db);
themis::graph::GraphQueryOptimizer optimizer(graph_mgr);

// Collect statistics
auto stats_result = optimizer.collectStatistics();

// Optimize shortest path query
auto plan_result = optimizer.optimizeShortestPath("vertex_A", "vertex_B");
if (plan_result) {
    const auto& plan = plan_result.value();
    std::cout << "Selected algorithm: " << plan.algorithm << std::endl;
    std::cout << "Estimated cost: " << plan.estimated_cost << std::endl;
    std::cout << optimizer.explainPlan(plan) << std::endl;
}

// Execute optimized query
themis::graph::GraphQueryOptimizer::ExecutionStats exec_stats;
auto result = optimizer.executeBFS("vertex_A", 3, {}, &exec_stats);
if (result) {
    const auto& vertices = result.value();
    std::cout << "Found " << vertices.size() << " vertices" << std::endl;
    std::cout << "Nodes explored: " << exec_stats.nodes_explored << std::endl;
    std::cout << "Execution time: " << exec_stats.execution_time_ms << " ms" << std::endl;
}

// Execute Dijkstra for weighted path
auto path_result = optimizer.executeDijkstra("vertex_A", "vertex_B");
if (path_result) {
    const auto& path = path_result.value();
    std::cout << "Path length: " << path.path.size() << std::endl;
    std::cout << "Total cost: " << path.totalCost << std::endl;
}
```

## Query Constraints

The optimizer supports various constraints:

```cpp
themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
constraints.max_depth = 5;                    // Limit traversal depth
constraints.max_results = 100;                // Early terminate after N results
constraints.unique_vertices = true;           // No vertex visited twice
constraints.edge_type = "FRIEND";            // Filter by edge type
constraints.graph_id = "social_network";     // Filter by graph ID
constraints.forbidden_vertices = {"blocked"}; // Skip certain vertices
```

## Performance Characteristics

### Memory Usage
- BFS: O(V) for visited set and queue
- DFS: O(V) for visited set and stack (better than BFS for deep graphs)
- Dijkstra/A*: O(V) for priority queue and distance map
- Bidirectional: O(V) for two frontiers

### Time Complexity
| Algorithm      | Best Case       | Average Case    | Worst Case      |
|----------------|----------------|-----------------|-----------------|
| BFS            | O(1)           | O(b^d)          | O(V + E)        |
| DFS            | O(1)           | O(b^d)          | O(V + E)        |
| Dijkstra       | O(E log V)     | O((V+E) log V)  | O((V+E) log V)  |
| A*             | O(E log V)     | O(E log V)      | O((V+E) log V)  |
| Bidirectional  | O(b^(d/2))     | O(b^(d/2))      | O(V + E)        |

### Optimization Impact
- Index usage: ~20% cost reduction
- Adjacency cache: ~30% cost reduction
- Early termination: up to 90% reduction for reachability queries
- Edge type filtering: varies based on selectivity

## Integration

The Graph Query Optimizer integrates with:
- **GraphIndexManager**: Core graph operations (BFS, Dijkstra, A*)
- **QueryOptimizer**: General query optimization framework
- **Result<T>**: Error handling using the tl::expected pattern
- **Error Registry**: Structured error reporting

## Testing

Comprehensive test suite in `tests/test_graph_query_optimizer.cpp`:
- Statistics collection tests
- Plan optimization tests for all query patterns
- Algorithm execution tests (BFS, DFS, Dijkstra, A*, Bidirectional)
- Constraint handling tests
- Plan caching tests
- Execution history tests
- Performance and scalability tests

Run tests:
```bash
./build/test_graph_query_optimizer
```

## Future Enhancements

Potential improvements for future versions:
1. **Adaptive Learning**: Learn from execution history to improve cost estimates
2. **Multi-Source BFS**: Parallel BFS from multiple sources
3. **Approximate Algorithms**: Trade accuracy for speed in large graphs
4. **Query Rewriting**: Transform queries for better performance
5. **Distributed Execution**: Partition large graphs across nodes
6. **GPU Acceleration**: Offload traversals to GPU for massive parallelism

## References

- [ArangoDB Graph Queries](https://www.arangodb.com/docs/stable/aql/graphs.html)
- [Neo4j Query Optimization](https://neo4j.com/docs/cypher-manual/current/query-tuning/)
- Brandes, U. (2001). "A faster algorithm for betweenness centrality"
- Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). "A Formal Basis for the Heuristic Determination of Minimum Cost Paths"

## License

Part of ThemisDB - Multi-Model Database System
