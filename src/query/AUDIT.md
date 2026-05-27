<!-- Status: S0 fixed 2026-05-04 | S1 fixed 2026-05-04 | OI-05/OI-06 fixed 2026-05-26 | KL-01 closed 2026-05-26 | CCF-01..CCF-05 fixed 2026-05-27 | CQE-01..CQE-03 fixed 2026-05-27 | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

# Audit Record — Query Module

## Module Identity

| Field        | Value                                      |
|--------------|--------------------------------------------|
| Module       | query                                      |
| Source path  | `src/query/`                               |
| Audit date   | 2026-04-21 (S0 fixes: 2026-05-04, S1 fixes: 2026-05-04, OI-05/OI-06: 2026-05-26, KL-01 closed: 2026-05-26, CCF-01..CCF-05 fixed: 2026-05-27, CQE-01..CQE-03 fixed: 2026-05-27) |
| Audited by   | Copilot (source code analysis)             |
| Status       | ✅ All critical findings resolved — 0 S0, 0 S1, 0 critical OI open; KL-01 closed; CCF-01..CCF-05 closed; CQE-01..CQE-03 closed |

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
| Performance benchmarks                | ❌ Pending    | Vectorized + federated paths (Q2 2026)        |
| Full security audit                   | ✅ All critical findings resolved | QE-1..QE-5 ✅, PA-1..PA-2 ✅, TR-1..TR-2 ✅, CCF-01..CCF-05 ✅, CQE-01..CQE-03 ✅ — see Findings section |

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
| OI-02 | Performance benchmarks (vectorized, federated)               | Q2 2026 | High     |
| OI-03 | Full security audit (injection, resource exhaustion)         | Q2 2026 | High     |
