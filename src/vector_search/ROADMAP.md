# Vector Search Module Roadmap

<!-- Status: current | live implementation under src/index and include/index | validated: 2026-10-12 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The vector-search feature is active in the shared index subsystem. This module acts as the governance-facing summary for the feature, while the implementation remains under `src/index/` and `include/index/`.

- [x] Vector-search contract and operational expectations documented for release review
- [x] Module lifecycle and risk tracking aligned to the live index implementation
- [x] Test and benchmark artifacts connected to release gates
- [x] Module docs aligned to the canonical source ownership boundary

## In Progress

- [~] Phase 5 hardening and performance tuning in the shared index subsystem (Target: Q4 2026)
- [~] Distributed index persistence and sharding planning for larger-scale search deployments (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Define module scope and release boundaries for vector search
- [x] Document the source-of-truth ownership boundary under `src/index/`
- [x] Capture correctness, metric validation, and search-result expectations

### Phase 2: Core Implementation

- [x] Confirm the canonical implementation remains under `src/index/` and `include/index/`
- [x] Align module docs to the real implementation footprint and validation evidence
- [x] Keep the module summary focused on operational contract and risk tracking

### Phase 3: Error Handling & Edge Cases

- [x] Document invalid-dimension and NaN/Inf rejection paths
- [x] Maintain explicit handling for empty-index and unsupported-metric cases
- [x] Track planning gaps in `MODULE_GAPS.md` instead of implying unavailable features

### Phase 4: Tests

- [x] Connect module docs to soak and stress tests in `tests/integration/` and `tests/vector_search/`
- [x] Link release gates to the actual benchmark entry points
- [x] Keep the module aligned to the live validation artifacts

### Phase 5: Performance / Hardening

- [~] Continue performance tuning in the shared index subsystem
- [~] Validate benchmark gates under the existing release matrix
- [x] Keep the module docs synchronized with the current optimization scope

### Phase 6: Documentation & Acceptance

- [x] Restore and align the required governance set for the module
- [x] Keep the module reviewable and traceable to live source/test artifacts
- [x] Remove stale markdown that no longer reflects the active contract

## Production Readiness Checklist

- [x] Module governance docs present and current
- [x] Module source ownership boundaries clarified
- [x] Risk and limitation tracking updated
- [x] Benchmark, soak, and stress test references connected to the module scope
- [x] Release-facing docs aligned with the actual implementation footprint
- [~] Ongoing performance hardening remains in the shared index subsystem

## Known Issues & Limitations

1. **Implementation is externalized** — the live code is owned by the shared `index` subsystem rather than by this directory.
2. **Distributed persistence and sharding are still planned** — they are documented as future work rather than implied as current features.
3. **Performance hardening remains ongoing** — benchmark gates are valid only with the current shared index implementation.

## Breaking Changes

No breaking API or governance change is introduced by this documentation alignment; the change is a corrective stabilization of the module contract to match the actual source footprint.

## Module Evidence Sources

- `src/index/README.md`
- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/ann_index.cpp`
- `src/index/multi_vector_search.cpp`
- `include/index/multi_vector_search.h`
- `tests/integration/test_vector_search_soak.cpp`
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`

---

See [`../../ROADMAP.md`](../../ROADMAP.md) for the broader release model and exit criteria.
