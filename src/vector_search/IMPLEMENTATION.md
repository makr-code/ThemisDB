---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Vector Search Implementation Notes

## Purpose

This document summarizes the implementation boundaries and invariants for the vector-search subsystem in ThemisDB. It is a source-grounded operating guide for the shared implementation under `src/index/` and `include/index/`, with the module directory acting as the governance and documentation boundary.

## Source of truth

The implementation is primarily in:

- `include/index/vector_index.h`
- `include/index/multi_vector_search.h`
- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/multi_vector_search.cpp`

The module docs in `src/vector_search/` should be read as a contract layer describing how those APIs are expected to be used and validated, not as a second runtime implementation.

## Runtime structure

### VectorIndexManager

`VectorIndexManager` provides the main lifecycle around the vector index.

Key responsibilities:

- initialize a namespace and dimension,
- select the metric (`L2`, `COSINE`, `DOT`),
- manage HNSW/ANN tuning and advanced backend configuration,
- add/update/remove entities,
- persist and reload state,
- reconcile storage drift through rebuild and incremental reindex flows.

The implementation also includes audit-logging hooks, expression-evaluation hooks, and optional advanced-index configuration that is rejected when required support is absent.

### MultiVectorSearch

`MultiVectorSearch` provides a higher-order search layer built on top of the vector index manager. It combines multiple vector inputs or multiple fields using a fusion strategy such as:

- linear combination,
- average or max/min score fusion,
- rank fusion,
- reciprocal-rank fusion,
- learned fusion (documented as future work rather than a current production guarantee).

This is a retrieval-aggregation layer, not a separate storage backend.

## Invariants

The implementation expects all of the following to remain true during operation:

1. Dimension consistency across index creation, insertion, and query.
2. Metric consistency across the runtime configuration and query path.
3. Fail-closed behavior for unsupported or malformed runtime states.
4. Explicit persistence and recovery operations for index lifecycle.
5. Deterministic result ordering where the result distance is treated as ranking evidence.

When these invariants are not met, the code returns explicit statuses or logs warnings rather than proceeding with silent data corruption or implicit fallback.

## Error-handling model

The source treats invalid states as explicit operational failures. Examples include:

- invalid dimensions,
- NaN/Inf vector values,
- unsupported metric selection,
- reindex triggers when storage drift becomes excessive,
- advanced backend activation without runtime support.

This is consistent with the repository's documentation model: fail closed, make the error visible, and keep the operator or developer informed.

## Reindex and recovery flows

The implementation exposes an incremental reindex path and a full rebuild path. This matters because vector indexes may drift out of sync with RocksDB-backed storage during updates or deletions.

The important operational contract is:

- detect drift using storage-vs-index comparison,
- add missing entries,
- remove stale entries,
- update changed entries,
- trigger a full rebuild when the stale proportion exceeds a threshold.

This is a reliability feature, not a hidden optimization.

## Performance and hardening constraints

The implementation and related module docs treat performance as a bounded operational concern. Specific expectations are captured in `src/vector_search/PERFORMANCE_EXPECTATIONS.md` and `src/vector_search/FUTURE_ENHANCEMENTS.md`:

- search latency remains bounded for the production target profile,
- insertion throughput remains measured against benchmark gates,
- parallel read/write workloads are stress-tested,
- advanced backend selection remains gated behind support and validation evidence.

## Validation coverage

The repo's relevant validation surface includes:

- `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- `tests/integration/test_vector_search_soak.cpp`
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`

These are the current evidence sources that establish the intended behavior and performance expectations.

## Implementation guidance for changes

When editing the vector-search runtime, keep the following priorities in order:

1. preserve metric and dimension invariants,
2. maintain reindex and persistence safety,
3. verify the public contract in the relevant test or benchmark,
4. update the module docs in `src/vector_search/` when behavior or risk changes,
5. keep the governance boundary clear: this module documents the shared implementation, it does not re-implement it.

## Conclusion

The vector-search implementation is a controlled retrieval subsystem that balances ANN search with explicit validation, recovery, and persistence. The most important rule is simple: keep the runtime contract explicit, fail closed on invalid state, and maintain the evidence trail that makes the operational behavior measurable.
