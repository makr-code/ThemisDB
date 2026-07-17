# ThemisDB Graph Module - Header Reference

<!-- Status: current | validated: 2026-07-17 -->
<!-- Primary sources: ../../src/graph/README.md · ../../src/graph/ROADMAP.md -->

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

## Current Implementation Status

- Backing implementation status follows the canonical graph module docs in `src/graph/README.md`
  and `src/graph/ROADMAP.md`.
- The 2026-06-25 L0 re-validation closed the earlier scanner alert: all 9 initial detections were
  verified as defensive patterns, leaving **0 verified gaps** in the graph implementation.
- Headers listed below remain the public API reference for a production-ready graph module; runtime
  hardening and follow-on roadmap items continue to live in `src/graph/`.

---

## Overview

This directory contains the public header files for ThemisDB's Graph module. These headers define the API for graph query optimization, constrained path finding, and advanced graph operations.

## Header Files

## Public Header Entry Points

| Header | Primary Public Entry Points | Backing Implementation |
|--------|------------------------------|------------------------|
| `graph_query_optimizer.h` | `GraphQueryOptimizer`, `QueryConstraints`, `OptimizationPlan` | `src/graph/graph_query_optimizer.cpp` |
| `path_constraints.h` | `PathConstraints`, `PathResult` | `src/graph/path_constraints.cpp` |
| `distributed_graph.h` | `DistributedGraph`, `DistributedGraphConfig` | `src/graph/distributed_graph.cpp` |
| `parallel_traversal.h` | `ParallelTraversal`, `ParallelTraversal::Config` | `src/graph/parallel_traversal.cpp` |
| `gpu_traversal.h` | `GPUGraphTraversal`, `GPUGraphTraversal::Config` | `src/graph/gpu_traversal.cpp`, `src/graph/gpu_traversal_cuda.cu` |
| `scheduled_edge_refresh.h` | `ScheduledGraphEdgeRefreshEngine`, `RefreshPolicy`, `RefreshStats` | `src/graph/scheduled_edge_refresh.cpp` |
| `graph_query_rewriter.h` | `GraphQueryRewriter`, `RewriteConfig` | `src/graph/graph_query_rewriter.cpp` |
| `knowledge_graph_reasoner.h` | `KnowledgeGraphReasoner`, inference/LoRA integration API | `src/graph/knowledge_graph_reasoner.cpp` |
| `ontology_manager.h` | `OntologyManager`, semantic ontology loading/lookup | `src/graph/ontology_manager.cpp` |
| `graph_embedding.h` | `GraphEmbeddingModel`, `GraphEmbeddingConfig` | (header-level API; module implementation integrated via consumers) |
| `graph_watermark.h` | `GraphWatermark` | `src/graph/graph_watermark.cpp` |
| `explain_plan.h` | `ExplainPlan` | `src/graph/explain_plan.cpp` |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph`, `FingerprintGraphConfig` | `src/graph/tensor_fingerprint_graph.cpp` |
| `tensor_deduplication_manager.h` | `TensorDeduplicationManager`, `DeduplicationConfig` | `src/graph/tensor_deduplication_manager.cpp` |

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
- [Graph Module Roadmap](../../src/graph/ROADMAP.md) - Delivery phases and production-readiness checklist
- [Graph Future Enhancements](../../src/graph/FUTURE_ENHANCEMENTS.md) - Planned features
- [Graph Architecture](../../src/graph/ARCHITECTURE.md) - Runtime design and control flow
- [Graph Performance Expectations](../../src/graph/PERFORMANCE_EXPECTATIONS.md) - Targets and benchmark mappings
- [Graph Security Notes](../../src/graph/SECURITY.md) - Security assumptions and controls
- [Index Module](../index/README.md) - Graph infrastructure (GraphIndexManager, GraphAnalytics)
- [Query Module](../query/README.md) - AQL integration
- [German Graph Overview](../../docs/de/graph/README.md) - Einstieg, Usage, Troubleshooting (DE)

## Additional Public Headers

| Header | Purpose |
|--------|---------|
| `distributed_graph.h` | `DistributedGraph` — partitioned graph execution across shards |
| `explain_plan.h` | `ExplainPlan` — human-readable query plan explanation output |
| `gpu_traversal.h` | `GPUGraphTraversal` — GPU-accelerated traversal with CPU fallback |
| `graph_embedding.h` | `GraphEmbedding` — node and edge embedding computation |
| `graph_query_rewriter.h` | `GraphQueryRewriter` — rewrite rules for graph query optimisation |
| `graph_watermark.h` | `GraphWatermark` — temporal watermark tracking for streaming graph updates |
| `knowledge_graph_reasoner.h` | `KnowledgeGraphReasoner` — rule-based + LoRA-assisted graph inference |
| `ontology_manager.h` | `OntologyManager` — ontology loading and semantic type checks |
| `parallel_traversal.h` | `ParallelTraversal` — multi-threaded BFS/DFS traversal engine |
| `scheduled_edge_refresh.h` | `ScheduledEdgeRefresh` — periodic refresh of derived/cached edges |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph` — tensor-similarity graph via MinHash/LSH |
| `tensor_deduplication_manager.h` | `TensorDeduplicationManager` — similarity-based tensor deduplication |

## Configuration Surfaces (Runtime)

| API | Configuration Surface | Typical Knobs |
|-----|------------------------|---------------|
| `GraphQueryOptimizer` | `QueryConstraints` + setter APIs | `max_depth`, `max_results`, `edge_type`, `graph_id`, cache size/TTL, adaptive learning, QPS limits |
| `PathConstraints` | constraint builder methods | Min/max length, required/forbidden nodes or edges, acyclic/unique, semantic constraints |
| `ParallelTraversal` | `ParallelTraversal::Config` | `max_depth`, `num_threads`, `fan_out_threshold`, `timeout_ms`, `max_results` |
| `GPUGraphTraversal` | `GPUGraphTraversal::Config` | `gpu_device`, `min_vertices_for_gpu`, `max_depth`, `max_results`, forbidden vertices |
| `ScheduledGraphEdgeRefreshEngine` | `RefreshPolicy` | `refresh_interval`, thresholds, add/remove limits, anomaly threshold, ANN switch |
| `GraphQueryRewriter` | `RewriteConfig` | enabled rules, aggressive mode, rewrite-time budget |
| Tensor redundancy APIs | `FingerprintGraphConfig`, `DeduplicationConfig` | similarity threshold, LSH parameters, top-k candidates, delta constraints |

## Runtime Behavior, Failure Cases, and Limits

- All fallible graph APIs return `Result<T>`; inspect `error().message` and code paths from `utils/error_registry.h`.
- `GraphQueryOptimizer`, `PathConstraints`, and traversal helpers are not safe for concurrent mutation on the same instance.
- GPU traversal can return valid results via CPU fallback when GPU runtime or size thresholds do not qualify for device execution.
- `PathConstraints` and optimizer constraints can intentionally return empty results (`NOT_FOUND`) when filters are contradictory.
- Refresh engine safety gates can abort a cycle before commit when removal/addition thresholds are exceeded.

## Troubleshooting

| Symptom | Likely Cause | Action |
|---------|--------------|--------|
| `INVALID_STATE` on optimization/constraint calls | Graph manager not set or object lifecycle mismatch | Ensure `GraphIndexManager` is configured before calling optimization/traversal APIs. |
| Unexpectedly empty constrained paths | Contradictory `required_*` + `forbidden_*` constraints or too low `max_depth` | Relax constraints and inspect `describeConstraints()` output. |
| No GPU path observed | `min_vertices_for_gpu` not reached or runtime without GPU support | Tune config for tests and rely on documented CPU fallback behavior in production. |
| Refresh cycles repeatedly abort | `RefreshPolicy` thresholds too strict for current graph churn | Increase removal/add limits or rebalance thresholds incrementally. |

*Last Updated: May 2026*
*Module Version: v1.5.0*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
