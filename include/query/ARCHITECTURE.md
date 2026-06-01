> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/query/ARCHITECTURE.md -->

# Query Module — Public Header Architecture

**Module Path:** `include/query/`
**Implementation:** `../../src/query/`
**Canonical architecture doc:** [`../../src/query/ARCHITECTURE.md`](../../src/query/ARCHITECTURE.md)

---

## 1. Overview

`include/query/` defines the **public query execution and optimization contract** for ThemisDB. Headers cover AQL/SQL/Cypher/SPARQL/Gremlin parsing, query planning and optimization, continuous queries, vectorized execution, and tensor-aware query processing.

For full pipeline details — parse → plan → optimize → execute flow, cost model, tensor routing — see:
→ [`../../src/query/ARCHITECTURE.md`](../../src/query/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Parsing and Language Frontends

| Header | Public Type | Language |
|--------|------------|---------|
| `aql_parser.h` | `AQLParser` (stateless) | AQL |
| `aql_translator.h` | `AQLTranslator` | AQL → query plan |
| `aql_runner.h` | `AQLRunner` | AQL end-to-end execution |
| `aql_safety_validator.h` | `AQLSafetyValidator` | Injection and safety validation |
| `sql_parser.h` | `SQLParser` | SQL-92 / SQL:2016 |
| `cypher_parser.h` | `CypherParser` | openCypher |
| `sparql_parser.h` | `SPARQLParser` | SPARQL 1.1 |
| `gremlin_parser.h` | `GremlinParser` | Gremlin 3.x |
| `graphql_dialect.h` | `GraphQLDialectAdapter` | GraphQL → AQL bridge |

### 2.2 Query Engine and Execution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `query_engine.h` | `IQueryEngine` | Primary query execution interface |
| `query_compiler.h` | `QueryCompiler` | Plan → executable bytecode |
| `parallel_executor.h` | `ParallelExecutor` | Multi-threaded plan execution |
| `parallel_scan.h` | `ParallelScan` | Parallel storage scan operator |
| `vectorized_execution.h` | `VectorizedExecutor` | SIMD-batched columnar execution |
| `result_stream.h` | `ResultStream` | Lazy streaming result iterator |
| `result_type_annotation.h` | `ResultTypeAnnotator` | Result column type inference |
| `query_canceller.h` | `QueryCanceller` | Cooperative query cancellation |
| `query_profiler.h` | `QueryProfiler` | Per-operator timing and cardinality |
| `query_resource_limits.h` | `QueryResourceLimits` | Per-query CPU/memory/time budget |

### 2.3 Optimization

| Header | Public Type | Purpose |
|--------|------------|---------|
| `query_optimizer.h` | `QueryOptimizer` | Rule + cost-based optimizer |
| `adaptive_optimizer.h` | `AdaptiveOptimizer` | Runtime re-optimization |
| `runtime_reoptimizer.h` | `RuntimeReoptimizer` | Mid-execution plan switching |
| `optimizer_cost_model.h` | `ICostModel` | Pluggable cost model interface |
| `query_rewrite_rule.h` | `IQueryRewriteRule` | Pluggable rewrite rule interface |
| `subquery_optimizer.h` | `SubqueryOptimizer` | Subquery decorrelation and unnesting |
| `plan_cache.h` | `PlanCache` | Prepared-plan LRU cache |

### 2.4 Joins

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adaptive_join.h` | `AdaptiveJoin` | Hash/merge/NL join with adaptive switching |
| `let_evaluator.h` | `LETEvaluator` | AQL LET binding evaluator |

### 2.5 Aggregation and Windows

| Header | Public Type | Purpose |
|--------|------------|---------|
| `approximate_aggregator.h` | `ApproximateAggregator` | HLL / Count-Min sketch aggregation |
| `statistical_aggregator.h` | `StatisticalAggregator` | Exact statistical functions |
| `synopsis_store.h` | `SynopsisStore` | Histogram/sample-based statistics |
| `incremental_agg.h` | `IncrementalAggregator` | Incremental aggregation for streaming |
| `window_evaluator.h` | `WindowEvaluator` | SQL window function evaluator |
| `window_spec.h` | `WindowSpec` | Window frame specification |

### 2.6 Caching

| Header | Public Type | Purpose |
|--------|------------|---------|
| `query_cache.h` | `QueryCache` | Result cache with TTL and invalidation |
| `query_cache_manager.h` | `QueryCacheManager` | Cache policy and eviction management |
| `semantic_cache.h` | `SemanticCache` | Embedding-based semantic query deduplication |
| `workload_cache_strategy.h` | `WorkloadCacheStrategy` | Adaptive cache sizing to workload |
| `cte_cache.h` | `CTECache` | CTE intermediate result cache |

### 2.7 CTEs and Materialized Views

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cte_subquery.h` | `CTESubquery` | WITH clause subquery representation |
| `materialized_cte.h` | `MaterializedCTE` | Materialized CTE execution |
| `materialized_view.h` | `MaterializedView` | Incremental materialized view |
| `incremental_view.h` | `IncrementalViewRefresher` | Differential refresh of materialized views |

### 2.8 Continuous Queries

| Header | Public Type | Purpose |
|--------|------------|---------|
| `continuous_query_engine.h` | `ContinuousQueryEngine` | Streaming query processing engine |
| `continuous_query_engine_impl.h` | `ContinuousQueryEngineImpl` | Default implementation (embedder use) |
| `continuous_query_planner.h` | `ContinuousQueryPlanner` | Streaming-specific plan generation |
| `continuous_query_registry.h` | `ContinuousQueryRegistry` | Active CQ lifecycle management |
| `cq_watermark.h` | `CQWatermark` | Event-time watermark tracking |

### 2.9 Federation and Distribution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `query_federation.h` | `IQueryFederation` | Multi-shard / multi-cluster query federation |
| `cross_cluster_federation.h` | `CrossClusterFederation` | Cross-cluster query routing |

### 2.10 Tensor-Aware Query

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tensor_aware_query_optimizer.h` | `TensorAwareQueryOptimizer` | Tensor-cost-aware query rewriting |
| `tensor_contraction_engine.h` | `TensorContractionEngine` | Tensor contraction within query plans |
| `tensor_rag_cost_model.h` | `TensorRAGCostModel` | Cost model for tensor + RAG hybrid plans |
| `query_plan_visualizer.h` | `QueryPlanVisualizer` | DOT/JSON plan visualization |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::query` | All query engine and optimization types |
| `themis::query::aql` | AQL-specific parser/runner/translator |
| `themis::query::continuous` | Continuous query types |
| `themis::query::tensor` | Tensor-aware query types |

---

## 4. Relationship to Strategic Architecture

- **ANN Frontdoor**: `tensor_aware_query_optimizer.h` routes vector sub-plans to ANN operators
- **Tensor Mid-Layer**: `tensor_contraction_engine.h` and `tensor_rag_cost_model.h` integrate tensor operations into plans
- **Graph Truth Layer**: Cypher/SPARQL/Gremlin parsers feed the graph truth execution path
- **LLM/LoRA Final Layer**: `semantic_cache.h` provides embedding-based cache deduplication; `continuous_query_engine.h` feeds live signals to the LLM layer
