# ThemisDB Graph Module - Implementation

## Module Purpose

The Graph module implements ThemisDB's advanced graph database capabilities, providing cost-based query optimization, constrained path finding, and integration with the broader graph infrastructure. It extends the GraphIndexManager with sophisticated query planning, algorithm selection, and constraint-based traversal to enable efficient graph operations across property graphs, temporal graphs, and multi-model queries.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `graph_query_optimizer.cpp` | Cost-based algorithm selection and plan generation |
| `traversal_engine.cpp` | BFS/DFS/Dijkstra/A* traversal execution |
| `graph_index_manager.h` | Graph index operations interface |
| `query_plan_cache.cpp` | Query plan caching and execution statistics |

## Scope

**In Scope:**
- Graph query optimization with cost-based algorithm selection
- Constrained path finding (min/max length, required/forbidden nodes/edges)
- Traversal algorithm selection (BFS, DFS, Dijkstra, A*, Bidirectional)
- Query plan generation and cost estimation
- Path validation and constraint checking
- Integration with GraphIndexManager for graph operations
- Integration with AQL for graph query execution
- Query plan caching and execution statistics
- Adaptive optimization based on execution history

**Out of Scope:**
- Graph storage and indexing (handled by index module's GraphIndexManager)
- Graph analytics algorithms (handled by GraphAnalytics in index module)
- Basic graph traversal operations (handled by GraphIndexManager)
- Property graph model management (handled by PropertyGraphManager)
- Temporal graph operations (handled by TemporalGraphManager)
- Graph visualization and UI (handled by client SDKs)

## Key Components

### Graph Query Optimizer
**Location:** `graph_query_optimizer.cpp`, `../include/graph/graph_query_optimizer.h`

Cost-based optimizer that selects optimal traversal algorithms and generates execution plans for graph queries.

**Purpose:**
- Analyzes query patterns and graph statistics to select best algorithm
- Generates execution plans with cost estimates
- Provides plan explanation and alternative strategies
- Tracks execution statistics for adaptive optimization
- Integrates with PathConstraints for constrained path queries

**Features:**
```cpp
#include "graph/graph_query_optimizer.h"

// Initialize optimizer with graph manager
GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);

// Collect graph statistics
auto stats = optimizer.collectStatistics();
// Stats include: vertex count, edge count, avg degree, branching factor

// Optimize shortest path query
auto plan = optimizer.optimizeShortestPath("user_A", "user_B");
if (plan) {
    std::cout << "Algorithm: " << algorithmName(plan->algorithm) << std::endl;
    std::cout << "Estimated cost: " << plan->estimated_cost << std::endl;
    std::cout << "Estimated time: " << plan->estimated_time_ms << "ms" << std::endl;
    std::cout << "Explanation: " << plan->explanation << std::endl;
}

// Execute optimized query with statistics
GraphQueryOptimizer::ExecutionStats stats;
auto result = optimizer.executeBFS("user_A", 3, constraints, &stats);
std::cout << "Nodes explored: " << stats.nodes_explored << std::endl;
std::cout << "Execution time: " << stats.execution_time_ms << "ms" << std::endl;
```

**Supported Query Patterns:**
- **SHORTEST_PATH**: Finding shortest path between two vertices
- **K_HOP_NEIGHBORS**: Finding all vertices within k hops
- **PATTERN_MATCH**: Subgraph pattern matching
- **REACHABILITY**: Checking if one vertex is reachable from another
- **ALL_PATHS**: Enumerating all paths between vertices
- **CONNECTED_COMPONENT**: Finding connected components

**Traversal Algorithms:**

| Algorithm | Time Complexity | Best Use Case |
|-----------|----------------|---------------|
| BFS | O(V + E) or O(b^d) | Shortest unweighted path, k-hop queries |
| DFS | O(V + E) | Deep exploration, pattern matching |
| Dijkstra | O((V+E) log V) | Weighted shortest path |
| A* | O((V+E) log V) | Heuristic-guided search with domain knowledge |
| Bidirectional | O(b^(d/2)) | Long-distance paths in large graphs |

**Algorithm Selection Logic:**
```cpp
// BFS: Best for short distances, unweighted graphs
if (pattern == SHORTEST_PATH && !has_weights && estimated_depth <= 5) {
    return TraversalAlgorithm::BFS;
}

// Dijkstra: Best for weighted shortest path
if (pattern == SHORTEST_PATH && has_weights) {
    return TraversalAlgorithm::DIJKSTRA;
}

// A*: Best when heuristic available (e.g., spatial coordinates)
if (pattern == SHORTEST_PATH && has_heuristic && has_weights) {
    return TraversalAlgorithm::ASTAR;
}

// Bidirectional: Best for long distances
if (pattern == SHORTEST_PATH && estimated_depth > 10) {
    return TraversalAlgorithm::BIDIRECTIONAL;
}

// DFS: Best for pattern matching and deep exploration
if (pattern == PATTERN_MATCH || pattern == ALL_PATHS) {
    return TraversalAlgorithm::DFS;
}
```

**Query Constraints:**
```cpp
GraphQueryOptimizer::QueryConstraints constraints;
constraints.max_depth = 5;                    // Limit traversal depth
constraints.max_results = 100;                // Early terminate after N results
constraints.unique_vertices = true;           // No vertex visited twice
constraints.unique_edges = true;              // No edge traversed twice
constraints.edge_type = "FOLLOWS";           // Filter by edge type
constraints.graph_id = "social_network";     // Filter by graph ID
constraints.forbidden_vertices = {"blocked_user", "deactivated_user"};
constraints.required_vertices = {"checkpoint_node"};
```

**Cost Estimation:**
```cpp
// Cost factors
double cost = 0.0;

// Base cost: estimated nodes to explore
cost += estimated_depth * avg_branching_factor;

// Index usage reduces cost
if (has_edge_index) {
    cost *= 0.8;  // 20% reduction
}

// Cache usage reduces cost
if (has_adjacency_cache) {
    cost *= 0.7;  // 30% reduction
}

// Edge type filtering reduces cost
if (edge_type_specified) {
    double selectivity = edge_type_selectivity[edge_type];
    cost *= selectivity;
}

// Parallel execution reduces cost
if (enable_parallel && estimated_nodes > 10000) {
    cost *= 0.5;  // 50% reduction with parallelism
}

return cost;
```

**Structural Plan Reuse (Query Plan Reuse Across Structurally Similar Queries):**

Two queries are "structurally similar" when they share the same `QueryPattern` and
`QueryConstraints` but differ only in start/target vertex IDs. Because algorithm
selection is driven entirely by pattern type, constraint shape, and graph-level
statistics — not by specific vertex IDs — the optimizer can reuse the same
`OptimizationPlan` for all such queries.

The plan cache employs a two-level lookup strategy:
1. **Exact key** (`pattern:start:target[:depth=N][:type=T]…`) — fastest lookup
   for repeated identical queries.
2. **Structural key** (`struct:pattern[:depth=N][:type=T][:uv][:ue][:par]…`) —
   used when the exact key misses. If found, the plan is promoted to the exact
   key for faster future lookups.

```cpp
optimizer.setPlanCachingEnabled(true);

// First query — cold start; populates both the exact and structural cache keys.
auto plan1 = optimizer.optimizeShortestPath("user_A", "user_B", constraints);

// Second query — different vertices, same constraints.
// Served from the structural cache entry (cache hit, no re-planning).
auto plan2 = optimizer.optimizeShortestPath("user_C", "user_D", constraints);

// plan1 and plan2 carry identical algorithm, cost, and node estimates.
assert(plan1->algorithm == plan2->algorithm);
assert(plan1->estimated_cost == plan2->estimated_cost);

// Monitor cache efficiency
const auto& m = optimizer.getQueryMetrics();
uint64_t hits   = m.plan_cache_hits.load();
uint64_t misses = m.plan_cache_misses.load();
```

Structural key components (all constraint factors that affect plan selection):
- `QueryPattern` (SHORTEST_PATH, K_HOP_NEIGHBORS, PATTERN_MATCH, REACHABILITY, …)
- `max_depth` or depth hint (e.g., `k` for K_HOP queries, vertex/edge count for PATTERN_MATCH)
- `edge_type` (selectivity-driven cost)
- `unique_vertices` / `unique_edges` flags
- `enable_parallel` flag
- Count of `forbidden_vertices` / `required_vertices`

Fields intentionally excluded from the structural key (they affect execution, not planning):
- `timeout_ms` (execution deadline only)
- `max_results` (early termination only)
- `num_threads` (parallelism tuning only)
- `graph_id` (statistics are global; per-graph statistics can be collected separately)

**Thread Safety:**
- Optimizer instances are NOT thread-safe (create per-query or use mutex)
- Statistics reads are thread-safe
- Execution methods are thread-safe with separate contexts

### Path Constraints
**Location:** `path_constraints.cpp`, `../include/graph/path_constraints.h`

Advanced constraint-based path finding with complex requirement specifications.

**Purpose:**
- Enables complex path queries with multiple constraints
- Validates paths against constraint rules
- Integrates with GraphQueryOptimizer for query planning
- Provides flexible constraint API for diverse use cases

**Features:**
```cpp
#include "graph/path_constraints.h"

// Create constraints object with graph manager
GraphIndexManager graph_mgr(storage);
PathConstraints constraints(&graph_mgr);

// Add length constraints
constraints.addMinLength(3);      // Path must have at least 3 nodes
constraints.addMaxLength(10);     // Path cannot exceed 10 nodes

// Add node constraints
constraints.addForbiddenNode("blocked_user");    // Cannot pass through
constraints.addRequiredNode("checkpoint");       // Must pass through

// Add edge constraints
constraints.addForbiddenEdge("edge_123");        // Cannot use this edge
constraints.addRequiredEdge("edge_456");         // Must use this edge

// Add uniqueness constraints
constraints.requireUniqueNodes();   // No node appears twice
constraints.requireUniqueEdges();   // No edge appears twice
constraints.requireAcyclic();       // Path must be acyclic

// Add custom validation
constraints.addCustomPredicate([](const std::vector<std::string>& nodes) {
    return nodes.size() % 2 == 0;  // Only even-length paths
});

// Find paths satisfying all constraints
auto result = constraints.findConstrainedPaths("start", "end", 10);
if (result) {
    for (const auto& path : result.value()) {
        std::cout << "Path cost: " << path.cost << std::endl;
        std::cout << "Satisfies constraints: " << path.satisfies_all_constraints << std::endl;
        std::cout << "Nodes: ";
        for (const auto& node : path.nodes) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
}
```

**Constraint Types:**

| Type | Description | Example |
|------|-------------|---------|
| MIN_LENGTH | Minimum path length | `addMinLength(3)` |
| MAX_LENGTH | Maximum path length | `addMaxLength(10)` |
| FORBIDDEN_NODE | Cannot pass through node | `addForbiddenNode("blocked")` |
| REQUIRED_NODE | Must pass through node | `addRequiredNode("checkpoint")` |
| FORBIDDEN_EDGE | Cannot use edge | `addForbiddenEdge("edge_123")` |
| REQUIRED_EDGE | Must use edge | `addRequiredEdge("edge_456")` |
| NO_CYCLES | Path must be acyclic | `requireAcyclic()` |
| UNIQUE_NODES | All nodes unique | `requireUniqueNodes()` |
| UNIQUE_EDGES | All edges unique | `requireUniqueEdges()` |
| CUSTOM_PREDICATE | Custom validation | `addCustomPredicate(fn)` |
| NODE_PROPERTY | Node property requirement | Partial support |
| EDGE_PROPERTY | Edge property requirement | Partial support |

**Path Validation:**
```cpp
// Validate existing path
std::vector<std::string> nodes = {"A", "B", "C", "D"};
std::vector<std::string> edges = {"e1", "e2", "e3"};

auto valid = constraints.validatePath(nodes, edges);
if (!valid) {
    std::cerr << "Path validation failed: " << valid.error().message << std::endl;
}
```

**Integration with Query Optimizer:**
```cpp
// Get optimization plan for constrained path
GraphQueryOptimizer optimizer(graph_mgr);
PathConstraints constraints(&graph_mgr);
constraints.addMinLength(2);
constraints.addMaxLength(5);
constraints.requireUniqueNodes();

auto plan = optimizer.optimizeConstrainedPath("start", "end", constraints);
if (plan) {
    std::cout << "Recommended algorithm: " << algorithmName(plan->algorithm) << std::endl;
    std::cout << "Estimated cost: " << plan->estimated_cost << std::endl;
    std::cout << optimizer.explainPlan(plan.value()) << std::endl;
}
```

**Constraint Resolution:**
```cpp
// BFS traversal with constraint validation
// 1. Start from source node
// 2. For each neighbor:
//    a. Check if node is forbidden -> skip
//    b. Check if adding node violates max_length -> skip
//    c. Check if path would create cycle (if NO_CYCLES) -> skip
//    d. Add to queue
// 3. When reaching target:
//    a. Check if min_length satisfied
//    b. Check if all required nodes visited
//    c. Check if all required edges used
//    d. Run custom predicates
//    e. Add to results if all pass
// 4. Sort results by cost
```

**Thread Safety:**
- PathConstraints instances are NOT thread-safe (create per-query)
- Read operations (validatePath, describeConstraints) are thread-safe on immutable constraints
- findConstrainedPaths uses const GraphIndexManager (thread-safe reads)

## Architecture

### Query Optimization Pipeline

```
Graph Query (AQL or Direct API)
      ↓
[Pattern Recognition] ─────> Query Pattern (SHORTEST_PATH, K_HOP, etc.)
      ↓
[Statistics Lookup] ───────> Graph Statistics (vertex/edge counts, branching)
      ↓
[Plan Generation] ─────────> Generate plans for each algorithm
      ↓                       (BFS, DFS, Dijkstra, A*, Bidirectional)
[Cost Estimation] ─────────> Estimate cost for each plan
      ↓
[Algorithm Selection] ─────> Select lowest-cost algorithm
      ↓
[Plan Optimization] ───────> Apply optimizations (caching, indexing, parallel)
      ↓
[Plan Caching] ────────────> Cache plan for reuse (optional)
      ↓
[Execution] ───────────────> Execute via GraphIndexManager
      ↓
[Statistics Recording] ────> Update execution history
      ↓
Return Results
```

### Constraint-Based Path Finding Flow

```
Path Query with Constraints
      ↓
[Constraint Parsing] ──────> Build constraint objects
      ↓
[Constraint Validation] ───> Check for contradictions
      ↓
[Query Optimization] ──────> Get recommended algorithm from optimizer
      ↓
[BFS Traversal] ───────────> Explore graph with constraint checks
      ↓                       - Validate during traversal (efficiency)
      │                       - Validate completed paths (correctness)
      ↓
[Path Collection] ─────────> Collect valid paths
      ↓
[Path Sorting] ────────────> Sort by cost
      ↓
[Result Filtering] ────────> Apply max_results limit
      ↓
Return Paths
```

### Integration Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     AQL Query Layer                      │
│  FOR v IN 1..3 OUTBOUND 'start' GRAPH 'social'          │
│    FILTER ... RETURN v                                   │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Graph Module                           │
│  ┌──────────────────┐      ┌──────────────────┐        │
│  │ GraphQueryOpt    │      │ PathConstraints   │        │
│  │ - Plan generation│◄─────│ - Constraint API  │        │
│  │ - Cost estimation│      │ - Path validation │        │
│  │ - Algorithm sel  │      │ - BFS traversal   │        │
│  └──────────────────┘      └──────────────────┘        │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│              Index Module (Graph Infrastructure)         │
│  ┌──────────────────┐  ┌──────────────────┐            │
│  │ GraphIndexMgr    │  │ GraphAnalytics    │            │
│  │ - BFS/DFS        │  │ - PageRank        │            │
│  │ - Dijkstra/A*    │  │ - Betweenness     │            │
│  │ - Adjacency      │  │ - Communities     │            │
│  └──────────────────┘  └──────────────────┘            │
│  ┌──────────────────┐  ┌──────────────────┐            │
│  │ PropertyGraph    │  │ TemporalGraph     │            │
│  │ - Labels/Types   │  │ - Temporal filter │            │
│  │ - Multi-graph    │  │ - Time-range      │            │
│  └──────────────────┘  └──────────────────┘            │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                    Storage Module                        │
│                    (RocksDB)                             │
└─────────────────────────────────────────────────────────┘
```

## Integration Points

### GraphIndexManager Integration

```cpp
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"

// GraphQueryOptimizer requires GraphIndexManager
GraphIndexManager graph_mgr(storage);
GraphQueryOptimizer optimizer(graph_mgr);

// Optimizer uses GraphIndexManager for:
// - Graph statistics (vertex/edge counts)
// - Traversal execution (BFS, DFS, Dijkstra, A*)
// - Adjacency queries (outNeighbors, inNeighbors)
// - Path result retrieval
```

### Query Module Integration

```cpp
#include "query/query_engine.h"
#include "graph/graph_query_optimizer.h"

// QueryEngine uses graph optimizer for graph queries
QueryEngine engine(storage, index_mgr);

// AQL graph traversal query
std::string query = R"(
  FOR v, e, p IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
    FILTER v.country == 'USA'
    RETURN p
)";

// Internal flow:
// 1. QueryEngine parses AQL
// 2. Recognizes graph traversal pattern
// 3. Calls GraphQueryOptimizer.optimizeKHopNeighborhood()
// 4. Gets optimized plan (e.g., BFS with depth 3)
// 5. Executes via GraphIndexManager
// 6. Applies FILTER predicate
// 7. Returns paths
```

### Index Module Integration

```cpp
// GraphQueryOptimizer integrates with multiple index components

// 1. GraphAnalytics (for centrality algorithms)
#include "index/graph_analytics.h"
GraphAnalytics analytics(graph_mgr);
auto pagerank = analytics.pageRank(node_pks);

// 2. PropertyGraphManager (for label/type filtering)
#include "index/property_graph.h"
PropertyGraphManager prop_graph(storage);
auto persons = prop_graph.getNodesByLabel("Person", "social");

// 3. TemporalGraphManager (for temporal queries)
#include "index/temporal_graph.h"
TemporalGraphManager temp_graph(storage);
auto edges = temp_graph.getEdgesAtTime(timestamp);
```

### Error Handling Integration

```cpp
#include "utils/expected.h"
#include "utils/error_registry.h"

// All graph methods use Result<T> pattern
Result<OptimizationPlan> plan = optimizer.optimizeShortestPath("A", "B");

if (!plan) {
    // Handle error
    std::cerr << "Optimization failed: " << plan.error().message << std::endl;

    // Error codes:
    // - ErrorCode::INVALID_STATE: GraphIndexManager not set
    // - ErrorCode::VALIDATION_FAILED: Constraints contradictory
    // - ErrorCode::NOT_FOUND: No paths found
    // - ErrorCode::INTERNAL_ERROR: Algorithm failure

    return;
}

// Use result
auto& optimal_plan = plan.value();
execute(optimal_plan);
```

## API Reference

### GraphQueryOptimizer API

```cpp
class GraphQueryOptimizer {
public:
    // Construction
    explicit GraphQueryOptimizer(GraphIndexManager& graph_manager);

    // Statistics
    Result<GraphStatistics> collectStatistics(
        std::optional<std::string_view> graph_id = std::nullopt
    );
    const GraphStatistics& getStatistics() const;
    double estimateEdgeTypeSelectivity(std::string_view edge_type) const;

    // Query Optimization
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

    // Plan Management
    std::string explainPlan(const OptimizationPlan& plan) const;
    void setPlanCachingEnabled(bool enabled);
    void clearPlanCache();
    const std::vector<ExecutionStats>& getExecutionHistory() const;
};
```

### PathConstraints API

```cpp
class PathConstraints {
public:
    // Construction
    PathConstraints() = default;
    explicit PathConstraints(GraphIndexManager* graph_mgr);
    void setGraphManager(GraphIndexManager* graph_mgr);

    // Length Constraints
    void addMinLength(int min_length);
    void addMaxLength(int max_length);

    // Node Constraints
    void addForbiddenNode(std::string_view node_id);
    void addRequiredNode(std::string_view node_id);

    // Edge Constraints
    void addForbiddenEdge(std::string_view edge_id);
    void addRequiredEdge(std::string_view edge_id);

    // Uniqueness Constraints
    void requireAcyclic();
    void requireUniqueNodes();
    void requireUniqueEdges();

    // Custom Validation
    void addCustomPredicate(
        std::function<bool(const std::vector<std::string>&)> predicate
    );

    // Path Operations
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

## HTTP API Reference

The graph module exposes its functionality through a set of HTTP REST endpoints
via `server::GraphApiHandler`.  All endpoints require authentication unless
configured otherwise.

| Method | Path | Description |
|--------|------|-------------|
| `POST`   | `/graph/traverse`                           | BFS traversal from a start vertex |
| `POST`   | `/graph/edge`                               | Create a directed graph edge |
| `DELETE` | `/graph/edge/:id`                           | Delete a graph edge by ID |
| `POST`   | `/api/v1/graph/query/explain`               | **Dry-run** — returns the selected algorithm and cost plan without executing (Issue: #1816) |
| `GET`    | `/api/v1/graph/metrics`                     | JSON snapshot of cumulative query metrics |
| `GET`    | `/api/v1/graph/metrics/prometheus`          | Prometheus text exposition of metrics |
| `POST`   | `/graph/query/incremental`                  | Register a BFS query for incremental re-execution |
| `DELETE` | `/graph/query/incremental/:handle`          | Unregister an incremental query |
| `POST`   | `/graph/changes`                            | Notify the optimizer of graph mutations |
| `POST`   | `/api/v1/graph/cost-model/calibrate`        | Recalibrate the cost model from execution history |
| `GET`    | `/api/v1/graph/cost-model`                  | Export the current learned cost model as JSON |
| `POST`   | `/api/v1/graph/cost-model`                  | Import a previously exported cost model |

### EXPLAIN Endpoint (`POST /api/v1/graph/query/explain`)

Performs a dry-run optimization and returns the selected algorithm, cost
estimates, and a human-readable explanation without executing the traversal.

**Supported `query_type` values:** `shortest_path`, `k_hop`, `reachability`,
`pattern_match`, `constrained_path`.

```json
// Request
{
  "query_type": "shortest_path",
  "start_vertex": "A",
  "end_vertex": "D",
  "constraints": {
    "max_depth": 10,
    "edge_type": "knows",
    "enable_parallel": false
  }
}

// Response
{
  "algorithm": "BFS",
  "pattern": "Shortest Path",
  "estimated_cost": 42.5,
  "estimated_time_ms": 8.3,
  "estimated_nodes_explored": 150,
  "use_index": true,
  "use_cache": false,
  "early_termination": true,
  "parallel_execution": false,
  "is_distributed": false,
  "recommended_parallelism": 1,
  "shard_ids": [],
  "alternatives": [
    {"algorithm": "DFS", "estimated_cost": 65.2}
  ],
  "explanation": "Query Pattern: Shortest Path\nSelected Algorithm: BFS\n..."
}
```

For `k_hop`, provide `"start_vertex"` and optionally `"max_depth"` (top-level
or inside `"constraints"`).  For `pattern_match`, provide
`"pattern_vertices"` (array of strings) and `"pattern_edges"` (array of
`[from, to]` pairs).  For `constrained_path`, provide `"path_constraints"`:

```json
{
  "query_type": "constrained_path",
  "start_vertex": "A",
  "end_vertex": "D",
  "path_constraints": {
    "min_length": 2,
    "max_length": 6,
    "forbidden_vertices": ["B"],
    "unique_nodes": true,
    "acyclic": true
  }
}
```

## AQL Integration

### Graph Traversal Queries

```aql
-- Basic k-hop traversal (uses GraphQueryOptimizer internally)
FOR v IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
  RETURN v

-- Shortest path query
FOR v, e, p IN OUTBOUND SHORTEST_PATH
  'users/alice' TO 'users/bob'
  GRAPH 'social'
  RETURN p

-- Pattern matching
FOR v1, e1, v2, e2, v3 IN PATTERN
  (v1)-[e1:FRIEND]->(v2)-[e2:COLLEAGUE]->(v3)
  GRAPH 'professional'
  FILTER v1.industry == 'Tech'
  RETURN {person1: v1, connection: e2, person2: v3}

-- Graph traversal with spatial constraints
FOR v, e, p IN 1..3 OUTBOUND 'places/seattle' GRAPH 'locations'
  FILTER ST_Distance(v.location, [47.6062, -122.3321]) < 5000
  RETURN p
```

### Constrained Path Queries (Future Enhancement)

```aql
-- Extended AQL syntax for path constraints (v1.7.0)
FOR p IN CONSTRAINED_PATHS
  FROM 'users/alice'
  TO 'users/bob'
  GRAPH 'social'
  WITH {
    min_length: 2,
    max_length: 5,
    forbidden_nodes: ['users/blocked'],
    required_nodes: ['users/checkpoint'],
    unique_nodes: true
  }
  LIMIT 10
  RETURN p
```

## Performance Characteristics

### Query Optimization Overhead

- **Statistics collection**: 10-100ms (cached after first collection)
- **Plan generation**: 0.1-5ms for simple queries
- **Complex queries** (pattern matching with constraints): 5-50ms
- **Plan cache lookup**: ~1μs (hit rate: 80-90% for repeated queries)

### Algorithm Performance

| Algorithm | Memory | Time (Best) | Time (Avg) | Time (Worst) |
|-----------|--------|-------------|------------|--------------|
| BFS | O(V) | O(1) | O(b^d) | O(V+E) |
| DFS | O(V) | O(1) | O(b^d) | O(V+E) |
| Dijkstra | O(V) | O(E log V) | O((V+E) log V) | O((V+E) log V) |
| A* | O(V) | O(E log V) | O(E log V) | O((V+E) log V) |
| Bidirectional | O(V) | O(b^(d/2)) | O(b^(d/2)) | O(V+E) |

### Constraint Validation Overhead

- **Single constraint**: ~0.1μs per constraint check
- **Path validation** (10 constraints): ~1μs per path
- **findConstrainedPaths** (1000 paths explored, 10 valid): 10-100ms

### Optimization Impact

- **Index usage**: 20% cost reduction
- **Adjacency cache**: 30% cost reduction
- **Early termination**: 50-90% reduction for reachability queries
- **Edge type filtering**: varies by selectivity (10-90% reduction)
- **Parallel execution**: 40-60% reduction for large graphs (10K+ nodes)

### Memory Usage

- **GraphQueryOptimizer instance**: ~1KB
- **GraphStatistics cache**: 10-100KB depending on graph size
- **Plan cache**: ~1KB per cached plan (default: 1000 plans max)
- **Execution history**: ~100B per execution (max: 1000 entries)
- **PathConstraints instance**: ~1KB + constraint data

### Tuning Recommendations

1. **Statistics Collection**:
   - Collect statistics once at startup or periodically
   - Update statistics after bulk graph changes
   - Cache statistics for query planning

2. **Plan Caching**:
   - Enable plan caching for production workloads
   - Clear plan cache after graph schema changes
   - Monitor cache hit rate (should be >70%)

3. **Algorithm Selection**:
   - Use BFS for k <= 5 in unweighted graphs
   - Use Dijkstra for weighted graphs
   - Use Bidirectional for k > 10
   - Use A* when spatial/domain heuristics available

4. **Constraint Optimization**:
   - Order constraints from most to least restrictive
   - Use forbidden_nodes for early pruning
   - Minimize custom predicates (they can't be optimized)
   - Prefer unique_nodes over custom cycle detection

## Dependencies

### Internal Dependencies

- **index/graph_index.h**: GraphIndexManager for graph operations
- **index/graph_analytics.h**: GraphAnalytics for centrality algorithms
- **index/property_graph.h**: PropertyGraphManager for label/type queries
- **index/temporal_graph.h**: TemporalGraphManager for temporal queries
- **storage/base_entity.h**: BaseEntity for graph data model
- **utils/expected.h**: Result<T> for error handling
- **utils/error_registry.h**: Error codes and messages

### External Dependencies

**Required:**
- None (graph module has no external dependencies)

**Optional:**
- None

### Build Configuration

```cmake
# CMakeLists.txt
add_library(themisdb_graph
  src/graph/graph_query_optimizer.cpp
  src/graph/path_constraints.cpp
)

target_link_libraries(themisdb_graph
  PUBLIC themisdb_index
  PUBLIC themisdb_storage
  PUBLIC themisdb_core
)

target_include_directories(themisdb_graph
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

## Known Limitations

1. **Algorithm Limitations**:
   - A* requires user-provided heuristic function
   - Bidirectional search assumes symmetric graph
   - No parallel execution within single query yet (planned v1.7.0)

2. **Constraint Limitations**:
   - NODE_PROPERTY and EDGE_PROPERTY constraints partially supported
   - Custom predicates evaluated after path completion (not during traversal)
   - Required edges/nodes constraint validation is post-hoc (not pruned early)

3. **Cost Model Limitations**:
   - Assumes uniform data distribution (inaccurate for skewed graphs)
   - Branching factor is global average (not per-node)
   - Edge type selectivity not updated adaptively (manual refresh needed)

4. **Plan Caching Limitations**:
   - Cache key includes all constraint details (low hit rate for varying constraints)
   - No cache invalidation on graph changes (manual clear required)
   - Plan cache shared across all graphs (no per-graph isolation)

5. **Statistics Limitations**:
   - Statistics collection scans full graph (expensive for large graphs)
   - No incremental statistics updates
   - Statistics not persisted across restarts

6. **Execution Limitations**:
   - No timeout support for long-running traversals
   - No partial result returns (all-or-nothing)
   - Execution history limited to 1000 entries (FIFO eviction)

7. **Integration Limitations**:
   - PathConstraints requires GraphIndexManager (not standalone)
   - No integration with distributed graph queries yet
   - Limited interop with vector/spatial hybrid queries

8. **Thread Safety**:
   - Optimizer instances are NOT thread-safe
   - PathConstraints instances are NOT thread-safe
   - Statistics collection requires exclusive access

## Status

**Production Ready** (as of v1.8.0)

✅ **Stable Features:**
- Graph query optimization with cost-based algorithm selection
- Path constraints with 14+ constraint types (including edge/node property, weight)
- BFS/DFS/Dijkstra/A*/Bidirectional algorithm execution
- Query plan generation, caching (LRU + TTL eviction), and structural reuse
- Execution statistics tracking and adaptive cost model (EMA-based)
- GraphIndexManager integration
- Parallel multi-source BFS/DFS (fan-out threshold, configurable thread count)
- Incremental graph query execution on live mutations (BFS)
- Distributed graph query execution across shards
- GPU-accelerated BFS/DFS with automatic CPU fallback
- Subgraph isomorphism (VF2-style pattern matching)
- HTTP REST API with EXPLAIN endpoint (`POST /api/v1/graph/query/explain`)
- Prometheus metrics export

⚠️ **Beta Features:**
- Custom predicate constraints (v1.5.0+)
- Multi-graph query optimization (v1.5.0+)

🔬 **Planned:**
- Cypher query language support (community request)
- Cross-shard edge following (caller-coordinated today)

## Related Documentation

- [Index Module - Graph Components](../../include/index/README.md) - GraphIndexManager, GraphAnalytics, PropertyGraph, TemporalGraph
- [Query Module](../query/README.md) - AQL query engine and graph query integration
- [Storage Module](../storage/README.md) - RocksDB storage for graph data
- [Graph Advanced Features](ADVANCED_FEATURES_README.md) - PathConstraints detailed documentation
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - Overall ThemisDB architecture

*Last Updated: April 2026*
*Module Version: v1.8.0*
*Next Review: v1.9.0 Release*

## Scientific References

1. Angles, R., & Gutierrez, C. (2008). **Survey of Graph Database Models**. *ACM Computing Surveys*, 40(1), 1:1–1:39. https://doi.org/10.1145/1322432.1322433

2. Rodriguez, M. A., & Neubauer, P. (2010). **The Graph Traversal Pattern**. *Graph Data Management: Techniques and Applications*. https://arxiv.org/abs/1004.1001

3. Robinson, I., Webber, J., & Eifrem, E. (2015). **Graph Databases: New Opportunities for Connected Data (2nd ed.)**. O'Reilly Media. ISBN: 978-1-491-93089-2

4. Dijkstra, E. W. (1959). **A Note on Two Problems in Connexion with Graphs**. *Numerische Mathematik*, 1, 269–271. https://doi.org/10.1007/BF01386390

5. Nguyen, D., Lenharth, A., & Pingali, K. (2013). **A Lightweight Infrastructure for Graph Analytics**. *Proceedings of the 24th ACM Symposium on Operating Systems Principles (SOSP)*, 456–471. https://doi.org/10.1145/2517349.2522739

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
