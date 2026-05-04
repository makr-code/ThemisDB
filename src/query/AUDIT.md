<!-- Status: RESOLVED | updated: 2026-05-04 (all S0+S1 findings fixed) -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

# Audit Record — Query Module

## Module Identity

| Field        | Value                                      |
|--------------|--------------------------------------------|
| Module       | query                                      |
| Source path  | `src/query/`                               |
| Audit date   | 2026-04-21                                 |
| Re-audit     | 2026-05-04                                 |
| Audited by   | Copilot (source code analysis)             |
| Status       | ✅ All S0+S1 findings resolved (2026-05-04) |

## Source File Inventory

| File | Purpose | Test Coverage |
|-------|---------|---------------|
| `adaptive_join.cpp` | Adaptive join strategy selection based on runtime statistics | ✅ Covered |
| `adaptive_optimizer.cpp` | Adaptive cost-based optimizer with runtime learning | ✅ Covered |
| `approximate_aggregator.cpp` | Approximate aggregation using HyperLogLog and Count-Min Sketch | ✅ Covered |
| `aql_parser.cpp` | Full AQL grammar parser producing AST | ✅ Covered |
| `aql_parser_json.cpp` | JSON serialization of AQL AST | ✅ Covered |
| `aql_runner.cpp` | End-to-end AQL execution pipeline | ✅ Covered |
| `aql_translator.cpp` | Cross-dialect normalization (SPARQL/SQL → AQL) | ✅ Covered |
| `cross_cluster_federation.cpp` | Cross-cluster federated AQL with cost estimation | ✅ Covered |
| `cte_cache.cpp` | CTE result caching | ✅ Covered |
| `cte_subquery.cpp` | CTE subquery evaluation | ✅ Covered |
| `cypher_parser.cpp` | Cypher graph query parser → AQL translation | ✅ Covered |
| `gremlin_parser.cpp` | Gremlin traversal parser → AQL translation | ✅ Covered |
| `let_evaluator.cpp` | LET expression evaluation | ✅ Covered |
| `materialized_cte.cpp` | Incremental view maintenance for materialized CTEs | ✅ Covered |
| `materialized_view.cpp` | Materialized view creation, refresh, and invalidation | ✅ Covered |
| `optimizer_cost_model.cpp` | Cost model with cardinality feedback | ✅ Covered |
| `parallel_executor.cpp` | Parallel query execution with work-stealing thread pool | ✅ Covered |
| `plan_cache.cpp` | Execution plan cache with parameterized plan reuse | ✅ Covered |
| `query_cache.cpp` | Exact/semantic/CTE query cache | ✅ Covered |
| `query_cache_manager.cpp` | Cache lifecycle and eviction management | ✅ Covered |
| `query_canceller.cpp` | Query cancellation via request ID | ✅ Covered |
| `query_compiler.cpp` | Query compilation to optimized execution plan | ✅ Covered |
| `query_engine.cpp` | Core multi-model execution engine | ✅ Covered |
| `query_federation.cpp` | Distributed query federation | ✅ Covered |
| `query_optimizer.cpp` | Cost-based query optimizer | ✅ Covered |
| `query_plan_visualizer.cpp` | EXPLAIN / EXPLAIN ANALYZE plan output | ✅ Covered |
| `query_profiler.cpp` | Per-query execution profiling and timing breakdown | ✅ Covered |
| `query_rewrite_rule.cpp` | Query rewrite rule engine for logical transformations | ✅ Covered |
| `result_stream.cpp` | Result streaming and pagination | ✅ Covered |
| `result_type_annotation.cpp` | Query result type annotations | ✅ Covered |
| `runtime_reoptimizer.cpp` | Runtime adaptive re-optimization | ✅ Covered |
| `semantic_cache.cpp` | Embedding-based semantic cache hit detection | ✅ Covered |
| `sparql_parser.cpp` | SPARQL parser → AQL translation | ✅ Covered |
| `sql_parser.cpp` | SQL dialect parser → AQL translation | ✅ Covered |
| `statistical_aggregator.cpp` | Statistical aggregation functions | ✅ Covered |
| `vectorized_execution.cpp` | Column-store batch processing with SIMD | ✅ Covered |
| `window_evaluator.cpp` | Window function evaluation with frame semantics | ✅ Covered |
| `workload_cache_strategy.cpp` | Workload-aware cache admission strategy | ✅ Covered |

**Total: 38 source files**

## Test Coverage

| Metric          | Value  |
|-----------------|--------|
| Line coverage   | > 80%  |
| Branch coverage | > 80%  |

## Security Audit Summary

| Control                               | Status        | Notes                                         |
|---------------------------------------|---------------|-----------------------------------------------|
| AQL injection detection               | ⚠️ Partial   | Security module detector present but bypassed via LLM path (see LLM-1/LLM-2 in aql/AUDIT.md) |
| SPARQL/SQL parse-and-translate        | ✅ Complete   | No direct dialect execution                   |
| Per-query resource limits             | ✅ Fixed      | `timeout_ms=0` default noted; result-set cap added (QE-4 fix) |
| Query cancellation                    | ✅ Complete   | Via request ID                                |
| Tenant namespace isolation            | ✅ Fixed      | `setCollectionAccessChecker()` + `ERR_QUERY_ACCESS_DENIED` gate (QE-2 fix) |
| AQLParser thread-safety               | ⚠️ Open      | Per-thread or mutex required (KL-01)          |
| Parser recursion depth limit          | ✅ Fixed      | Depth counter added (PA-1 fix, depth 500)      |
| Graph traversal depth limit           | ✅ Fixed      | `kMaxTraversalDepth = 1000` in `parseForClause()` (PA-2 fix) |
| DNF expansion size limit              | ✅ Fixed      | `kMaxDNFDisjuncts = 1000` in `aql_translator.cpp` (TR-2 fix) |
| Spatial filter bypass                 | ✅ Fixed      | TR-1: fail-closed; QE-5: ST_Within fail-closed |
| Performance benchmarks                | ❌ Pending    | Vectorized + federated paths (Q2 2026)        |

## Findings

### S0 — Critical (all resolved 2026-05-04)

#### ~~QE-1~~ · `query_engine.cpp` · `executeAndKeys()` / `executeOrKeys()` — Data race on shared `errors` vector ✅ FIXED

Added `std::mutex errors_mutex` protecting the `errors` vector in both `executeAndKeys()` and `executeOrKeys()`. Lambda captures updated accordingly.

---

#### ~~QE-2~~ · `query_engine.cpp` · All `execute*` methods — No authorization check on collection access ✅ FIXED

`setCollectionAccessChecker()` added to `QueryEngine`; `checkCollectionAccess()` called at the top of every `execute*` method. New `ERR_QUERY_ACCESS_DENIED` error code added.

---

#### ~~PA-1~~ · `aql_parser.cpp` · `parseUnary()` / `parsePrimary()` — Unbounded recursion → stack overflow ✅ FIXED

Depth counter added in `parseExpression()` with a hard limit of 500, propagated through all recursive calls.

---

### S1 — High (all resolved 2026-05-04)

| ID | Function | Description | Fix |
|----|----------|-------------|-----|
| ~~QE-3~~ | `executeOrKeysWithFallback()` | Disjunct storage errors silently swallowed | Added `fb_errors_mutex` + error vector; returns `ERR_QUERY_EXECUTION_FAILED` on any disjunct failure |
| ~~QE-4~~ | `executeAndEntities()` et al. | No result-set size cap | Added `kMaxEntityResultCap = 1'000'000` check before `reserve()` in all entity-loading methods |
| ~~QE-5~~ | `qe_evalFunction()` / `ST_Within` | Geometry parse failure returns `true` (fail-open) | Changed to `return Err<>(ERR_QUERY_EXECUTION_FAILED, "fail-closed")` |
| ~~PA-2~~ | `parseForClause()` | No upper bound on parsed graph traversal depth | `kMaxTraversalDepth = 1000` enforced before assigning `maxDepth` |
| ~~TR-1~~ | `translate()` in `aql_translator.cpp` | ST_* spatial filter silently dropped for non-literal geometry → geo-fence bypass | Changed to `return TranslationResult::Error(...)` (fail-closed) |
| ~~TR-2~~ | `translate()` in `aql_translator.cpp` | DNF cartesian product of OR-clauses is O(M^N) with no size limit → query planning OOM | Added `kMaxDNFDisjuncts = 1000` check before expansion |

---

## Open Items

| ID    | Description                                                  | Target  | Priority |
|-------|--------------------------------------------------------------|---------|----------|
| OI-01 | AQLParser thread-safety refactor                             | Planned | High     |
| ~~OI-04~~ | ~~Add recursion depth limit to all recursive-descent functions (PA-1)~~ | ✅ Done | — |
| ~~OI-05~~ | ~~Add ACL check on collection name in all execute* methods (QE-2)~~ | ✅ Done | — |
| ~~OI-06~~ | ~~Fix data race on `errors` vector in `executeAndKeys` (QE-1)~~ | ✅ Done | — |
| OI-02 | Performance benchmarks (vectorized, federated)               | Q2 2026 | High     |
| OI-03 | Full security audit (injection, resource exhaustion)         | Q2 2026 | High     |
