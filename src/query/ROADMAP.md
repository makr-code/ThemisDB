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