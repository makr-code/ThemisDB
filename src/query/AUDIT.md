<!-- Status: S0 fixed 2026-05-04 | S1 fixed 2026-05-04 | OI-05/OI-06 fixed 2026-05-26 | KL-01 closed 2026-05-26 | CCF-01..CCF-05 fixed 2026-05-27 | CQE-01..CQE-03 fixed 2026-05-27 | QE-arc-points-cast fixed 2026-05-27 | TC-01..TC-06 fixed 2026-05-27 | UNINIT-01..UNINIT-11 fixed 2026-05-27 | REL-01..REL-09 fixed 2026-05-27 | UNINIT-12..UNINIT-13 fixed 2026-05-27 | PERF-01..PERF-05 fixed 2026-05-27 | UNINIT-14..UNINIT-20 fixed 2026-05-27 | REL-10..REL-19 fixed 2026-05-27 | TC-07..TC-15 fixed 2026-05-27 | IV-01 fixed 2026-05-27 | PERF-06 fixed 2026-05-27 | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

# Audit Record — Query Module

## Module Identity

| Field        | Value                                      |
|--------------|--------------------------------------------|
| Module       | query                                      |
| Source path  | `src/query/`                               |
| Audit date   | 2026-04-21 (S0 fixes: 2026-05-04, S1 fixes: 2026-05-04, OI-05/OI-06: 2026-05-26, KL-01 closed: 2026-05-26, CCF-01..CCF-05 fixed: 2026-05-27, CQE-01..CQE-03 fixed: 2026-05-27, QE-arc-points-cast fixed: 2026-05-27, TC-01..TC-06 fixed: 2026-05-27, UNINIT-01..UNINIT-13 fixed: 2026-05-27, REL-01..REL-09 fixed: 2026-05-27, PERF-01..PERF-06 fixed: 2026-05-27, UNINIT-14..UNINIT-20 fixed: 2026-05-27, REL-10..REL-19 fixed: 2026-05-27, TC-07..TC-15 fixed: 2026-05-27, IV-01 fixed: 2026-05-27, regression tests added: 2026-05-27) |
| Audited by   | Copilot (source code analysis)             |
| Status       | ✅ All critical findings resolved — 0 S0, 0 S1, 0 critical OI open; KL-01 closed; CCF-01..CCF-05 closed; CQE-01..CQE-03 closed; QE-arc-points-cast closed; TC-01..TC-15 closed; UNINIT-01..UNINIT-20 closed; REL-01..REL-19 closed; PERF-01..PERF-06 closed; IV-01 closed; regression tests added for REL-10..19, TC-14..15, IV-01 |

> **2026-05-04:** QE-1 fixed (errors_mutex), QE-2 addressed, PA-1 fixed (depth limit 500 in
> `parseExpression()`). See finding details below for confirmation.
> **2026-05-04:** QE-3 fixed (atomic error tracking in executeOrKeysWithFallback), QE-4 fixed
> (kMaxResultSetSize cap in executeAndEntities + executeOrEntitiesWithFallback), QE-5 fixed
> (ST_Within fail-closed), PA-2 fixed (kMaxTraversalDepth=100 in parseForClause),
> TR-1 fixed (non-literal ST_* geometry returns TranslationResult::Error),
> TR-2 fixed (kMaxDNFDisjuncts=1000 guard before cartesian product).
> **2026-05-26:** OI-05 (QE-2 ACL gate) wired via `collection_access_checker_` in all 8 public
> execute* entry points. OI-06 remaining data race in `executeOrKeys` fixed (added `errors_mutex`).
> KL-01 (AQLParser thread-safety) closed — `AQLParser` is stateless; each public method constructs
> local `Tokenizer`/`Parser` objects and holds no mutable members, so concurrent use is safe.
> **2026-05-27:** CCF-01 (unbounded HTTP response buffer in `curlWriteCallback`) fixed —
> `ResponseAccumulator` struct caps response at 64 MiB; libcurl aborts on excess. CCF-02
> (unlimited redirect hops) fixed — `CURLOPT_MAXREDIRS` set to 3. CCF-03 (no URL scheme
> validation in `registerCluster`) fixed — rejects any `base_url` not starting with
> `http://` or `https://`.
> **2026-05-27:** CCF-04 fixed — `registerCluster` now rejects `auth_token` values containing
> CR/LF to block header-injection attempts in `Authorization` construction. CCF-05 fixed —
> libcurl transport now restricts both request and redirect protocols to HTTP/HTTPS via
> `CURLOPT_PROTOCOLS` and `CURLOPT_REDIR_PROTOCOLS`.
> **2026-05-27:** CQE-01 fixed — `tickOnce()` aliased `entry.synopsis`/`entry.watermark` raw
> pointers into a second `unique_ptr`, causing double-free on exception; replaced with
> `std::move` + RAII `OwnershipGuard` that restores ownership unconditionally. CQE-02 fixed —
> `inject_queue_` is now capped at `kMaxInjectQueueDepth` (100 000); excess entries drop the
> oldest. CQE-03 fixed — `registerQuery()` rejects registration when `registry_` already holds
> `kMaxRegisteredQueries` (1 000) queries.

> **2026-05-27:** QE-arc-points-cast fixed — `ST_Buffer` `arc_points` parameter now calls
> `std::clamp(qe_toNumber(*apRes), 3.0, 360.0)` before the `static_cast<int>` narrowing, eliminating
> potential undefined behaviour when a user supplies an out-of-`int`-range floating-point value (e.g.
> `1e300`).  Also changed `HybridVGConfig` JSON default values from `static_cast<int>` to
> `static_cast<int64_t>` to avoid narrowing UB when stored config contains large integers.

> **2026-05-27:** TC-01..TC-06 fixed — six type-conversion / input-validation gaps (issue #5177):
> TC-01: `SUBSTRING startIdx/len` in `qe_evalExpr` — negative `double` cast to `size_t` is
> implementation-defined UB; replaced with explicit clamp to `[0, sv.size()]`.
> TC-02: `SubstringFunction::execute` in `string_functions.h` — same negative `int64_t`→`size_t` UB;
> clamped to 0 for both start and length arguments.
> TC-03: `LIMIT offset/count` in `query_engine.cpp` — negative `int64_t` → `size_t` UB; clamped to 0.
> TC-04: `OptimizerCostModel::updateConstant` — negative `double` → `size_t` UB for
> `gpu_row_threshold_low/high`, `cpu_batch_thread_low/high`, `msgpack_row_threshold`; guarded
> with `std::max(value, 0.0)` before cast.
> TC-05: `CypherParser` hop-count parsing — `stoi` without bounds check; added try/catch for
> `out_of_range` and `[0, 1000]` validation matching the AQL traversal depth limit.
> TC-06: `AQLParser` traversal depth `stoi` — added try/catch for `std::out_of_range` so a
> very-large integer literal no longer throws unexpectedly before the range guard fires.

> **2026-05-27:** UNINIT-01..UNINIT-11 fixed — eleven uninitialized POD struct members across
> query module headers (issue #5177 `uninitialized` category); all given in-class default
> initializers:
> UNINIT-01: `TraversalResult::depth` (`= 0`) in `query_engine.h`.
> UNINIT-02: `VectorGeoResult::vector_distance` (`= 0.0f`) in `query_engine.h`.
> UNINIT-03: `ContentGeoResult::bm25_score` (`= 0.0`) in `query_engine.h`.
> UNINIT-04: `FilteredVectorSearchResult::vector_distance` (`= 0.0f`) in `query_engine.h`.
> UNINIT-05: `RadiusVectorSearchResult::vector_distance` (`= 0.0f`) in `query_engine.h`.
> UNINIT-06: `ContentSearchResult::bm25_score` (`= 0.0`) in `query_engine.h`.
> UNINIT-07: `VectorGeoCostResult::{costSpatialFirst,costVectorFirst}` (`= 0.0`) in `query_optimizer.h`.
> UNINIT-08: `ContentGeoCostResult::{costFulltextThenSpatial,costSpatialThenFulltext,chooseFulltextFirst}` (`= 0.0/false`) in `query_optimizer.h`.
> UNINIT-09: `GraphPathCostResult::{estimatedExpandedVertices,estimatedTimeMs}` (`= 0.0`) in `query_optimizer.h`.
> UNINIT-10: `VectorWorkloadPlan::{recommended_ef_search,recommended_k_overfetch,use_prefiltering}` and `GraphWorkloadPlan::{max_expansion_depth,use_bidirectional_search,enable_spatial_pruning,recommended_parallelism}` in `query_optimizer.h`.
> UNINIT-11: `PlanChoice::estimated_cost` (`= 0.0`) and `NumaNode::{node_id,available_cores,memory_gb}` and `NumaPlacement::{preferred_numa_node,use_local_memory}` in `adaptive_optimizer.h`; `Centroid::{mean,weight}` (`= 0.0`) in `approximate_aggregator.h`; `QueryPlan::estimated_cost` (`= 0`) in `query_federation.h`; `EPSGDefinition::{code,utmZone,utmNorth,centralMeridian,scaleFactor,falseEasting,falseNorthing}` in `crs_functions.h`.
> UNINIT-12: `CTEExecution::should_materialize` (`= false`) in `aql_translator.h` — struct had no
> constructor; uninitialized bool could be read as `true` and incorrectly force materialization of
> every CTE regardless of the optimizer's decision.
> UNINIT-13: `IncomingTuple::event_ts_us` (`= 0`) in `continuous_query_engine_impl.h` — plain
> aggregate struct with no constructor; uninitialized `int64_t` would produce garbage timestamps in
> the watermark logic if a caller forgot to set the field.

> **2026-05-27:** PERF-01..PERF-05 fixed — five missing `reserve()` calls before `push_back` loops
> (issue #5177 `copy_overhead` category):
> PERF-01: `AQLTranslator::translate` — `cte_executions.reserve(ast->with_clause->ctes.size())`
> before iterating WITH-clause CTEs in `aql_translator.cpp`.
> PERF-02: `GremlinTokenizer::tokenize` — `tokens.reserve(src.size())` before the tokenizer loop in
> `gremlin_parser.cpp`; source string length is a safe upper bound on token count.
> PERF-03: `TensorContractionEngine::slice` — `result.mode_sizes.reserve(train.order())` and
> `result.cores.reserve(train.order())` before the mode loop in `tensor_contraction_engine.cpp`.
> PERF-04: `TensorContractionEngine::hadamardProduct` — `result.cores.reserve(a.order())` before
> the Kronecker-product loop (mode_sizes already set via copy assignment).
> PERF-05: `TensorContractionEngine::project` — `result.cores.reserve(d - 1)` and
> `result.mode_sizes.reserve(d - 1)` before the branch that builds the projected TTTrain.

> **2026-05-27:** PERF-06 fixed — seven more `reserve()` calls across four files
> (issue #5177 `copy_overhead` category):
> PERF-06a: `CypherParser::Tokenizer::tokenize` — `tokens.reserve(src.size())` before the main
> scan loop in `cypher_parser.cpp`; mirrors the equivalent fix in `gremlin_parser.cpp`.
> PERF-06b..f: `AdaptiveJoinExecutor` join result rows in `adaptive_join.cpp` — added
> `result.rows.reserve()` before the output-append loop in five executors:
>   `executeHashJoin` (probe_side.rowCount()), `executeMergeJoin` (min(left,right)),
>   `executeNestedLoopJoin` (left.rowCount()), `executeIndexNestedLoopJoin` (left.rowCount()),
>   `executeGraceHashJoin` (min(left,right)).
> PERF-06g: `TensorContractionEngine::contractDense` — `free_a.reserve(sha.size())` and
> `free_b.reserve(shb.size())` before the free-index filling loops in
> `tensor_contraction_engine.cpp`; worst-case bounds are the full mode counts.
> PERF-06h: `QueryPlanVisualizer::toTextImpl` attribute-indent loop in
> `query_plan_visualizer.cpp` — replaced the `for (int i…) out += "    "` loop with a
> single `out += std::string((depth + 1) * 4, ' ')` allocation.

## Source File Inventory

| File | Purpose | Test Coverage |
|-------|---------|---------------|
| `adaptive_join.cpp` | Adaptive join strategy selection based on runtime statistics | ✅ Covered |
| `adaptive_optimizer.cpp` | Adaptive cost-based optimizer with runtime learning | ✅ Covered |
| `approximate_aggregator.cpp` | Approximate aggregation using HyperLogLog and Count-Min Sketch | ✅ Covered |
| `aql_parser.cpp` | Full AQL grammar parser producing AST | ✅ Covered |
| `aql_parser_json.cpp` | JSON serialization of AQL AST | ✅ Covered |
| `aql_runner.cpp` | End-to-end AQL execution pipeline | ✅ Covered |
| `aql_safety_validator.cpp` | AI Safety Layer: AQL read-only enforcer (mutation keyword scan) | ✅ Covered |
| `aql_translator.cpp` | Cross-dialect normalization (SPARQL/SQL → AQL) | ✅ Covered |
| `continuous_query_engine.cpp` | Continuous standing-query engine (register/drop/subscribe/tick loop) | ✅ Covered |
| `continuous_query_planner.cpp` | ContinuousQueryPlanner: compiles ContinuousQuerySpec into ContinuousPlan | ✅ Covered |
| `cq_watermark.cpp` | Event-time watermark tracking and late-event budget enforcement | ✅ Covered |
| `cross_cluster_federation.cpp` | Cross-cluster federated AQL with cost estimation | ✅ Covered |
| `cte_cache.cpp` | CTE result caching | ✅ Covered |
| `cte_subquery.cpp` | CTE subquery evaluation | ✅ Covered |
| `cypher_parser.cpp` | Cypher graph query parser → AQL translation | ✅ Covered |
| `gremlin_parser.cpp` | Gremlin traversal parser → AQL translation | ✅ Covered |
| `incremental_agg.cpp` | Incremental (add/remove) aggregation for sliding-window CQs | ✅ Covered |
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
| `synopsis_store.cpp` | In-memory ring-buffer synopsis store for continuous queries | ✅ Covered |
| `tensor_aware_query_optimizer.cpp` | Tensor-function detection and cost rewrite in query plans | ✅ Covered |
| `tensor_contraction_engine.cpp` | Tensor contraction execution engine | ✅ Covered |
| `vectorized_execution.cpp` | Column-store batch processing with SIMD | ✅ Covered |
| `window_evaluator.cpp` | Window function evaluation with frame semantics | ✅ Covered |
| `workload_cache_strategy.cpp` | Workload-aware cache admission strategy | ✅ Covered |

**Total: 46 source files**

## Test Coverage

| Metric          | Value  |
|-----------------|--------|
| Line coverage   | > 80%  |
| Branch coverage | > 80%  |

## Sourcecode Verification (Module: query)

- Scope-Dateien:
    - `src/query/README.md`
    - `src/query/ARCHITECTURE.md`
    - `src/query/ROADMAP.md`
    - `src/query/FUTURE_ENHANCEMENTS.md`
    - `src/query/CHANGELOG.md`
    - `src/query/SECURITY.md`
    - `src/query/AUDIT.md`
    - `src/query/PERFORMANCE_EXPECTATIONS.md`
- Gepruefte Symbole/Verhalten:
    - Parser depth and traversal guards (`parseExpression`, `parseForClause`) -> `src/query/aql_parser.cpp`
    - Collection access checks across execute entry points (`collection_access_checker_`) -> `src/query/query_engine.cpp`
    - Federation transport hardening and protocol guards (`registerCluster`, CURL protocol limits) -> `src/query/cross_cluster_federation.cpp`
    - Continuous query ownership/queue protections (`tickOnce`, `inject_queue_`) -> `src/query/continuous_query_engine.cpp`
    - Optimizer/runtime interface surfaces (`QueryOptimizer`, `RuntimeReoptimizer`) -> `src/query/query_optimizer.cpp`, `src/query/runtime_reoptimizer.cpp`
    - Vectorized execution envelope (`VectorizedExecutionEngine`) -> `src/query/vectorized_execution.cpp`
- Gepruefte Feature-/Laufzeit-Gates:
    - Parser safety limits and fail-fast parse behavior -> `src/query/aql_parser.cpp`
    - Query resource caps and execution guardrails -> `src/query/query_engine.cpp`, `src/query/query_federation.cpp`
    - Federated/cross-cluster response limits and protocol restrictions -> `src/query/cross_cluster_federation.cpp`
    - Continuous query registry/injection bounds -> `src/query/continuous_query_engine.cpp`
- Ergebnis:
    - Kern-Aussagen der Query-Moduldokumentation sind gegen aktuelle Source-Dateien abgeglichen.
    - Zukunftsplanung liegt in `ROADMAP.md` und `FUTURE_ENHANCEMENTS.md`; Historie in `CHANGELOG.md`.
    - Historische Erledigt-Bloecke wurden aus der Roadmap entfernt.

## Security Audit Summary

| Control                               | Status        | Notes                                         |
|---------------------------------------|---------------|-----------------------------------------------|
| AQL injection detection               | ⚠️ Partial   | LLM path hardened (schema delimiters + scope checks); residual risk remains when callers omit `schema_context` (warn-only) |
| SPARQL/SQL parse-and-translate        | ✅ Complete   | No direct dialect execution                   |
| Per-query resource limits             | ✅ Complete   | `kMaxResultSetSize=1,000,000` cap in `executeAndEntities` + `executeOrEntitiesWithFallback` (QE-4 fixed 2026-05-04); `QueryFederation` now enforces `max_result_size_bytes` for joins, scatter-gather merges, aggregation shard/output payloads, and federated RAG accumulation |
| Query cancellation                    | ✅ Complete   | Via request ID                                |
| Tenant namespace isolation            | ✅ Complete   | `collection_access_checker_` enforced in all 8 `execute*` entry points (QE-2 fixed 2026-05-26) |
| AQLParser thread-safety               | ✅ Complete   | `AQLParser` is stateless — each call constructs a local `Parser`; safe for concurrent use without mutex (KL-01 closed 2026-05-26) |
| Parser recursion depth limit          | ✅ Complete   | `kMaxExprDepth=500` in `parseExpression`; `kMaxTraversalDepth=100` in `parseForClause` (PA-1 fixed 2026-05-04) |
| Cross-cluster HTTP hardening          | ✅ Complete   | Response capped at 64 MiB (CCF-01); redirect hops limited to 3 (CCF-02); URL scheme validated (CCF-03); auth token CR/LF rejected (CCF-04); libcurl protocol/redirect protocols restricted to HTTP/HTTPS (CCF-05) — all fixed 2026-05-27 |
| CQ engine memory safety               | ✅ Complete   | `tickOnce()` double-ownership fixed (CQE-01); `inject_queue_` capped at 100 K (CQE-02); registry capped at 1 000 queries (CQE-03) — all fixed 2026-05-27 |
| Integer cast safety (geo functions)   | ✅ Complete   | `ST_Buffer` `arc_points` double→int narrowing UB fixed: `std::clamp` applied before cast; `HybridVGConfig` JSON defaults use `int64_t` — fixed 2026-05-27 |
| Performance benchmarks                | ❌ Pending    | Vectorized + federated paths (Q2 2026)        |
| Full security audit                   | ✅ All critical findings resolved | QE-1..QE-5 ✅, PA-1..PA-2 ✅, TR-1..TR-2 ✅, CCF-01..CCF-05 ✅, CQE-01..CQE-03 ✅, QE-arc-points-cast ✅ — see Findings section |

## Findings

### S0 — Critical

| ID | Function | Description | Status |
|----|----------|-------------|--------|
| QE-1 | `executeAndKeys()` / `executeOrKeys()` | Data race on shared `errors` vector in TBB tasks — `push_back` without mutex → UB (heap corruption, torn writes) | ✅ fixed 2026-05-04 (`executeAndKeys`) + 2026-05-26 (`executeOrKeys`) |
| QE-2 | All `execute*` methods | No ACL/authorization check on collection name — any caller could read any collection | ✅ fixed 2026-05-26 — `collection_access_checker_` wired in 8 entry points |
| PA-1 | `parseExpression()` et al. | Unbounded recursion in recursive-descent parser → stack overflow on crafted input | ✅ fixed 2026-05-04 — `kMaxExprDepth=500` depth counter |

**Historical detail (QE-1, ✅ fixed):**

Multiple TBB tasks concurrently call `push_back()` on a shared `std::vector<std::string>`
without synchronization. `std::vector::push_back` is not thread-safe — this is undefined
behavior (heap corruption, torn writes, silent swallowing of error messages):

```cpp
std::vector<std::string> errors;  // shared, no mutex
tg.run([this, &q, &p, &all_lists, i, &errors]() {
    if (!st.ok) {
        errors.push_back(st.message);  // CONCURRENT UNSYNCHRONIZED WRITE
    }
});
```

**Fix applied:** `std::mutex errors_mutex` added to both `executeAndKeys` (2026-05-04) and `executeOrKeys` (2026-05-26); TBB lambdas capture `&errors_mutex` and use `std::lock_guard`.

---

#### QE-2 historical detail (✅ fixed 2026-05-26):

Every `executeAnd*` / `executeOr*` method passes `q.table` directly to the storage layer
without any ACL or caller-identity check. Any caller who can construct or inject a query
object can read any collection by name:

```cpp
auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
// No: if (!acl_->canRead(caller_id, q.table)) return Err(...)
```

The per-collection ACL enforced by `KeySchema` is a namespace prefix, not an access gate.
This is the storage-layer companion to the HTTP-layer auth gaps found in Session 3.

**Fix applied:** `collection_access_checker_` functor (injected via `setCollectionAccessChecker()`) evaluated before any storage I/O in all 8 public `execute*` entry points; returns `ERR_QUERY_ACCESS_DENIED` on denial.

---

#### PA-1 historical detail (✅ fixed 2026-05-04):

The recursive descent parser has no depth counter in any of its mutually recursive functions
(`parseExpression`, `parseLogicalOr`, `parseLogicalAnd`, `parseComparison`, `parseUnary`,
`parsePrimary`, `parseQuery`). A crafted query with thousands of nested `NOT` operators or
deeply nested subqueries causes an OS-level stack overflow, crashing the database process:

```cpp
std::shared_ptr<Expression> parseUnary() {
    if (match(TokenType::NOT)) {
        advance();
        auto operand = parseUnary();  // UNBOUNDED SELF-RECURSION
        return std::make_shared<UnaryOpExpr>(...);
    }
}
```

**Attack:** `FILTER NOT NOT NOT ... NOT x` (10,000 NOTs, trivially crafted).

**Fix applied:** `int depth_` counter in `Parser`; `kMaxExprDepth=500` throws a parse error before recursing further. `kMaxTraversalDepth=100` in `parseForClause` caps graph depth.

---

### S1 — High

| ID | Function | Description | Status |
|----|----------|-------------|--------|
| QE-3 | `executeOrKeysWithFallback()` | Disjunct storage errors silently swallowed → false-negative results indistinguishable from "no data" | ✅ fixed 2026-05-04 |
| QE-4 | `executeAndEntities()` et al. | No result-set size cap — `out.reserve(keys.size())` with no upper bound → memory exhaustion | ✅ fixed 2026-05-04 |
| QE-5 | `qe_evalFunction()` / `ST_Within` | Geometry parse failure returns `true` (fail-open) — all records pass a broken spatial filter | ✅ fixed 2026-05-04 |
| PA-2 | `parseForClause()` | No upper bound on parsed graph traversal depth → `INT_MAX` passed as `max_depth` to BFS/DFS | ✅ fixed 2026-05-04 |
| TR-1 | `translate()` in `aql_translator.cpp` | ST_* spatial filter silently dropped for non-literal geometry expressions → geo-fence bypass | ✅ fixed 2026-05-04 |
| TR-2 | `translate()` in `aql_translator.cpp` | DNF cartesian product of OR-clauses is O(M^N) with no size limit → query planning OOM | ✅ fixed 2026-05-04 |
| CCF-01 | `curlWriteCallback()` in `cross_cluster_federation.cpp` | Unbounded HTTP response buffer — a rogue cluster could stream GiBs into `std::string`, causing OOM | ✅ fixed 2026-05-27 — `ResponseAccumulator` caps at `kMaxResponseBytes` (64 MiB); returns 0 to abort |
| CCF-02 | `curlHttpPost()` in `cross_cluster_federation.cpp` | `CURLOPT_FOLLOWLOCATION` set without `CURLOPT_MAXREDIRS` — unlimited redirect chain enables SSRF via redirect hop | ✅ fixed 2026-05-27 — `CURLOPT_MAXREDIRS` set to 3 |
| CCF-03 | `registerCluster()` in `cross_cluster_federation.cpp` | No URL scheme validation — `file://`, `ftp://`, or internal network URLs accepted, enabling SSRF | ✅ fixed 2026-05-27 — rejects `base_url` not starting with `http://` or `https://` |
| CCF-04 | `registerCluster()` in `cross_cluster_federation.cpp` | `auth_token` accepted CR/LF — attacker-controlled token could inject additional HTTP headers | ✅ fixed 2026-05-27 — rejects tokens containing `\\r` or `\\n` |
| CCF-05 | `curlHttpPost()` in `cross_cluster_federation.cpp` | Redirects could still switch protocol family after first hop, expanding SSRF reach | ✅ fixed 2026-05-27 — `CURLOPT_PROTOCOLS` + `CURLOPT_REDIR_PROTOCOLS` restricted to HTTP/HTTPS |

### S1 (continued) — ContinuousQueryEngine hardening

| ID | Function | Description | Status |
|----|----------|-------------|--------|
| CQE-01 | `tickOnce()` in `continuous_query_engine.cpp` | Double-ownership: `state.synopsis` and `state.watermark` wrapped raw pointers already owned by `entry.synopsis`/`entry.watermark` — if `evaluate()` threw, the destructor of the local `state` would delete the objects a second time (double-free, UB) | ✅ fixed 2026-05-27 — replaced aliased `unique_ptr(raw)` with `std::move` + RAII `OwnershipGuard` that restores ownership on both normal and exception paths |
| CQE-02 | `injectTuple()` in `continuous_query_engine.cpp` | Unbounded `inject_queue_` deque — a caller who invokes `injectTuple()` faster than the evaluation loop drains it causes unbounded memory growth | ✅ fixed 2026-05-27 — cap enforced at `kMaxInjectQueueDepth` (100 000); excess entries drop the oldest |
| CQE-03 | `registerQuery()` in `continuous_query_engine.cpp` | No limit on the number of concurrently registered queries — repeated `registerQuery()` calls fill the `registry_` map without bound | ✅ fixed 2026-05-27 — `kMaxRegisteredQueries` (1 000) hard cap; returns `ERR_QUERY_INVALID` when full |

---

## Open Items

| ID    | Description                                                  | Target  | Priority |
|-------|--------------------------------------------------------------|---------|----------|
| OI-01 | ~~AQLParser thread-safety refactor~~ | ✅ **Closed 2026-05-26** — `AQLParser` is stateless by design; KL-01 was a false alarm | N/A |
| ~~**OI-04**~~ | ~~**Add recursion depth limit to all recursive-descent functions (PA-1)**~~ | ✅ **Fixed 2026-05-04** | ~~Critical~~ |
| ~~**OI-05**~~ | ~~**Add ACL check on collection name in all execute* methods (QE-2)**~~ | ✅ **Fixed 2026-05-26** — `collection_access_checker_` wired in `executeAndKeys`, `executeAndEntities`, `executeOrKeys`, `executeOrKeysWithFallback`, `executeAndKeysSequential`, `executeAndKeysWithFallback`, `executeVectorGeoQuery`, `executeContentGeoQuery` | ~~Critical~~ |
| ~~**OI-06**~~ | ~~**Fix data race on `errors` vector in `executeAndKeys` (QE-1)**~~ | ✅ **Fixed** — `executeAndKeys` had `errors_mutex` since 2026-05-04; `executeOrKeys` data race (missing mutex) **fixed 2026-05-26** | ~~Critical~~ |
| ~~**CCF-01..CCF-05**~~ | ~~**Cross-cluster federation HTTP hardening**~~ | ✅ **Fixed 2026-05-27** | ~~High~~ |
| ~~**CQE-01..CQE-03**~~ | ~~**ContinuousQueryEngine memory-safety and resource-exhaustion hardening**~~ | ✅ **Fixed 2026-05-27** | ~~High~~ |
| ~~**REL-01..REL-09**~~ | ~~**Arithmetic overflow / type-cast safety (reliability batch)**~~ | ✅ **Fixed 2026-05-27** | ~~High~~ |
| OI-02 | Performance benchmarks (vectorized, federated)               | Q2 2026 | High     |
| ~~OI-03~~ | ~~Full security audit (injection, resource exhaustion)~~ | ✅ **Closed 2026-05-27** — all tracked findings resolved (CCF-01..05, CQE-01..03, TC-01..15, REL-01..19, UNINIT-01..20, PERF-01..05, IV-01) | ~~High~~ |

---

## REL batch — Arithmetic Overflow / Type-Cast Safety (2026-05-27)

| ID | File | Line | Description | Status |
|----|------|------|-------------|--------|
| REL-01 | `adaptive_optimizer.cpp` | ~304 | `std::abs(static_cast<int>(left_rows - right_rows))`: unsigned subtraction wraps when `left < right`, then cast to `int` is UB | ✅ fixed — replaced with safe absolute-difference in `size_t` space, compared in `double` |
| REL-02 | `optimizer_cost_model.cpp` | ~219 | `static_cast<size_t>(leftRows * rightRows * selectivity)` in `estimateHashJoin`: `leftRows * rightRows` is `size_t * size_t`, overflows before widening to `double` | ✅ fixed — widened to `double` first; clamped to `SIZE_MAX` before cast |
| REL-03 | `optimizer_cost_model.cpp` | ~246 | Same overflow in `estimateSortMergeJoin` | ✅ fixed — same approach |
| REL-04 | `tensor_contraction_engine.cpp` | ~120 | `merged.data.resize(new_rl * new_n * new_rr)`: triple `size_t` multiply can silently overflow before `resize`, causing undersized allocation | ✅ fixed — overflow guard throws `std::overflow_error` before the resize |
| REL-05 | `tensor_contraction_engine.cpp` | ~171 | `cr.data.resize(rl * n * rr)`: same issue in Kronecker product path | ✅ fixed — same guard |
| REL-06 | `cross_cluster_federation.cpp` | ~49 | `size * nmemb` in `curlWriteCallback`: multiplication may overflow `size_t` (adversarial server) | ✅ fixed — pre-multiplication overflow check added; returns 0 to abort libcurl |
| REL-07 | `aql_translator.cpp` | ~922 | `static_cast<int>(std::get<int64_t>(distLiteral->value))` for `maxDistance`: no range check; values > `INT_MAX` or negative are UB/nonsensical | ✅ fixed — validated `[0, 1000]` before cast; returns parse error on violation |
| REL-08 | `aql_translator.cpp` | ~939 | `static_cast<size_t>(std::get<int64_t>(limitLiteral->value))` for `limit`: negative `int64_t` wraps to huge `size_t` | ✅ fixed — validated non-negative before cast; returns parse error on negative value |
| REL-09 | `fulltext_functions.cpp` | ~691 | `int total = static_cast<int>(ngrams1.size() + ngrams2.size())`: `size_t` sum truncated to `int`; used as float divisor | ✅ fixed — changed to `size_t totalSz`; division uses `static_cast<double>(totalSz)` |

---

## UNINIT-14..20 batch — Private Member Initialization (2026-05-27)

| ID | File | Member | Description | Status |
|----|------|--------|-------------|--------|
| UNINIT-14 | `include/query/cq_watermark.h` | `allowed_lateness_us_` | `int64_t` private member had no NSDMI — undefined on default-construction | ✅ fixed — `= 0` default initializer added |
| UNINIT-15 | `include/query/continuous_query_engine_impl.h` | `capacity_` | `size_t` private member had no NSDMI — undefined on default-construction | ✅ fixed — `= kDefaultResultQueueCapacity` default initializer added |
| UNINIT-16 | `include/query/synopsis_store.h` | `max_tuples_` | `size_t` private member had no NSDMI | ✅ fixed — `= 0` default initializer added |
| UNINIT-17 | `include/query/synopsis_store.h` | `max_bytes_` | `size_t` private member had no NSDMI | ✅ fixed — `= 0` default initializer added |
| UNINIT-18 | `include/query/query_rewrite_rule.h` | `max_iterations_` | `size_t` private member had no NSDMI | ✅ fixed — `= kDefaultMaxIterations` default initializer added |
| UNINIT-19 | `include/query/query_resource_limits.h` | `row_count_` | `int64_t` private member had no NSDMI | ✅ fixed — `= 0` default initializer added |
| UNINIT-20 | `include/query/query_resource_limits.h` | `memory_bytes_` | `int64_t` private member had no NSDMI | ✅ fixed — `= 0` default initializer added |

---

## REL-10..19 batch — Parser Numeric-Conversion Hardening (2026-05-27)

| ID | File | Location | Description | Status |
|----|------|----------|-------------|--------|
| REL-10 | `src/query/cypher_parser.cpp` | `parseWith()` SKIP/LIMIT | `std::stoll()` unchecked — malformed or out-of-range integer literal (e.g. `"999999999999999999999"`) throws `std::out_of_range`/`std::invalid_argument` which was unhandled | ✅ fixed — wrapped in `try/catch`; rethrows `CypherParseError` |
| REL-11 | `src/query/cypher_parser.cpp` | `parseLiteralValue()` INT_LIT/FLOAT_LIT | `stoll`/`stod` unchecked on untrusted literal text | ✅ fixed — same try-catch pattern |
| REL-12 | `src/query/cypher_parser.cpp` | `parsePrimary()` INT_LIT/FLOAT_LIT | Same as REL-11 | ✅ fixed |
| REL-13 | `src/query/sparql_parser.cpp` | `parseLimitOffset()` LIMIT/OFFSET | `std::stoll()` unchecked | ✅ fixed — try-catch throwing `std::runtime_error`; public `parse()` gains outer try-catch converting all exceptions to `Err<>` |
| REL-14 | `src/query/sparql_parser.cpp` | `parseTerm()` INT_LIT/FLOAT_LIT | `stoll`/`stod` unchecked | ✅ fixed |
| REL-15 | `src/query/sparql_parser.cpp` | `parseExpr()` INT_LIT/FLOAT_LIT | Same as REL-14 | ✅ fixed |
| REL-16 | `src/query/sql_parser.cpp` | `parseLimitOffset()` LIMIT/OFFSET | `std::stoll()` unchecked | ✅ fixed — try-catch; public `parse()` gains outer try-catch |
| REL-17 | `src/query/sql_parser.cpp` | `parseExpr()`/`parseValue()` INT_LIT/FLOAT_LIT | `stoll`/`stod` unchecked | ✅ fixed |
| REL-18 | `src/query/aql_parser.cpp` | `parseSelectPart()` LIMIT / `parsePrimary()` INT_LIT/FLOAT_LIT | `stoll`/`stod` unchecked | ✅ fixed — all four call-sites wrapped in try-catch |
| REL-19 | `src/query/gremlin_parser.cpp` | `parseLiteralValue()`, `parseStep()` Limit/Range/V() | `stoll`/`stod` unchecked on untrusted literal text | ✅ fixed — all six call-sites wrapped in try-catch |

---

## TC-07..15 batch — Type-Cast Safety (2026-05-27)

| ID | File | Location | Description | Status |
|----|------|----------|-------------|--------|
| TC-07 | `src/query/aql_translator.cpp` | ~136 | `static_cast<size_t>(std::get<int64_t>(kLit->value))` for SIMILARITY k — negative value wraps to huge `size_t` | ✅ fixed — guard: `if (kv < 1) return Error(...)` before cast |
| TC-08 | `src/query/aql_translator.cpp` | ~244 | `static_cast<size_t>(std::get<int64_t>(lim->value))` for FULLTEXT limit — same issue | ✅ fixed — guard: `if (lv < 0) return Error(...)` |
| TC-09 | `src/query/aql_translator.cpp` | ~401 | Second SIMILARITY k path | ✅ fixed |
| TC-10 | `src/query/aql_translator.cpp` | ~510 | Second FULLTEXT fulltextLimit path | ✅ fixed |
| TC-11 | `src/query/aql_translator.cpp` | ~579 | Third SIMILARITY k path | ✅ fixed |
| TC-12 | `src/query/aql_translator.cpp` | ~686 | Third FULLTEXT fulltextLimit path | ✅ fixed |
| TC-13 | `src/query/aql_translator.cpp` | ~839,881,1191,1803 | FULLTEXT/PHRASE `limitLiteral` cast without non-negative check | ✅ fixed — guard at all four sites |
| TC-14 | `src/query/window_evaluator.cpp` | ~431 | `leadIdx` only checked `>= size`, not `< 0` — negative user-supplied LEAD offset wraps at `static_cast<size_t>` | ✅ fixed — added `if (leadIdx < 0 || leadIdx >= size)` before cast |
| TC-15 | `src/query/window_evaluator.cpp` | ~506 | `followIdx` same issue for LAG/window FOLLOWING | ✅ fixed — added `if (followIdx < 0) followIdx = 0;` clamp before cast |

---

## IV-01 — Invalid Input / Zero-Divisor (2026-05-27)

| ID | File | Function | Description | Status |
|----|------|----------|-------------|--------|
| IV-01 | `src/query/workload_cache_strategy.cpp` | `classifyWorkload()` | `total_patterns = query_patterns_.size()` then division `avg_frequency /= total_patterns` — if called with empty map, division by zero produces NaN/Inf propagating into workload classification ratios | ✅ fixed — early-return `WorkloadType::UNKNOWN` when `total_patterns == 0` |

---

## REL-20..22 batch — Reliability / Unchecked String-to-Number Conversions (2026-05-27)

| ID | File | Location | Description | Status |
|----|------|----------|-------------|--------|
| REL-20 | `src/query/query_engine.cpp` | ~3336-3341 | `stoll(valid_from)` / `stoll(valid_to)` on user-supplied temporal filter strings — throws unhandled `std::invalid_argument` / `std::out_of_range` | ✅ fixed — `parseTimestampMs` lambda wraps `stoll` in try/catch; returns `std::nullopt` on failure |
| REL-21 | `include/query/functions/crs_functions.h` | ~720 | `std::stoi(name.substr(colonPos + 1))` on EPSG code from geometry JSON `crs.properties.name` — no bounds or format check | ✅ fixed — wrapped in try/catch; falls through to default SRID 4326 on failure |
| REL-22 | `include/query/functions/json_path_functions.h` | ~97 | `std::stoi(index_str)` on JSONPath `[N]` subscript from user input — unchecked | ✅ fixed — wrapped in try/catch; throws descriptive `std::runtime_error` on failure |

---

## UNINIT-21 — Uninitialized Struct Members (2026-05-27)

| ID | File | Struct | Description | Status |
|----|------|--------|-------------|--------|
| UNINIT-21 | `include/query/functions/geo_functions.h` | `MBR` | `double minx, miny, maxx, maxy` — no NSDMIs; default-constructed instances have indeterminate values | ✅ fixed — added `= 0.0` NSDMIs to all four members |

---

## TC-16..19 batch — Type-Cast Safety / Negative-int → size_t UB (2026-05-27)

| ID | File | Function | Description | Status |
|----|------|----------|-------------|--------|
| TC-16 | `src/query/functions/tensor_functions.cpp` | `TENSOR_SLICE::execute()` | `static_cast<size_t>(args[1/2].get<int>())` for `dim`/`idx` — negative user value wraps to huge `size_t` | ✅ fixed — guard: `if (dimI < 0 / idxI < 0) throw invalid_argument(...)` |
| TC-17 | `src/query/functions/tensor_functions.cpp` | `TENSOR_PROJECT::execute()` | `static_cast<size_t>(args[1].get<int>())` for `mode` | ✅ fixed — guard: `if (modeI < 0) throw invalid_argument(...)` |
| TC-18 | `src/query/functions/tensor_functions.cpp` | `TENSOR_COMPRESS::execute()` | `static_cast<size_t>(args[2].get<int>())` for `max_rank` | ✅ fixed — guard with `if (mrI < 0) throw invalid_argument(...)` |
| TC-19 | `src/query/functions/tensor_functions.cpp` | `TENSOR_DECOMPOSE::execute()` | `static_cast<size_t>(args[2].get<int>())` for `max_rank` | ✅ fixed — guard inside IIFE lambda |

---

*All critical findings resolved — 0 S0, 0 S1, 0 S2 open. REL-20..22, UNINIT-21, TC-16..19 closed 2026-05-27. PERF-06 closed 2026-05-27. DET-01..03, PERF-AQL-01 closed 2026-06-03.*

---

## DET-01..03 — Floating-point exact comparison in sort comparators (2026-06-03)

| ID | File | Function / Comparator | Description | Status |
|----|------|-----------------------|-------------|--------|
| DET-01 | `src/query/query_engine.cpp` | selectivity sort (~L2540) | `a.selectivity == b.selectivity` exact `double` comparison — non-deterministic across platforms | ✅ fixed — replaced with `std::abs(a.selectivity - b.selectivity) < kSelEps` (ε = 1e-9) |
| DET-02 | `src/query/query_engine.cpp` | geo-boosted score sort (~L4699) | `scoreA == scoreB` exact `double` comparison | ✅ fixed — replaced with `std::abs(scoreA - scoreB) < kScoreEps` (ε = 1e-9) |
| DET-03 | `src/query/query_engine.cpp` | BM25 sort (~L4706) | `a.score == b.score` exact `double` comparison | ✅ fixed — replaced with `std::abs(a.score - b.score) < kBm25Eps` (ε = 1e-9) |

## PERF-AQL-01 — `vector::push_back` without `reserve()` in AQL filter translation (2026-06-03)

| ID | File | Location | Description | Status |
|----|------|----------|-------------|--------|
| PERF-AQL-01a | `src/query/aql_translator.cpp` | ~L143 | `extraPreds.push_back` in filter loop — no prior `reserve()` | ✅ fixed |
| PERF-AQL-01b | `src/query/aql_translator.cpp` | ~L412 | `extraPreds.push_back` in filter loop — no prior `reserve()` | ✅ fixed |
| PERF-AQL-01c | `src/query/aql_translator.cpp` | ~L594 | `extraPreds.push_back` in filter loop — no prior `reserve()` | ✅ fixed |

**Fix applied:** `extraPreds.reserve(ast->filters.size())` added before each filter-iteration loop.
