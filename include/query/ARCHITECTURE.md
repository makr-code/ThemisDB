<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Query Module — Architecture Guide

## Overview

The query module is the primary data retrieval engine for ThemisDB, supporting AQL (Arango Query Language), SQL, and SPARQL parsers, a multi-stage optimizer pipeline, vectorized execution, parallel scan, CTE caching, semantic caching, cross-cluster federation, adaptive join strategies, runtime re-optimization, and workload-aware cache strategies.

## Design Principles

- **Multi-dialect** — AQL, SQL, and SPARQL parsed to a common logical plan representation.
- **Vectorized execution** — `vectorized_execution.h` processes data in SIMD-friendly column batches.
- **Adaptive optimization** — `adaptive_optimizer.h` and `runtime_reoptimizer.h` adjust plans based on runtime statistics.
- **Federated queries** — `cross_cluster_federation.h` and `query_federation.h` transparently route sub-plans to remote shards.
- **Semantic caching** — `semantic_cache.h` matches semantically equivalent queries to cached results.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `adaptive_join.h` | `AdaptiveJoin` | Runtime-adaptive join strategy selection (hash/merge/NL) |
| `adaptive_optimizer.h` | `AdaptiveOptimizer` | Statistics-driven plan adaptation |
| `aql_parser.h` | `AqlParser` | AQL tokenizer and parser |
| `aql_runner.h` | `AqlRunner` | AQL query execution driver |
| `aql_translator.h` | `AqlTranslator` | AQL-to-logical-plan translator |
| `cross_cluster_federation.h` | `CrossClusterFederation` | Cross-cluster query routing with result merge |
| `cte_cache.h` | `CteCache` | CTE result caching |
| `cte_subquery.h` | `CteSubquery` | CTE subquery representation and evaluation |
| `let_evaluator.h` | `LetEvaluator` | AQL LET clause expression evaluator |
| `materialized_cte.h` | `MaterializedCte` | Materialized CTE with lazy evaluation |
| `materialized_view.h` | `MaterializedView` | Materialized view lifecycle and refresh |
| `optimizer_cost_model.h` | `OptimizerCostModel` | Cardinality and cost estimation |
| `parallel_executor.h` | `ParallelExecutor` | Multi-threaded plan execution |
| `parallel_scan.h` | `ParallelScan` | Range-partitioned parallel table/index scan |
| `plan_cache.h` | `PlanCache` | Parameterized plan cache |
| `query_cache.h` | `QueryCache` | Result-level query cache |
| `query_cache_manager.h` | `QueryCacheManager` | Cache eviction and invalidation policies |
| `query_canceller.h` | `QueryCanceller` | Cooperative cancellation tokens for running queries |
| `query_compiler.h` | `QueryCompiler` | Logical-to-physical plan compilation |
| `query_engine.h` | `QueryEngine` | Top-level query dispatch and lifecycle |
| `query_federation.h` | `QueryFederation` | Federated query planning for remote shards |
| `query_optimizer.h` | `QueryOptimizer` | Rule-based + cost-based optimizer pipeline |
| `query_plan_visualizer.h` | `QueryPlanVisualizer` | Execution plan text/JSON/DOT visualization |
| `query_resource_limits.h` | `QueryResourceLimits` | Per-query CPU/memory/time limits |
| `result_stream.h` | `ResultStream` | Streaming result cursor |
| `result_type_annotation.h` | `ResultTypeAnnotation` | Runtime type annotation for result columns |
| `runtime_reoptimizer.h` | `RuntimeReoptimizer` | Mid-execution plan adjustment based on row counts |
| `semantic_cache.h` | `SemanticCache` | Embedding-based semantic query result cache |
| `sparql_parser.h` | `SparqlParser` | SPARQL 1.1 parser |
| `sql_parser.h` | `SqlParser` | SQL-92/SQL-2003 parser |
| `statistical_aggregator.h` | `StatisticalAggregator` | Streaming statistical aggregation (sum/avg/stddev) |
| `subquery_optimizer.h` | `SubqueryOptimizer` | Subquery unnesting and decorrelation |
| `vectorized_execution.h` | `VectorizedExecution` | SIMD column-batch execution |
| `window_evaluator.h` | `WindowEvaluator` | SQL window function evaluation |
| `workload_cache_strategy.h` | `WorkloadCacheStrategy` | Workload-pattern-driven cache admission policy |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `storage` | `ParallelScan`, `MaterializedView` | Storage-layer scan and view refresh |
| `performance` | `VectorizedExecution`, `AdaptiveQueryCompiler` | SIMD and JIT for hot paths |
| `observability` | `QueryPlanVisualizer`, `QueryProfiler` | Plan and execution telemetry |
| `network` | `WireProtocolServer` | Result streaming to clients |
| `replication` | `CrossClusterFederation` | Remote shard query routing |

## Implementation

Implementation in `../../src/query/`.
