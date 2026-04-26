# Graph Module – Production Readiness Roadmap

**Version:** 1.0  
**Last Updated:** April 2026  
**Scope:** Graph module hardening, observability, parallel/distributed execution, and production readiness

---

## Overview

This roadmap tracks the phased approach to making ThemisDB's graph module production-ready. It addresses gaps identified in the [Graph Advanced Features README](../src/graph/ADVANCED_FEATURES_README.md) and [Future Enhancements](../src/graph/FUTURE_ENHANCEMENTS.md) documents, and maps them to concrete deliverables with target versions.

**Reference Implementations:**
- `src/graph/path_constraints.cpp` – Constraint-based path finding (BFS, constraint validation)
- `src/graph/graph_query_optimizer.cpp` – Cost-based query planning and traversal execution
- `include/graph/graph_query_optimizer.h` – Public API for query optimization and metrics
- `include/graph/path_constraints.h` – Public API for constraint-based path finding

---

## Phase 1: Observability & SLO Support

**Status:** ✅ Complete  
**Target Version:** v1.7.0

### 1.1 Query Timeout / SLO Budget ✅ DONE

Added `timeout_ms` to `QueryConstraints`. When set to a non-zero value the
BFS and DFS traversals abort after the given number of milliseconds and return
`ERR_QUERY_TIMEOUT`. This is the first line of defence for SLO enforcement.

**Deliverables:**
- `QueryConstraints::timeout_ms` field in `include/graph/graph_query_optimizer.h`
- Timeout checks in `executeBFS` and `executeDFS` in `src/graph/graph_query_optimizer.cpp`
- `ERR_QUERY_TIMEOUT` (error code 6103) propagated to callers

**Usage:**
```cpp
GraphQueryOptimizer::QueryConstraints constraints;
constraints.max_depth = 5;
constraints.timeout_ms = 500; // abort after 500 ms
auto result = optimizer.executeBFS("start", 5, constraints);
if (!result) {
    // result.error() contains ERR_QUERY_TIMEOUT message
}
```

### 1.2 Aggregate Query Metrics ✅ DONE

`GraphQueryOptimizer` now accumulates thread-safe counters that can be scraped
by a Prometheus exporter or forwarded to an OpenTelemetry collector without
modifying the graph module itself.

**Deliverables:**
- `GraphQueryMetrics` struct in `include/graph/graph_query_optimizer.h`
- `getQueryMetrics()` accessor on `GraphQueryOptimizer`
- Counters: `total_queries`, `failed_queries`, `timed_out_queries`,
  `total_execution_time_ms`, `max_execution_time_ms`,
  `total_nodes_explored`, `total_edges_traversed`,
  `plan_cache_hits`, `plan_cache_misses`

**Exported via:**
```cpp
const auto& m = optimizer.getQueryMetrics();
// Expose to Prometheus / OTel:
// themis_graph_queries_total          m.total_queries
// themis_graph_query_errors_total     m.failed_queries
// themis_graph_query_timeouts_total   m.timed_out_queries
// themis_graph_query_duration_ms_sum  m.total_execution_time_ms
// themis_graph_query_duration_ms_max  m.max_execution_time_ms
// themis_graph_nodes_explored_total   m.total_nodes_explored
// themis_graph_edges_traversed_total  m.total_edges_traversed
// themis_graph_plan_cache_hits_total  m.plan_cache_hits
// themis_graph_plan_cache_misses_total m.plan_cache_misses
```

---

## Phase 2: Structured Errors & API Contract

**Status:** ✅ Complete  
**Target Version:** v1.7.0

### 2.1 Graph-Specific Error Codes ✅ DONE

`errors::ErrorCode` in `include/utils/error_registry.h` extended with dedicated
graph error codes in range **6400-6499** (6200-6299 was already reserved for API errors):

| Code | Constant                        | Description                                     |
|------|---------------------------------|-------------------------------------------------|
| 6400 | `ERR_GRAPH_NO_SUCH_VERTEX`      | Referenced vertex does not exist in the graph   |
| 6401 | `ERR_GRAPH_NO_SUCH_EDGE`        | Referenced edge does not exist in the graph     |
| 6402 | `ERR_GRAPH_CONSTRAINT_CONFLICT` | Contradictory path constraints                  |
| 6403 | `ERR_GRAPH_PATH_NOT_FOUND`      | No path satisfies all constraints               |
| 6404 | `ERR_GRAPH_CYCLE_DETECTED`      | Cycle encountered in acyclic-required traversal |
| 6405 | `ERR_GRAPH_DEPTH_EXCEEDED`      | Query depth exceeded configured limit           |

All codes are registered with full `ErrorMetadata` (category, severity, message template,
solution steps, keywords) in `src/utils/error_registry.cpp`.

`executeBFS` and `executeDFS` now return `ERR_GRAPH_NO_SUCH_VERTEX` (instead of the
generic `ERR_QUERY_EXECUTION_FAILED`) when a vertex's adjacency cannot be retrieved.

### 2.2 Explain / Dry-Run Query API ✅ DONE

`explainConstrainedPath()` added to `GraphQueryOptimizer`. It returns an
`OptimizationPlan` without executing any traversal, enabling query introspection
before committing to actual graph traversal:

```cpp
themis::graph::PathConstraints constraints(&graph_mgr);
constraints.addMinLength(2);
constraints.addMaxLength(6);
constraints.addRequiredNode("checkpoint");

// Dry-run: no traversal, no graph I/O
auto plan = optimizer.explainConstrainedPath("start", "end", constraints);
if (plan) {
    std::cout << optimizer.explainPlan(plan.value()); // prints algorithm choice, cost, etc.
    // Then decide whether to actually execute:
    auto result = constraints.findConstrainedPaths("start", "end", 5);
}
```

Note: `explainConstrainedPath()` does **not** increment `getQueryMetrics().total_queries`.

### 2.3 Admin Metrics Endpoint ✅ DONE

`GET /api/v1/graph/metrics` now returns a JSON snapshot of `GraphQueryMetrics`:

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

Implemented in `GraphApiHandler::handleMetrics()` in `src/server/graph_api_handler.cpp`.
Registered as `Route::GraphMetricsGet` in `src/server/http_server.cpp`.

---

## Phase 3: Parallel Execution & Adaptive Cost Model

**Status:** ✅ Complete (3.1 + 3.2 + 3.3 done)  
**Target Version:** v1.7.0

### 3.1 Parallel BFS ✅ DONE

`QueryConstraints` now has two new fields:

| Field | Default | Description |
|-------|---------|-------------|
| `enable_parallel` | `false` | When `true`, enables level-parallel BFS frontier expansion |
| `num_threads` | `0` (auto) | Worker threads (0 = `hardware_concurrency/2`, clamped to [2,16]) |

`executeBFS` was rewritten to use a level-by-level frontier approach. Each level
is expanded either sequentially (small graphs / `enable_parallel=false`) or in
parallel using `std::async` tasks (one per chunk of frontier nodes). Each task
returns its neighbor list independently; de-duplication against the shared
`visited` set is done serially after all tasks complete.

```cpp
GraphQueryOptimizer::QueryConstraints c;
c.enable_parallel = true;
c.num_threads = 4;          // or 0 for auto
auto result = optimizer.executeBFS("start", 5, c);
```

The optimizer's `optimizeShortestPath` now propagates `enable_parallel` from
the caller's constraints to the `OptimizationPlan`.

**Correctness guarantee:** the parallel path produces the same set of reachable
nodes as the sequential path (verified by `BFS_Parallel_ProducesSameResultAsSequential`).

### 3.2 Parallel Dijkstra (Δ-Stepping) ✅ DONE

`executeDijkstra` now supports the Δ-Stepping shortest-path algorithm when
`constraints.enable_parallel = true`.

**How it works:**
1. **Δ selection**: the average edge weight of the start vertex's outgoing edges
   (defaults to 1.0 when there are no outgoing edges or Δ would be zero).
2. **Bucket structure**: tentative distances are partitioned into buckets of
   width Δ using a `std::map<size_t, unordered_set<string>>`.  `begin()` always
   returns the minimum-index non-empty bucket in O(log n).
3. **Light-edge relaxation (parallel)**: vertices in the current bucket are
   chunked into `num_threads` groups; each group is dispatched as a
   `std::async` task.  Each task returns `vector<RelaxResult>` – no shared
   writes, no locks needed.
4. **Serial update phase**: the main thread applies all `RelaxResult` entries
   from the parallel tasks, maintaining `dist[]` and `parent[]` without data
   races.
5. **Heavy-edge relaxation (serial)**: after the bucket is stable, heavy edges
   (weight > Δ) from all settled vertices are relaxed by the main thread.
6. **Early exit**: once the target vertex is settled, the algorithm terminates.

**Thread count**: controlled by `QueryConstraints::num_threads`
(0 = `hardware_concurrency/2`, clamped to [2, 16]).

**Example:**
```cpp
GraphQueryOptimizer::QueryConstraints c;
c.enable_parallel = true;
c.num_threads = 4;   // or 0 for auto
auto result = optimizer.executeDijkstra("start", "end", c);
// result->totalCost == optimal weighted shortest-path cost
// result->path == reconstructed path from start to end
```

**Correctness guarantee:** the parallel path produces the same `totalCost` and
valid path as the sequential Dijkstra
(`Dijkstra_Parallel_ProducesSameResultAsSequential` test passes).

**Timeout:** if `timeout_ms` is set, `ERR_QUERY_TIMEOUT` is returned when the
limit is exceeded between bucket iterations.

### 3.3 Adaptive Cost Model ✅ DONE

`GraphQueryOptimizer` now maintains a per-algorithm EMA (Exponential Moving
Average) cost model that automatically calibrates cost estimates towards
observed execution times.

**New APIs:**

| API | Description |
|-----|-------------|
| `enableAdaptiveLearning(bool)` | Enable / disable (default: enabled) |
| `isAdaptiveLearningEnabled()` | Query current state |
| `exportCostModel()` | Serialise learned model to JSON string |
| `importCostModel(json_str)` | Seed optimizer with pre-learned data |
| `getAlgorithmCostModels()` | Inspect per-algorithm EMA, count, confidence |

**`AlgorithmCostModel` struct:**
```cpp
struct AlgorithmCostModel {
    double ema_cost_ms;   // EMA of observed durations
    uint32_t exec_count;  // Number of observations
    double confidence;    // [0,1] – grows to 1.0 at 100 observations
};
```

**Blending into `estimateCost`:**
```
blended = (1 - confidence) * theory_cost + confidence * (ema_ms * 10)
```
When confidence is 0 (no data), plans are purely theory-driven. As observations
accumulate the estimate converges to actual observed performance.

**Persistence:**
```cpp
// Save after a workload run
std::ofstream f("graph_model.json");
f << optimizer.exportCostModel();

// Load in the next process to skip the warm-up period
GraphQueryOptimizer opt2(graph_mgr);
std::string json = read_file("graph_model.json");
opt2.importCostModel(json);
```

`importCostModel` silently skips unknown algorithm names and is safe to use with
models from older versions.

---

## Phase 3.4: Latency Histogram, Prometheus Scrape Endpoint & Query Rate Limiter

**Status:** ✅ Complete  
**Target Version:** v1.7.0

### 3.4.1 Latency Histogram (p50/p95/p99) ✅ DONE

`GraphQueryMetrics` now contains a nested `LatencyHistogram` struct with 10
fixed-width buckets (upper bounds in ms: 1, 5, 10, 25, 50, 100, 250, 500, 1000,
+Inf).  `recordExecution` records each query's duration into the histogram so
that the Prometheus exporter can publish exact bucket counts as well as computed
p50/p95/p99 gauges.

```cpp
const auto& hist = optimizer.getQueryMetrics().latency_histogram;
double p99 = hist.percentileMs(0.99); // approximate p99 latency in ms
double p50 = hist.percentileMs(0.50); // approximate median latency in ms
```

### 3.4.2 Prometheus Scrape Endpoint ✅ DONE

`GET /api/v1/graph/metrics/prometheus` returns metrics in the **Prometheus text
exposition format** (`text/plain; version=0.0.4`) so that a Prometheus server
can scrape this endpoint without a custom exporter:

```http
GET /api/v1/graph/metrics/prometheus HTTP/1.1
```

Sample output:
```
# HELP themis_graph_queries_total Total graph traversal executions since startup
# TYPE themis_graph_queries_total counter
themis_graph_queries_total 42
# HELP themis_graph_query_errors_total Graph traversal executions that returned no result
# TYPE themis_graph_query_errors_total counter
themis_graph_query_errors_total 1
...
# HELP themis_graph_latency_ms Latency histogram of graph query execution
# TYPE themis_graph_latency_ms histogram
themis_graph_latency_ms_bucket{le="1"} 5
themis_graph_latency_ms_bucket{le="5"} 12
...
themis_graph_latency_ms_bucket{le="+Inf"} 42
themis_graph_latency_ms_sum 350
themis_graph_latency_ms_count 42
# HELP themis_graph_latency_p99_ms Approximate p99 graph query latency in milliseconds
# TYPE themis_graph_latency_p99_ms gauge
themis_graph_latency_p99_ms 87.500000
```

Implemented in `GraphApiHandler::handleMetricsPrometheus()` in
`src/server/graph_api_handler.cpp` and registered as
`Route::GraphMetricsPrometheusGet` in `src/server/http_server.cpp`.

### 3.4.3 Query Rate Limiter ✅ DONE

`GraphQueryOptimizer` now supports per-second query rate limiting via a
token-window `QueryRateLimiter` struct.  When the budget is exhausted,
`ERR_GRAPH_RATE_LIMIT_EXCEEDED` (6406) is returned immediately before any
traversal work begins.

**New API:**
```cpp
optimizer.setMaxQueriesPerSecond(100); // limit to 100 QPS
uint32_t current = optimizer.getMaxQueriesPerSecond(); // 100
optimizer.setMaxQueriesPerSecond(0);   // disable rate limiting
```

**Error code:**

| Code | Constant                          | Description                              |
|------|-----------------------------------|------------------------------------------|
| 6406 | `ERR_GRAPH_RATE_LIMIT_EXCEEDED`   | Query rejected by rate limiter           |

The rate limiter uses an atomic sliding-window (per-second epoch with CAS reset),
so it is thread-safe without mutexes.  Applies to all five execute methods:
`executeBFS`, `executeDFS`, `executeDijkstra`, `executeAStar`,
`executeBidirectional`.

---

## Phase 4: Distributed Graph Queries

**Status:** 📋 Planned  
**Target Version:** v1.8.0

### 4.1 Cross-Shard Traversal

Extend `GraphQueryOptimizer` to route traversal steps to the correct shard
when vertices are partitioned across multiple ThemisDB nodes.

### 4.2 Partial-Result Aggregation

Collect partial paths from individual shards and merge them into a globally
optimal result, with timeout propagation across network hops.

---

## Phase 5: Advanced Constraints & Query Rewriting

**Status:** ✅ Partially Complete (5.1 EDGE_PROPERTY + 5.2 NODE_PROPERTY + 5.3 weight constraints done)  
**Target Version:** v1.8.0

### 5.1 Edge Property-Based Constraints ✅ DONE

`PathConstraints` now supports filtering edges by arbitrary string field values
during `findConstrainedPaths` traversal, using the `EDGE_PROPERTY` constraint type.

**New API:**
```cpp
PathConstraints c(&graph_mgr);
c.addEdgePropertyConstraint("type", "follows"); // only traverse "follows" edges
auto paths = c.findConstrainedPaths("user1", "user5", 10);
```

**Implementation details:**
- `addEdgePropertyConstraint(field_name, expected_value)` creates an `EDGE_PROPERTY`
  constraint with `property_key` = field name and `string_value` = expected value
- During BFS in `findConstrainedPaths`, each candidate edge is checked against all
  `EDGE_PROPERTY` constraints before being added to the queue (early pruning)
- `validatePath` also enforces `EDGE_PROPERTY` constraints on complete paths
- `describeConstraints()` now shows "Edge property: key = value"
- `GraphIndexManager::getEdgeField(edgeId, fieldName)` added as the backing
  low-level accessor for edge entity fields

### 5.2 Node Property-Based Constraints ✅ DONE

`PathConstraints` now supports filtering nodes by arbitrary string field values,
using the `NODE_PROPERTY` constraint type.

**New API:**
```cpp
PathConstraints c(&graph_mgr);
c.addNodePropertyConstraint("country", "USA"); // only visit nodes where country == "USA"
auto paths = c.findConstrainedPaths("A", "Z", 10);
```

**Implementation details:**
- `addNodePropertyConstraint(field_name, expected_value)` creates a `NODE_PROPERTY` constraint
- During BFS each candidate `next_node` is checked against all `NODE_PROPERTY` constraints
  before being enqueued (early pruning)
- `validatePath` enforces `NODE_PROPERTY` for all nodes in the path
- `GraphIndexManager::getNodeField(vertexId, fieldName)` added as the backing
  accessor using `KeySchema::makeGraphNodeKey` (`node:<pk>` key format)

### 5.3 Weight Constraints ✅ DONE

`PathConstraints` now supports total-path-weight constraints.

**New API:**
```cpp
PathConstraints c(&graph_mgr);
c.addMaxWeight(10.0);  // Prune paths with accumulated cost > 10.0 (BFS pruning)
c.addMinWeight(2.0);   // Reject completed paths with cost < 2.0 (acceptance check)
auto paths = c.findConstrainedPaths("A", "D", 5);
```

**Implementation details:**
- Two new `ConstraintType` values: `MAX_WEIGHT`, `MIN_WEIGHT`
- `Constraint::double_value` field stores the threshold
- BFS loop checks `max_weight` after computing `next_state.cost`; states that
  exceed the budget are pruned before enqueueing
- Path acceptance at target node checks `min_weight` before calling `validatePath`
- `describeConstraints()` shows "Maximum path weight: N" / "Minimum path weight: N"
- Edge weights come from the `_weight` field of each edge entity (default 1.0)

### 5.4 Automatic Query Rewriting

Rewrite graph sub-queries (e.g., `k`-hop + filter) into cheaper equivalent
forms using algebraic equivalences in the optimizer.

---

## Phase 6: GPU Acceleration

**Status:** 📋 Planned  
**Target Version:** v1.9.0

Offload BFS/Dijkstra kernels to GPU using the existing GPU acceleration
framework (`include/acceleration/`).  Falls back to CPU when no GPU is
available (safe-fail).

---

## Phase 7: ML-Guided Optimization

**Status:** 🔬 Research  
**Target Version:** v2.0.0

Use learned multi-graph execution statistics and graph topology embeddings to
continuously improve algorithm selection beyond the per-algorithm EMA model
already implemented in Phase 3.3.

---

## Observability Checklist

| Item                                    | Status   | Owner        |
|-----------------------------------------|----------|--------------|
| `timeout_ms` in `QueryConstraints`      | ✅ Done  | graph module |
| `GraphQueryMetrics` counters            | ✅ Done  | graph module |
| Prometheus metric names defined         | ✅ Done  | this doc     |
| Graph-specific error codes 6400+        | ✅ Done  | graph module |
| `explainConstrainedPath()` dry-run      | ✅ Done  | graph module |
| `enable_parallel` + `num_threads` BFS   | ✅ Done  | graph module |
| Parallel Dijkstra (Δ-Stepping)          | ✅ Done  | graph module |
| `addEdgePropertyConstraint()` pruning   | ✅ Done  | graph module |
| Admin API `GET /api/v1/graph/metrics`   | ✅ Done  | server       |
| Adaptive Cost Model (EMA + export)      | ✅ Done  | graph module |
| `addNodePropertyConstraint()` pruning   | ✅ Done  | graph module |
| `addMaxWeight()` / `addMinWeight()`     | ✅ Done  | graph module |
| Latency histogram (p50/p95/p99)         | ✅ Done  | graph module |
| Prometheus scrape endpoint              | ✅ Done  | server       |
| Query rate limiter (max QPS)            | ✅ Done  | graph module |
| OTel span export in traversal loops     | 📋 TODO  | observability|
| Heatmap: nodes-explored per query       | 📋 TODO  | observability|
| Alerting rule: error_rate > 5%          | 📋 TODO  | ops          |
| Alerting rule: p99 latency > SLO        | 📋 TODO  | ops          |
---

## Test Matrix

| Area                                  | Tests                              | Status      |
|---------------------------------------|------------------------------------|-------------|
| Basic BFS/DFS correctness             | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Timeout / SLO enforcement             | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Aggregate metrics                     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Graph error code values (6400+)       | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| `explainConstrainedPath()` dry-run    | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Parallel BFS correctness              | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Parallel BFS default thread count     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| `enable_parallel` default = false     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Dijkstra sequential shortest path     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Dijkstra parallel (Δ-stepping) result | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Dijkstra parallel updates metrics     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Dijkstra parallel single-hop path     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Edge property constraint API          | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Edge property constraint validation   | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| `GET /api/v1/graph/metrics` endpoint  | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Adaptive learning enabled/disabled    | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| EMA model populated by executions     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Separate per-algo model tracking      | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Export / import roundtrip             | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Invalid JSON import returns false     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Unknown algo import silently ignored  | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Node property constraint API          | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Node property constraint validation   | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| `getNodeField` accessor               | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Max/min weight constraint API         | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Max weight BFS pruning                | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Min weight acceptance check           | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Latency histogram populated           | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| p50/p95/p99 percentile non-negative   | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Rate limiter default disabled         | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Rate limiter high limit allows queries| `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Rate limit exceeded returns error     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Prometheus endpoint OK status         | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Prometheus content-type text/plain    | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Prometheus body has counter lines     | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Prometheus body has +Inf bucket       | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Path constraint validation            | `test_graph_advanced_features.cpp` | ✅ Exists   |
| Constrained path finding              | `test_path_constraints_direct.cpp` | ✅ Exists   |
| AQL integration                       | `test_aql_path_constraints.cpp`    | ✅ Exists   |
| Distributed traversal                 | (planned for Phase 4)              | 📋 TODO     |
| Temporal / geo-fence constraints      | (planned for Phase 5.4)            | 📋 TODO     |
| Chaos / fuzz tests                    | (planned)                          | 📋 TODO     |

---

## CI Gates

The graph module must pass the following CI checks before merging:

1. **Build** – `cmake --build` with `-DBUILD_TESTS=ON`
2. **Unit tests** – all `test_graph_*.cpp` and `test_path_constraints*.cpp` suites pass
3. **No new lint warnings** – `.clang-tidy` clean on changed files
4. **Structured error propagation** – `ERR_QUERY_TIMEOUT` returned on exceeding `timeout_ms`; `ERR_GRAPH_NO_SUCH_VERTEX` (6400) returned for unknown vertices
5. **Metrics non-zero** – `total_queries` counter incremented after each execution (including parallel BFS)
6. **Dry-run API** – `explainConstrainedPath()` returns a plan without incrementing `total_queries`
7. **Parallel BFS correctness** – `BFS_Parallel_ProducesSameResultAsSequential` test passes
8. **Metrics endpoint** – `GET /api/v1/graph/metrics` returns JSON with all 11 metric keys
9. **Adaptive cost model** – EMA updated after each execution; export/import roundtrip is lossless
10. **Node property constraint** – `addNodePropertyConstraint` prunes BFS; `validatePath` enforces for all nodes
11. **Weight constraints** – `addMaxWeight` prunes BFS states; `addMinWeight` rejects under-weight completed paths
12. **Parallel Dijkstra correctness** – `Dijkstra_Parallel_ProducesSameResultAsSequential` test passes; `totalCost` matches sequential
13. **Latency histogram** – at least one bucket non-zero after a BFS execution; `percentileMs(0.99) >= 0`
14. **Prometheus endpoint** – `GET /api/v1/graph/metrics/prometheus` returns `text/plain` with counter lines and `+Inf` bucket
15. **Rate limiter** – `setMaxQueriesPerSecond(1)` causes second rapid BFS to return `ERR_GRAPH_RATE_LIMIT_EXCEEDED`

---

## Related Documents

- [`src/graph/ADVANCED_FEATURES_README.md`](../src/graph/ADVANCED_FEATURES_README.md) – Implementation status and algorithm details
- [`src/graph/FUTURE_ENHANCEMENTS.md`](../src/graph/FUTURE_ENHANCEMENTS.md) – Feature backlog and research topics
- [`docs/gpu_roadmap.md`](gpu_roadmap.md) – GPU acceleration roadmap (includes graph kernels)
- [`include/graph/graph_query_optimizer.h`](../include/graph/graph_query_optimizer.h) – Public optimizer API
- [`include/graph/path_constraints.h`](../include/graph/path_constraints.h) – Public path-constraints API
