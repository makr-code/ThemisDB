# Graph Advanced Features

This directory contains advanced graph analytics and query features for ThemisDB. These modules extend the basic graph query capabilities with sophisticated algorithms for path analysis.

## Status: GAP-006 Implementation (Partial)

**Current Version:** Stub/Placeholder Implementation  
**Implementation Date:** February 2026  
**Status:** PathConstraints stub complete; Centrality and Community Detection already implemented in GraphAnalytics

## Important Note on Existing Implementations

**Centrality and Community Detection algorithms are already fully implemented** in the `GraphAnalytics` class:
- **Location:** `include/index/graph_analytics.h` and `src/index/graph_analytics.cpp`
- **Implemented Algorithms:**
  - ✅ Degree Centrality
  - ✅ PageRank
  - ✅ Betweenness Centrality (Brandes' algorithm)
  - ✅ Closeness Centrality
  - ✅ Louvain Community Detection
  - ✅ Label Propagation Community Detection
  - ✅ K-Shortest Paths (Yen's algorithm)

**Use the existing `GraphAnalytics` class for these features instead of waiting for future implementations.**

## New Module: Path Constraints

### Path Constraints (`path_constraints.h/cpp`)

Advanced path finding with complex constraint specifications.

**Features:**
- Length constraints (min/max)
- Node/edge requirements and prohibitions
- Uniqueness constraints (nodes/edges)
- Acyclic path requirements
- Custom validation predicates

**Example Usage:**
```cpp
#include "graph/path_constraints.h"

using namespace themis::graph;

PathConstraints constraints;
constraints.addMinLength(3);
constraints.addMaxLength(10);
constraints.addForbiddenNode("blocked_node");
constraints.requireAcyclic();

// Future: Full implementation will enable
// auto paths = constraints.findConstrainedPaths("start", "end", 5);
```

**Use Cases:**
- Supply chain routing with restrictions
- Social network path analysis with privacy constraints
- Network security path enumeration
- Knowledge graph reasoning with constraints

## Using Existing Graph Analytics

For centrality and community detection, use the fully-implemented `GraphAnalytics` class:

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

## Planned Implementation Timeline

## Planned Implementation Timeline

### Phase 1: Path Constraints (Q2 2026)
- Implement constraint validation
- Add constrained BFS/DFS
- Integrate with query optimizer
- Add comprehensive tests

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

### Path Constraints (Future Implementation)
1. **Index Utilization**: Leverage graph indices for neighbor access
2. **Caching**: Cache intermediate results (adjacency, paths)
3. **Parallelization**: Use multi-threading for independent operations
4. **Approximation**: Trade accuracy for speed on large graphs

### GraphAnalytics (Current Implementation)
- Optimized batch lookups (10-100× faster for large graphs)
- Pre-allocated maps to avoid rehashing
- Efficient adjacency structure building
- See `src/index/graph_analytics.cpp` for implementation details

## Error Handling

All methods return `Result<T>` for consistent error handling:

```cpp
auto result = analytics.computePageRank(config);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return;
}

const auto& page_rank = result.value();
// Use page_rank...
```

Current stub implementations return:
```cpp
ErrorRegistry::ErrorCode::NOT_IMPLEMENTED
"Method is not yet implemented. This is a stub for GAP-006..."
```

## Testing

### Current Tests
Basic interface tests verify:
- Constructor/destructor behavior
- Method signatures and return types
- Error handling for unimplemented methods
- Integration with GraphIndexManager

### Planned Tests
Comprehensive test suites will include:
- Correctness tests with known graphs
- Performance benchmarks
- Scalability tests
- Edge case handling
- Integration tests with query optimizer

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

- [Graph Analytics](../include/index/graph_analytics.h) - **Existing centrality and community detection implementations**
- [Graph Query Optimizer](./README.md) - Existing graph query optimization
- [Graph Index](../include/index/graph_index.h) - Core graph storage
- [Error Handling](../include/utils/expected.h) - Result<T> pattern
- [GAP Analysis](../../docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md) - Implementation gaps

## Future Enhancements

### Path Constraints (Planned)
- Constraint validation algorithms
- Constrained BFS/DFS traversal
- Integration with query optimizer
- Performance optimization for large graphs

### Additional Graph Features
- **Temporal Path Analysis**: Time-aware path constraints
- **Weighted Constraints**: Support for edge/node weights
- **Approximate Algorithms**: Fast approximations for massive graphs
- **Streaming Constraints**: Real-time constraint evaluation

### Integration Features
- **AQL Integration**: Native query language support for constraints
- **Visualization**: Path visualization with constraint highlighting
- **Export**: JSON/CSV export of constrained path results
- **Monitoring**: Real-time metrics for constraint evaluation
