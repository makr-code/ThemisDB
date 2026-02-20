# Graph Module - Future Enhancements

## Planned Features

### Parallel Graph Execution ✅ DONE
**Priority:** High  
**Target Version:** v1.7.0

Enable parallel execution of graph traversals for improved performance on large graphs.

**Implemented Features:**
- ✅ Multi-threaded BFS (level-parallel frontier expansion via `std::async`)
- ✅ Parallel Δ-Stepping Dijkstra (bucket-based parallelism, no global locks)
- ✅ Configurable thread pool size (`num_threads`, 0 = auto-detect)
- ✅ Thread-safe adjacency access via `GraphIndexManager::outAdjacency`

**Planned (not yet implemented):**
- Work-stealing queue for load balancing
- Lock-free visited sets for BFS

---

### Adaptive Cost Model ✅ DONE
**Priority:** High  
**Target Version:** v1.7.0

Automatically improve cost estimates based on actual execution statistics.

**Implemented:** See `GraphQueryOptimizer::AlgorithmCostModel` below.

**Features:**
- ✅ Learning from execution history (EMA per algorithm)
- ✅ Per-algorithm cost model with confidence level
- ✅ Automatic model re-calibration via `recordExecution`
- ✅ `exportCostModel()` / `importCostModel()` for persistence
- ✅ Disabled when `enableAdaptiveLearning(false)` is called
- Persistence to disk (use `exportCostModel()` + file I/O)
- Decay of old statistics (future enhancement)
- Separate models per graph type/size (future)

**API:**
```cpp
GraphQueryOptimizer optimizer(graph_mgr);
// Adaptive learning is enabled by default

// After many executions, cost estimates improve automatically
optimizer.executeBFS("A", 5, c);   // observed ~8ms → EMA updates
optimizer.executeBFS("A", 5, c);   // EMA converges towards actual timing

// Export learned model to persist across restarts
std::string model_json = optimizer.exportCostModel();
// e.g. {"BFS":{"ema_cost_ms":8.1,"exec_count":2,"confidence":0.02}}

// Import model in another instance (seeds with pre-learned data)
GraphQueryOptimizer optimizer2(graph_mgr2);
optimizer2.importCostModel(model_json);

// Disable adaptive learning if deterministic plans are required
optimizer.enableAdaptiveLearning(false);
bool is_on = optimizer.isAdaptiveLearningEnabled(); // false
```

**Learning Algorithm:**
```
ema_cost_ms = alpha * observed_ms + (1 - alpha) * ema_cost_ms   (alpha = 0.1)
confidence  = min(1.0, exec_count / 100)
blended_cost = (1 - confidence) * base_cost + confidence * (ema_cost_ms * 10)
```

**Implementation:**
- `AlgorithmCostModel` struct: EMA cost, execution count, confidence
- `recordExecution()` calls `AlgorithmCostModel::update(ms)` for the executed algorithm
- `estimateCost()` blends the learned EMA cost (scaled from ms to cost units) proportional to confidence
- `exportCostModel()` serialises all entries to a JSON string
- `importCostModel()` deserialises with unknown-algorithm and malformed-JSON safety

---

### Distributed Graph Queries
**Priority:** Medium  
**Target Version:** v1.8.0

Enable graph queries across distributed ThemisDB instances.

**Features:**
- Partition-aware graph traversal
- Cross-partition edge following
- Distributed shortest path algorithms
- Result aggregation across nodes
- Fault-tolerant execution

**Benefits:**
- Scale to billion-edge graphs
- Handle graphs larger than single-node memory
- Geographic distribution for latency optimization
- High availability and fault tolerance

**API:**
```cpp
// Define graph partitioning strategy
DistributedGraphConfig config;
config.partitioning = PartitionStrategy::HASH;  // or RANGE, GEO
config.replication_factor = 3;
config.consistency = ConsistencyLevel::EVENTUAL;

DistributedGraphManager dist_graph(cluster, config);

// Execute distributed traversal
auto result = dist_graph.shortestPath(
    "node_A@shard1",
    "node_B@shard5",
    constraints
);

// Query spans multiple shards transparently
```

**Partitioning Strategies:**
- **Hash Partitioning**: Uniform distribution by node ID hash
- **Range Partitioning**: Partition by node ID ranges
- **Geographic Partitioning**: Partition by geographic region
- **Community-Based**: Partition by detected communities
- **Hybrid**: Combine multiple strategies

**Distributed Algorithms:**
- **Distributed BFS**: Level-synchronous parallel BFS
- **Distributed Dijkstra**: Delta-stepping algorithm
- **Distributed PageRank**: Bulk synchronous parallel
- **Cross-Shard Joins**: Hash join with data shuffling

**Challenges:**
- Cross-partition communication overhead
- Partition skew and load balancing
- Consistency guarantees
- Fault tolerance and recovery

---

### GPU-Accelerated Graph Processing
**Priority:** Medium  
**Target Version:** v1.9.0

Offload graph computations to GPU for massive parallelism.

**Features:**
- CUDA/OpenCL graph kernels
- GPU-accelerated BFS/DFS
- GPU PageRank and centrality
- Hybrid CPU-GPU execution
- Automatic GPU memory management

**Benefits:**
- 10-100x speedup for large dense graphs
- Handle graphs with millions of edges
- Real-time analytics on massive graphs
- Reduced cloud compute costs

**API:**
```cpp
GraphQueryOptimizer::QueryConstraints constraints;
constraints.use_gpu = true;
constraints.gpu_device = 0;  // GPU index

auto result = optimizer.executeBFS("start", 10, constraints);
// Automatically uses GPU if graph size > threshold
```

**Suitable Workloads:**
- Dense graphs (high edge-to-node ratio)
- Large traversal depth (k > 10)
- Analytics algorithms (PageRank, betweenness)
- Pattern matching with many patterns

**Implementation:**
- Use NVIDIA cuGraph library
- Implement custom CUDA kernels for ThemisDB-specific operations
- Transfer graph data to GPU memory
- Execute kernels with optimal block/grid sizes
- Transfer results back to CPU

**Challenges:**
- GPU memory limitations
- PCIe transfer overhead
- Algorithm suitability for GPU
- Complexity of CUDA programming

---

### Advanced Constraint Types (Partially Implemented)
**Priority:** Medium  
**Target Version:** v1.7.0

Extend PathConstraints with more sophisticated constraint types.

**Features:**
- **Node Property Constraints** ✅ DONE – `addNodePropertyConstraint(key, value)` prunes BFS traversal
- **Weight Constraints** ✅ DONE – `addMaxWeight(threshold)` prunes BFS; `addMinWeight(threshold)` rejects at acceptance
- **Temporal Constraints**: Path valid at specific time ⏳ Planned
- **Probability Constraints**: Min probability for uncertain graphs ⏳ Planned
- **Resource Constraints**: Capacity limits on paths ⏳ Planned
- **Semantic Constraints**: Ontology-based path rules ⏳ Planned
- **Geo-Fence Constraints**: Spatial boundaries for paths ⏳ Planned

**Implemented API:**
```cpp
PathConstraints constraints(&graph_mgr);

// Node property constraint (v1.7.0)
constraints.addNodePropertyConstraint("country", "USA");
// → Only traverse nodes where node.country == "USA"

// Weight constraints (v1.7.0)
constraints.addMaxWeight(100.0);  // Total path weight <= 100 (BFS pruning)
constraints.addMinWeight(10.0);   // Total path weight >= 10 (acceptance check)

auto paths = constraints.findConstrainedPaths("start", "end", 10);
```

**Planned API (not yet implemented):**
```cpp
// Temporal constraint
constraints.addTemporalConstraint(
    start_time_ms,
    end_time_ms,
    TemporalMode::VALID_DURING
);

// Resource constraint
constraints.addResourceCapacity("bandwidth", 1000);

// Geo-fence constraint
constraints.addGeoFence(
    center_lat, center_lon, radius_km,
    GeoFenceMode::MUST_STAY_INSIDE
);
```

**Implementation Notes:**
- `getNodeField(vertexId, fieldName)` added to `GraphIndexManager` (uses `node:<pk>` key format)
- `ConstraintType::MAX_WEIGHT` / `MIN_WEIGHT` added to `PathConstraints::ConstraintType` enum
- `Constraint::double_value` field stores threshold for weight constraints
- BFS pruner checks `MAX_WEIGHT` after each edge weight accumulation
- `validatePath` enforces `NODE_PROPERTY` for all nodes; weight constraints handled by `findConstrainedPaths`

---

### Query Rewriting for Graph Optimization
**Priority:** Medium  
**Target Version:** v1.8.0

Automatically rewrite graph queries for better performance.

**Features:**
- Common subexpression elimination
- Predicate pushdown to graph layer
- Join reordering for graph patterns
- Materialized view utilization
- Query decomposition for parallelism

**Benefits:**
- Improved query performance without user intervention
- Better integration with relational/document queries
- Optimal execution plans for complex queries
- Reduced redundant computation

**Example Rewrite:**
```aql
-- Original query
FOR v1 IN vertices
  FILTER v1.type == "Person"
  FOR v2 IN 1..3 OUTBOUND v1 GRAPH "social"
    FILTER v2.country == "USA"
    RETURN {person: v1, friend: v2}

-- Rewritten query
FOR v1 IN vertices
  FILTER v1.type == "Person"
  FOR v2 IN 1..3 OUTBOUND v1 GRAPH "social"
    PRUNE v2.country != "USA"  // Early pruning
    FILTER v2.country == "USA"
    RETURN {person: v1, friend: v2}
```

**Rewrite Rules:**
- Push predicates into graph traversal (prune early)
- Decompose multi-pattern queries into independent subqueries
- Materialize frequently accessed subgraphs
- Convert repeated traversals to single traversal with caching
- Reorder multi-hop traversals based on selectivity

---

### Approximate Graph Algorithms
**Priority:** Low  
**Target Version:** v2.0.0

Trade accuracy for speed with approximate algorithms.

**Features:**
- Approximate shortest paths (A* with relaxed heuristic)
- Approximate PageRank (power iteration with early stop)
- Approximate reachability (sketching techniques)
- Approximate community detection (sampling-based)
- Confidence bounds on approximations

**Benefits:**
- Sub-second response on billion-edge graphs
- Handle interactive queries on massive graphs
- Reduce cloud compute costs
- Enable exploratory graph analytics

**API:**
```cpp
GraphQueryOptimizer::QueryConstraints constraints;
constraints.approximation_mode = ApproximationMode::FAST;
constraints.approximation_error = 0.05;  // 5% error tolerance

auto result = optimizer.optimizeShortestPath("A", "B", constraints);
// May return path within 5% of optimal length

// Access approximation metadata
std::cout << "Approximation error bound: " 
          << result.metadata.error_bound << std::endl;
std::cout << "Confidence: " 
          << result.metadata.confidence << std::endl;
```

**Algorithms:**
- **Bidirectional A* with beam search**: Prune low-probability paths
- **Landmark-based shortest paths**: Precompute distances to landmarks
- **Sketching for reachability**: Probabilistic data structures
- **Sampling for PageRank**: Monte Carlo estimation
- **Local clustering**: Expand only relevant subgraph

---

### Multi-Layer Graph Support
**Priority:** Low  
**Target Version:** v2.0.0

Support graphs with multiple edge types and layers.

**Features:**
- Layer-specific traversals
- Cross-layer path finding
- Layer aggregation queries
- Layer-aware analytics
- Heterogeneous graph queries

**Benefits:**
- Model complex multi-relational data
- Support social networks with multiple edge types
- Enable knowledge graph reasoning
- Better domain modeling

**Example:**
```cpp
// Define multi-layer graph
MultiLayerGraph mlg;
mlg.addLayer("friendship", EdgeType::UNDIRECTED);
mlg.addLayer("follows", EdgeType::DIRECTED);
mlg.addLayer("colleague", EdgeType::UNDIRECTED);

// Query across layers
auto result = mlg.shortestPath(
    "user_A", "user_B",
    layers = {"friendship", "colleague"},  // Use these layers
    layer_weights = {1.0, 2.0}             // Friendship preferred
);

// Aggregate across layers
auto centrality = mlg.pageRank(
    layers = {"friendship", "follows"},
    aggregation = AggregationMode::SUM  // or AVG, MAX
);
```

---

### Graph Machine Learning Integration
**Priority:** Low  
**Target Version:** v2.0.0

Integrate graph neural networks and embeddings.

**Features:**
- Graph embedding generation (Node2Vec, DeepWalk)
- Graph neural network inference
- Link prediction
- Node classification
- Graph similarity search

**Benefits:**
- Enable AI/ML on graph data
- Predict missing edges
- Classify unlabeled nodes
- Find similar subgraphs
- Integrate with LLM module

**API:**
```cpp
// Train graph embeddings
GraphEmbedding embedding(graph_mgr);
embedding.train(
    algorithm = EmbeddingAlgorithm::NODE2VEC,
    dimensions = 128,
    walk_length = 80,
    num_walks = 10
);

// Get node embeddings
auto vec = embedding.getNodeEmbedding("user_A");

// Link prediction
auto predictions = embedding.predictLinks("user_A", k=10);
// Returns top-k most likely edges

// Node classification
auto label = embedding.classifyNode("user_B", model);
```

---

### Graph Visualization Integration
**Priority:** Low  
**Target Version:** v2.0.0

Built-in graph visualization and exploration.

**Features:**
- Graph layout algorithms (force-directed, hierarchical)
- Interactive exploration UI
- Subgraph extraction for visualization
- Real-time updates on graph changes
- Export to common formats (GraphML, GEXF)

**Benefits:**
- Explore graphs visually
- Debug graph queries interactively
- Present graph analytics results
- Integrate with BI tools

**API:**
```cpp
// Extract subgraph for visualization
GraphVisualizer viz(graph_mgr);
auto subgraph = viz.extractSubgraph(
    center_node = "user_A",
    max_depth = 2,
    max_nodes = 100,
    layout = LayoutAlgorithm::FORCE_DIRECTED
);

// Export to GraphML
viz.exportGraphML(subgraph, "output.graphml");

// Generate interactive HTML
viz.exportInteractiveHTML(subgraph, "output.html");
```

---

## Research Topics

### Quantum Graph Algorithms
**Priority:** Research  
**Target Version:** TBD

Explore quantum algorithms for graph problems.

**Potential Applications:**
- Quantum walk for graph search
- Grover's algorithm for pattern matching
- Quantum annealing for optimization problems
- Exponential speedup for specific problems

**Challenges:**
- Quantum hardware availability
- Algorithm design complexity
- Limited problem applicability
- Noise and error correction

---

### Graph Streaming Algorithms
**Priority:** Research  
**Target Version:** TBD

Process graphs as streams of edge insertions/deletions.

**Potential Applications:**
- Real-time social network analysis
- Continuous PageRank updates
- Incremental community detection
- Dynamic shortest paths

**Challenges:**
- Maintaining accuracy with limited memory
- Handling high-velocity streams
- Dealing with concept drift
- Balancing latency and accuracy

---

## Implementation Priorities

**v1.7.0 (Q3 2026):**
1. ✅ Parallel Graph Execution (BFS + Dijkstra Δ-Stepping)
2. ✅ Adaptive Cost Model
3. ✅ Advanced Constraint Types

**v1.8.0 (Q1 2027):**
1. Distributed Graph Queries
2. Query Rewriting

**v1.9.0 (Q3 2027):**
1. GPU-Accelerated Graph Processing
2. Approximate Algorithms

**v2.0.0 (Q1 2028):**
1. Multi-Layer Graph Support
2. Graph ML Integration
3. Graph Visualization

## Implemented Features (v1.7.0)

### Query Timeout / SLO Enforcement ✅ DONE

`QueryConstraints::timeout_ms` – when set to a non-zero value BFS and DFS
traversals abort after the given number of milliseconds and return
`ERR_QUERY_TIMEOUT`. This provides a first line of defence for SLO budgets.

```cpp
GraphQueryOptimizer::QueryConstraints constraints;
constraints.timeout_ms = 500; // abort after 500 ms
auto result = optimizer.executeBFS("start", 5, constraints);
if (!result) {
    // result.error().code == ERR_QUERY_TIMEOUT
}
```

### Aggregate Observability Metrics ✅ DONE

`GraphQueryOptimizer::getQueryMetrics()` returns a `GraphQueryMetrics` snapshot
with cumulative counters that can be scraped by a Prometheus exporter or
forwarded to an OpenTelemetry collector:

| Metric                              | Description                              |
|-------------------------------------|------------------------------------------|
| `total_queries`                     | Total traversal executions since startup |
| `failed_queries`                    | Executions that returned no paths        |
| `timed_out_queries`                 | Executions aborted by `timeout_ms`       |
| `total_execution_time_ms`           | Sum of all execution durations (ms)      |
| `max_execution_time_ms`             | Peak single-query duration (ms)          |
| `total_nodes_explored`              | Cumulative nodes visited                 |
| `total_edges_traversed`             | Cumulative edges traversed               |
| `plan_cache_hits` / `misses`        | Plan-cache efficiency counters           |

See `docs/graph_roadmap.md` for the full observability checklist.

### Graph-Specific Structured Error Codes ✅ DONE

Six dedicated error codes added to `errors::ErrorCode` in range **6400-6499**,
each registered with full metadata (category, severity, solution, keywords):

| Code | Constant                        | Meaning                                         |
|------|---------------------------------|-------------------------------------------------|
| 6400 | `ERR_GRAPH_NO_SUCH_VERTEX`      | Vertex not found in graph                       |
| 6401 | `ERR_GRAPH_NO_SUCH_EDGE`        | Edge not found in graph                         |
| 6402 | `ERR_GRAPH_CONSTRAINT_CONFLICT` | Contradictory path constraints                  |
| 6403 | `ERR_GRAPH_PATH_NOT_FOUND`      | No path satisfies constraints                   |
| 6404 | `ERR_GRAPH_CYCLE_DETECTED`      | Cycle in acyclic-required traversal             |
| 6405 | `ERR_GRAPH_DEPTH_EXCEEDED`      | Traversal depth limit exceeded                  |

`executeBFS`/`executeDFS` now return `ERR_GRAPH_NO_SUCH_VERTEX` instead of the
generic `ERR_QUERY_EXECUTION_FAILED` for unknown vertex lookups.

### Explain / Dry-Run Query API ✅ DONE

`GraphQueryOptimizer::explainConstrainedPath()` – returns an `OptimizationPlan`
without executing any traversal, enabling callers to inspect the chosen algorithm,
cost estimate, and constraint summary before committing to actual graph traversal.
Does **not** increment `getQueryMetrics().total_queries`.

```cpp
themis::graph::PathConstraints constraints(&graph_mgr);
constraints.addRequiredNode("checkpoint");
constraints.addMaxLength(6);

// Inspect the plan without touching the graph
auto plan = optimizer.explainConstrainedPath("start", "end", constraints);
std::cout << optimizer.explainPlan(plan.value()); // algorithm, cost, constraints
```

### Parallel BFS (`enable_parallel` + `num_threads`) ✅ DONE

`executeBFS` now supports level-parallel frontier expansion. The BFS is
rewritten as a level-by-level loop; each level's neighbor lookups are
dispatched as independent `std::async` tasks when `enable_parallel=true`.

```cpp
GraphQueryOptimizer::QueryConstraints c;
c.enable_parallel = true;  // opt-in; default is false (backward-compatible)
c.num_threads = 4;         // 0 = hardware_concurrency/2, max 16
auto result = optimizer.executeBFS("start", 5, c);
```

Produces the same set of reachable nodes as the sequential path (no correctness
regression). The `num_threads` field defaults to `0` (auto-detect).

### Parallel Dijkstra (Δ-Stepping) ✅ DONE

`GraphQueryOptimizer::executeDijkstra` now uses the Δ-Stepping algorithm when
`constraints.enable_parallel = true`, giving bucket-based parallelism without
global locks.

**Algorithm:**
1. Δ is sampled from the start vertex's first-hop average edge weight (default 1.0).
2. Vertices are partitioned into buckets of width Δ.
3. Within each bucket, light-edge (weight ≤ Δ) relaxations are dispatched as
   `std::async` tasks (one per thread chunk); each task returns a local
   `vector<RelaxResult>` with no shared writes.
4. The main thread applies updates serially – no data races on `dist[]` / `parent[]`.
5. Heavy edges (weight > Δ) are relaxed serially after the bucket is stable.

```cpp
GraphQueryOptimizer::QueryConstraints c;
c.enable_parallel = true;   // opt-in; default is false (backward-compatible)
c.num_threads = 4;          // 0 = hardware_concurrency/2, max 16
auto result = optimizer.executeDijkstra("A", "D", c);
// result->totalCost == optimal weighted shortest-path cost
// result->path      == reconstructed path [A, ..., D]
```

Produces the same `totalCost` as sequential Dijkstra (verified by
`Dijkstra_Parallel_ProducesSameResultAsSequential`).

### Edge Property Constraints ✅ DONE

`PathConstraints::addEdgePropertyConstraint(field_name, expected_value)` –
prunes edges during `findConstrainedPaths` BFS traversal by checking each
candidate edge's field value against the required value.

```cpp
PathConstraints c(&graph_mgr);
c.addEdgePropertyConstraint("type", "follows"); // only traverse "follows" edges
auto paths = c.findConstrainedPaths("user1", "user5", 10);
```

`validatePath` also enforces `EDGE_PROPERTY` on complete paths.
`describeConstraints()` lists each edge property constraint as:
`"Edge property: <key> = <value>"`.

New backing API: `GraphIndexManager::getEdgeField(edgeId, fieldName)` returns
an `std::optional<std::string>` without needing the graph ID.

### Adaptive Cost Model ✅ DONE

`GraphQueryOptimizer::AlgorithmCostModel` – per-algorithm EMA cost tracking with
confidence-weighted blending into `estimateCost()`.

```cpp
// Enabled by default; runs automatically with each execute* call
optimizer.executeBFS("start", 5, c);   // records 8.1ms → EMA updates

// Export / import for warm-start across restarts
std::string json = optimizer.exportCostModel();
optimizer2.importCostModel(json);

// Opt out for deterministic plans
optimizer.enableAdaptiveLearning(false);
```

Key properties:
- EMA alpha = 0.1 (smoothes out outliers)
- Confidence = `min(1.0, exec_count / 100)` (0 → purely theoretical, 1 → fully learned)
- `estimateCost()` blends: `(1 - conf) * base + conf * (ema_ms * 10)`
- `ExecutionStats::algorithm` field enables `recordExecution` to route to the correct model
- `exportCostModel()` / `importCostModel()` use JSON; unknown algo keys are silently ignored

### Node Property Constraints ✅ DONE

`PathConstraints::addNodePropertyConstraint(field, value)` – prunes BFS traversal
and validates complete paths by looking up each node's field in the graph store.

```cpp
PathConstraints c(&graph_mgr);
c.addNodePropertyConstraint("country", "USA");
// BFS skips any next_node whose country field ≠ "USA"
auto paths = c.findConstrainedPaths("user1", "user5", 10);
```

Backed by new `GraphIndexManager::getNodeField(vertexId, fieldName)` which reads
from `node:<pk>` key format (same as `KeySchema::makeGraphNodeKey`).

### Weight Constraints ✅ DONE

`PathConstraints::addMaxWeight(threshold)` and `addMinWeight(threshold)` implement
total-path-weight constraints backed by `ConstraintType::MAX_WEIGHT` / `MIN_WEIGHT`
and a new `Constraint::double_value` field.

```cpp
PathConstraints c(&graph_mgr);
c.addMaxWeight(10.0);   // BFS prunes states where accumulated cost > 10.0
c.addMinWeight(2.0);    // Final acceptance rejects paths with cost < 2.0
auto paths = c.findConstrainedPaths("A", "D", 5);
```

Edge weights are read from each edge's `_weight` field (default 1.0 when absent).

---

## Community Requests

Track user-requested features:
- **Cypher Query Support**: Neo4j-compatible query language (requested by 15 users)
- **Graph Backup/Restore**: Snapshot and restore graph state (requested by 12 users)
- **Graph Diff**: Compare two graph versions (requested by 8 users)
- **Graph Validation**: Schema validation for property graphs (requested by 10 users)

---

*Last Updated: February 2026*  
*Next Review: Q3 2026*
