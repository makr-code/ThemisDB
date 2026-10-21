---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Query Module Research Notes

## Scope and evidence base

This document is grounded in the concrete Query module source and the module-level architecture narrative in `src/query/ARCHITECTURE.md`, `src/query/README.md`, and `src/query/ROADMAP.md`. The key implementation evidence lives in:

- `src/query/aql_parser.cpp` and `include/query/aql_parser.h` — query syntax, expression handling, and AST construction.
- `src/query/aql_translator.cpp` — translation from AST to internal logical representation.
- `src/query/query_optimizer.cpp` and `src/query/optimizer_cost_model.cpp` — predicate ordering, join planning, and cost estimation.
- `src/query/query_engine.cpp` — execution of filters, scans, sorts, aggregations, and traversals.
- `src/query/query_cache.cpp`, `src/query/query_cache_manager.cpp`, and `src/query/semantic_cache.cpp` — exact and semantic cache behavior.
- `src/query/cross_cluster_federation.cpp` and `src/query/query_federation.cpp` — distributed query execution and result merging.
- `src/query/continuous_query_engine.cpp` and `src/query/query_canceller.cpp` — runtime cancellation and continuous query control.

## Why the module exists

ThemisDB needs a single query layer that can speak across many storage and execution models without forcing callers to reason about each backend separately. The Query module is that layer. It normalizes AQL input, turns it into a plan, chooses execution strategies based on cost and access patterns, and then marshals results back through one bounded execution boundary.

The implementation reflects four underlying design assumptions:

1. Query syntax should be expressive and deterministic.
   - AQL parsing is intentionally rich enough for multi-model operations such as graph traversal, geospatial filters, vector similarity, and window queries.
   - The parser does not silently accept malformed or ambiguous syntax; instead it emits structured parse failures.

2. Optimization is a control problem, not just a syntax problem.
   - The optimizer decides whether to push predicate evaluation, prefer index lookups, or reorder hybrid plans using estimated selectivity and cardinality.
   - Runtime feedback is incorporated through adaptive optimization and workload-aware cache behavior.

3. Runtime safety matters as much as query correctness.
   - `QueryEngine` enforces capability and access checks through collection access callbacks when configured.
   - Cancellation, timeout, and result-stream limits are treated as first-class execution concerns rather than incidental additions.

4. Production operators need the ability to reason about cache, federation, and failure domains.
   - Exact match and semantic cache layers provide different optimization levers without hiding the cost of invalidation or expiry.
   - Federated execution requires explicit control of shard routing, result merging, and timeout or retry semantics.

## Research questions the implementation addresses

### 1. Can a single query language span multiple data models?

The Query module is built around the premise that a single AQL syntax can capture relational, document, graph, vector, geospatial, and time-series operations. The parser and translator convert a textual statement into a common internal representation that downstream components can reason about without each backend needing a bespoke client contract.

### 2. Can planner decisions be cost-aware instead of purely rule-based?

The optimizer stack is deliberately structured around cost estimation. `optimizer_cost_model.cpp` and the adaptive enhancements encode the idea that predicate selectivity, index availability, and runtime statistics change plan quality. This allows the engine to prioritize low-cardinality predicates, better index choices, and adaptive reoptimization.

### 3. Can query execution remain bounded under overload or cancellation?

The module includes operational controls around continuous query loops, cancellation propagation, result streaming, and federated timeouts. These are not afterthoughts. They make the engine usable in production by limiting runaway work and allowing operators to intentionally stop or isolate a bad query.

### 4. Can the cache layer improve repeat workloads without becoming unsafe?

The exact-cache path (`query_cache.cpp`) and the semantic path (`semantic_cache.cpp`) provide two distinct optimization strategies. Exact cache is deterministic and dependency-aware; semantic cache is approximate and requires a similarity threshold and an embedding model for query representation. Both require care around invalidation, memory bounds, and expiry.

## Assumptions and constraints

- The parser is the canonical boundary for syntactic validity and query safety checks. It is assumed to be the first stage where malformed or unsafe expressions are rejected.
- The optimizer is allowed to trade a small amount of planning latency for better execution efficiency; this is intentional for repeated workloads and complex hybrid queries.
- Query runtime is expected to operate under strict resource budgets: result streaming, timeouts, and cancellation are there to keep execution bounded.
- Distributed execution cannot assume every shard behaves identically; fallback, timeout, and result-merging semantics must remain explicit.
- Query correctness is not equivalent to perfect retrieval quality; the system must also be explainable, debuggable, and operator-safe.

## Trade-offs and failure domains

### Safety vs. permissive execution

The module deliberately prefers explicit denial or bounded failure over a loose, best-effort execution path. When collection access checks are configured or a query exceeds execution constraints, the behavior is supposed to be clear and actionable rather than silent.

### Planning cost vs. latency

The cost-model and adaptive optimizer provide better execution plans, but they also add planning work. That is acceptable because the query module is optimized for repeated, production-like workloads where the cost is amortized over many executions.

### Exact cache vs. approximate cache

Exact caches are safer and simpler but less tolerant of paraphrased or slightly modified queries. Semantic caches are stronger for repeated user patterns, but they depend on embedding semantics, similarity thresholds, and the validity of the index state.

### Local execution vs. federation

Federation broadens workloads and cluster reach, but it also introduces remote failure modes, network timeouts, and merge complexity. Those are managed in the Query module through explicit federation and retry logic rather than by assuming all shards are equally healthy.

## Evaluation and validation path

The module is validated through the living test and benchmark surfaces associated with query execution, optimizer behavior, and runtime safety. The practical validation points are:

- query parser/safety tests for malformed input, mutation validation, and collection access checks,
- optimizer regression tests for plan choices and cache invalidation,
- federation and timeout tests covering retry metadata and distributed failure behavior,
- OOM/timeout and hot-path checks for continuous and FTS-heavy scenarios,
- benchmark-derived evidence in the query roadmap and module references.

## Conclusion

The Query module is a control-plane layer for ThemisDB's data access engine. It is where language interpretation, optimization, execution, caching, federation, and failure management are assembled into a single, auditable runtime path. The design is intentionally explicit: keep the language expressive, the execution bounded, and the operator able to understand why a query was denied, retried, or optimized in a particular way.
