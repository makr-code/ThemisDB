> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Graph Module - Future Enhancements

- Graph query optimization: cost-based algorithm selection (BFS, DFS, Dijkstra, A*, Bidirectional)
- Adaptive plan caching with EMA-based cost model learning and TTL/size-based eviction
- Parallel multi-source traversal for large fan-out queries (`fan_out_threshold`)
- Subgraph isomorphism via VF2-style backtracking pattern matching
- Distributed graph query execution across shards with intra-shard parallelism
- Incremental graph query execution on live edge/node mutations (BFS-only)
- GPU-accelerated BFS/DFS for massive graphs (≥1M nodes, CUDA/HIP backends)
- EXPLAIN output integration with AQL for graph query plan inspection

## Design Constraints

- [ ] Algorithm selection must complete in < 1 ms for graphs with up to 10M nodes
- [ ] Plan cache must enforce configurable max size (default 1,000 plans) and TTL (default 300 s)
- [ ] Adaptive cost model must converge to < 10% mean absolute error after 100 executions per algorithm
- [ ] Parallel traversal must not exceed configured `fan_out_threshold` per frontier expansion
- [ ] Incremental query handles must be explicitly released; no implicit memory growth
- [ ] GPU traversal kernel must produce bit-identical results to CPU baseline (deterministic)
- [ ] Distributed query execution is intra-shard parallel; cross-shard edge following is caller-coordinated
- [ ] All public APIs are thread-safe via read-write locking; incremental execution is explicitly single-threaded

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `GraphQueryOptimizer::optimize(query)` | AQL execution engine | Returns a `QueryPlan` with cost estimate and selected algorithm |
| `PlanCache::get/put/evict` | Query optimizer | Keyed by structural query fingerprint; bounded by `max_plans` and TTL |
| `ParallelTraversal::execute(sources, query)` | Query optimizer | Spawns intra-frontier worker threads up to `max_parallel_workers` |
| `SubgraphIsomorphism::executeVF2(pattern, target)` | AQL graph pattern match | Returns all matching subgraph mappings |
| `DistributedGraphQuery::executeAcrossShards(plan)` | Distributed query coordinator | Returns globally cheapest path; shard results merged by caller |
| `IncrementalQueryHandle::applyMutation(edge)` | Graph API handler (`POST /graph/edge`) | BFS-only; notifies registered listeners |
| `GPUTraversal::executeBFS(graph, source)` | Query optimizer (CUDA path) | Requires `THEMIS_ENABLE_CUDA`; CPU fallback active otherwise |
| `GraphQueryOptimizer::explain(plan)` | AQL EXPLAIN | Returns human-readable plan tree with cost breakdown |

## Planned Features

### Structural Plan Reuse (Query Plan Reuse Across Structurally Similar Queries) ✅ DONE
**Priority:** High
**Target Version:** v1.7.0

Reuse optimization plans across graph queries that share the same query pattern and
constraints but have different start/target vertex IDs, eliminating redundant
re-planning work.

**Implemented Features:**
- ✅ `generateStructuralCacheKey` — builds a cache key from pattern + constraints,
  omitting vertex IDs, so all structurally identical queries map to the same entry
- ✅ Two-level cache lookup in all four `optimize*` methods:
  1. Exact key (`pattern:start:target[:depth][:type]…`) — fastest path for repeated identical queries
  2. Structural key (`struct:pattern[:depth][:type][:uv][:ue][:par]…`) — fallback for same-shape queries
- ✅ Structural-to-exact key promotion on hit (avoids repeated structural lookups)
- ✅ Structural key covers: `max_depth`/depth-hint, `edge_type`, `unique_vertices`,
  `unique_edges`, `enable_parallel`, count of `forbidden_vertices`, count of `required_vertices`
- ✅ Cache hit/miss counters exposed via `getQueryMetrics().plan_cache_hits/misses`
- ✅ Disabled cleanly when `setPlanCachingEnabled(false)` is called
- ✅ Cleared by `clearPlanCache()` (removes both exact and structural entries)

**API:**
```cpp
optimizer.setPlanCachingEnabled(true);  // default

// Cold start: populates exact key "0:A:D" + structural key "struct:0"
auto plan1 = optimizer.optimizeShortestPath("A", "D", constraints);

// Structural hit: finds "struct:0", promotes plan to exact key "0:B:C"
auto plan2 = optimizer.optimizeShortestPath("B", "C", constraints);

// plan1 and plan2 are identical (same algorithm, cost, estimates)
assert(plan1->algorithm           == plan2->algorithm);
assert(plan1->estimated_cost      == plan2->estimated_cost);
assert(plan1->estimated_nodes_explored == plan2->estimated_nodes_explored);

// Queries with different constraints get different structural keys — no reuse
QueryConstraints c1; c1.max_depth = 2;
QueryConstraints c2; c2.max_depth = 5;
optimizer.optimizeShortestPath("A", "D", c1);  // stores "struct:0:depth=2"
optimizer.optimizeShortestPath("B", "C", c2);  // miss — "struct:0:depth=5" not present

// Monitor cache efficiency
const auto& m = optimizer.getQueryMetrics();
std::cout << "Cache hits:   " << m.plan_cache_hits.load()   << "\n";
std::cout << "Cache misses: " << m.plan_cache_misses.load() << "\n";
```

**Key Design Decisions:**
- Structural key intentionally excludes `timeout_ms`, `max_results`, `num_threads`,
  and `graph_id` because these fields affect only execution behaviour, not which
  algorithm is selected or how costs are estimated.
- For K_HOP queries, the hop count `k` is included as a depth hint in the structural
  key, so `k=2` and `k=3` never share a plan.
- For PATTERN_MATCH queries, both vertex count and edge count are encoded in the key
  to distinguish patterns of different shapes.

---

### Parallel Graph Execution ✅ DONE
**Priority:** High
**Target Version:** v1.7.0

Enable parallel execution of graph traversals for improved performance on large graphs.

**Implemented Features:**
- ✅ Multi-threaded BFS (level-parallel frontier expansion via `std::async`)
- ✅ Parallel Δ-Stepping Dijkstra (bucket-based parallelism, no global locks)
- ✅ Configurable thread pool size (`num_threads`, 0 = auto-detect)
- ✅ Thread-safe adjacency access via `GraphIndexManager::outAdjacency`
- ✅ Intra-frontier fan-out parallelism for BFS (`fan_out_threshold`: when frontier ≥ threshold, neighbor lookups are dispatched to multiple threads; 0 = disabled)

**Planned (not yet implemented):**
- Work-stealing queue for load balancing
- Lock-free visited sets for BFS

---

### Adaptive Cost Model ✅ DONE
**Priority:** High
**Target Version:** v1.7.0 / v1.8.0

Automatically improve cost estimates based on actual execution statistics.

**Implemented:** See `GraphQueryOptimizer::AlgorithmCostModel` below.

**Features:**
- ✅ Learning from execution history (EMA per algorithm)
- ✅ Per-algorithm cost model with confidence level
- ✅ Automatic model re-calibration via `recordExecution`
- ✅ Adaptive plan selection: `selectAlgorithm` compares estimated costs for all feasible algorithms using learned EMA data when confidence > 0
- ✅ `exportCostModel()` / `importCostModel()` for persistence
- ✅ Disabled when `enableAdaptiveLearning(false)` is called
- ✅ Batch calibration from full execution history via `calibrateFromHistory()` (Issue: #2386)
- ✅ Cost accuracy tracking: `ExecutionStats::estimated_cost_ms` populated by all execute* methods; `AlgorithmCalibrationStats` reports `mean_estimated_ms`, `mean_absolute_error_ms`, and `cost_ratio` after calibration (Issue: #2386)
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

### Distributed Graph Queries ✅ DONE
**Priority:** Medium
**Target Version:** v1.8.0

Enable graph queries across distributed ThemisDB instances.

**Implemented Features:**
- ✅ `DistributedGraphManager` — coordinator that fans out graph traversals to registered `ShardGraphExecutor` instances in parallel, merges results
- ✅ `LocalShardGraphExecutor` — thin wrapper around `GraphQueryOptimizer` for in-process / single-node shards (tests + embedded mode)
- ✅ `ShardGraphExecutor` — pluggable interface for per-shard execution (supports remote implementation via RPC transport)
- ✅ `DistributedGraphConfig` — configurable partition strategy (HASH, RANGE, GEO, CUSTOM), consistency level (EVENTUAL, STRONG), replication factor, timeout, parallelism cap
- ✅ Partition-aware vertex routing via `resolveShardForVertex` (FNV-1a hash → uniform bucket assignment)
- ✅ Shard qualifier syntax: `"<vertex_id>@<shard_id>"` for explicit routing
- ✅ Distributed shortest path (`shortestPath`): Dijkstra on each healthy shard in parallel; globally cheapest path returned
- ✅ Distributed k-hop neighbors (`kHopNeighbors`): BFS on all healthy shards in parallel; de-duplicated merged result
- ✅ Shard-aware plan generation (`optimizePlan`): returns `OptimizationPlan` with `is_distributed=true`, `shard_ids`, `recommended_parallelism` fields
- ✅ Fault tolerance: unhealthy shards (`isHealthy() == false`) skipped automatically
- ✅ `OptimizationPlan` extended with shard-aware fields (`is_distributed`, `shard_ids`, `recommended_parallelism`) — backward-compatible with single-node (defaults to `false`/empty/`1`)
- ✅ `explainPlan()` updated to print distributed shard info when `is_distributed=true`

**API:**
```cpp
// Define graph partitioning strategy
DistributedGraphConfig config;
config.partitioning = PartitionStrategy::HASH;  // or RANGE, GEO, CUSTOM
config.replication_factor = 3;
config.consistency = ConsistencyLevel::EVENTUAL;

DistributedGraphManager dist_graph(config);
dist_graph.addShard("shard1", std::make_shared<LocalShardGraphExecutor>("shard1", db1));
dist_graph.addShard("shard2", std::make_shared<LocalShardGraphExecutor>("shard2", db2));

// Vertex IDs may carry explicit shard qualifiers:
auto result = dist_graph.shortestPath("node_A@shard1", "node_B@shard2", constraints);

// K-hop neighbors across all shards:
auto neighbors = dist_graph.kHopNeighbors("node_A", 3, constraints);

// Shard-aware plan:
auto plan = dist_graph.optimizePlan("A", "D",
    GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
// plan->is_distributed == true, plan->shard_ids == {"shard1","shard2"}
```

---

### GPU-Accelerated Graph Processing ✅ IMPLEMENTED (BFS/DFS)
**Priority:** Medium
**Target Version:** v1.9.0

Offload graph computations to GPU for massive parallelism.

**Implemented Features (Issue #1829):**
- ✅ `GPUGraphTraversal` class — CSR-based BFS/DFS with CPU fallback (`include/graph/gpu_traversal.h`, `src/graph/gpu_traversal.cpp`)
- ✅ Level-synchronous BFS (mirrors GPU parallel frontier expansion)
- ✅ Iterative DFS with depth tracking
- ✅ CSR (Compressed Sparse Row) graph representation for cache-efficient traversal
- ✅ Integer node ID mapping (string ↔ `uint32_t`) for performance
- ✅ `use_gpu` / `gpu_device` fields added to `GraphQueryOptimizer::QueryConstraints`
- ✅ GPU dispatch in `executeBFS()` / `executeDFS()` of `GraphQueryOptimizer`
- ✅ Automatic CPU fallback when no GPU hardware is present
- ✅ `GraphIndexManager::allVertices()` — enumerate all vertices (with RocksDB fallback)

**API (Implemented):**
```cpp
// Via GPUGraphTraversal directly:
GPUGraphTraversal gpu_trav(graph_manager);
gpu_trav.load();
auto result = gpu_trav.bfs("start_vertex", cfg);
// result->visited_vertices, result->distances, result->used_cpu_fallback

// Via GraphQueryOptimizer (recommended):
GraphQueryOptimizer::QueryConstraints constraints;
constraints.use_gpu    = true;
constraints.gpu_device = 0;  // GPU index; ignored on CPU fallback

auto result = optimizer.executeBFS("start", 10, constraints);
// Automatically uses GPUGraphTraversal; falls back to CPU when no GPU present
```

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

### `DistributedGraphManager`: Read-Path Lock Upgrade
**Priority:** Medium
**Target Version:** v1.8.0

`distributed_graph.cpp` uses `std::lock_guard<std::mutex>` for all shard operations including read-only lookups (`getShard` at line 110, `listShards` at line 120, `execute` at line 126). All reader threads serialize unnecessarily.

**Implementation Notes:**
- `[ ]` Replace `std::mutex shards_mutex_` with `std::shared_mutex` in `DistributedGraphManager`; upgrade `getShard`, `listShards`, and `execute` (read path) to `std::shared_lock`.
- `[ ]` Keep `addShard` and `removeShard` (write path) on `std::unique_lock`.
- `[ ]` Add a TSAN-enabled stress test: 8 concurrent `execute()` threads + 1 `addShard()` thread.

---


**Priority:** Medium
**Target Version:** v1.7.0

Extend PathConstraints with more sophisticated constraint types.

**Features:**
- **Node Property Constraints** ✅ DONE – `addNodePropertyConstraint(key, value)` prunes BFS traversal
- **Weight Constraints** ✅ DONE – `addMaxWeight(threshold)` prunes BFS; `addMinWeight(threshold)` rejects at acceptance
- **Schema-Aware Node Label Hints** ✅ DONE – `QueryConstraints::node_labels` filters BFS/DFS by `_labels` field
- **Excluded Edge Type Hints** ✅ DONE – `QueryConstraints::excluded_edge_types` reduces cost-model fanout estimate
- **Temporal Constraints**: Path valid at specific time ⏳ Planned
- **Probability Constraints**: Min probability for uncertain graphs ⏳ Planned
- **Resource Constraints**: Capacity limits on paths ⏳ Planned
- **Semantic Constraints**: Ontology-based path rules ⏳ Planned — see [Ontology-based Semantic Constraints](#ontology-based-semantic-constraints) below
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

---

### Ontology-based Semantic Constraints
**Priority:** Medium
**Target Version:** v2.1.0
**Issue:** #PLANNED

Enforce semantic path validity rules derived from a domain ontology during graph traversal.
This enables knowledge-representation-aware queries: paths may only follow edges whose
types are permitted by the ontology relationship hierarchy, and nodes may only be visited
if they satisfy the declared class membership or property restrictions.

**Scope**
- Affected files: `include/graph/path_constraints.h`, `src/graph/path_constraints.cpp`,
  `include/graph/ontology_manager.h` (new), `src/graph/ontology_manager.cpp` (new)
- OWL/RDF-compatible concept hierarchy stored in a compact in-memory adjacency map
  (`OntologyManager`) loaded from YAML or JSON schema files
- Constraint evaluation is side-effect-free and deterministic; no I/O during traversal

**Design Constraints**
- `[x]` Ontology load time: ≤ 100 ms for schemas with ≤ 10 000 concepts (JSON/YAML)
- `[x]` Per-edge constraint check: ≤ 5 µs including class-membership lookup
- `[x]` Constraint violations must return a structured error (`ConstraintViolation`) containing
  the violating edge ID, the expected class, and the actual class
- `[x]` `OntologyManager` must be immutable after `build()` (thread-safe read; no write locks during traversal)
- `[x]` Graceful degradation: unknown concept IDs are treated as unconstrained (warn, not fail)

**Required Interfaces**

| Interface | Consumer | Notes |
|---|---|---|
| `OntologyManager::loadFromJson(path)` | Server startup, schema admin | Parses OWL-lite JSON; builds concept DAG |
| `OntologyManager::loadFromYaml(path)` | Server startup | YAML alternative to JSON loader |
| `OntologyManager::isA(concept, superConcept)` | PathConstraints evaluator | Transitive class-membership check via ancestor walk |
| `OntologyManager::allowedEdgeTypes(sourceClass, targetClass)` | PathConstraints evaluator | Returns set of edge types permitted by domain axioms |
| `PathConstraints::addSemanticConstraint(OntologyManager*, ruleset)` | AQL query compiler | Attaches ontology rule set to an active constraint object |
| `PathConstraints::validateSemanticPath(path, graph)` | `findConstrainedPaths` | Validates full path post-discovery; returns `ConstraintViolation` list |

**Planned API:**
```cpp
// Load ontology schema
auto onto = std::make_shared<OntologyManager>();
onto->loadFromJson("schemas/legal_ontology.json");
onto->build();  // finalise concept DAG

// Attach semantic constraint
PathConstraints constraints(&graph_mgr);
constraints.addSemanticConstraint(onto.get(), OntologyRuleset::STRICT);
// → edge type "hasParty" only valid between "LegalEntity" nodes
// → edge type "ruledBy" only valid from "Case" to "Statute"

auto paths = constraints.findConstrainedPaths("case_001", "statute_42", 10);
// Violations stored in constraints.lastViolations()
```

**Implementation Notes:**
- `[x]` Implement `OntologyManager` with a flat `std::unordered_map<std::string, ConceptNode>`
  where each `ConceptNode` stores `parents`, `allowed_edge_types_as_source`, and
  `allowed_edge_types_as_target`
- `[x]` `isA()` performs BFS over the ancestor chain (depth-limited to 20 hops) and caches
  results in a `std::unordered_map<std::pair<string,string>, bool>` LRU with 1 000 entries
- `[x]` `PathConstraints::validateSemanticPath()` iterates over all edges in the discovered
  path and calls `OntologyManager::allowedEdgeTypes(srcClass, dstClass)` for each edge;
  returns a `std::vector<ConstraintViolation>` (empty = valid)
- `[x]` BFS pruner in `findConstrainedPaths` calls `allowedEdgeTypes` at each frontier
  expansion to avoid generating invalid paths early (prune-first strategy)
- `[x]` Serialisation: `OntologyManager::toJson()` / `toYaml()` round-trips for hot-reload
- `[ ]` LoRA-enhanced semantic constraint scoring (v2.2.0): a fine-tuned LoRA adapter
  on the `LLMPluginManager` provides a soft-plausibility score for each traversed edge
  (0.0–1.0); paths with cumulative score < threshold are pruned (see AI/ML + LoRA integration)

**Test Strategy**
- `tests/graph/test_ontology_manager.cpp` — OM-01..OM-12
  - OM-01: `loadFromJson` round-trip equality
  - OM-02: `isA` transitive closure (3-hop hierarchy)
  - OM-03: `allowedEdgeTypes` returns correct set for known source/target pair
  - OM-04: unknown concept → unconstrained (no throw)
  - OM-05: thread-safety: 16 concurrent `isA` calls on shared `OntologyManager`
- `tests/graph/test_path_constraints_semantic.cpp` — SC-01..SC-10
  - SC-01: valid path accepted with schema-conformant edges
  - SC-02: invalid edge type → `ConstraintViolation` returned
  - SC-03: prune-first reduces discovered nodes by ≥ 30% vs unconstrained traversal
  - SC-04: `STRICT` vs `WARN` ruleset modes
  - SC-05: `loadFromYaml` + `findConstrainedPaths` end-to-end

**Performance Targets**
- `addSemanticConstraint`: ≤ 1 µs (pointer capture)
- `validateSemanticPath` for 100-edge path: ≤ 200 µs
- Ontology load (10 000 concepts, JSON): ≤ 100 ms

---



### Query Rewriting for Graph Optimization
**Status: ✅ DONE (Issue #250, delivered v1.9.0)**
**Priority:** Medium
**Target Version:** v1.8.0 → delivered v1.9.0

Automatically rewrite graph queries for better performance.

**Implemented Features:**
- ✅ `GraphQueryRewriter` class (`include/graph/graph_query_rewriter.h`, `src/graph/graph_query_rewriter.cpp`)
- ✅ `PREDICATE_PUSHDOWN` / `PRUNE_EARLY` rule — promotes `vertex_filters` to `prune_conditions` for early BFS/DFS branch pruning
- ✅ `COMMON_SUBEXPRESSION` rule — detects duplicate graph traversal sub-expressions and replaces repeated occurrences with LET-scoped `ref` nodes
- ✅ `JOIN_REORDERING` rule — swaps `traversal_join` operands so the more selective (lower cardinality) side is executed first
- ✅ `MATERIALIZED_VIEW` rule — tags traversal nodes for subgraph materialisation (activated automatically for multi-access patterns or when `aggressive_optimization = true`)
- ✅ `QUERY_DECOMPOSITION` rule — splits `multi_traversal` nodes into independent `parallel_subqueries` for concurrent execution
- ✅ `RewriteConfig` — per-rewrite configuration: `enabled_rules`, `aggressive_optimization`, `rewrite_time_limit_ms`
- ✅ `RewriteResult` — structured return value with rewritten plan and `GraphRewriteStats` (rules_applied, applied_rule_names, total_transformations)
- ✅ `explainRewrites(original, rewritten)` — human-readable multi-line summary of applied transformations
- ✅ `estimateSpeedup(original, rewritten)` — heuristic speedup factor estimate based on applied rules
- ✅ `addCustomRule(name, fn)` / `clearCustomRules()` — extensible custom rule hook
- ✅ Factory helpers: `makeTraversalPlan`, `makeFilterScanPlan`, `makeJoinPlan`, `makeMultiTraversalPlan`, `estimateCardinality`
- ✅ 38 unit tests in `tests/test_graph_query_rewriter.cpp` covering all acceptance criteria
- ✅ Standalone CMake target `test_graph_query_rewriter`

**Rewrite Rules:**
- Push predicates into graph traversal (prune early) — PREDICATE_PUSHDOWN converts vertex_filters to prune_conditions
- Decompose multi-pattern queries into independent subqueries — QUERY_DECOMPOSITION splits multi_traversal nodes
- Materialize frequently accessed subgraphs — MATERIALIZED_VIEW tags traversals for precomputed results
- Convert repeated traversals to single traversal with caching — COMMON_SUBEXPRESSION via LET-scoped refs
- Reorder multi-hop traversals based on selectivity — JOIN_REORDERING uses heuristic cardinality estimation

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
- Enable knowledge graph reasoning (see [Knowledge Graph Reasoning with Ontology & ML/LoRA](#knowledge-graph-reasoning-with-ontology--mlloRA) below)
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

### Knowledge Graph Reasoning with Ontology & ML/LoRA
**Priority:** High
**Target Version:** v2.1.0
**Issue:** #PLANNED

Enable ThemisDB to perform symbolic and neural reasoning over knowledge graphs.  This
combines the `OntologyManager` (semantic constraints above), the `MultiLayerGraph` (multi-
relational structure), and LoRA-fine-tuned LLM adapters to produce explainable, domain-
grounded inference chains.

**Scope**
- Affected files:
  - `include/graph/knowledge_graph_reasoner.h` (new)
  - `src/graph/knowledge_graph_reasoner.cpp` (new)
  - `include/graph/ontology_manager.h` (new, see Semantic Constraints above)
  - `src/graph/ontology_manager.cpp` (new)
  - `include/rag/knowledge_graph_retriever.h` (existing — extend with reasoning hooks)
  - `src/rag/knowledge_graph_retriever.cpp` (existing — extend)
  - Integration with `src/llm/multi_lora_manager.cpp` for LoRA adapter selection

**Wissensrepräsentation — Komponenten**

| Komponente | Funktion |
|---|---|
| `OntologyManager` | OWL-lite Konzepthierarchie; `isA()`, `allowedEdgeTypes()`, transitive Schließung |
| `KnowledgeGraphReasoner` | Regelerstellung + Forward-Chaining über Property-Paths; Erklärungsketten |
| `MultiLayerGraph` | Heterogene Graphschicht mit typisierten Kanten für multi-relationale KG-Abfragen |
| `KnowledgeGraphRetriever` (RAG) | Entity-linking + KG-augmentiertes Retrieval (existierend, wird erweitert) |
| LoRA Adapter (LLM) | Domänenspezifische Mustererkennung + soft-plausibility scoring über Kanten |

**Design Constraints**
- `[ ]` Reasoning depth limit: configurable `max_inference_hops` (default 5; hard cap 20)
- `[ ]` Inference chain must be serialisable as an ordered list of `(subject, predicate, object)` triples for explanation output
- `[ ]` Forward-chaining must be incremental (triggered by new edge/node events via CDC, not full re-evaluation)
- `[ ]` LoRA adapter inference must be optional (`THEMIS_ENABLE_LLM` guard); deterministic rule-based fallback always active
- `[ ]` Thread-safety: `KnowledgeGraphReasoner::infer()` is read-only; rule-set updates use `std::unique_lock`
- `[ ]` Memory: derived facts stored in a dedicated in-memory `InferenceStore`; TTL-based eviction; max 1 M derived triples

**Required Interfaces**

| Interface | Consumer | Notes |
|---|---|---|
| `KnowledgeGraphReasoner::addRule(Rule)` | Schema admin, AQL DDL | Adds a Horn-clause rule: `IF (A, rel1, B) AND (B, rel2, C) THEN (A, rel3, C)` |
| `KnowledgeGraphReasoner::infer(subjectId, depth)` | AQL query layer, RAG retriever | Returns `InferenceChain` with all derived facts up to `depth` hops |
| `KnowledgeGraphReasoner::explain(factId)` | Explanation API | Returns proof trace as ordered triple sequence |
| `KnowledgeGraphReasoner::applyLoRAScore(chain, adapter_id)` | LLM integration | Attaches soft-plausibility score (0.0–1.0) from LoRA adapter to each inferred edge |
| `KnowledgeGraphReasoner::onCDCEvent(event)` | CDC pipeline | Incremental forward-chaining on new edge inserts |
| `OntologyManager::loadFromJson(path)` | Server startup | Loads OWL-lite concept hierarchy |
| `OntologyManager::getSubclasses(concept)` | Reasoner | Returns direct + transitive subclasses |

**Semantisches Netz — Regel-Beispiele (Horn-Klauseln):**
```yaml
# Transitives Vorgesetzten-Verhältnis
- id: transitive_reports_to
  if:
    - [?A, reports_to, ?B]
    - [?B, reports_to, ?C]
  then:
    - [?A, indirectly_reports_to, ?C]

# Rollenhierarchie (Ontologie-Klausel)
- id: manager_is_employee
  if:
    - [?X, rdf:type, Manager]
  then:
    - [?X, rdf:type, Employee]

# Wissensrepräsentation: Kompetenz-Zuordnung via LoRA-Score
- id: expert_in
  if:
    - [?P, authored, ?D]
    - [?D, hasKeyword, ?T]
  then:
    - [?P, expertIn, ?T]
  lora_plausibility_adapter: "domain_expertise_v1"
  min_lora_score: 0.75
```

**AI/ML + LoRA Integration für Mustererkennung:**
- `[ ]` `MultiLoRAManager::selectAdapterForGraph(graph_context)` wählt den passenden
  LoRA-Adapter anhand von Graph-Statistiken (Knotentypen-Verteilung, Edge-Type-Häufigkeit)
- `[ ]` LoRA-Adapter wird während `KnowledgeGraphReasoner::applyLoRAScore()` mit
  strukturierten Graph-Prompts gespeist; Output ist ein Konfidenzwert pro Inferenz-Kante
- `[ ]` Training neuer LoRA-Adapter für Graphdomänen via `IncrementalLoRATrainer`
  (Training-Modul); Checkpoints über `exportWeights()` / `importWeights()` austauschbar
- `[ ]` Mustererkennung in Graphpfaden: LoRA-Adapter klassifiziert strukturelle Muster
  (Zyklen, Hub-Spoke, Chain-of-Authority) und gibt domänenspezifische Labels zurück

**Implementation Phases:**

| Phase | Beschreibung | Target |
|---|---|---|
| Phase 1 | `OntologyManager` + JSON/YAML-Loader + `isA()` / `allowedEdgeTypes()` | Q3 2026 |
| Phase 2 | `KnowledgeGraphReasoner` Horn-Klausel-Forward-Chaining + `InferenceStore` | Q4 2026 |
| Phase 3 | Incremental CDC-Trigger + TTL-Eviction + Erklärungsketten | Q1 2027 |
| Phase 4 | LoRA-Adapter-Integration + `applyLoRAScore()` + Mustererkennung | Q2 2027 |
| Phase 5 | RAG-Integration: `KnowledgeGraphRetriever` nutzt `KnowledgeGraphReasoner` | Q3 2027 |
| Phase 6 | Tests, Benchmarks, Dokumentation, Produktion | Q3 2027 |

**Test Strategy**
- `tests/graph/test_knowledge_graph_reasoner.cpp` — KGR-01..KGR-20
  - KGR-01..05: Horn-Klausel-Regelanwendung (transitiv, reflexiv, invers)
  - KGR-06..10: Erklärungsketten-Serialisierung
  - KGR-11..13: Incremental CDC-Trigger-Tests
  - KGR-14..16: LoRA-Score-Integration (Mock-Adapter)
  - KGR-17..18: Mustererkennung — Hub-Spoke, Chain-of-Authority
  - KGR-19..20: Performance (100 k Kanten, ≤ 500 ms Reasoning)
- `tests/graph/test_ontology_manager.cpp` — OM-01..OM-12 (s. Semantic Constraints)

**Performance Targets**
- Forward-chaining over 1 M graph edges: ≤ 2 s (cold start); ≤ 50 ms incremental
- `explain(factId)` proof trace: ≤ 10 ms
- LoRA plausibility scoring for 1 000 inferred edges: ≤ 500 ms

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
4. ✅ Latency Histogram & Prometheus Scrape Endpoint
5. ✅ Query Rate Limiter (per-second budget, ERR_GRAPH_RATE_LIMIT_EXCEEDED)
6. ✅ Property Graph Schema-Aware Optimizer Hints (Issue: #1819)

**v1.8.0 (Q1 2027):**
1. ✅ Distributed Graph Queries
2. ✅ ANN-accelerated candidate edge discovery (`setANNIndex(IAnnIndex*)` + `rebuildANNIndex()`)
3. ✅ CEP event emission for edge mutations (`setCEPEventCallback(std::function<void(themisdb::analytics::Event)>)`)
4. Query Rewriting

**v1.9.0 (Q3 2027):**
1. GPU-Accelerated Graph Processing
2. Approximate Algorithms

**v2.0.0 (Q1 2028):**
1. Multi-Layer Graph Support
2. Graph ML Integration
3. Graph Visualization

## Implemented Features (v1.7.0)

### Property Graph Schema-Aware Optimizer Hints ✅ DONE

`QueryConstraints::node_labels` and `QueryConstraints::excluded_edge_types` allow
callers to embed property-graph schema information directly in a query so that the
optimizer can choose a more accurate cost estimate and the traversal runtime can
prune the search space accordingly.

**node_labels** (OR semantics): only visit nodes that carry at least one of the
listed labels (matched against the comma-separated `_labels` field stored on each
node entity by `PropertyGraphManager`).

**excluded_edge_types**: cost-model hint that reduces the estimated edge fanout by
the fraction represented by each excluded type (uses `edge_type_selectivity` from
`GraphStatistics`).

`setNodeLabelStats(label_counts)` lets callers supply per-label node counts so that
the optimizer can derive label selectivity automatically and apply it during
`estimateCost`.

```cpp
// Register schema statistics (e.g. loaded from PropertyGraphManager)
optimizer.setNodeLabelStats({{"Person", 400}, {"Company", 100}});
// → Person selectivity = 400/total_nodes, Company = 100/total_nodes

// Restrict BFS to Person nodes only
GraphQueryOptimizer::QueryConstraints c;
c.node_labels = {"Person"};       // only traverse Person-labeled nodes
c.excluded_edge_types = {"DEPRECATED"};  // cost model reduces fanout

auto plan = optimizer.optimizeShortestPath("alice", "bob", c);
// plan.active_schema_hints describes the active hints
// plan.explanation includes "Schema Hints Active:" section

auto result = optimizer.executeBFS("alice", 3, c);
// BFS skips neighbors without a "Person" label in their _labels field
```

**Key design decisions:**
- `node_labels` filtering is applied only to *outgoing neighbors*, never to the
  explicitly provided start vertex (which is controlled by the caller).
- `nodeMatchesLabels` performs whole-token matching so "Person" never accidentally
  matches "SuperPerson".
- When a label is not present in `node_label_selectivity`, a conservative default
  selectivity of 0.5 is applied to cost estimation.
- Schema hints are included in both exact and structural plan-cache keys so that
  queries with different hints never share a cached plan.



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

### Latency Histogram & Prometheus Export ✅ DONE

`GraphQueryMetrics::LatencyHistogram` – 10-bucket fixed-width histogram with
upper bounds (ms): 1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf.

```cpp
const auto& hist = optimizer.getQueryMetrics().latency_histogram;
double p99 = hist.percentileMs(0.99);
double p50 = hist.percentileMs(0.50);
```

`GET /api/v1/graph/metrics/prometheus` exports all counters and the full latency
histogram in Prometheus text exposition format.  p50, p95, and p99 are also
exported as computed gauges.

### Query Rate Limiter ✅ DONE

Per-second token-window rate limiting for graph queries.

```cpp
optimizer.setMaxQueriesPerSecond(200);  // 200 QPS max
// Excess queries return ERR_GRAPH_RATE_LIMIT_EXCEEDED (6406)
optimizer.setMaxQueriesPerSecond(0);    // disable
```

Applies to all five execute methods.  Uses atomic CAS sliding-window for
thread-safe operation without mutexes.

---

### Subgraph Isomorphism (Pattern Matching) ✅ DONE

`GraphQueryOptimizer::executeSubgraphIsomorphism` – finds all injective mappings
from a pattern graph onto subgraphs of the data graph (VF2-style backtracking).

```cpp
// Pattern: u -> v -> w (chain of three vertices)
std::vector<std::string> pattern_verts = {"u", "v", "w"};
std::vector<std::pair<std::string,std::string>> pattern_edges = {{"u","v"},{"v","w"}};

auto result = optimizer.executeSubgraphIsomorphism(pattern_verts, pattern_edges);
// result.value().matches[i] is an unordered_map<string,string>
// mapping pattern vertex labels to data vertex IDs

// With constraints
GraphQueryOptimizer::QueryConstraints c;
c.max_results = 10;          // stop after first 10 matches
c.timeout_ms = 500;          // abort after 500ms
c.forbidden_vertices = {"X"}; // X must not appear in any match

auto limited = optimizer.executeSubgraphIsomorphism(pattern_verts, pattern_edges, c);
```

Key properties:
- Injective: each data vertex appears at most once per match
- Directed edge consistency: every pattern edge (u,v) must be present as a
  directed edge in the data graph for the matched vertices
- Pattern vertices are user-defined labels (not data vertex IDs)
- Supports `max_results`, `timeout_ms`, and `forbidden_vertices` constraints
- Execution statistics available via optional `ExecutionStats*` output parameter
- Rate-limited by `setMaxQueriesPerSecond()` like all other execute methods
- Integrates with `optimizePatternMatch()` for cost estimation and plan caching

---

## Scheduled Semantic Graph Edge Refresh

**Status:** ✅ Production Ready — Core engine, safety gates, audit trail, anomaly detection, ChangeFeed integration, ANN-accelerated candidate discovery, and CEP event emission are complete.

**Issue:** #FEATURE/ScheduledGraphEdgeRefresh
**Files:** `include/graph/scheduled_edge_refresh.h`, `src/graph/scheduled_edge_refresh.cpp`
**Tests:** `tests/graph/test_scheduled_edge_refresh.cpp` (60+ tests)
**Docs:** `docs/scheduled_edge_refresh.md`, `docs/de/scheduled_edge_refresh.md`

### Background & State of the Art

Keeping a graph semantically current as the underlying data evolves is a well-studied problem. The key research areas that inform this feature are:

| Research Area | Approach | Relevance to ThemisDB |
|---|---|---|
| Dynamic Graph Maintenance (Brandes, 2008) | Incremental edge updates based on betweenness centrality | Analytics module already exposes centrality; `centrality_weight` in `EdgeScore` |
| STGCN (Yu et al., 2017) | Spatio-temporal GNN embeddings for evolving graphs | GNN embeddings available via `graph/gnn_embeddings.cpp`; used as `NodeEmbeddingProvider` |
| Incremental Materialized Views (Maccioni et al., 2015) | Maintain derived graph state incrementally | `analytics/incremental_view.cpp` follows this pattern |
| Adaptive Graph Sampling (Leskovec & Faloutsos, 2006) | Smart edge sampling during refresh to reduce cost | Informs `top_k_candidates` and brute-force-vs-ANN threshold |
| Temporal Graph Evolution (Leskovec et al., 2008) | Exponential decay of edge relevance over time | `temporal_factor = 2^(−age / half_life)` in `computeTemporalDecay()` |
| Link Prediction via Embeddings (Hamilton et al., 2017) | Cosine/dot-product similarity in embedding space for candidate edges | `SimilarityMetric::COSINE / DOT_PRODUCT / EUCLIDEAN` in `computeSimilarity()` |

### Scope

- **Inputs:** Existing property graph with node embedding vectors (GNN index or user-supplied `NodeEmbeddingProvider` callback); `RefreshPolicy` configuration.
- **Outputs:**
  - Low-relevance edges removed (relevance = `similarity × temporal_factor × centrality_weight < relevance_threshold`)
  - New high-similarity edges added (top-k ANN candidates per node above `add_threshold`)
  - `RefreshStats` with per-cycle metrics (edges evaluated / removed / added, cycle duration, removal rate, anomaly flag)
  - Bounded in-memory audit trail (`RefreshAuditEntry`, max 10,000 entries)
  - Optional `Changefeed` events (`EVENT_PUT` / `EVENT_DELETE` keyed `graph_edge_refresh:<edge_id>`)
- **Frequency:** Configurable — time-based (`refresh_interval`), event-triggered (`triggerRefresh()`), or adaptive (caller-driven).
- **Graph size targets:** Brute-force similarity for ≤ `policy.ann_min_vertices` nodes (default 10,000); ANN-accelerated path via `setANNIndex(IAnnIndex*)` for larger graphs.

### Design Constraints

- [x] `RefreshPolicy` fields validated at construction; out-of-range values throw `std::invalid_argument` (no silent defaults after construction)
- [x] `max_removal_fraction` safety gate must abort the entire batch before any storage writes if violated — no partial mutations
- [x] All edge mutations (removals + additions) for a single cycle are committed as one `WriteBatchWrapper`; atomic at the RocksDB level
- [x] `anomaly_threshold_removal_rate = 0.0` disables anomaly detection (zero = off); enabling requires explicit opt-in
- [x] Background scheduler must not start concurrent cycles; `cycle_mutex_` serialises all `runRefreshCycle()` invocations
- [x] `setPolicy()` takes effect on the *next* cycle; a currently running cycle completes with the old policy
- [x] `NodeEmbeddingProvider` returning an empty vector is treated as "embedding unavailable" → similarity skipped → `similarity = 1.0`
- [x] Audit trail bounded at `kMaxAuditEntries = 10,000`; oldest entries evicted FIFO (no unbounded growth)
- [x] `Changefeed` attachment is optional; `recordEvent` failures are logged as warnings, never as errors that abort the cycle
- [x] ANN integration: `setANNIndex(IAnnIndex*)` + `rebuildANNIndex()` + ANN path in `discoverCandidateEdges()` when vertex count > `policy.ann_min_vertices`; brute-force fallback when below threshold or no index attached

### Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `GraphIndexManager::getAllEdges(graph_id)` | `collectEdges()` | Returns all edges scoped to `graph_id` (empty = all) |
| `GraphIndexManager::getAllVertices(graph_id)` | `discoverCandidates()` | Needed for per-vertex top-k candidate enumeration |
| `GraphIndexManager::getOutDegree(vertex_id)` | `scoreEdge()` | Used to compute `centrality_weight` |
| `GraphIndexManager::edgeExists(from, to)` | `discoverCandidates()` | Prevents adding duplicate edges |
| `GraphIndexManager::createWriteBatch()` | `applyBatch()` | Returns a `WriteBatchWrapper` for atomic multi-edge writes |
| `GraphIndexManager::addEdge(entity, batch)` | `applyBatch()` | Batched insertion |
| `GraphIndexManager::deleteEdge(id, batch)` | `applyBatch()` | Batched deletion |
| `NodeEmbeddingProvider` (`std::function<std::vector<float>(std::string)>`) | `computeSimilarity()` | User-supplied; may be backed by GNN index or HNSW vector store |
| `Changefeed::recordEvent(ChangeEvent)` | `appendAudit()` | Optional; CDC downstream consumers |
| `index::IAnnIndex::search(query, dim, k)` ✅ | `discoverCandidates()` | Attached via `setANNIndex()`; active when vertex count > `policy.ann_min_vertices` |
| `setCEPEventCallback(std::function<void(themisdb::analytics::Event)>)` ✅ | `applyBatch()` | Real-time `EDGE_CREATE`/`EDGE_DELETE` events after successful batch commit |

### Implementation Notes

**Scoring model** (already implemented):
```
relevance = similarity × temporal_factor × centrality_weight
```
- `similarity`: cosine / dot-product / Euclidean between embedding vectors of edge endpoints.
- `temporal_factor = 2^(−age_s / half_life_s)` where `age_s` is read from the edge's `_created_at` field. If absent or `half_life_s = 0`, factor = 1.0.
- `centrality_weight = 1 / (1 + log(1 + out_degree))` — dampens hub nodes.

**Candidate discovery** (✅ implemented — brute-force + ANN):
- For each vertex `v`, compute `similarity(embedding(v), embedding(u))` for all `u ≠ v`.
- Keep the top-k pairs above `add_threshold` that do not already have an edge `v → u`.
- **ANN path:** when vertex count > `policy.ann_min_vertices` (default 10,000) and an `IAnnIndex` is attached via `setANNIndex()`, uses `knnSearch(embedding(v), top_k)` to reduce O(V²) to O(V · log V) per cycle.

**CEP integration** (✅ implemented):
- After `applyBatch()` succeeds, emits one `themisdb::analytics::Event` per mutation via `setCEPEventCallback()`.
- Event types: `EDGE_CREATE` / `EDGE_DELETE` with payload `{ edge_id, from, to, relevance_score, cycle_number }`.
- Downstream CEP rules can react (e.g., alert on burst of removals, trigger reindexing on cluster-like additions).
- Additive to the existing `Changefeed` integration; both may be active simultaneously.

**Scheduling strategies** (configurable via `RefreshPolicy`):
| Strategy | How to configure | Best for |
|---|---|---|
| Time-based | `refresh_interval = std::chrono::seconds(N)` | Predictable, low-overhead background maintenance |
| Manual only | `refresh_interval = std::chrono::seconds(0)` | Caller controls timing via `triggerRefresh()` |
| Event-triggered | Caller calls `triggerRefresh()` after N mutations | Responsive; combine with change-log threshold |
| Adaptive | External orchestrator observes `getStats().removal_rate` and adjusts interval | Self-optimising; suitable for variable data drift |

### Test Strategy

- **Unit tests** (45+ in `tests/graph/test_scheduled_edge_refresh.cpp`):
  - `RefreshPolicy` validation (invalid thresholds throw `std::invalid_argument`)
  - `computeSimilarity()` for COSINE, DOT_PRODUCT, EUCLIDEAN — exact float comparisons
  - `computeTemporalDecay()` — half-life formula, absent `_created_at`, zero half-life
  - `scoreEdge()` — combined relevance with all three factors
  - Safety gate abort: `aborted_safety_gate = true` when removal fraction exceeded
  - Full refresh cycle with removal and with addition
  - Audit trail population after removal and addition
  - `getStats()` after a completed cycle
  - `start()` / `stop()` lifecycle with zero interval (manual-only mode)
  - `setPolicy()` runtime update takes effect on next cycle
  - Empty graph: graceful handling (no edges, no crash)
  - Multiple cycles: `cycle_number` increments correctly
  - Anomaly detection: `anomaly_high_removal_rate` flag set and logged
  - ChangeFeed: `recordEvent()` called for each mutation with correct event type and metadata
  - Large-graph integration: 100-node graph, cluster embeddings → correct add/remove behaviour
  - Regression: stable graph (all edges above threshold) → zero mutations over multiple cycles
- **ANN/CEP tests** (implemented):
  - ANN path active when vertex count > `policy.ann_min_vertices`; brute-force fallback below threshold
  - ANN-accelerated discovery produces same candidate count as brute-force (BruteForceANN fixture)
  - CEP callback invoked with `EDGE_REMOVED` events after successful removal (event_name + fields validated)
  - CEP callback invoked with `EDGE_ADDED` events after successful addition (all required fields present)
  - No CEP events emitted when safety gate aborts cycle
  - Detaching CEP callback (empty function) prevents further event emission

### Performance Targets

- Single refresh cycle on a **10,000-node graph** (brute-force): ≤ 5 s wall time on a single core, ≤ 200 ms with 8 parallel scoring workers.
- Single refresh cycle on a **1,000,000-node graph** (ANN via `setANNIndex()`): ≤ 30 s wall time; top-k candidate discovery O(V · log V · D) where D = embedding dimension.
- Audit trail `appendAudit()` overhead: < 1 µs per mutation (bounded ring buffer, no heap allocation on steady state).
- `ChangeFeed::recordEvent()` path: < 5 µs per event (RocksDB single put).
- Safety gate check: O(1) — computed as a fraction before any storage access.
- Background scheduler wake-up jitter: < 50 ms from configured `refresh_interval` under normal system load.
- Memory overhead per engine instance: < 10 MB for audit trail (10,000 entries × ~200 bytes/entry) + policy + stats.

### Security / Reliability

- `max_removal_fraction` safety gate (default 0.10) prevents runaway deletions from a misconfigured or adversarially crafted `NodeEmbeddingProvider`; the gate fires before any write is issued.
- `NodeEmbeddingProvider` is user-supplied code; it must not be trusted to return well-formed vectors. `computeSimilarity()` defensively returns 0.0 for empty, mismatched-length, or NaN-containing vectors.
- Audit trail eviction is FIFO and bounded; no unbounded memory growth regardless of cycle count.
- `Changefeed::recordEvent()` and CEP callback exceptions are caught and logged as warnings; they never abort a refresh cycle or roll back committed mutations.
- Batch commit failure is logged as an error; the cycle reports failure via `RefreshStats::last_error` without crashing the engine or stopping the background thread.
- `setPolicy()` is thread-safe (protected by `policy_mutex_`); a concurrent `triggerRefresh()` sees either the old or the new policy consistently, never a partial mix.
- ANN index integration only queries the vertex set present in the current cycle; no cross-graph data access is possible through the `IAnnIndex` interface.

---

## Community Requests

Track user-requested features:
- **Cypher Query Support**: Neo4j-compatible query language (requested by 15 users)
- **Graph Backup/Restore**: Snapshot and restore graph state (requested by 12 users)
- **Graph Diff**: Compare two graph versions (requested by 8 users)
- **Graph Validation**: Schema validation for property graphs (requested by 10 users)

---

*Last Updated: April 2026*
*Next Review: Q3 2026*

## Test Strategy

- Unit test coverage ≥ 80% across `graph/query_optimizer.cpp`, `graph/plan_cache.cpp`, `graph/parallel_traversal.cpp` (tracked by Issue #1830)
- Integration tests: AQL graph query round-trip, constrained path finding with required/forbidden nodes, plan cache hit/miss under concurrent load
- Property-based tests: VF2 subgraph isomorphism correctness verified against brute-force matcher for graphs up to 50 nodes
- GPU/CPU parity tests: BFS/DFS results must be bit-identical across CUDA and CPU backends for all test graphs
- Adaptive cost model convergence test: after 100 synthetic executions per algorithm, `mean_absolute_error_ms` must be < 10% of mean measured latency
- Benchmark suite: traversal latency vs. graph size (1K, 100K, 1M, 10M nodes) with pass/fail regression gate (< 5% throughput degradation)

## Performance Targets

- Algorithm selection (`selectAlgorithm`) latency: < 1 ms p99 for graphs up to 10M nodes
- Plan cache lookup: < 100 µs p99 including structural fingerprint comparison
- Parallel BFS on 1M-node graph: ≥ 4× speedup over single-threaded BFS on an 8-core machine
- GPU BFS on RTX-class GPU (≥1M nodes): ≥ 8× speedup over CPU baseline
- Subgraph isomorphism on 100-node pattern against 1M-node graph: < 500 ms p95
- Distributed query (4 shards): < 20% latency overhead vs. equivalent single-shard query
- Plan cache eviction: O(1) amortized using LRU + TTL heap; no stop-the-world pauses

## Security / Reliability

- Path constraint inputs (required/forbidden node/edge IDs) must be validated against the graph schema before execution; invalid IDs return a structured error, not a crash (Issue #1832)
- AQL graph traversal depth must be bounded by a configurable `max_depth` limit (default 100) to prevent unbounded traversal DoS
- Plan cache keys must not embed raw user-supplied strings; use structural fingerprints to prevent cache poisoning
- Incremental query handles expire after a configurable TTL (default 60 s) to prevent resource exhaustion from abandoned handles
- GPU memory allocation failures must fall back to CPU execution without data loss
- Distributed shard queries must be protected by per-request timeouts (default 5 s) with automatic cancellation
- All traversal results must be deterministic for a given graph snapshot (no non-deterministic ordering unless explicitly randomized)
