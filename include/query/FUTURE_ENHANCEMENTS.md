> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/query/FUTURE_ENHANCEMENTS.md -->

# Query Module — Public Header Future Enhancements

**Module Path:** `include/query/`
**Canonical implementation enhancements:** [`../../src/query/FUTURE_ENHANCEMENTS.md`](../../src/query/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/query/`. Implementation-level enhancements are in:

→ [`../../src/query/FUTURE_ENHANCEMENTS.md`](../../src/query/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` `AQLParser` must remain stateless and trivially constructable.
- `[x]` `IQueryEngine` must remain a pure interface.
- `[x]` `[[nodiscard]]` on all factory functions and error-returning query methods.
- `[x]` `#pragma once` on every header.
- `[x]` No Boost or third-party headers leaked into `include/query/` public surface.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `AQLParser::parse()` | `aql_parser.h` | AQL runner, tests | ✅ Stable |
| `IQueryEngine::execute()` | `query_engine.h` | Server, tests | ✅ Stable |
| `QueryOptimizer::optimize()` | `query_optimizer.h` | Query compiler | ✅ Stable |
| `PlanCache::get() / put()` | `plan_cache.h` | Query engine | ✅ Stable |
| `ContinuousQueryEngine::submit()` | `continuous_query_engine.h` | CDC, server | ✅ Stable |
| `SemanticCache::lookup() / store()` | `semantic_cache.h` | Query engine, LLM bridge | ✅ Stable |
| `QueryCanceller::cancel()` | `query_canceller.h` | Server, admin API | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `query_explain_api.h` — structured EXPLAIN output with JSON/DOT representations; enables tooling-level plan inspection without string parsing.
- Extend `IQueryEngine` with `executeAsync()` returning `std::future<ResultStream>` for non-blocking execution.
- Extend `semantic_cache.h` to accept embedding provider as a template parameter for testability.

### Medium-Term (Q4 2026)

- `query_admission_controller.h` — per-priority admission control based on `QueryResourceLimits`; queues or rejects low-priority queries under resource pressure.
- `query_tracing_context.h` — OpenTelemetry span context propagation through the query pipeline.
- Deprecate direct `continuous_query_engine_impl.h` include; redirect to factory via `continuous_query_engine.h`.

### Long-Term

- Unified tensor-query API: merge `tensor_aware_query_optimizer.h` and `tensor_contraction_engine.h` into a single `ITensorQueryExtension` interface that plugs into `IQueryEngine`.
- SPARQL 1.2 federation protocol support via extension of `cross_cluster_federation.h`.
