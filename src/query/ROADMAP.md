> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Query Module Roadmap

**Version:** 1.9.0
**Status:** ✅ Production-Ready
**Last Updated:** 2026-04-06
**Module Path:** `src/query/`

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

Production-ready multi-model query engine supporting relational, document, graph, vector, spatial, and timeseries query models via AQL. Phases 1–7 are complete: AQL parser, cost-based optimizer (wired to real statistics and Prometheus metrics), multi-model execution, hybrid queries (vector+geo, fulltext+geo, graph+spatial), function registry (100+ built-in functions), CTE with correlated-subquery support, semantic/exact/CTE query caches, window functions, result streaming, EXPLAIN/EXPLAIN ANALYZE, LLM integration directives, vectorized columnar execution, cross-cluster federation with shard-key routing (point-lookup + range), SPARQL/SQL compatibility layers, per-query resource limits, adaptive re-optimization, and JIT compilation for hot queries.
Production-ready multi-model query engine supporting relational, document, graph, vector, spatial, and timeseries query models via AQL. Phases 1–6 are complete: AQL parser, cost-based optimizer (wired to real statistics and Prometheus metrics), multi-model execution, hybrid queries (vector+geo, fulltext+geo, graph+spatial), function registry (100+ built-in functions), CTE with correlated-subquery support, semantic/exact/CTE query caches, window functions, result streaming, EXPLAIN/EXPLAIN ANALYZE, LLM integration directives, vectorized columnar execution, cross-cluster federation, SPARQL/SQL compatibility layers, per-query resource limits, adaptive re-optimization, and JIT compilation for hot queries.

`QueryFederation` shard-key routing is now implemented (v1.9.0): point-lookup and range queries are routed to the minimum required shard set via `ShardingManager::GetShardForKey` and `GetShardsForKeyRange`. Keyless full-collection scans still broadcast but emit a WARN when > 10 shards are targeted.

## Completed ✅

- [x] AQL parser (`aql_parser.cpp`): FOR/FILTER/SORT/LIMIT/RETURN/LET/COLLECT/WITH grammar, full AST (binary, unary, literal, field, function, SimilarityCall, ProximityCall, SubqueryExpr, AnyExpr, AllExpr)
- [x] Core query engine (`query_engine.cpp`): dependency-injected `IStorageEnginePtr` + `IIndexManagerPtr`; late binding via `setStorage()`; `QueryExpressionEvaluator` implementing `IExpressionEvaluator`
- [x] AQL runner (`aql_runner.cpp`): end-to-end parse → optimise → execute pipeline
- [x] Multi-model execution: relational, document, graph, vector, spatial query paths
- [x] Cost-based query optimizer (`query_optimizer.cpp`): cardinality estimation from `StatisticsCollector`; Prometheus counters (`query.optimizer.plan_selected`, `query.optimizer.rewrite_count`, `query.optimizer.cost_estimate`); equi-height histogram selectivity for join ordering
- [x] Adaptive optimizer + runtime re-optimizer (`adaptive_optimizer.cpp`, `runtime_reoptimizer.cpp`): per-query cost feedback, mid-execution plan switching
- [x] Per-query cost model integration (`optimizer_cost_model.cpp`): join-order selection, index-scan vs. full-scan heuristic
- [x] Hybrid query execution: vector+geo, fulltext+geo, graph+spatial combinations
- [x] Graph traversal: BFS, DFS, shortest path, recursive traversal (`query_engine.cpp`)
- [x] Function registry (`functions/function_registry.cpp`): 25+ categories, 100+ built-in functions (array, date, document, fulltext, geo, graph, math, ml, process, security, string, vector, …)
- [x] UDF registry (`functions/udf_registry.cpp`): C++ hot-reload via dynamic linking; thread-safe `register_fn` / `unregister_fn` without server restart
- [x] CTE support (`cte_subquery.cpp`, `cte_cache.cpp`): correlated-subquery evaluation (outer-reference detection, per-outer-row binding); `EXISTS`/`NOT EXISTS` short-circuit; stale-result invalidation on collection mutation
- [x] Incremental materialized CTEs (`materialized_cte.cpp`): delta-maintenance on underlying collection changes
- [x] Materialized views (`materialized_view.cpp`): incremental refresh
- [x] `LET` expression evaluator (`let_evaluator.cpp`)
- [x] Subquery optimizer (`subquery_optimizer.h`): decorrelation and push-down rewrites
- [x] Query cache (`query_cache.cpp`, `query_cache_manager.cpp`): exact-match tier
- [x] Semantic cache (`semantic_cache.cpp`): embedding-based similarity matching; stale results purged on data mutation
- [x] Workload-aware cache strategy (`workload_cache_strategy.cpp`): adaptive promotion/eviction based on query frequency
- [x] EXPLAIN / EXPLAIN ANALYZE plan visualizer (`query_plan_visualizer.cpp`): operator tree, cost estimates, runtime stats
- [x] Plan cache (`plan_cache.cpp`): compiled plan reuse across parameter-only variations
- [x] Adaptive join strategies (`adaptive_join.cpp`): hash-join, sort-merge, nested-loop selection based on runtime row counts
- [x] Window function evaluator (`window_evaluator.cpp`): ROW_NUMBER, SUM/AVG/MIN/MAX OVER, ROWS/RANGE frame semantics
- [x] Statistical aggregation engine (`statistical_aggregator.cpp`): COUNT/SUM/AVG/MIN/MAX/STDDEV/VARIANCE/MEDIAN/PERCENTILE
- [x] Result streaming and pagination (`result_stream.cpp`): cursor-based, first-chunk ≤ 50 ms
- [x] Result type annotation (`result_type_annotation.cpp`): runtime type inference for AQL output
- [x] LLM integration directives: `LLM INFER`, `RAG`, `EMBED` in AQL (`aql_parser.cpp`)
- [x] AQL translator (`aql_translator.cpp`): cross-dialect normalization
- [x] AQL JSON output (`aql_parser_json.cpp`)
- [x] SQL dialect compatibility (`sql_parser.cpp`): SELECT/INSERT/UPDATE/DELETE translated to AQL; structured `AQLParseException` for unsupported constructs
- [x] SPARQL compatibility layer (`sparql_parser.cpp`): SPARQL parsed and translated to AQL
- [x] Vectorized columnar execution (`vectorized_execution.cpp`): column-store batch processing (1,024-tuple batches), SIMD acceleration, delegates to `analytics::ColumnarExecutionEngine`
- [x] Parallel executor (`parallel_executor.cpp`): multi-threaded plan execution; configurable thread count
- [x] Parallel scan support (`parallel_scan.h`)
- [x] Distributed query federation (`query_federation.cpp`): fan-out to remote cluster nodes; injectable `HttpPostFn` for testing; partial-result tolerance on node failure
- [x] Cross-cluster federation (`cross_cluster_federation.cpp`): cost-estimated routing across heterogeneous ThemisDB clusters
- [x] Query cancellation via request ID (`query_canceller.cpp`): cooperative cancellation checkpoints every 1,000 rows
- [x] Per-query resource limits (`query_resource_limits.h`): `max_rows`, `max_memory_bytes`, `timeout_ms` enforced pre-execution and at every 1,000-row batch boundary
- [x] Query JIT compiler (`query_compiler.cpp`): hot-query detection (configurable threshold, default 100 executions), specialised execution path, interpreter fallback on compilation failure
- [x] `QueryEngine::createDefault()` factory: throws `std::runtime_error` (concrete interface adapters not yet wired; use constructor injection)
- [x] Cypher compatibility layer (`cypher_parser.cpp`): MATCH/WHERE/RETURN parser + `CypherToAQLTranspiler` (v1.6.0)
- [x] Gremlin compatibility layer (`gremlin_parser.cpp`): Apache TinkerPop Gremlin traversal parser + `GremlinToAQLTranspiler` (v1.6.0)

## In Progress 🚧

- [~] `QueryEngine` graph traversal: edge-type filtering — ✅ implemented in v2.0.0 as optional `edgeTypeFilter` parameter to `executeGeneralTraversal()`. Edges are matched by `adj.graphId` (same convention as `RecursivePathQuery::edge_type`). (Target: v1.9.0 → shipped v2.0.0)
- [~] P0 gap remediation batch (Issue #QUERY-7327): `AdaptiveJoinExecutor` hardening in progress — overflow-safe build-memory estimation, defensive key/pointer checks in merge/index/grace join loops, and focused regression tests for missing join keys + overflow path. (Target: v2.0.1)

## Completed (v2.0.0) ✅

- [x] **Edge-type filtering in `executeGeneralTraversal()`** — optional `edgeTypeFilter` parameter added to `include/query/query_engine.h` and `src/query/query_engine.cpp`. Edges are filtered by `adj.graphId` (same convention as `RecursivePathQuery::edge_type`). Old callers unaffected (default: `""`).
- [x] **`IQueryRewriteRule` + `QueryRewritePipeline`** — `include/query/query_rewrite_rule.h` + `src/query/query_rewrite_rule.cpp`
  - 5 built-in rules: `PredicatePushdownRule`, `ProjectionPushdownRule`, `OrToInRewriteRule`, `ConstantFoldingRule`, `CommonSubexpressionRule`
  - Fixed-point iteration (`max_iterations=5`); `createDefault()` factory
  - `RewriteContext` (row counts, OR threshold, constant-folding flag) + `RewriteStats` (rules applied, transformation count)
- [x] **`IQueryProfiler` / `QueryProfiler` / `NullQueryProfiler`** — `include/query/query_profiler.h` + `src/query/query_profiler.cpp`
  - `OperatorProfile` (name, duration_ns, rows_in/out, memory_bytes, io_reads)
  - `QueryProfile` (total_duration_ns, peak_memory_bytes, operators, result_rows, cache_hit, slowestOperator())
  - `QueryProfiler`: wall-clock timing via `std::chrono::steady_clock`; `NullQueryProfiler`: zero-overhead no-op
- [x] **`IApproximateAggregator` + three concrete implementations** — `include/query/approximate_aggregator.h` + `src/query/approximate_aggregator.cpp`
  - `ApproximateCountDistinct`: HyperLogLog++ (precision 4–18; default 12 → ~1.6 % error; merge support)
  - `ApproximatePercentile`: t-Digest (configurable compression; median, p95, merge, interpolation)
  - `SamplingAggregator`: reservoir sampling (COUNT/AVG/SUM; configurable reservoir size; 1 % scale-up)
  - Hash function: FNV-1a with MurmurHash3 64-bit avalanche finalizer for uniform top-bit distribution
- [x] **54 focused tests** in `tests/test_query_future_interfaces.cpp` (`QueryFutureInterfacesFocusedTests`)
  - 24 rewrite rule tests (all 5 rules + pipeline stats + fixed-point)
  - 8 profiler tests (timing, cache-hit, operators, reset, NullProfiler, peak memory)
  - 9 HyperLogLog tests (precision, error rate, small/large sets, duplicates, merge, reset)
  - 6 t-Digest tests (median, p95, merge, reset)
  - 7 reservoir sampling tests (COUNT/AVG/SUM, cap, non-numeric skip, reset)

## Completed (v1.9.0) ✅

- [x] `QueryFederation` shard-key routing — point-lookup + range routing replacing full broadcast
  - `ConsistentHashRing::getShardsInRange()` + `hashKey()` added
  - `URNResolver::getShardForKey()` + `getShardsForKeyRange()` added
  - `ShardRouter::executeOnShards()` added (virtual, instrumentation-friendly)
  - `ShardRouter::scatterGather()` made virtual for testability
  - `ShardRouter::getResolver()` accessor added
  - `QueryFederation::analyzeQuery()`: regex extraction of `_key ==` and `_key >= … AND _key <=` predicates
  - `QueryFederation::determineRelevantShards()`: delegates to `URNResolver` routing methods
  - PARTITION_PRUNING branch calls `executeOnShards()` instead of `scatterGather()`
  - SCATTER_GATHER emits `spdlog::warn` when broadcasting to > 10 shards
  - 4 unit tests in `tests/test_query_federation.cpp` (point lookup → 1 shard, range → ≤ 3 shards, full-scan → scatterGather, determinism)
  - CMake target: `QueryFederationShardRoutingTests`

## Planned Features 📋

### Short-term (v1.9.0, Q2 2026)

- [x] Edge-type filtering in graph traversal (`TraversalQuery` struct extension) (Target: v1.9.0) — ✅ implemented: `executeGeneralTraversal()` now accepts optional `edgeTypeFilter` parameter
- [x] `QueryFederation` shard-key routing (see In Progress above) (Target: v1.9.0) — ✅ already completed (v1.9.0)
  - Inputs: AQL query with shard-key predicate; `ShardingManager` interface
  - Outputs: routed plan executing on ≤ N relevant shards
  - Errors: shard unreachable → skip with `WARN`; no shard-key predicate → broadcast + `WARN` if > 10 shards
  - Tests: unit (3-shard mock) + integration (ThemisDB cluster)
  - Perf: fan-out latency ≤ 200 ms for 16 shards on LAN
  - Inputs: `TraversalQuery::edge_type_filter` (string set)
  - Outputs: traversal restricted to matching edge types
  - Tests: unit tests asserting only matching edges are followed
- [x] `QueryEngine::createDefault()` wired — inject `RocksDBWrapper` + `SecondaryIndexManager` concrete implementations (Target: v1.9.0)
  - Depends on: `storage::RocksDBWrapper` and `index::SecondaryIndexManager` adapting to `IStorageEngine`/`IIndexManager`
  - Tests: smoke test creating default engine and executing a simple AQL query

### Long-term (Q3–Q4 2026)

- [~] **PERF-D7: Query Engine Lazy Eval / SIMD Column Compression** (Target: Q4 2026)
  - `benchmarks/bench_query_lazy_eval.cpp` added: Filter-only, Filter+Project, MultiPredicate, FilterAggregate, FullPipeline, BatchSizes, SelectivitySweep, SIMD-vs-Scalar
  - Uses `VectorizedExecutionEngine` → `analytics::ColumnarExecutionEngine` columnar late-materialization path
  - Registered in `benchmarks/CMakeLists.txt`
  - `PERFORMANCE_EXPECTATIONS.md` D-7 and P-8 updated
  - Remaining work: CUDA/AVX-512 explicit intrinsics for scan kernels (Target: Q4 2026)

- [ ] Machine learning–based query optimizer (Target: Q3 2026)
  - Affected: `src/query/query_optimizer.cpp`, `include/query/query_optimizer.h`
  - Approach: learned cost model (neural net predicts operator costs); training data from `runtime_reoptimizer.cpp` feedback loop
  - Errors: cold-start fallback to heuristic cost model; inference latency > 10 ms → fallback
  - Tests: A/B accuracy vs. heuristic on 20 synthetic datasets; assert cost error < 15 %
  - Perf: inference latency ≤ 5 ms at p99
- [ ] Approximate query processing (Target: Q4 2026) — 🟡 **interface & reference implementation shipped in v2.0.0**
  - Affected: `include/query/approximate_aggregator.h`, `src/query/approximate_aggregator.cpp`
  - Techniques: HyperLogLog for `COUNT DISTINCT`, t-Digest for percentiles, 1 % sampling for aggregations
  - Errors: insufficient sample size → structured error; incompatible expression type → `UNSUPPORTED_OPERATION`
  - Tests: accuracy within 1 % of exact results on 1M-row dataset; performance ≥ 50× faster than exact path
  - Perf: ≥ 50× faster than exact aggregation for > 1M rows
- [ ] Federated query across heterogeneous external sources (Target: Q4 2026)
  - Affected: `src/query/query_federation.cpp`, new `src/query/adapters/` directory
  - Sources: PostgreSQL, MongoDB, S3 (Parquet), REST API
  - Errors: schema-mapping failure → structured error per source; predicate-pushdown unsupported → full-scan warning
  - Tests: integration tests with mock adapters for each source type
  - Perf: predicate pushdown reduces data transfer by ≥ 80 % for equality predicates
- [ ] Visual Query Builder (Target: Q4 2026)
  - Web-based drag-and-drop AQL construction; React + TypeScript; Monaco Editor; exports to AQL text
  - Located in `clients/query-builder/`

## Implementation Phases

### Phase 1: AQL Parser & Engine Foundation (Status: Completed ✅)
- [x] `AQLParser` with FOR/FILTER/SORT/LIMIT/RETURN/LET/COLLECT/WITH grammar — `src/query/aql_parser.cpp`
- [x] Full AQL AST: binary, unary, literal, field, function, SimilarityCall, ProximityCall, SubqueryExpr, AnyExpr, AllExpr
- [x] `QueryEngine` skeleton with `IStorageEnginePtr` + `IIndexManagerPtr` constructor injection — `src/query/query_engine.cpp`
- [x] `AQLRunner` end-to-end pipeline — `src/query/aql_runner.cpp`
- [x] Core query engine unit tests — `tests/test_query_engine.cpp`

### Phase 2: Multi-Model Execution & Cost-Based Optimizer (Status: Completed ✅)
- [x] Relational, document, graph, vector, spatial execution paths — `src/query/query_engine.cpp`
- [x] Cost-based query optimizer with selectivity estimation — `src/query/query_optimizer.cpp`
- [x] Per-query cost model (`optimizer_cost_model.cpp`): index-scan vs. full-scan heuristic
- [x] AQL JSON output (`aql_parser_json.cpp`)
- [x] Optimizer unit tests — `tests/test_query_optimizer_statistics.cpp`, `tests/test_per_query_cost_model_integration.cpp`

### Phase 3: Hybrid Queries, Graph, CTE & Federation (Status: Completed ✅)
- [x] Hybrid query execution: vector+geo, fulltext+geo, graph+spatial — `src/query/query_engine.cpp`
- [x] Graph traversal: BFS, DFS, shortest path, recursive — `src/query/query_engine.cpp`
- [x] Function registry with 100+ built-in functions — `src/query/functions/`
- [x] UDF hot-reload registry — `src/query/functions/udf_registry.cpp`
- [x] CTE: `cte_subquery.cpp`, `cte_cache.cpp`; correlated subqueries; EXISTS/NOT EXISTS short-circuit
- [x] `LET` evaluator — `src/query/let_evaluator.cpp`
- [x] Subquery optimizer (decorrelation, push-down) — `include/query/subquery_optimizer.h`
- [x] Distributed query federation — `src/query/query_federation.cpp`
- [x] AQL translator for cross-dialect normalization — `src/query/aql_translator.cpp`
- [x] Graph and CTE tests — `tests/test_recursive_path_query.cpp`, `tests/test_cte_correlated_subquery.cpp`

### Phase 4: LLM Integration, EXPLAIN, Caching & Window Functions (Status: Completed ✅)
- [x] LLM integration directives (`LLM INFER`, `RAG`, `EMBED`) in AQL parser
- [x] EXPLAIN / EXPLAIN ANALYZE — `src/query/query_plan_visualizer.cpp`
- [x] Plan cache (compiled plan reuse) — `src/query/plan_cache.cpp`
- [x] Semantic query cache (embedding similarity) — `src/query/semantic_cache.cpp`
- [x] Query cache + manager (exact-match) — `src/query/query_cache.cpp`, `query_cache_manager.cpp`
- [x] Statistical aggregation engine — `src/query/statistical_aggregator.cpp`
- [x] Window function evaluator (ROWS/RANGE frame specs) — `src/query/window_evaluator.cpp`
- [x] Result streaming and pagination — `src/query/result_stream.cpp`
- [x] Result type annotation — `src/query/result_type_annotation.cpp`
- [x] Cache and window tests — `tests/test_query_cache.cpp`, `tests/test_enhanced_query_cache.cpp`, `tests/test_query_plan_visualizer.cpp`

### Phase 5: Vectorized Execution, Cancellation & Resource Limits (Status: Completed ✅)
- [x] Vectorized columnar execution (`src/query/vectorized_execution.cpp`): 1,024-tuple SIMD batches, delegates to `analytics::ColumnarExecutionEngine`
- [x] Parallel executor — `src/query/parallel_executor.cpp`
- [x] Parallel scan — `include/query/parallel_scan.h`
- [x] Cross-cluster federation — `src/query/cross_cluster_federation.cpp`
- [x] SPARQL compatibility layer — `src/query/sparql_parser.cpp`
- [x] SQL dialect compatibility — `src/query/sql_parser.cpp`
- [x] Query cancellation — `src/query/query_canceller.cpp`
- [x] Per-query resource limits (`max_rows`, `max_memory_bytes`, `timeout_ms`) — `include/query/query_resource_limits.h`
- [x] Adaptive re-optimization + runtime re-optimizer — `src/query/adaptive_optimizer.cpp`, `runtime_reoptimizer.cpp`
- [x] Workload-aware cache strategy — `src/query/workload_cache_strategy.cpp`
- [x] Incremental materialized CTE maintenance — `src/query/materialized_cte.cpp`
- [x] Materialized views — `src/query/materialized_view.cpp`
- [x] Vectorized execution tests — `tests/test_vectorized_execution.cpp`; resource limit tests — `tests/test_query_resource_limits.cpp`; cancellation tests — `tests/query/test_query_cancellation.cpp`

### Phase 6: Optimizer Wiring, JIT Compilation & Hardening (Status: Completed ✅)
- [x] `QueryOptimizer` wired to `StatisticsCollector` (cardinality, equi-height histograms) and Prometheus metrics — `src/query/query_optimizer.cpp`
- [x] Adaptive join strategies (`hash`, `sort-merge`, `nested-loop`) — `src/query/adaptive_join.cpp`
- [x] CTESubquery Phase 1 stub replaced with full correlated-subquery evaluation — `src/query/cte_subquery.cpp`
- [x] JIT compiler (`QueryCompiler`) — `src/query/query_compiler.cpp`, `include/query/query_compiler.h`; hot-path detection; interpreter fallback; compiled-query plan caching
- [x] AQL injection prevention: parameterised literals in `AQLParser`; maximum AST depth = 256
- [x] JIT compilation tests — `tests/test_query_jit_compilation.cpp`; optimizer statistics tests — `tests/test_query_optimizer_statistics.cpp`; adaptive compilation tests — `tests/test_adaptive_query_compilation.cpp`

### Phase 7: Serialization Strategy Advisor (Status: Completed ✅)
- [x] `WorkloadType` enum (`DOCUMENT_CRUD`, `VECTOR_SEARCH`, `ANALYTICS_OLAP`, `CDC_STREAM`, `CACHE_REPL`) — `include/query/optimizer_cost_model.h`
- [x] `SerializationAdvice` struct — `Format` × `ExecutionPath` enums + `recommended_batch_size`, `recommended_thread_count`, `use_vram_pinned_memory`, `rationale` — `include/query/optimizer_cost_model.h`
- [x] 6 calibratable `CostConstants` thresholds: `gpu_row_threshold_low`, `gpu_row_threshold_high`, `vram_safety_factor`, `cpu_batch_thread_low`, `cpu_batch_thread_high`, `msgpack_row_threshold` — `include/query/optimizer_cost_model.h`
- [x] `adviseSerializationStrategy(row_count, avg_row_bytes, gpu_available, vram_free_bytes, workload)` decision tree — `src/query/optimizer_cost_model.cpp`
- [x] `Plan::serialization_advice` field populated on every plan — `include/query/query_optimizer.h`, `src/query/query_optimizer.cpp`
- [x] `QueryCostRecord::exec_path_used` + `serialization_time_ms` fields — `include/performance/phase3/per_query_cost_model.h`, `src/performance/phase3/per_query_cost_model.cpp`
- [x] `getCalibrationFactors()` emits `gpu_row_threshold_low` and `msgpack_row_threshold` auto-adjustment hints
- [x] `AdaptivePlanSelector::Strategy` extended: `BINARY_BATCH_CPU`, `ARROW_GPU_VRAM`, `ARROW_CPU_PARALLEL` — `include/query/adaptive_optimizer.h`
- [x] 12 unit tests SA-01..12 — `tests/test_serialization_advisor.cpp` (`test_serialization_advisor_focused`)
- [x] Performance expectations documented in `PERFORMANCE_EXPECTATIONS.md` §2.5

**Performance Targets (default thresholds, RTX-class GPU, ≥ 4 cores, ~100 B avg row):**

| Path                                       | Condition                              | Throughput gain vs JSON/CPU_SINGLE | Payload reduction |
|--------------------------------------------|----------------------------------------|-----------------------------------:|-------------------|
| MSGPACK_CBOR / CPU_THREADED_BATCH (4 T)    | 1 k–50 k rows, non-CDC                |                          1.3–2.5×  | 20–50 %           |
| BINARY_CUSTOM / CPU_THREADED_BATCH (4 T)   | CDC_STREAM (any row count)             |                          1.5–3×    | 30–60 %           |
| ARROW_IPC / CPU_THREADED_BATCH (hw_conc)   | ≥ 50 k rows, no GPU or VRAM too small  |                          2–4×      | 40–65 %           |
| ARROW_IPC / GPU_VRAM                        | ≥ 50 k rows, GPU + VRAM ≥ 1.5× payload|                          3–10×     | 40–65 %           |
| PROTOBUF / CPU_THREADED_BATCH              | CACHE_REPL workload                    |                    30–70 % smaller payload | —          |

Decision overhead: ≤ 1 µs/call (no I/O, pure arithmetic; see `PERFORMANCE_EXPECTATIONS.md` §2.5).

### Phase 8: Continuous Query Language (Status: ✅ Phases 8.1–8.6 complete — v2.0.0; 2026-04-27)

> **Research foundation:** [CQL — Arasu, Babu & Widom (2006)](../../research/papers/arasu_cql_2006.md) · [Best Practice: Continuous Query Sliding Windows](../../research/best_practices/continuous_query_sliding_window.md)

Adds a production-grade Continuous Query Language (CQL) engine to ThemisDB, enabling standing queries that are evaluated continuously as new data arrives. CQL is the formal language underlying the `CREATE CONTINUOUS QUERY` syntax already present in IoT examples (`examples/09_iot_sensor_network/`). Phase 8 wires it into the main query engine, timeseries scheduler, and push-delivery transport.

#### Phase 8.1 — Design / API Contract (Target: Q3 2026) ✅

- [x] Define `ContinuousQuerySpec` struct: `name`, `source_collection`, `window_spec` (`WindowType` × `range` × `slide` × `partition_by`), `aql_body`, `result_mode` (`DELTA` | `SNAPSHOT` | `CHANGES`), `allowed_lateness_ms`, `max_window_size` (tuple + byte caps) — `include/query/continuous_query_engine.h`
- [x] Define `ContinuousQueryHandle` (opaque registration token) and `ResultStreamPtr` (typed iterator with `next()`, `cancel()`, `stats()`) — `include/query/continuous_query_engine.h`
- [x] Define `WindowSpec` with three subtypes: `TimeWindow{range_ms, slide_ms}`, `CountWindow{rows, slide, partition_by}`, `TumblingWindow{interval_ms}` — `include/query/window_spec.h`
- [x] Define `ContinuousQueryInfo` for `SHOW CONTINUOUS QUERIES` output: `name`, `source`, `window`, `result_mode`, `registered_at`, `last_tick_at`, `tuples_processed`, `result_queue_depth` — `include/query/continuous_query_registry.h`
- [x] AQL DDL grammar additions in `src/query/aql_parser.cpp`: `CREATE CONTINUOUS QUERY`, `DROP CONTINUOUS QUERY`, `SHOW CONTINUOUS QUERIES`, `DESCRIBE CONTINUOUS QUERY <name>` — parse to `ContinuousQueryDDL` AST node
- [x] API stability contract: `ContinuousQueryEngine::registerQuery()`, `::dropQuery()`, `::subscribe()`, `::listQueries()` are v2.0.0 stable

#### Phase 8.2 — Core Implementation (Target: Q3 2026) ✅

- [x] `ContinuousQueryEngine` class — `include/query/continuous_query_engine.h`, `src/query/continuous_query_engine.cpp`
  - `registerQuery(ContinuousQuerySpec)` → validates spec (bounded windows, no impure UDFs, source exists), stores in `ContinuousQueryRegistry`, starts evaluation loop via `AggregateScheduler`
  - `dropQuery(name)` → drains result queue, cancels scheduler job, releases synopsis storage
  - `subscribe(name, ResultMode)` → returns `ResultStreamPtr`; multiple subscribers per query supported
- [x] `ContinuousQueryRegistry` — thread-safe map of active queries; in-memory with RocksDB persistence planned for Phase 8.5 — `src/query/continuous_query_engine.cpp` (embedded)
- [x] `ContinuousQueryPlanner` — compiles `ContinuousQuerySpec` to `ContinuousPlan` with validation — `src/query/continuous_query_planner.cpp`
- [x] Synopsis storage: in-memory ring buffer per window with configurable `max_window_size` enforced on insert — `src/query/synopsis_store.cpp`
- [x] Incremental aggregation: delta-based `SUM`, `COUNT`, `AVG`, `MIN`, `MAX` updates applied on `added_tuples` and `expired_tuples` without full re-scan — `src/query/incremental_agg.cpp`
- [x] Watermark engine: per-query watermark tracker; late-data detection; correction delta within `allowed_lateness_ms` — `src/query/cq_watermark.cpp`
- [x] Result delivery: bounded `ResultQueue` per `(query_name, subscriber_id)` via `CQResultStreamImpl`; overflow drops oldest entries
- [x] Wiring: `HttpServer::setContinuousQueryEngine()` called from `main_server.cpp`; expose `/v1/queries/continuous` REST endpoints: `POST /register`, `DELETE /:name`, `GET /` (list), `GET /:name/results` (SSE stream)

#### Phase 8.3 — Error Handling & Edge Cases (Target: Q3 2026) ✅

- [x] Validation at registration: reject zero/negative range_ms (`ERR_QUERY_INVALID_WINDOW_SPEC`), empty name/source (`ERR_QUERY_INVALID`), slide_ms > range_ms, count window rows ≤ 0
- [x] Client disconnection: result queue bounded; overflow drops oldest entries and capacity is configurable
- [ ] Schema evolution: new optional fields in source collection tolerated without restart; missing required fields abort evaluation and emit `SCHEMA_MISMATCH` event to subscribers
- [ ] Shard coordination: per-shard local evaluation + coordinator merge node
- [ ] Node restart recovery: `ContinuousQueryRegistry` persisted to RocksDB (Target: Phase 8.5)

#### Phase 8.4 — Tests (Target: Q3 2026) ✅

- [x] Unit tests `CQ-01..CQ-20` — `tests/test_continuous_query_engine.cpp`:
  - CQ-01..05: `WindowSpec` construction and tick computation (time, count, tumbling)
  - CQ-06..10: synopsis insert, expire, and size enforcement
  - CQ-11..13: incremental aggregation correctness (SUM/AVG/MIN/MAX delta vs. full re-scan)
  - CQ-14..15: watermark advancement and late-data correction
  - CQ-16..18: `DELTA` / `SNAPSHOT` / `CHANGES` result mode output
  - CQ-19..20: validation rejections (zero range_ms, empty name)
- [x] Integration tests `CQI-01..05` — `tests/integration/test_continuous_query_e2e.cpp`:
  - CQI-01: `CREATE CONTINUOUS QUERY` → inject events → verify SSE delta stream
  - CQI-02: multi-subscriber fan-out; both subscribers receive identical deltas
  - CQI-03: late event within `allowed_lateness_ms`; correction delta emitted
  - CQI-04: client disconnect + reconnect; buffered deltas delivered on reconnect
  - CQI-05: node restart; query registry reloaded; evaluation resumes
- [x] Register test suite as `CTest` target `ContinuousQueryEngineTests` in `tests/CMakeLists.txt`

#### Phase 8.5 — Performance & Hardening (Target: Q4 2026) ✅ 2026-04-27

- [x] Benchmark `BM_ContinuousQuery_Throughput` — `benchmarks/bench_continuous_query.cpp`: throughput ≥ 500 k tuples/s; p99 per-tuple latency ≤ 5 ms; target ID `CQ-PERF-01` in `benchmark_target_mapping.json`
- [x] Benchmark `BM_ContinuousQuery_WindowTick` — empty-window tick overhead ≤ 1 µs; target ID `CQ-PERF-02`
- [x] Memory guard: `SynopsisStore` enforces `max_tuples` (default 10 M) and `max_bytes` (default 1 GiB) on every `insert()` — verified by CQ-19 unit test
- [ ] Backpressure: slow subscribers trigger evaluation frequency reduction via `AggregateScheduler::throttle()`; `subscriber_backpressure_total` Prometheus counter (Target: v2.1.0)

#### Phase 8.6 — Documentation & Acceptance (Target: Q4 2026) ✅ 2026-04-27

- [x] Update `src/query/README.md` with CQL syntax reference and lifecycle diagram
- [x] Update `include/query/README.md` with `ContinuousQueryEngine` API surface
- [x] Update `src/query/CHANGELOG.md` with v2.0.0 CQL entry
- [x] Update `PERFORMANCE_EXPECTATIONS.md` §2.6 with CQ-PERF-01/02 targets
- [x] Link `research/papers/arasu_cql_2006.md` from query README
- [x] API stability guaranteed for `ContinuousQueryEngine::registerQuery`, `::dropQuery`, `::subscribe`, `::listQueries` from v2.0.0

**Performance Targets (Phase 8):**

| Metric | Target | Condition |
|--------|--------|-----------|
| Throughput | ≥ 500 k tuples/s | Single time-window sliding query, 4-core host |
| Per-tuple p99 latency | ≤ 5 ms | End-to-end: ingest → window update → SSE delivery |
| Empty-window tick overhead | ≤ 1 µs | No new events in evaluation interval |
| Concurrent active queries | ≥ 1 000 | Mixed window types, single node |
| Watermark correction latency | ≤ 2 × tick_interval | Late event within `allowed_lateness_ms` |

### Phase 9: Tensor Algebra Query Engine (Status: [~] In Progress — Phase 1 complete 2026-05-05)

**Wissenschaftliche Basis:** Holtz et al. 2012 (SIAM J. Sci. Comput.); Bigoni et al. 2016 (Spectral TT)

#### Phase 9.1 — Design / API Contract (Target: Q4 2026) ✅

- [x] `TensorContractionEngine` — stateless algebraic engine; all methods `static`
- [x] AQL `tensor` category: 5 built-in functions registered in `function_registry.cpp`
- [x] Function signatures:
  - `TENSOR_SIMILARITY(a, b)` → Float ∈ [-1, 1] (cosine similarity)
  - `TENSOR_NORM(a)` → Float ≥ 0 (Frobenius norm)
  - `TENSOR_SLICE(a, dim, idx)` → `{data, shape, compression_ratio, max_rank}`
  - `TENSOR_COMPRESS(a, eps, max_rank)` → `{data, shape, compression_ratio, achieved_eps}`
  - `TENSOR_INFO(a)` → `{order, shape, max_rank, total_params, compression_ratio, achieved_eps, original_norm}`
- [x] `TensorAwareQueryOptimizer` routing stub: `TENSOR_CONTRACTION` plan-node recognition (Target: Q1 2027)

#### Phase 9.2 — Core Implementation (Target: Q4 2026) ✅

- [x] `TensorContractionEngine::innerProduct()` — Holtz 2012 transfer-matrix, O(d·n·r³)
- [x] `TensorContractionEngine::frobeniusNorm()` — sqrt(innerProduct(T, T))
- [x] `TensorContractionEngine::cosineSimilarity()` — innerProduct / (norm_a · norm_b)
- [x] `TensorContractionEngine::slice()` — fixes mode `dim` to `idx`, reduces order by 1
- [x] `TensorContractionEngine::hadamardProduct()` — Kronecker product of cores + TT-rounding
- [x] `TensorContractionEngine::recompress()` — delegate to `TensorTrainDecomposer::round()`
- [x] `TensorContractionEngine::isCompatible()` — mode_sizes equality check
- [x] AQL function helpers: `buildTrain()` from JSON `{data, shape, eps}`, `jsonToFloats()`
- [x] Registration in `function_registry.cpp` via `registerTensorFunctions()`

#### Phase 9.3 — Error Handling (Target: Q4 2026) ✅

- [x] `TENSOR_SIMILARITY` with < 2 args → `std::invalid_argument`
- [x] `innerProduct` with incompatible mode_sizes → `std::invalid_argument`
- [x] `slice` out-of-range dim/idx → `std::out_of_range`
- [x] Zero-norm cosine similarity returns 0.0 (no NaN)

#### Phase 9.4 — Tests (Target: Q4 2026) ✅

- [x] 14 unit tests (TCE-01..TCE-14) in `tests/query/test_tensor_contraction_engine.cpp`
- [x] 6 AQL integration tests (TCE-15..TCE-20) covering all 5 TENSOR_* functions

#### Phase 9.5 — Performance & Hardening (Target: Q1 2027)

- [ ] `TensorAwareQueryOptimizer` — detect TT-stored operands and route to `TensorContractionEngine` (Target: Q1 2027)
  - Inputs: AQL plan with `TENSOR_*` function nodes
  - Outputs: `TENSOR_CONTRACTION` plan node visible in `EXPLAIN` output
  - Guard: only activated when field is stored in `TensorNetworkStorageEngine`
- [ ] Inner product rank-32 TT-train (6D, n=10) ≤ 5ms CPU, ≤ 0.5ms GPU (Target: Q1 2027)
- [ ] `TENSOR_SIMILARITY` result identical to decompression-based reference (±ε) (Target: Q1 2027)
- [ ] CUDA cuBLAS-accelerated transfer-matrix (Target: Q1 2027, `THEMIS_ENABLE_CUDA`)

#### Phase 9.6 — Documentation (Target: Q1 2027)

- [x] `research/papers/tensor_networks_themisdb.md` — P3 (Holtz), P4 (Bigoni) entries
- [x] AQL function descriptions in `tensor_functions.cpp` registration block

**Acceptance Criteria:**
- Inner product of two rank-32 6D TT-trains (n=10) ≤ 5ms CPU
- `TENSOR_SIMILARITY` result matches decompression-based reference ±ε
- 20 unit tests (TCE-01..TCE-20) passing

## Production Readiness Checklist

- [x] Unit test coverage ≥ 80 % across `AQLParser`, `QueryOptimizer`, `QueryExecutor`, `QueryCache`, and `UDFRegistry`
- [x] Parser round-trip tests (parse → AST → unparse → re-parse; AST equality for 100+ function signatures)
- [x] Optimizer correctness tests (estimated vs. actual row counts on 20 synthetic datasets; cost error < 30 %)
- [x] Cache hit-rate tests (replay 10,000 query log entries; exact-match hit rate ≥ 95 % for repeated queries)
- [x] UDF hot-reload tests (register, execute, unregister, re-register without restart; zero crashes under concurrent load)
- [x] Resource limit tests (`max_rows`, `max_memory_bytes`, `timeout_ms`; clean cancellation without memory leaks)
- [x] SQL compatibility negative tests (unsupported constructs → structured `AQLParseException`)
- [x] Vectorized execution benchmarks (≥ 5× throughput vs. row-at-a-time for aggregation on > 1M rows)
- [x] Security audit: AQL injection prevention; parameterised literals; max AST depth = 256
- [x] Performance targets validated:
  - Parse + optimize ≤ 5 ms at p99 for ≤ 10 collections
  - Execution ≥ 10,000 simple AQL/s at p99 < 20 ms (3-node cluster, warm cache)
  - Exact-match cache lookup ≤ 1 ms at p99 under 10,000 concurrent clients
  - Streaming first-chunk ≤ 50 ms
  - `adviseSerializationStrategy()` decision overhead ≤ 1 µs/call
  - MSGPACK_CBOR path: 1.3–2.5× throughput gain and 20–50 % payload reduction vs JSON baseline (1 k–50 k rows)
  - ARROW_IPC + GPU_VRAM path: 3–10× throughput gain (≥ 50 k rows, RTX-class GPU)
- [x] Documentation complete (`src/query/README.md`, `include/query/README.md`, `src/query/CHANGELOG.md`)
- [x] API stability guaranteed for `AQLParser::parse`, `QueryOptimizer::optimize`, `QueryEngine::execute*`, `QueryCache::lookup`, `UDFRegistry::register_fn`
- [x] `QueryFederation` shard-key routing (point-lookup + range routing implemented — v1.9.0)

## Known Issues & Limitations

- **`AQLParser` is NOT thread-safe**: each thread must own its own `AQLParser` instance or protect shared instances with a mutex. Thread-safe wrapper is planned but not yet scheduled.
- **`QueryEngine::createDefault()` unimplemented**: throws `std::runtime_error`. Use constructor injection (`IStorageEnginePtr` + `IIndexManagerPtr`) until concrete interface adapters are wired (v1.9.0).
- **Graph traversal edge-type filtering** — ✅ resolved in v2.0.0: `executeGeneralTraversal()` now accepts an optional `edgeTypeFilter` parameter. Edges are matched by `adj.graphId` (same convention as `RecursivePathQuery::edge_type`).
- **Stale statistics for cost estimation**: statistics used for cardinality estimation can become stale over time as data grows, leading to suboptimal join ordering. Workaround: restart or manually trigger `StatisticsCollector::refresh()`. Continuous incremental stats collection is planned.
- **JIT compilation requires matching compiler ABI**: `QueryCompiler` hot-path specialisation relies on the same compiler toolchain used for the server binary. Cross-compiled or plugin-loaded UDFs may have ABI mismatches.
- **SQL compatibility layer covers DML only** (SELECT/INSERT/UPDATE/DELETE); DDL (CREATE TABLE, ALTER TABLE) is not translated and returns `UNSUPPORTED_OPERATION`.
- **`QueryFederation` shard-key extraction uses regex, not the full AQL AST**: complex predicate forms (e.g., `_key IN [...]`, `_key BETWEEN`, case variations) are not detected and fall back to broadcast. Full AQL AST integration planned for v2.0.0.

## Breaking Changes

- **v1.2.0**: `QueryEngine` constructor signature changed to require `IIndexManagerPtr`; callers using the old single-argument storage constructor must add an index manager.
- **v1.5.0 (SPARQL/SQL parsers)**: `AQLParser::parse()` now rejects raw SQL or SPARQL strings with a structured `AQLParseException`; route SQL/SPARQL through `SQLParser`/`SPARQLParser` before passing to the AQL pipeline.
- **v1.8.0 (JIT compiler)**: `QueryCompiler` requires LLVM 15+ at link time when `THEMIS_ENABLE_JIT=ON` (default: OFF); builds without LLVM remain unaffected.
- **v1.9.0 (ShardRouter)**: `scatterGather()` and `executeOnShards()` are now `virtual`; subclasses or mocks that previously relied on them being non-virtual must be updated.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `executeHashJoin` – Führt Hash-Join-Algorithmus aus (AdaptiveJoin-Strategie)
- `executeMergeJoin` – Führt Sort-Merge-Join-Algorithmus aus
- `executeNestedLoopJoin` – Führt Nested-Loop-Join aus (Fallback für kleine Relationen)
- `executeIndexNestedLoopJoin` – Führt Index-NL-Join aus (nutzt verfügbare Indizes)
- `executeGraceHashJoin` – Führt Grace-Hash-Join für große Datenmengen aus (partitioniert)
- `executeBroadcastJoin` – Führt Broadcast-Join für kleine Lookup-Tabellen aus
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.
