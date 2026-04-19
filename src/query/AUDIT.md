<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

# Audit Record — Query Module

## Module Identity

| Field        | Value                                      |
|--------------|--------------------------------------------|
| Module       | query                                      |
| Source path  | `src/query/`                               |
| Audit date   | 2026-04-19                                 |
| Audited by   | ThemisDB core team                         |
| Status       | Production-ready; benchmark/security audit in progress |

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
| AQL injection detection               | ✅ Complete   | Security module detector + parameterized API  |
| SPARQL/SQL parse-and-translate        | ✅ Complete   | No direct dialect execution                   |
| Per-query resource limits             | ✅ Complete   | max-rows, max-memory, timeout enforced        |
| Query cancellation                    | ✅ Complete   | Via request ID                                |
| Tenant namespace isolation            | ✅ Complete   | KeySchema enforced                            |
| AQLParser thread-safety               | ⚠️ Open      | Per-thread or mutex required (KL-01)          |
| Performance benchmarks                | ❌ Pending    | Vectorized + federated paths (Q2 2026)        |
| Full security audit                   | ⚠️ In progress | Injection + resource exhaustion (Q2 2026)   |

## Open Items

| ID    | Description                                                  | Target  | Priority |
|-------|--------------------------------------------------------------|---------|----------|
| OI-01 | AQLParser thread-safety refactor                             | Planned | High     |
| OI-02 | Performance benchmarks (vectorized, federated)               | Q2 2026 | High     |
| OI-03 | Full security audit (injection, resource exhaustion)         | Q2 2026 | High     |

## Build Audit

| Check                        | Result     |
|------------------------------|------------|
| Compilation (all 38 files) | ✅ Pass    |
| Static analysis              | ✅ Pass    |
| Test coverage > 80%          | ✅ Pass    |
| Audit completed              | 2026-03-12 |
