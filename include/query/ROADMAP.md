<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Query Module Roadmap

## Current Status

v1.5.0 — production. AQL, SQL, SPARQL parsers; adaptive optimizer; vectorized execution; CTE materialization; cross-cluster federation; semantic cache; cooperative cancellation are all operational.

## Completed

- [x] AQL parse/run/translate pipeline
- [x] Rule + cost-based query optimizer
- [x] Logical-to-physical plan compilation
- [x] Multi-level caching (plan, result, CTE, semantic)
- [x] Federated query planning
- [x] Parallel scan and multi-threaded execution
- [x] Adaptive join, window functions, subquery unnesting
- [x] Materialized views and materialized CTEs
- [x] Vectorized SIMD column-batch execution
- [x] Cross-cluster federation with result merge
- [x] SPARQL and SQL parsers
- [x] Cooperative query cancellation
- [x] Adaptive optimizer + runtime re-optimizer
- [x] Workload-driven cache strategy

## Implementation Phases

### Phase 1 — Core Parser & Optimizer ✅
- [x] AQL parser, runner, translator
- [x] Rule + cost-based optimizer
- [x] Plan and result cache

### Phase 2 — Advanced Execution ✅
- [x] Parallel scan, parallel executor
- [x] Adaptive join, window evaluator
- [x] Vectorized SIMD execution

### Phase 3 — Federation & Multi-Dialect ✅
- [x] Cross-cluster federation
- [x] SQL and SPARQL parsers
- [x] Query cancellation

### Phase 4 — Semantic Caching & Adaptation ✅
- [x] SemanticCache embedding-based cache
- [x] AdaptiveOptimizer + RuntimeReoptimizer
- [x] Workload cache strategy

### Phase 5 — Future Enhancements (Planned)
- [x] GraphQL query dialect support (Target: Q3 2026)
- [ ] GPU-accelerated vectorized execution (Target: Q4 2026)
- [x] Incremental view maintenance for materialized views (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 35 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] AQL fuzzing with 1M random inputs
- [x] Federation tested across 8-shard cluster
- [x] Vectorized execution benchmarked at ≥ 4x scalar baseline
- [ ] GPU vectorized execution (Target: Q4 2026)
