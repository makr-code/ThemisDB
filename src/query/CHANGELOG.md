> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Query Module

All notable changes to the Query module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Security audit: injection prevention hardening and resource exhaustion edge cases
- AQL parser thread-safety refactor (per-thread instances or mutex protection)

## [2.0.0] — 2026-04-27

### Added
- **Continuous Query Language (CQL) engine** (Phase 8.1–8.5) — production-grade standing-query support:
  - `WindowSpec` — `slidingTime()`, `tumblingTime()`, `slidingCount()` factory methods; `nextTick()` and `startOf()` evaluation
  - `SynopsisStore` — in-memory ring-buffer synopsis per query; `max_tuples` (10 M default) and `max_bytes` (1 GiB default) capacity limits; thread-safe via `std::mutex`
  - `IncrementalAgg` — delta-based `SUM`, `COUNT`, `AVG`, `MIN`, `MAX` aggregation without full re-scan; MIN/MAX fallback re-scan on eviction
  - `CQWatermark` — per-query watermark tracker; late-data detection; correction delta within `allowed_lateness_ms`
  - `ContinuousQueryPlanner` — compiles `ContinuousQuerySpec` to `ContinuousPlan` with validation (zero-range check, empty-name check, slide ≤ range invariant)
  - `ContinuousQueryRegistry` — thread-safe `std::unordered_map` of active queries; in-memory (RocksDB persistence planned in v2.1.0)
  - `ContinuousQueryEngine` — abstract base; `ContinuousQueryEngineImpl` concrete implementation
  - `CQResultStream` / `ResultQueue` — bounded per-subscriber result queue; overflow drops oldest entries
  - Result modes: `DELTA` (additions + retractions), `SNAPSHOT` (full window state), `CHANGES` (additions only)
  - `ContinuousQueryEngine::injectTuple()` — synchronous CDC-feed and test injection API
  - 20 unit tests (`CQ-01..CQ-20`) in `tests/test_continuous_query_engine.cpp`; registered as CTest target `ContinuousQueryEngineTests`
- **CQL performance benchmarks** (Phase 8.5) in `benchmarks/bench_continuous_query.cpp`:
  - `BM_ContinuousQuery_Throughput` (CQ-PERF-01): sustained tuples/s for a sliding time-window query; target ≥ 500 k tuples/s
  - `BM_ContinuousQuery_TupleLatency` (CQ-PERF-01): per-tuple p99 latency; target ≤ 5 ms
  - `BM_ContinuousQuery_WindowTick` (CQ-PERF-02): empty-window tick overhead; target ≤ 1 µs
  - `BM_ContinuousQuery_WindowTick_1k` / `BM_ContinuousQuery_WindowExpiry_10k` — additional hardening variants
  - Registered as `bench_continuous_query` CMake target; mapped as CQ-PERF-01/02 in `benchmark_target_mapping.json`

### Stability Guarantees (API — v2.0.0+)
- `ContinuousQueryEngine::registerQuery()` — stable public API; breaking changes require major version bump
- `ContinuousQueryEngine::dropQuery()` — stable
- `ContinuousQueryEngine::subscribe()` — stable
- `ContinuousQueryEngine::listQueries()` — stable

### Research Foundation
- Arasu, Babu, Widom (2006) — *CQL: A Language for Continuous Queries over Streams and Relations* — directly informs `WindowSpec` semantics and `ResultMode`; see `docs/research/papers/arasu_cql_2006.md`

## [1.9.0] — 2026-03-24


### Added
- **`QueryFederation` shard-key routing** — eliminates the O(N shards) broadcast for queries containing a `_key` equality or range predicate:
  - `ConsistentHashRing::getShardsInRange(hash_start, hash_end)` — clockwise ring walk returning the deduplicated set of shards responsible for a hash interval; wrap-around ranges return all shards.
  - `ConsistentHashRing::hashKey(key)` — public accessor to the internal FNV-1a + mix64 hash function.
  - `URNResolver::getShardForKey(collection, key)` — point-routes an arbitrary string key via the consistent hash ring.
  - `URNResolver::getShardsForKeyRange(collection, min_key, max_key)` — range-routes via `getShardsInRange`; falls back to all healthy shards if the ring returns nothing.
  - `ShardRouter::executeOnShards(query, shard_ids)` (virtual) — executes a query on a specified subset of shards, mirroring `scatterGather` concurrency semantics.
  - `ShardRouter::getResolver()` — const/non-const accessor exposing the `URNResolver` to higher-level components.
  - `QueryFederation::QueryMetadata::ShardKeyPredicate` — new inner struct (`Kind::POINT` / `Kind::RANGE`, `collection`, `key_value` / `key_min`+`key_max`).
  - `QueryFederation::QueryMetadata::shard_key_predicate` — optional field populated by `analyzeQuery()`.
  - `QueryFederation::analyzeQuery()`: regex extraction of `FILTER <var>._key == "<value>"` (point) and `FILTER <var>._key >= "<min>" AND <var>._key <= "<max>"` (range) predicates using `std::regex`.
  - `QueryFederation::determineRelevantShards()`: delegates to `URNResolver::getShardForKey` / `getShardsForKeyRange`; falls back to all healthy shards for keyless queries.
  - PARTITION_PRUNING branch in `QueryFederation::execute()` now calls `shard_router_->executeOnShards()`.
  - `spdlog::warn` emitted when full-scan broadcasts to > 10 shards.
  - 4 unit tests in `tests/test_query_federation.cpp`: point-lookup → 1 shard, range → ≤ 3 shards, full-scan → `scatterGather`, determinism.
  - CMake focused target: `QueryFederationShardRoutingTests`.

### Changed
- `ShardRouter::scatterGather()` is now `virtual` to allow test instrumentation.
- `QueryFederation::createExecutionPlan()`: shard-key predicate check takes priority over generic predicate-based pruning; JOIN table-count guard added.

### Fixed
- `QueryFederation::createExecutionPlan()`: previously accessed `metadata.tables[0]`/`[1]` without bounds check when `metadata.tables.size() < 2`; now guarded.

## [1.5.0] — 2026-03-12

### Added
- Vectorized execution engine (`vectorized_execution.cpp`): column-store batch processing with SIMD acceleration
- Cross-cluster federated AQL with cost estimation (`cross_cluster_federation.cpp`)
- SPARQL compatibility layer (`sparql_parser.cpp`) — parsed and translated to AQL, not executed directly
- SQL dialect compatibility (`sql_parser.cpp`) — SELECT/INSERT/UPDATE/DELETE translated to AQL
- Multi-statement transaction AQL (`BEGIN`/`COMMIT`) support
- Query cancellation via request ID (`query_canceller.cpp`)
- Per-query resource limits (max-rows, max-memory, timeout)
- Adaptive re-optimization at runtime (`adaptive_optimizer.cpp` + `runtime_reoptimizer.cpp`)
- Incremental view maintenance for materialized CTEs (`materialized_cte.cpp`)
- Query result type annotations (`result_type_annotation.cpp`)
- Workload-aware cache strategy (`workload_cache_strategy.cpp`)
- Parallel scan support

### Changed
- Cost-based optimizer updated with adaptive learning from runtime statistics
- Query federation (`query_federation.cpp`) extended with cross-cluster routing

### Fixed
- CTE cache returning stale results after underlying collection mutation
- Window function frame boundary off-by-one on ROWS BETWEEN semantics

## [1.4.0] — 2025-09-01

### Added
- LLM integration directives: `LLM INFER`, `RAG`, `EMBED` in AQL
- EXPLAIN and EXPLAIN ANALYZE plan visualization (`query_plan_visualizer.cpp`)
- Semantic query cache (`semantic_cache.cpp`) for embedding-based cache hit detection
- Statistical aggregation engine (`statistical_aggregator.cpp`)
- Window function evaluator (`window_evaluator.cpp`) with standard frame semantics
- Result streaming and pagination (`result_stream.cpp`)

### Changed
- Query cache (`query_cache.cpp` + `query_cache_manager.cpp`) extended with exact/semantic/CTE tiers
- Optimizer cost model (`optimizer_cost_model.cpp`) updated with cardinality feedback

## [1.3.0] — 2025-03-01

### Added
- Hybrid query execution: vector+geo, fulltext+geo, graph+spatial combinations
- Graph traversal: BFS, DFS, shortest path, recursive traversal
- Function registry with 25+ categories and 100+ built-in functions
- CTE support with subquery evaluation (`cte_subquery.cpp`) and caching (`cte_cache.cpp`)
- `LET` expression evaluator (`let_evaluator.cpp`)
- Distributed query federation (`query_federation.cpp`)
- AQL translator (`aql_translator.cpp`) for cross-dialect normalization

## [1.2.0] — 2024-08-01

### Added
- Full AQL AST: FOR/FILTER/SORT/LIMIT/RETURN/LET/COLLECT/WITH
- Expression types: binary, unary, literal, field, function, SimilarityCall, ProximityCall, SubqueryExpr, AnyExpr, AllExpr
- Multi-model execution: relational, document, graph, vector, spatial
- AQL JSON output format (`aql_parser_json.cpp`)
- Cost-based query optimizer (`query_optimizer.cpp`)

## [1.1.0] — 2024-04-01

### Added
- AQL runner (`aql_runner.cpp`) end-to-end execution pipeline
- Query engine foundation (`query_engine.cpp`)

## [1.0.0] — 2024-01-01

### Added
- Initial AQL parser (`aql_parser.cpp`) with basic FOR/FILTER/RETURN grammar
- Core query engine skeleton
