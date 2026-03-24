<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Query Module

All notable changes to the Query module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Performance benchmarks for vectorized execution and federated query paths
- Security audit: injection prevention hardening and resource exhaustion edge cases
- AQL parser thread-safety refactor (per-thread instances or mutex protection)

## [1.6.0] — 2026-03-24

### Added
- Cypher compatibility layer (`cypher_parser.cpp`, `include/query/cypher_parser.h`): full
  MATCH/WHERE/RETURN parser + `CypherToAQLTranspiler` emitting AQL FOR/FILTER/SORT/LIMIT/RETURN;
  supports node patterns (variable, label, inline properties), relationship patterns
  (OUT/IN/BOTH, variable-length *m..n), WHERE expressions (=, <>, AND, OR, NOT, IS NULL,
  STARTS WITH, ENDS WITH, CONTAINS, IN), RETURN DISTINCT, ORDER BY, SKIP, LIMIT.
  30 focused tests (`CypherParserFocusedTests`).
- Gremlin compatibility layer (`gremlin_parser.cpp`, `include/query/gremlin_parser.h`): full
  Apache TinkerPop Gremlin traversal parser + `GremlinToAQLTranspiler`; supports g.V()/g.E(),
  hasLabel/has/hasNot, P.eq/neq/lt/lte/gt/gte/within/without predicates, out/in/both traversals,
  values/valueMap/id/label projection, count (LENGTH() wrapper), dedup (COLLECT), limit/range,
  order().by() sorting, as/select aliases.
  30 focused tests (`GremlinParserFocusedTests`).

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
