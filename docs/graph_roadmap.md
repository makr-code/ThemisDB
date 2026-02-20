# Graph Module – Production Readiness Roadmap

**Version:** 1.0  
**Last Updated:** February 2026  
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

## Phase 1: Observability & SLO Support (Current)

**Status:** 🚧 In Progress  
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

**Status:** ✅ Partially Complete (2.1 + 2.2 done)  
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

### 2.3 OpenAPI / Admin Endpoint

Expose `GET /api/v1/graph/metrics` returning the `GraphQueryMetrics` snapshot
as JSON for operational dashboards and alerting.

---

## Phase 3: Parallel Execution

**Status:** 📋 Planned  
**Target Version:** v1.7.0

### 3.1 Thread-Pool-Backed BFS

Partition the BFS frontier across worker threads from the core thread pool.
Uses lock-free frontier queues and atomic visited-set updates.

**Success Criteria:**
- ≥ 40% throughput improvement on graphs with > 10 K nodes
- No correctness regressions on existing tests
- Configurable via `QueryConstraints::enable_parallel` and `num_threads`

### 3.2 Parallel Dijkstra (Δ-Stepping)

Implement the Δ-stepping shortest-path algorithm for weighted graphs,
exploiting bucket-based parallelism without global locks.

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

**Status:** 📋 Planned  
**Target Version:** v1.8.0

### 5.1 Property-Based Constraints

Complete the `NODE_PROPERTY` and `EDGE_PROPERTY` constraint types in
`path_constraints.cpp` to filter by vertex/edge attribute values during
traversal.

### 5.2 Automatic Query Rewriting

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

Use the adaptive cost model from `FUTURE_ENHANCEMENTS.md` to learn per-graph
execution statistics and continuously improve algorithm selection.

---

## Observability Checklist

| Item                                | Status   | Owner        |
|-------------------------------------|----------|--------------|
| `timeout_ms` in `QueryConstraints`  | ✅ Done  | graph module |
| `GraphQueryMetrics` counters        | ✅ Done  | graph module |
| Prometheus metric names defined     | ✅ Done  | this doc     |
| Graph-specific error codes 6400+    | ✅ Done  | graph module |
| `explainConstrainedPath()` dry-run  | ✅ Done  | graph module |
| OTel span export in traversal loops | 📋 TODO  | observability|
| Heatmap: nodes-explored per query   | 📋 TODO  | observability|
| Alerting rule: error_rate > 5%      | 📋 TODO  | ops          |
| Alerting rule: p99 latency > SLO    | 📋 TODO  | ops          |
| Admin API `/api/v1/graph/metrics`   | 📋 TODO  | server       |

---

## Test Matrix

| Area                              | Tests                              | Status      |
|-----------------------------------|------------------------------------|-------------|
| Basic BFS/DFS correctness         | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Timeout / SLO enforcement         | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Aggregate metrics                 | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Graph error code values (6400+)   | `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| `explainConstrainedPath()` dry-run| `test_graph_query_optimizer.cpp`   | ✅ Exists   |
| Path constraint validation        | `test_graph_advanced_features.cpp` | ✅ Exists   |
| Constrained path finding          | `test_path_constraints_direct.cpp` | ✅ Exists   |
| AQL integration                   | `test_aql_path_constraints.cpp`    | ✅ Exists   |
| Parallel BFS                      | (planned for Phase 3)              | 📋 TODO     |
| Distributed traversal             | (planned for Phase 4)              | 📋 TODO     |
| Property-based constraints        | (planned for Phase 5)              | 📋 TODO     |
| Chaos / fuzz tests                | (planned)                          | 📋 TODO     |

---

## CI Gates

The graph module must pass the following CI checks before merging:

1. **Build** – `cmake --build` with `-DBUILD_TESTS=ON`
2. **Unit tests** – all `test_graph_*.cpp` and `test_path_constraints*.cpp` suites pass
3. **No new lint warnings** – `.clang-tidy` clean on changed files
4. **Structured error propagation** – `ERR_QUERY_TIMEOUT` returned on exceeding `timeout_ms`; `ERR_GRAPH_NO_SUCH_VERTEX` (6400) returned for unknown vertices
5. **Metrics non-zero** – `total_queries` counter incremented after each execution
6. **Dry-run API** – `explainConstrainedPath()` returns a plan without incrementing `total_queries`

---

## Related Documents

- [`src/graph/ADVANCED_FEATURES_README.md`](../src/graph/ADVANCED_FEATURES_README.md) – Implementation status and algorithm details
- [`src/graph/FUTURE_ENHANCEMENTS.md`](../src/graph/FUTURE_ENHANCEMENTS.md) – Feature backlog and research topics
- [`docs/gpu_roadmap.md`](gpu_roadmap.md) – GPU acceleration roadmap (includes graph kernels)
- [`include/graph/graph_query_optimizer.h`](../include/graph/graph_query_optimizer.h) – Public optimizer API
- [`include/graph/path_constraints.h`](../include/graph/path_constraints.h) – Public path-constraints API
