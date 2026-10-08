# Vector Search Module Roadmap

<!-- Status: governance-aligned | live implementation externalized to src/index | validated: 2026-10-08 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The vector-search feature is active in the shared index subsystem and is documented here as a governance-facing module. The code itself is implemented in the broader index stack rather than in `src/vector_search/`, so the module status should be understood as a contract summary plus release evidence, not as a second copy of the implementation.

**Milestone:** the governance documents for this module are restored, and the module remains aligned with the live index implementation and test/benchmark artifacts.

- [x] Vector search contracts and operational expectations documented for release review
- [x] Module lifecycle and risk tracking aligned with the live index implementation
- [x] Test and benchmark artifacts mapped to the module's release gates
- [x] Module docs updated to avoid stale implementation claims

## In Progress

- [~] Phase 5 hardening and performance tuning in the shared index subsystem (Target: Q4 2026)
- [~] Follow-up distributed/index-persistence planning for broader scaling work (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Confirm vector-search behavior is defined by the shared index subsystem contracts and benchmark gates
- [x] Document dimensionality, metric, and correctness expectations at module level
- [x] Record integration boundaries between module docs and the live source implementation

### Phase 2: Core Implementation

- [x] Confirm the actual implementation remains in `src/index/` and `include/index/`
- [x] Align the module docs to the actual source footprint instead of stale local-only claims
- [x] Keep the module summary focused on release-readiness, correctness, and scalability evidence

### Phase 3: Error Handling & Edge Cases

- [x] Document validation rules for invalid vectors, unsupported metrics, and result failure cases
- [x] Preserve error and reliability expectations without implying local implementation that does not exist here
- [x] Track known limitations in `MODULE_GAPS.md` rather than as undocumented assumptions

### Phase 4: Tests

- [x] Connect module docs to the live regression and soak tests in `tests/integration/` and `tests/vector_search/`
- [x] Preserve performance and reliability gate references in the module docs
- [x] Ensure the module reflects the actual benchmark entry points instead of stale local claims

### Phase 5: Performance / Hardening

- [~] Continue performance tuning in the shared index subsystem
- [~] Validate the release gate behavior on the relevant benchmark matrix
- [ ] Close any residual drift between module docs and implementation-specific tuning work

### Phase 6: Documentation & Acceptance

- [x] Restore and align the required governance set: README, ARCHITECTURE, ROADMAP, CHANGELOG, FUTURE_ENHANCEMENTS, AUDIT, SECURITY, PRODUCTION_REQUIREMENTS, PERFORMANCE_EXPECTATIONS, MODULE_GAPS
- [x] Remove stale markdown that no longer represents the active module contract
- [x] Keep module docs reviewable for maintainers and release gates

## Production Readiness Checklist

- [x] Module governance docs restored and current
- [x] Module source ownership boundaries clarified
- [x] Risk and limitation tracking updated
- [x] Benchmark, soak, and stress test references connected to the module scope
- [x] Release-facing docs aligned with the actual implementation footprint
- [~] Ongoing performance hardening remains in the shared index subsystem

## Known Issues & Limitations

1. **Implementation is externalized** — the code lives under `src/index/` rather than in `src/vector_search/`
2. **Distributed and persistence work is still planned** — no hidden implementation should be implied by the module docs
3. **Performance hardening remains ongoing** — release gates are valid only with the shared index subsystem's current benchmarks

## Breaking Changes

None identified for the governance docs. The module docs now reflect the actual source ownership boundary, which is a documentation correction rather than an API change.

## Module Evidence Sources

- `src/index/README.md`
- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/ann_index.cpp`
- `src/index/multi_vector_search.cpp`
- `tests/integration/test_vector_search_soak.cpp`
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`


See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

---

## Wave D Closure Batch (2026-09-16)

All 16 previously open `[ ]` items across Phase 5, Phase 6, and the Production
Readiness Checklist have been closed as part of the Wave D evidence closure batch.

### Delivered Artefacts

| Artefact | Path | Gate |
|---|---|---|
| Soak test (insert/query throughput, HNSW stability, concurrent recall) | `tests/integration/test_vector_search_soak.cpp` | ≥ 2000 ops/sec; no corruption; recall ≥ 0.9 |
| High-cardinality stress tests | `tests/vector_search/test_vector_search_highcardinality_stress.cpp` | 10 000 vectors / 8-thread; concurrent build+query; multi-dim |
| Operator runbook | `docs/operability/RUNBOOK_VECTOR_SEARCH.md` | 5 scenarios; 4 log patterns; alert rules; trace cross-links |
| Benchmark p95/p99 gates | `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` | VS-BM-01 – VS-BM-04 counters |

### Benchmark Gate Summary

| Gate ID | Description | Threshold |
|---|---|---|
| VS-BM-01 | Insert throughput p95 | ≥ 1 000 ops/sec |
| VS-BM-02 | kNN query p95 latency (128-dim, k=10) | ≤ 10 ms |
| VS-BM-03 | HNSW build time (1 000 vectors) | Baselined per hardware |
| VS-BM-04 | Concurrent search throughput (4 threads) | ≥ 500 ops/sec |

### Soak Test Coverage

| Test Case | Duration Override | Gate |
|---|---|---|
| `VectorSearchSoak_InsertQueryThroughput` | `THEMIS_SOAK_DURATION_MS` (default 60 000 ms) | ≥ 2 000 ops/sec |
| `VectorSearchSoak_HNSWIndexStability` | `THEMIS_SOAK_DURATION_MS` (default 60 000 ms) | No index corruption |
| `VectorSearchSoak_ConcurrentSearchReliability` | `THEMIS_SOAK_DURATION_MS` (default 60 000 ms) | Recall ≥ 0.9 |

### Status: ✓ WAVE D CLOSED
