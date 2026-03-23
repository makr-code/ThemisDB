<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Query Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/query/CHANGELOG.md`.

## [1.5.0] — 2026-01

### Added
- `vectorized_execution.h` — `VectorizedExecution` SIMD column-batch execution engine.
- `cross_cluster_federation.h` — `CrossClusterFederation` cross-cluster query routing with result merge.
- `sparql_parser.h` — `SparqlParser` SPARQL 1.1 parser.
- `sql_parser.h` — `SqlParser` SQL-92/2003 parser.
- `query_canceller.h` — `QueryCanceller` cooperative cancellation tokens.
- `adaptive_optimizer.h` — `AdaptiveOptimizer` statistics-driven plan adaptation.
- `runtime_reoptimizer.h` — `RuntimeReoptimizer` mid-execution plan adjustment.
- `materialized_cte.h` — `MaterializedCte` lazy materialized CTE evaluation.
- `workload_cache_strategy.h` — `WorkloadCacheStrategy` workload-pattern-driven cache admission.

## [1.4.0] — 2025-09

### Added
- `semantic_cache.h` — `SemanticCache` embedding-based semantic query cache.
- `parallel_scan.h` — `ParallelScan` range-partitioned parallel scan.
- `parallel_executor.h` — `ParallelExecutor` multi-threaded plan execution.
- `adaptive_join.h` — `AdaptiveJoin` runtime join strategy selection.
- `window_evaluator.h` — `WindowEvaluator` SQL window function evaluation.
- `subquery_optimizer.h` — `SubqueryOptimizer` subquery unnesting and decorrelation.
- `statistical_aggregator.h` — `StatisticalAggregator` streaming aggregation.
- `materialized_view.h` — `MaterializedView` lifecycle and refresh.
- `result_type_annotation.h` — `ResultTypeAnnotation` runtime column type annotation.
- `query_resource_limits.h` — `QueryResourceLimits` per-query CPU/memory/time caps.
- `result_stream.h` — `ResultStream` streaming result cursor.
- `query_plan_visualizer.h` — `QueryPlanVisualizer` text/JSON/DOT output.

## [1.0.0] — 2025-01

### Added
- `query_engine.h` — `QueryEngine` top-level dispatch and lifecycle.
- `aql_parser.h` / `aql_runner.h` / `aql_translator.h` — AQL parse/run/translate pipeline.
- `query_optimizer.h` — Rule + cost-based optimizer pipeline.
- `query_compiler.h` — Logical-to-physical plan compilation.
- `optimizer_cost_model.h` — Cardinality and cost estimation.
- `plan_cache.h` / `query_cache.h` / `query_cache_manager.h` — Multi-level plan and result caching.
- `cte_cache.h` / `cte_subquery.h` / `let_evaluator.h` — CTE and LET expression support.
- `query_federation.h` — Federated query planning.
