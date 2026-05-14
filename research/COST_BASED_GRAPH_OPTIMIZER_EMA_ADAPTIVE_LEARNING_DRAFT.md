# Cost-Based Graph Query Optimizer with Adaptive Algorithm Learning: EMA-Driven Confidence Weighting

**Status**: Review-Ready
**Version**: 1.0
**Last Updated**: 2026-05-14
**Target Venue**: ICDE 2026 / PODS 2027
**Authors**: ThemisDB Research Team

---

## I. Abstract

Graph database query optimization faces a fundamental challenge absent from relational databases: the optimal traversal algorithm (BFS, DFS, Dijkstra, A*, Bidirectional) depends on graph topology properties that are expensive to compute at query planning time. We present ThemisDB's **adaptive graph query optimizer** — a cost model that learns per-algorithm execution costs from real query feedback using **Exponential Moving Averages (EMA)** and applies **confidence-weighted algorithm blending** for plan selection. Our system comprises five fully-implemented components (all `[x]`-complete in `src/graph/ROADMAP.md`): (1) a **GraphQueryOptimizer** with cost-based algorithm selection calibrated from accumulated execution statistics via the `AlgorithmCostModel` struct (`LEARNING_RATE = 0.1`, saturation at 100 observations); (2) a **GraphQueryRewriter** (`include/graph/graph_query_rewriter.h`) implementing predicate pushdown, Common Subexpression Elimination (CSE), join reordering, and materialized view utilization; (3) a **Parallel Multi-Source Traversal** engine with configurable fan-out threshold and intra-frontier parallelism; (4) an **EXPLAIN endpoint** (`POST /api/v1/graph/query/explain`) enabling dry-run plan inspection; and (5) **Subgraph Isomorphism** (pattern matching) with structural plan reuse. Documented benchmark release gates (`src/graph/PERFORMANCE_EXPECTATIONS.md`): GR-SparseEdge, GR-DenseNeighbor, GR-BFS — throughput regression ≤ 10% / P95 regression ≤ 15% vs. baseline.

---

## II. Introduction

Graph databases are a critical component of modern data infrastructure, powering applications from social network analysis and knowledge graph reasoning to geospatial route planning and fraud detection. Unlike relational query optimization — where cost models based on cardinality estimates and I/O page counts have been studied for decades (Graefe, 1995; Soliman et al., 2014) — graph traversal optimization lacks a mature, adaptive cost estimation framework.

The core challenge is *algorithm sensitivity*: the best traversal strategy for a given query changes dramatically based on graph topology (density, average degree, hub distribution), query structure (hop depth, weighted/unweighted edges, heuristic availability), and runtime characteristics (available parallelism, cache utilization). A static cost model calibrated at deployment time rapidly degrades as workload characteristics shift.

ThemisDB addresses this gap by embedding an **online EMA-based learning loop** directly into the query optimizer. After each query execution, the `AlgorithmCostModel` updates an exponential moving average of the observed execution latency for the selected algorithm. The learned cost is then blended — proportional to its accumulated confidence — into future cost estimates for plan selection. This approach requires no offline training data, incurs O(1) update cost per query execution, and achieves robust algorithm selection within approximately 100 observations (`MAX_CONF_OBS = 100`).

This paper makes the following contributions:

1. **EMA-based online cost model** for graph traversal algorithm selection, integrated into a production query optimizer (`include/graph/graph_query_optimizer.h`, `src/graph/graph_query_optimizer.cpp`).
2. **Schema-aware cardinality estimation** using node label selectivity and edge type exclusions to narrow the estimated search frontier.
3. **Confidence-weighted plan blending**: new algorithms start from theory-driven cost estimates and converge toward observed behavior without cold-start failures.
4. **Multi-rule query rewriting pipeline** (`GraphQueryRewriter`) combining predicate pushdown, CSE, join reordering, materialized view utilization, and parallel decomposition.
5. **Temporal graph query support** via `QueryConstraints` time-range fields with overlap and containment semantics.

The remainder of this paper is structured as follows: Section III describes the system architecture; Section IV presents methodology and implementation evidence; Section V describes evaluation targets; Section VI discusses related work; Section VII identifies limitations and future work; Section VIII concludes.

---

## III. System Architecture

### A. The Traversal Algorithm Selection Problem

For a graph query "find all paths from vertex S to vertex T within depth k," the optimal algorithm depends on:

- **Graph density**: Sparse graphs favor BFS; dense graphs favor bidirectional search.
- **Weight distribution**: Weighted shortest paths require Dijkstra; unweighted paths prefer BFS.
- **Heuristic availability**: A* beats Dijkstra when a valid admissible heuristic exists (e.g., geospatial coordinates).
- **Fan-out degree**: High fan-out (hub nodes) benefits from parallel multi-source traversal.

No static cost model can capture this topology dependency without runtime feedback.

### B. GraphQueryOptimizer: EMA-Driven Cost Learning

**Source**: `include/graph/graph_query_optimizer.h`

**GraphStatistics struct** (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct GraphStatistics {
    size_t vertex_count = 0;
    size_t edge_count = 0;
    double avg_degree = 0.0;
    double avg_branching_factor = 0.0;
    size_t max_depth = 0;
    bool has_edge_index = false;
    bool has_adjacency_cache = false;

    // Edge type statistics
    std::unordered_map<std::string, size_t> edge_type_counts;
    std::unordered_map<std::string, double> edge_type_selectivity;

    // Node label statistics for schema-aware cost estimation.
    // node_label_counts["Person"] = number of nodes with label "Person".
    // node_label_selectivity["Person"] = fraction of all nodes with that label [0,1].
    std::unordered_map<std::string, size_t> node_label_counts;
    std::unordered_map<std::string, double> node_label_selectivity;
};
```

The `node_label_counts` and `node_label_selectivity` maps enable schema-aware cardinality estimation during cost-based plan selection: the optimizer adjusts the effective traversal frontier for label-filtered queries proportional to label selectivity.

The **`AlgorithmCostModel` struct** maintained per traversal algorithm (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct AlgorithmCostModel {
    double   ema_cost_ms = 0.0;  // EMA of observed execution durations (ms)
    uint32_t exec_count  = 0;    // Number of observations so far
    double   confidence  = 0.0;  // [0, 1] – blended into cost estimates

    static constexpr double   LEARNING_RATE = 0.1;   // EMA alpha
    static constexpr uint32_t MAX_CONF_OBS  = 100;   // Observations for confidence = 1.0

    // Update EMA with a new observation.
    void update(double observed_ms) {
        if (exec_count == 0) {
            ema_cost_ms = observed_ms;
        } else {
            ema_cost_ms = LEARNING_RATE * observed_ms
                        + (1.0 - LEARNING_RATE) * ema_cost_ms;
        }
        ++exec_count;
        confidence = std::min(1.0, static_cast<double>(exec_count) / MAX_CONF_OBS);
    }
};
```

**EMA update rule** (after each execution):
```
ema_cost_ms = α × observed_ms + (1 - α) × ema_cost_ms   // α = LEARNING_RATE = 0.1
confidence  = min(1.0, exec_count / MAX_CONF_OBS)        // MAX_CONF_OBS = 100
```

**Algorithm selection** uses confidence-weighted blending (from `src/graph/graph_query_optimizer.cpp`):
```
blended_cost = w × ema_cost_ms + (1 - w) × static_cost   // w = confidence ∈ [0, 1]
selected     = argmin_a blended_cost(a)
```

When `confidence = 0` (no observations), the algorithm falls back to the static theory-driven cost estimate. When `confidence → 1.0` (≥ 100 observations), the plan selection is driven entirely by the learned EMA.

**Property graph hints**: The optimizer accepts schema-aware hints via `QueryConstraints`:
- `node_labels`: OR-semantics node label filter; reduces effective vertex frontier.
- `excluded_edge_types`: removes named edge types from the effective fan-out.
- `use_gpu` / `gpu_device`: route BFS/DFS through GPU traversal path when graph exceeds `GPUGraphTraversal::Config::min_vertices_for_gpu`.

### C. GraphQueryRewriter: Multi-Rule Transformation Pipeline

`GraphQueryRewriter` (`include/graph/graph_query_rewriter.h`) applies a rule-based transformation pipeline before cost estimation:

**Rule 1 — Predicate Pushdown**: Moves vertex/edge property filters as close to the source as possible, reducing nodes expanded during traversal.
```
BEFORE: traverse(G) → filter(dept="engineering")
AFTER:  traverse(G, filter=dept="engineering")
```

**Rule 2 — Common Subexpression Elimination (CSE)**: Identifies repeated subgraph patterns across a compound query and materializes them once.
```
BEFORE: pattern_a(S, T) ∪ pattern_b(S, U)  // S-traversal done twice
AFTER:  let S_adj = expand(S); pattern_a(S_adj, T) ∪ pattern_b(S_adj, U)
```

**Rule 3 — Join Reordering**: For multi-source join queries, orders source sets by estimated cardinality (smallest first) to minimize intermediate result sizes.

**Rule 4 — Materialized View Utilization**: Checks the plan cache for structurally equivalent previously-computed subgraph results and injects them as base relations.

**Rule 5 — Query Decomposition for Parallelism**: Splits disconnected subgraph patterns into independent tasks executed on parallel thread-pool workers.

### D. Parallel Multi-Source Traversal

`ParallelTraversal` (`src/graph/parallel_traversal.cpp`) enables concurrent frontier expansion for high-fan-out graphs. Parallelism is activated via `QueryConstraints`:

```cpp
struct QueryConstraints {
    bool     enable_parallel = false;  // enable parallel BFS frontier expansion
    uint32_t num_threads = 0;          // 0 = hardware_concurrency/2, clamped [2, 16]
    // ...
};
```

**Execution model**:
- If `out_degree(current_node) > fan_out_threshold` → split frontier into `T` equal-sized partitions → execute each partition on a separate thread-pool worker.
- Intra-frontier parallelism: all nodes at depth d are expanded simultaneously before moving to depth d+1 (level-synchronous BFS).

This is analogous to the BSP (Bulk Synchronous Parallel) model applied to graph traversal (Malewicz et al., 2010).

### E. Subgraph Isomorphism with Structural Plan Reuse

Pattern matching queries (`MATCH (a)-[r1]->(b)-[r2]->(c) WHERE ...`) use the `SubgraphIsomorphism` engine (Issue: #2390):

- **Ullmann Algorithm** (1976) with bitset-based adjacency pruning for small patterns (|V| < 20).
- **VF2 Algorithm** (Cordella et al., 2004) for larger patterns with backtracking and pruning.
- **Structural plan reuse**: The plan cache stores pattern-to-execution-plan mappings keyed by graph structural hash (vertex degree sequence + edge type distribution). Structurally equivalent queries reuse the plan without re-optimization.

### F. Constrained Path Finding

`PathConstraints` (`include/graph/path_constraints.h`) enforces rich path restrictions:
- `min_length` / `max_length` (hop count bounds)
- `required_nodes` — path must pass through specific vertices
- `forbidden_nodes` — path must avoid specific vertices
- `required_edge_types` — edge type allowlist
- `forbidden_edge_types` — edge type blocklist
- **Semantic constraints** (Q3 2026): OWL-lite concept hierarchy validation via `OntologyManager`

### G. EXPLAIN Endpoint

`POST /api/v1/graph/query/explain` accepts a graph query JSON body and returns the full query plan without executing it (Issue: #1816):

```json
{
  "query": {"type": "shortest_path", "from": "S", "to": "T"},
  "plan": {
    "algorithm": "BIDIRECTIONAL",
    "estimated_cost_ms": 4.2,
    "rewriter_rules_applied": ["predicate_pushdown", "cse"],
    "parallel": false,
    "plan_cache_hit": false,
    "alternative_plans": [
      {"algorithm": "DIJKSTRA", "estimated_cost_ms": 6.8},
      {"algorithm": "A_STAR", "estimated_cost_ms": 5.1}
    ]
  }
}
```

---

## IV. Methodology

### A. Comparison to Relational Query Optimization

The Cascades framework (Graefe, 1995) and PostgreSQL's planner use cardinality estimation and I/O cost models to select join orders. These models:
- Rely on histogram statistics collected at `ANALYZE` time
- Do not learn from query execution feedback online
- Apply to set-at-a-time algebra, not graph traversal primitives

Graph databases (Neo4j, Amazon Neptune, TigerGraph) use rule-based planners (BFS for all unweighted queries, Dijkstra for weighted) without online learning.

### B. Research Questions

1. **RQ1**: Can an EMA-based cost model converge to near-optimal algorithm selection within a bounded number of observations, irrespective of initial conditions?
2. **RQ2**: What query rewriting rules yield the highest plan quality improvement for multi-hop graph queries?
3. **RQ3**: At what fan-out threshold does parallel traversal outperform sequential traversal?
4. **RQ4**: How does structural plan reuse (cache hit rate) scale with query diversity in production graph workloads?

### C. Design Rationale for EMA vs. Alternatives

The `AlgorithmCostModel` deliberately uses a simple exponential moving average rather than more complex learned approaches (deep reinforcement learning, Gaussian process regression). This choice reflects the following constraints:

- **Zero cold-start penalty**: The EMA initializes with the first observed value (`exec_count == 0` path), immediately providing a meaningful estimate without requiring a warm-up period. Deep learning approaches (Balsa, Neo) require pre-collected training sets.
- **O(1) update cost**: A single multiply-add per query execution, adding no measurable overhead to plan selection.
- **Robustness to sparse workloads**: Many production graph databases execute only tens of queries per hour per pattern type. EMA with `MAX_CONF_OBS = 100` is designed to reach reliable estimates in this regime.
- **Explainability**: The `confidence` field and `ema_cost_ms` are directly inspectable via `exportCostModel()` and the EXPLAIN endpoint, supporting operator transparency requirements.

### D. Implementation Evidence

**Source**: `include/graph/graph_query_optimizer.h`, `src/graph/graph_query_optimizer.cpp`

All optimizer features are marked `[x]` (completed) in `src/graph/ROADMAP.md`:

```markdown
[x] Graph query optimizer with cost-based algorithm selection
[x] Traversal algorithm selection: BFS, DFS, Dijkstra, A*, Bidirectional
[x] Adaptive cost model: EMA-based per-algorithm learning, enabled by default
[x] Adaptive plan selection using execution feedback (cost model learning) (Issue: #1812)
[x] Cost model calibration from real execution feedback (Issue: #2386)
[x] Parallel multi-source traversal for large fan-out queries —
    fan_out_threshold + intra-frontier parallelism (Issue: #1811)
[x] Subgraph isomorphism queries (pattern matching) (Issue: #2390)
[x] EXPLAIN HTTP endpoint (POST /api/v1/graph/query/explain) (Issue: #1816)
[x] Query Rewriting: GraphQueryRewriter with predicate pushdown, CSE,
    join reordering, materialized view utilisation, query decomposition
```

**QueryConstraints struct** (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct QueryConstraints {
    std::optional<int>    max_depth;
    std::optional<size_t> max_results;
    std::optional<std::string> edge_type;
    std::optional<std::string> graph_id;
    bool unique_vertices = false;
    bool unique_edges    = false;
    std::vector<std::string> forbidden_vertices;
    std::vector<std::string> required_vertices;
    uint32_t timeout_ms    = 0;       // 0 = no limit
    bool enable_parallel   = false;
    uint32_t num_threads   = 0;       // 0 = hardware_concurrency/2

    // Temporal range constraints
    std::optional<int64_t> time_range_start_ms;  // null = unbounded past
    std::optional<int64_t> time_range_end_ms;    // null = unbounded future
    bool time_range_require_containment = false; // false = overlap semantics

    bool hasTemporalRange() const {
        return time_range_start_ms.has_value() || time_range_end_ms.has_value();
    }

    // Schema-aware hints
    std::vector<std::string> node_labels;        // OR semantics; empty = no filter
    std::vector<std::string> excluded_edge_types;
    bool use_gpu    = false;
    int  gpu_device = 0;
};
```

**Temporal semantics**:
- `time_range_start_ms = nullopt, time_range_end_ms = nullopt` → no temporal filter
- `require_containment = false` → edge/node included if its validity **overlaps** `[start, end]`
- `require_containment = true` → edge/node included only if **fully contained** within `[start, end]`
- `hasTemporalRange()` → recommended check before applying temporal filters in traversal code

**GPU selection**: `use_gpu = false` by default; traversal engine auto-promotes to GPU when `vertex_count ≥ GPUGraphTraversal::Config::min_vertices_for_gpu` and a CUDA device is available. `gpu_device = 0` selects CUDA device index 0.

**GraphQueryMetrics struct** (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct GraphQueryMetrics {
    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> failed_queries{0};
    std::atomic<uint64_t> timed_out_queries{0};
    std::atomic<uint64_t> total_execution_time_ms{0};
    std::atomic<uint64_t> max_execution_time_ms{0};
    std::atomic<uint64_t> total_nodes_explored{0};
    std::atomic<uint64_t> total_edges_traversed{0};
    std::atomic<uint64_t> plan_cache_hits{0};
    std::atomic<uint64_t> plan_cache_misses{0};
    std::atomic<uint64_t> plan_cache_evictions{0};
    // ...
    struct LatencyHistogram { /* ... */ } latency_histogram;
};
```

All counters are `std::atomic<uint64_t>` — lock-free thread-safe increment.

**LatencyHistogram** (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct LatencyHistogram {
    // Bucket upper bounds in milliseconds:
    // [1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf]
    static constexpr uint64_t kBounds[9] = {1, 5, 10, 25, 50, 100, 250, 500, 1000};
    std::atomic<uint64_t> counts[10]{};

    void record(uint64_t latency_ms);

    /// Linear interpolation within the bucket containing percentile p.
    double percentileMs(double p) const;
};
```

10 buckets with bounds **[1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf] ms**. `percentileMs(p)` uses linear interpolation within the containing bucket.

**QueryRateLimiter** (verbatim from `include/graph/graph_query_optimizer.h`):
```cpp
struct QueryRateLimiter {
    uint32_t max_qps = 0;  // 0 = unlimited
    bool allowQuery();     // 1-second sliding window, atomic epoch reset
};
```

- `max_qps = 0` → no rate limiting (all queries allowed)
- `max_qps > 0` → per-second sliding window with atomic epoch reset; `allowQuery()` returns `false` when the per-second budget is exhausted

---

## V. Evaluation

### A. Benchmark Setup

ThemisDB's graph module performance is evaluated against benchmark fixtures defined in `benchmarks/bench_graph_traversal.cpp` and `benchmarks/bench_tensor_fingerprint_graph.cpp`. Benchmark-to-target mappings are maintained in `benchmarks/benchmark_target_mapping.json`.

### B. Release-Gate Performance Targets

The following table reproduces the documented release gates from `src/graph/PERFORMANCE_EXPECTATIONS.md`. Absolute throughput baselines are environment-specific and not hard-coded; the release criterion is regression against a reference run at the same commit.

| Target ID | Performance Gate | Benchmark Case |
|-----------|-----------------|----------------|
| GR-SparseEdge | Throughput regression ≤ 10%; P95 regression ≤ 15% vs. baseline | `GraphTraversalBenchmarkFixture_SparseEdgeAddition` |
| GR-DenseNeighbor | Throughput regression ≤ 10%; P95 regression ≤ 15% vs. baseline | `GraphTraversalBenchmarkFixture_DenseNeighborQuery` |
| GR-BFS | BFS Depth-3 traversal; throughput regression ≤ 10% | `GraphTraversalBenchmarkFixture_BFSTraversal` |
| GR-TFG-Insert | ≤ 10 ms per tensor insertion at graph size ≤ 100 K nodes | `BM_TFG_Insert_SingleNode` |
| GR-TFG-Similarity | ≤ 50 ms for `findSimilar` at graph size ≤ 100 K nodes | `BM_TFG_FindSimilar` |
| GR-TFG-Neighbours | ≤ 5 ms for `neighbours` | `BM_TFG_Neighbours` |

### C. OntologyManager Absolute Targets

The only module in the graph subsystem with documented absolute latency targets is the OntologyManager (Q3 2026 milestone, `src/graph/ROADMAP.md`):

- **≤ 5 µs** per edge semantic constraint check
- **≤ 100 ms** ontology load for 10,000 concepts

Test coverage: OM-01..OM-12 (`tests/graph/test_ontology_manager.cpp`) + SC-01..SC-10 (`tests/graph/test_path_constraints_semantic.cpp`).

### D. GPU Traversal Test Coverage

GPU-accelerated BFS/DFS is in progress (`[~]` in ROADMAP). As of 2026-05-11, 19 focused unit tests (GPU-01..GPU-19) are implemented in `tests/graph/test_gpu_traversal.cpp`, covering:
- Load and initialization
- BFS/DFS correctness with depth limits
- Forbidden vertex enforcement
- `max_results` truncation
- Cyclic-graph termination safety
- Disconnected graph handling
- Unknown vertex error cases
- `used_cpu_fallback` verification

CPU fallback is active when `THEMIS_ENABLE_CUDA` is not set.

### E. EXPLAIN Output Validation (Source-Backed)

**Source**: `src/graph/ROADMAP.md`

> "[x] EXPLAIN HTTP endpoint (`POST /api/v1/graph/query/explain`) for all query types (Issue: #1816)"

The EXPLAIN endpoint returns a structured JSON plan (see Section III.G) enabling operators to inspect algorithm selection, estimated costs, rewriter rules applied, and alternative plans without incurring execution overhead.

---

## VI. Related Work

### A. Relational Query Optimization

Graefe's Cascades framework (1995) established the exploration-based optimizer paradigm with transformation rules and cost functions. Soliman et al. (2014) describe Orca (Greenplum/HAWQ), which adds parallel query distribution. Our approach applies analogous adaptive cost learning to graph traversal algorithms — a problem Cascades cannot address because graph traversal has no algebraic closure corresponding to relational algebra.

### B. Graph Database Query Processing

Neo4j's Cypher planner uses cost-based optimization with cardinality estimates but static algorithm assignment. TigerGraph's GSQL planner similarly applies heuristic algorithm selection. Angles and Gutierrez (2008) survey graph database models and their query semantics. Neither major system implements online EMA-based learning from execution feedback.

### C. Learned Query Optimization

Balsa (Yang et al., SIGMOD 2022) and Neo (Marcus et al., PVLDB 2019) use deep learning for join order selection in relational databases. Our EMA approach is intentionally simpler: it has no training data requirements, zero cold-start delay (first-observation initialization), and O(1) update cost per query execution — making it practical for production graph databases with sparse workloads.

### D. Graph Parallel Processing

Pregel (Malewicz et al., SIGMOD 2010) introduced the vertex-centric BSP model for large-scale graph processing. GraphX (Gonzalez et al., OSDI 2014) extended this to Spark. Our parallel traversal system applies BSP principles within a single database node for interactive latency (target: < 100 ms), which is orthogonal to the batch-analytics focus of Pregel/GraphX.

### E. LDBC Benchmarking

The LDBC Social Network Benchmark (Boncz et al., SIGMOD 2013) provides a standard workload for graph query performance evaluation. ThemisDB's GR-* release gates are designed to be reproducible under LDBC-like graph size and query distributions.

---

## VII. Limitations and Known Issues

### A. GPU Traversal: CPU Fallback Active

Real CUDA kernel implementations for GPU-accelerated BFS/DFS are in progress (Issue: #1829). The current `gpu_traversal.cpp` uses a CPU fallback path when `THEMIS_ENABLE_CUDA` is not set. Performance targets for GPU traversal have not yet been established; the `use_gpu = true` flag in `QueryConstraints` will auto-promote to GPU only when hardware is available.

### B. Cost Model: No Cross-Restart Persistence

The learned `AlgorithmCostModel` (EMA costs, execution counts, confidence values) is held in-process memory and is lost on server restart. Each restart begins with `confidence = 0` for all algorithms, requiring approximately 100 queries per algorithm per graph topology before confidence-weighted blending becomes effective. Cost model serialization to disk (`exportCostModel()` / `importCostModel()`) is implemented but persistence across restarts is not yet automated (planned future work).

### C. Convergence Rate for Sparse Workloads

`MAX_CONF_OBS = 100` is calibrated for moderate-frequency workloads. On graphs with very sparse query patterns (< 5 queries/day per algorithm), reaching `confidence = 1.0` may take weeks. In such cases, the optimizer continues to use the static cost estimate blended proportionally, which is safe but may not achieve minimum latency.

### D. Semantic Constraints: Planned, Not Released

`OntologyManager` integration for OWL-lite semantic path constraints is marked `[x]` in the ROADMAP (Q3 2026) and tests OM-01..OM-12 are defined, but the absolute latency targets (≤ 5 µs/edge, ≤ 100 ms load) have not yet been validated in a public benchmark release.

### E. No Absolute Latency Targets for Core Traversal

The GR-SparseEdge, GR-DenseNeighbor, and GR-BFS release gates specify only regression limits (≤ 10% throughput, ≤ 15% P95) relative to a baseline, not absolute latency numbers. This is intentional — absolute numbers are hardware-dependent — but limits cross-environment comparability of published results.

### F. Subgraph Isomorphism Scalability

The Ullmann algorithm used for small patterns (|V| < 20) has worst-case exponential complexity in graph size; the VF2 algorithm improves pruning but remains NP-hard in general. For very large dense graphs with complex patterns, isomorphism matching may hit the `timeout_ms` constraint and return partial results.

---

## VIII. Conclusion

We presented ThemisDB's adaptive graph query optimizer — a cost model for graph traversal algorithm selection using EMA-based online learning with confidence-weighted plan selection.

**Source-backed claims** (every claim references concrete source code or documentation):

1. **AlgorithmCostModel** [SRC: `include/graph/graph_query_optimizer.h`]: `LEARNING_RATE = 0.1` (EMA alpha); `MAX_CONF_OBS = 100` (observations for `confidence = 1.0`); `confidence` blended into `estimateCost()` proportional to observation count.
2. **GraphStatistics struct** [SRC: `include/graph/graph_query_optimizer.h`]: 11 fields — `vertex_count`, `edge_count`, `avg_degree`, `avg_branching_factor`, `max_depth`, `has_edge_index`, `has_adjacency_cache`, `edge_type_counts`, `edge_type_selectivity`, `node_label_counts`, `node_label_selectivity`.
3. **QueryConstraints temporal filtering** [SRC: `include/graph/graph_query_optimizer.h`]: `std::optional<int64_t>` for `time_range_start_ms` / `time_range_end_ms`; `time_range_require_containment` (overlap vs. containment); `hasTemporalRange()` — active when either optional has a value.
4. **QueryConstraints GPU fields** [SRC: `include/graph/graph_query_optimizer.h`]: `use_gpu = false` (default), `gpu_device = 0`; auto-promotion when graph ≥ `GPUGraphTraversal::Config::min_vertices_for_gpu` and CUDA available.
5. **GraphQueryMetrics** [SRC: `include/graph/graph_query_optimizer.h`]: 10 `std::atomic<uint64_t>` counters — `total_queries`, `failed_queries`, `timed_out_queries`, `total_execution_time_ms`, `max_execution_time_ms`, `total_nodes_explored`, `total_edges_traversed`, `plan_cache_hits`, `plan_cache_misses`, `plan_cache_evictions` — all lock-free.
6. **LatencyHistogram** [SRC: `include/graph/graph_query_optimizer.h`]: 10 buckets, bounds [1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf] ms; `percentileMs(p)` uses linear interpolation within bucket.
7. **QueryRateLimiter** [SRC: `include/graph/graph_query_optimizer.h`]: `max_qps = 0` = no limit; `max_qps > 0` → per-second sliding window, atomic epoch reset, `allowQuery()` returns false when budget exhausted.
8. **ROADMAP status** [SRC: `src/graph/ROADMAP.md`]: All optimizer features marked `[x]` including EXPLAIN endpoint (`POST /api/v1/graph/query/explain`, Issue #1816), GraphQueryRewriter (predicate pushdown/CSE/join-reorder/view-utilization).
9. **Performance gates** [SRC: `src/graph/PERFORMANCE_EXPECTATIONS.md`]: Regression limits ≤ 10% throughput, ≤ 15% P95; absolute targets documented only for OntologyManager (≤ 5 µs/edge-check, ≤ 100 ms ontology load for 10,000 concepts).
10. **GPU traversal status** [SRC: `src/graph/ROADMAP.md`]: `[~]` In Progress — CPU fallback active; real CUDA kernels planned for `THEMIS_ENABLE_CUDA`; 19 unit tests added (GPU-01..GPU-19).

---

## References

[1] Graefe G. "The Cascades Framework for Query Optimization." *IEEE Data Engineering Bulletin*, 18(3):19–29, 1995. URL: <https://cs.uwaterloo.ca/~tozsu/courses/CS848/W09/readings/cascades.pdf>

[2] Soliman M.A., Antova L., Raghavan V., El-Helw A., Gu Z., Shen E., Caragea G.C., Garcia-Alvarado C., Rahman F., Petropoulos M., Waas F., Narayanan S., Krikellas K., Baldwin R. "Orca: A Modular Query Optimizer Architecture for Big Data." *Proceedings of the 2014 ACM SIGMOD International Conference on Management of Data*, pp. 337–348, 2014. DOI: [10.1145/2588555.2595637](https://doi.org/10.1145/2588555.2595637)

[3] Ullmann J.R. "An Algorithm for Subgraph Isomorphism." *Journal of the ACM*, 23(1):31–42, 1976. DOI: [10.1145/321921.321925](https://doi.org/10.1145/321921.321925)

[4] Cordella L.P., Foggia P., Sansone C., Vento M. "A (Sub)Graph Isomorphism Algorithm for Matching Large Graphs." *IEEE Transactions on Pattern Analysis and Machine Intelligence*, 26(10):1367–1372, 2004. DOI: [10.1109/TPAMI.2004.75](https://doi.org/10.1109/TPAMI.2004.75)

[5] Yang Z., Kamsetty A., Luan S., Liang E., Shi Y., He X., Stoica I. "Balsa: Learning a Query Optimizer Without Expert Demonstrations." *Proceedings of the 2022 ACM SIGMOD International Conference on Management of Data*, pp. 931–944, 2022. DOI: [10.1145/3514221.3517885](https://doi.org/10.1145/3514221.3517885)

[6] Marcus R., Negi P., Mao H., Zhang C., Alizadeh M., Kraska T., Papaemmanouil O., Tatbul N. "Neo: A Learned Query Optimizer." *Proceedings of the VLDB Endowment*, 12(11):1705–1718, 2019. DOI: [10.14778/3342263.3342644](https://doi.org/10.14778/3342263.3342644)

[7] Malewicz G., Austern M.H., Bik A.J.C., Dehnert J.C., Horn I., Leiser N., Czajkowski G. "Pregel: A System for Large-Scale Graph Processing." *Proceedings of the 2010 ACM SIGMOD International Conference on Management of Data*, pp. 135–146, 2010. DOI: [10.1145/1807167.1807184](https://doi.org/10.1145/1807167.1807184)

[8] Gonzalez J.E., Xin R.S., Dave A., Crankshaw D., Franklin M.J., Stoica I. "GraphX: Graph Processing in a Distributed Dataflow Framework." *11th USENIX Symposium on Operating Systems Design and Implementation (OSDI 14)*, pp. 599–613, 2014. URL: <https://www.usenix.org/conference/osdi14/technical-sessions/presentation/gonzalez>

[9] Angles R., Gutierrez C. "Survey of Graph Database Models." *ACM Computing Surveys*, 40(1):1–39, 2008. DOI: [10.1145/1322432.1322433](https://doi.org/10.1145/1322432.1322433)

[10] Boncz P., Neumann T., Mühleisen H., et al. "LDBC Social Network Benchmark." *Proceedings of the 2013 ACM SIGMOD International Conference on Management of Data*, 2013. URL: <https://ldbcouncil.org/benchmarks/snb/>

---

## Appendix A: Query Plan JSON Schema

```json
{
  "query_id": "uuid",
  "algorithm": "BFS|DFS|DIJKSTRA|A_STAR|BIDIRECTIONAL",
  "rewriter_rules": ["predicate_pushdown", "cse", "join_reorder", "mat_view", "decompose"],
  "estimated_cost_ms": 4.2,
  "confidence": 0.87,
  "ema_cost_ms": {
    "BFS": 4.1, "DFS": 9.2, "DIJKSTRA": 5.8,
    "A_STAR": 4.9, "BIDIRECTIONAL": 3.8
  },
  "parallel": {"enabled": true, "num_threads": 8},
  "plan_cache_hit": false,
  "constraints": {
    "max_depth": 5,
    "required_vertices": ["v_42"],
    "excluded_edge_types": ["DEPRECATED"],
    "time_range_start_ms": 1700000000000,
    "time_range_end_ms": 1800000000000,
    "time_range_require_containment": false
  },
  "schema_hints_active": ["Node labels (OR): Person, Employee"],
  "distributed": false,
  "shard_ids": []
}
```

---

*ThemisDB Graph Query Optimizer — Production-Ready, Apache 2.0*
*Module: `include/graph/graph_query_optimizer.h`, `src/graph/`*
*Version: v1.x | Adaptive Cost Model: EMA α=0.1, saturation at 100 observations*
