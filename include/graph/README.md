> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Graph Module - Header Reference

## Overview

This directory contains the public header files for ThemisDB's Graph module. These headers define the API for graph query optimization, constrained path finding, and advanced graph operations.

## Header Files

### graph_query_optimizer.h
**Purpose:** Cost-based graph query optimization and algorithm selection

**Key Classes:**
- `GraphQueryOptimizer`: Main optimizer class for graph queries
  - Algorithm selection (BFS, DFS, Dijkstra, A*, Bidirectional)
  - Cost estimation and plan generation
  - Execution with statistics tracking
  - Plan caching and adaptive optimization

**Key Types:**
```cpp
// Traversal algorithms
enum class TraversalAlgorithm {
    BFS,              // Breadth-First Search
    DFS,              // Depth-First Search
    BIDIRECTIONAL,    // Bidirectional search
    ASTAR,            // A* heuristic search
    DIJKSTRA          // Weighted shortest path
};

// Query patterns
enum class QueryPattern {
    SHORTEST_PATH,           // Single shortest path
    ALL_PATHS,              // All paths between nodes
    K_HOP_NEIGHBORS,        // k-hop neighborhood
    PATTERN_MATCH,          // Subgraph pattern matching
    REACHABILITY,           // Reachability check
    CONNECTED_COMPONENT     // Component analysis
};

// Graph statistics
struct GraphStatistics {
    size_t vertex_count;
    size_t edge_count;
    double avg_degree;
    double avg_branching_factor;
    size_t max_depth;
    bool has_edge_index;
    bool has_adjacency_cache;
    std::unordered_map<std::string, size_t> edge_type_counts;
    std::unordered_map<std::string, double> edge_type_selectivity;
};

// Query constraints
struct QueryConstraints {
    std::optional<int> max_depth;
    std::optional<size_t> max_results;
    std::optional<std::string> edge_type;
    std::optional<std::string> graph_id;
    bool unique_vertices = false;
    bool unique_edges = false;
    std::vector<std::string> forbidden_vertices;
    std::vector<std::string> required_vertices;
};

// Optimization plan
struct OptimizationPlan {
    TraversalAlgorithm algorithm;
    QueryPattern pattern;
    double estimated_cost;
    double estimated_time_ms;
    size_t estimated_nodes_explored;
    bool use_index = false;
    bool use_cache = false;
    bool enable_early_termination = false;
    bool enable_parallel = false;
    std::string explanation;
    std::vector<std::pair<TraversalAlgorithm, double>> alternatives;
};

// Execution statistics
struct ExecutionStats {
    size_t nodes_explored = 0;
    size_t edges_traversed = 0;
    size_t paths_found = 0;
    double execution_time_ms = 0.0;
    size_t max_depth_reached = 0;
    double avg_branching_observed = 0.0;
    bool early_terminated = false;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
};
```

**Main Methods:**
```cpp
class GraphQueryOptimizer {
public:
    explicit GraphQueryOptimizer(GraphIndexManager& graph_manager);

    // Optimization
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start, std::string_view target,
        const QueryConstraints& constraints = {}
    );

    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start, int k,
        const QueryConstraints& constraints = {}
    );

    Result<OptimizationPlan> optimizePatternMatch(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        const QueryConstraints& constraints = {}
    );

    Result<OptimizationPlan> optimizeReachability(
        std::string_view start, std::string_view target,
        const QueryConstraints& constraints = {}
    );

    Result<OptimizationPlan> optimizeConstrainedPath(
        std::string_view start, std::string_view end,
        const PathConstraints& constraints
    );

    // Execution
    Result<std::vector<std::string>> executeBFS(
        std::string_view start, int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<std::vector<std::string>> executeDFS(
        std::string_view start, int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<PathResult> executeDijkstra(
        std::string_view start, std::string_view target,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<PathResult> executeAStar(
        std::string_view start, std::string_view target,
        std::function<double(const std::string&)> heuristic,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<PathResult> executeBidirectional(
        std::string_view start, std::string_view target,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    // Statistics
    Result<GraphStatistics> collectStatistics(
        std::optional<std::string_view> graph_id = std::nullopt
    );

    const GraphStatistics& getStatistics() const;
    double estimateEdgeTypeSelectivity(std::string_view edge_type) const;

    // Plan Management
    std::string explainPlan(const OptimizationPlan& plan) const;
    void setPlanCachingEnabled(bool enabled);
    void clearPlanCache();
    const std::vector<ExecutionStats>& getExecutionHistory() const;
};
```

**Usage Example:**
```cpp
#include "graph/graph_query_optimizer.h"

GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);

// Collect statistics
auto stats = optimizer.collectStatistics();

// Optimize query
GraphQueryOptimizer::QueryConstraints constraints;
constraints.max_depth = 5;
constraints.unique_vertices = true;

auto plan = optimizer.optimizeShortestPath("A", "B", constraints);
if (plan) {
    std::cout << optimizer.explainPlan(plan.value()) << std::endl;

    // Execute
    GraphQueryOptimizer::ExecutionStats exec_stats;
    auto result = optimizer.executeBFS("A", 5, constraints, &exec_stats);

    std::cout << "Nodes explored: " << exec_stats.nodes_explored << std::endl;
    std::cout << "Time: " << exec_stats.execution_time_ms << "ms" << std::endl;
}
```

---

### path_constraints.h
**Purpose:** Constraint-based path finding with validation

**Key Classes:**
- `PathConstraints`: Manages path constraints and validation
  - Length constraints (min/max)
  - Node/edge requirements and prohibitions
  - Uniqueness constraints
  - Custom validation predicates
  - Path finding with constraint satisfaction

**Key Types:**
```cpp
// Constraint types
enum class ConstraintType {
    MIN_LENGTH,           // Minimum path length
    MAX_LENGTH,           // Maximum path length
    NODE_PROPERTY,        // Node property requirement
    EDGE_PROPERTY,        // Edge property requirement
    FORBIDDEN_NODE,       // Cannot pass through node
    REQUIRED_NODE,        // Must pass through node
    FORBIDDEN_EDGE,       // Cannot use edge
    REQUIRED_EDGE,        // Must use edge
    NO_CYCLES,           // Path must be acyclic
    UNIQUE_NODES,        // All nodes unique
    UNIQUE_EDGES,        // All edges unique
    CUSTOM_PREDICATE     // Custom validation
};

// Constraint specification
struct Constraint {
    ConstraintType type;
    std::optional<int> int_value;
    std::optional<std::string> string_value;
    std::optional<std::function<bool(const std::vector<std::string>&)>> predicate;
};

// Path result
struct PathResult {
    std::vector<std::string> nodes;
    std::vector<std::string> edges;
    double cost = 0.0;
    bool satisfies_all_constraints = false;
    std::vector<std::string> violated_constraints;
};
```

**Main Methods:**
```cpp
class PathConstraints {
public:
    PathConstraints() = default;
    explicit PathConstraints(GraphIndexManager* graph_mgr);

    void setGraphManager(GraphIndexManager* graph_mgr);

    // Length constraints
    void addMinLength(int min_length);
    void addMaxLength(int max_length);

    // Node constraints
    void addForbiddenNode(std::string_view node_id);
    void addRequiredNode(std::string_view node_id);

    // Edge constraints
    void addForbiddenEdge(std::string_view edge_id);
    void addRequiredEdge(std::string_view edge_id);

    // Uniqueness constraints
    void requireAcyclic();
    void requireUniqueNodes();
    void requireUniqueEdges();

    // Custom validation
    void addCustomPredicate(
        std::function<bool(const std::vector<std::string>&)> predicate
    );

    // Path operations
    Result<bool> validatePath(
        const std::vector<std::string>& nodes,
        const std::vector<std::string>& edges
    ) const;

    Result<std::vector<PathResult>> findConstrainedPaths(
        std::string_view start_node,
        std::string_view end_node,
        int max_results = 10
    ) const;

    // Inspection
    const std::vector<Constraint>& getConstraints() const;
    std::string describeConstraints() const;
    void clearConstraints();
};
```

**Usage Example:**
```cpp
#include "graph/path_constraints.h"

GraphIndexManager graph_mgr(storage);
PathConstraints constraints(&graph_mgr);

// Add constraints
constraints.addMinLength(3);
constraints.addMaxLength(10);
constraints.addForbiddenNode("blocked_user");
constraints.addRequiredNode("checkpoint");
constraints.requireUniqueNodes();

// Custom validation
constraints.addCustomPredicate([](const std::vector<std::string>& nodes) {
    return nodes.size() % 2 == 0;  // Even length only
});

// Find paths
auto result = constraints.findConstrainedPaths("start", "end", 10);
if (result) {
    for (const auto& path : result.value()) {
        std::cout << "Path cost: " << path.cost << std::endl;
        std::cout << "Valid: " << path.satisfies_all_constraints << std::endl;
    }
}

// Validate existing path
std::vector<std::string> nodes = {"A", "B", "C"};
std::vector<std::string> edges = {"e1", "e2"};
auto valid = constraints.validatePath(nodes, edges);
```

---

## Integration with Other Modules

### Graph Index Manager
The graph module depends on `GraphIndexManager` from the index module for:
- Graph topology access (adjacency lists)
- Basic traversal operations (BFS, Dijkstra)
- Graph statistics collection
- Edge and node lookups

```cpp
#include "index/graph_index.h"
#include "graph/graph_query_optimizer.h"

GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);
// Optimizer uses graph_mgr for all graph operations
```

### Query Module
The query module uses the graph optimizer for AQL graph queries:

```cpp
#include "query/query_engine.h"
#include "graph/graph_query_optimizer.h"

// AQL: FOR v IN 1..3 OUTBOUND 'start' GRAPH 'social' RETURN v
// Internally calls:
optimizer.optimizeKHopNeighborhood("start", 3, constraints);
```

### Error Handling
All graph operations use `Result<T>` from `utils/expected.h`:

```cpp
#include "utils/expected.h"

Result<OptimizationPlan> plan = optimizer.optimizeShortestPath("A", "B");
if (!plan) {
    std::cerr << "Error: " << plan.error().message << std::endl;
    return;
}
// Use plan.value()
```

## Thread Safety

**GraphQueryOptimizer:**
- NOT thread-safe for concurrent use on same instance
- Create per-query instances or use mutex
- Statistics reads are thread-safe
- Execution methods are thread-safe with separate contexts

**PathConstraints:**
- NOT thread-safe for concurrent use on same instance
- Create per-query instances or use mutex
- Read operations (validatePath, describeConstraints) are thread-safe if constraints are immutable
- findConstrainedPaths uses const GraphIndexManager (thread-safe reads)

## Performance Considerations

**Optimization Overhead:**
- Statistics collection: 10-100ms (one-time, cached)
- Plan generation: 0.1-5ms
- Plan cache lookup: ~1μs

**Execution Performance:**
- BFS/DFS: O(V + E) or O(b^d)
- Dijkstra/A*: O((V+E) log V)
- Bidirectional: O(b^(d/2))

**Memory Usage:**
- GraphQueryOptimizer: ~1KB instance + plan cache
- PathConstraints: ~1KB + constraint data
- Execution contexts: O(V) for visited sets

## Design Patterns

**Cost-Based Optimization:**
- Collect statistics once, use many times
- Generate multiple plans, select lowest cost
- Cache plans for repeated queries
- Learn from execution history (adaptive)

**Constraint Satisfaction:**
- Validate early during traversal (efficiency)
- Validate post-hoc for correctness
- Sort results by cost/relevance
- Limit results for responsiveness

**Error Handling:**
- Use Result<T> for all fallible operations
- Provide detailed error messages
- Use error codes from error registry
- Return errors, never throw exceptions

## Examples

### Simple Shortest Path
```cpp
GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);

auto plan = optimizer.optimizeShortestPath("A", "B");
auto result = optimizer.executeDijkstra("A", "B", {});

if (result) {
    auto path = result.value();
    std::cout << "Path: ";
    for (const auto& node : path.path) {
        std::cout << node << " ";
    }
    std::cout << "\nCost: " << path.totalCost << std::endl;
}
```

### Constrained Path Finding
```cpp
GraphIndexManager graph_mgr(storage);
PathConstraints constraints(&graph_mgr);

constraints.addMinLength(2);
constraints.addMaxLength(5);
constraints.addForbiddenNode("avoid_this");
constraints.requireUniqueNodes();

auto paths = constraints.findConstrainedPaths("start", "end", 5);
if (paths) {
    for (const auto& path : paths.value()) {
        std::cout << "Path length: " << path.nodes.size() << std::endl;
        std::cout << "Cost: " << path.cost << std::endl;
    }
}
```

### Optimization with Statistics
```cpp
GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);

// Collect statistics
auto stats = optimizer.collectStatistics();
std::cout << "Vertices: " << stats.value().vertex_count << std::endl;
std::cout << "Edges: " << stats.value().edge_count << std::endl;
std::cout << "Avg degree: " << stats.value().avg_degree << std::endl;

// Use statistics for optimization
auto plan = optimizer.optimizeKHopNeighborhood("start", 3, {});
std::cout << optimizer.explainPlan(plan.value()) << std::endl;
```

## Related Documentation

- [Graph Module Implementation](../../src/graph/README.md) - Detailed implementation documentation
- [Graph Future Enhancements](../../src/graph/FUTURE_ENHANCEMENTS.md) - Planned features
- [Index Module](../index/README.md) - Graph infrastructure (GraphIndexManager, GraphAnalytics)
- [Query Module](../query/README.md) - AQL integration

*Last Updated: April 2026*
*Module Version: v1.5.0*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
