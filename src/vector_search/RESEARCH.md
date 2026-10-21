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

# Vector Search Research Notes

## Purpose

This research note captures the engineering assumptions behind the canonical vector-search implementation used by ThemisDB. The live implementation is not duplicated under `src/vector_search/`; the source-of-truth runtime is the shared index layer in `include/index/` and `src/index/`, especially `vector_index.h`, `multi_vector_search.h`, and `vector_index.cpp`.

## Problem statement

The vector-search subsystem exists to make nearest-neighbor retrieval deterministic, measurable, and safe under mixed workloads. The code organizes this around three constraints:

- index semantics must be valid for every metric (L2, COSINE, DOT),
- invalid inputs must fail closed rather than produce silent fallback results,
- large search workloads must be representable through a stable API surface for query, update, and reindex flows.

Those constraints are visible in `VectorIndexManager::Metric`, `VectorIndexManager::Status`, and the advanced-index configuration defined in `include/index/vector_index.h`.

## Research questions

### What is the actual contract in code?

The authoritative contract is a namespace-backed vector index with configurable dimensions, metric choice, and optional ANN acceleration. `VectorIndexManager` exposes lifecycle methods such as `init()`, `setAdvancedIndexConfig()`, `incrementalReindex()`, `saveIndex()`, `loadIndex()`, `shutdown()`, and direct entity mutation methods.

This is not a generic in-memory stub. It is designed as a persistent index manager whose data lives under a RocksDB-backed object namespace and keeps the HNSW/ANN layer as an optional optimization layer rather than the only valid path.

### Why do metric semantics matter?

The implementation explicitly enumerates `L2`, `COSINE`, and `DOT` metrics. This matters because ranking semantics and distance interpretation differ depending on the metric, and the implementation treats invalid or mismatched vectors as an error path rather than a silent downgrade.

The source files document several invariants:

- query vectors must match the configured dimension,
- index creation validates metric and configuration assumptions before operation,
- unsupported advanced-index combinations fail explicitly when required support is unavailable,
- no silent fallback is documented for invalid metric use.

### Why is reindexing modeled as a first-class concern?

The `incrementalReindex()` API indicates that index integrity is a runtime concern, not a one-time init concern. The implementation tracks:

- added entries,
- removed entries,
- updated entries,
- unchanged entries,
- full-rebuild triggers when stale or tombstoned state exceeds a threshold.

This design is consistent with a system that must reconcile storage state with in-memory ANN metadata without risking stale recall or inconsistent ranking.

## Assumptions and design constraints

1. Index dimension is a strict invariant.
   - `VectorIndexManager` must be initialized with a concrete dimension and all add/query operations must respect it.

2. Metric choice is part of the public contract.
   - The implementation does not treat a metric mismatch as a hidden compatibility layer.

3. Search quality is inseparable from operational safety.
   - HNSW tuning (`M`, `efConstruction`, `efSearch`) is a runtime configuration concern, not a hidden optimization.

4. Multi-vector search is an aggregation layer instead of a separate storage model.
   - `MultiVectorSearch` combines different vectors and fields using strategy-based fusion (linear combination, rank fusion, reciprocal-rank fusion, etc.).

5. Fail-closed behavior is preferred over heuristic recovery.
   - The codebase documents error paths, unsupported backends, and invalid inputs as explicit statuses or warnings, not as degraded success states.

## Methods and evaluation evidence

The vector-search stack is evaluated using the following evidence sources in the repo:

- `include/index/vector_index.h` — public lifecycle and invariant contract
- `include/index/multi_vector_search.h` — multi-query and fusion contract
- `src/index/vector_index.cpp` — runtime behavior and validation paths
- `src/index/advanced_vector_index.cpp` — advanced backend configuration and runtime integration
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp` — high-cardinality concurrency and insert validation
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` — release gate measurements

## Trade-offs

### ANN expressiveness vs. determinism

The HNSW/approximate path improves search latency through graph traversal, but it introduces approximations and reindex complexity. That is why the system keeps a strict configuration and validation layer around the graph rather than assuming approximate search is always safe.

### Multi-vector fusion vs. simplicity

`MultiVectorSearch` supports weighted and rank-based strategies, which is valuable for multi-modal and hybrid retrieval. The trade-off is more algorithmic complexity in ranking behavior, so the module must document both the strategy and its output semantics clearly.

### Shed invalid inputs vs. user convenience

The failure path is intentionally explicit. Returning an error for invalid metric, dimension, or unsupported advanced backend is more reliable than silently serving a partially valid result.

## Conclusion

The vector-search subsystem is best understood as a correctness-first retrieval layer. It favors explicit configuration, metric-aware ranking, and recoverable reindex operations over implicit heuristics. The source code strongly indicates that ThemisDB expects vector search to behave like a stable system boundary: deterministic under normal use, diagnosable under failure, and bounded by explicit operational constraints.
