# Index Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-02 -->
<!-- Evidence: test_ann_frontdoor_focused [46 PASSED] module_index_test_ann_frontdoor_focused.exe; 18 test files, 381+ test cases total -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase A–D), §7 (risk) -->

## Current Status

Production index runtime exists across vector/secondary/spatial/graph indexing, acceleration/compression pathways, and index lifecycle/rebuild/tiering operations.

**ANN Frontdoor formalized** (issue #5424): `AnnFrontdoor` is the single universal retrieval gate for all
ANN queries. All six artifact classes — Document, Chunk, Entity, Adapter, Package, ShardSummary — are
registered as first-class `AnnScopeKind` values with hot/cold routing and observability.

**Hybrid Retrieval Rollout Readiness**: 40% 🟡 (issue #5468).
- Phase A (exact-first, CPU fallback): ✅ AnnFrontdoor ready with CPU fallback enforced.
- Phase B (ANN + CPU validation): 🟡 Q3 2026 — buffer lifecycle RAII and concurrency gaps must be fixed.
- Phase C (GPU ANN): ❌ Q4 2026 or later — blocked by gpu module gaps.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] hardening backend parity and deterministic fallback across mixed GPU/runtime capabilities (Target: Q3 2026)
- [~] benchmark stabilization for vector search, rebuild, spatial, and quantization hot paths (Target: Q3 2026)
- [~] diagnostics consistency for lifecycle, rebuild, and distributed index incidents (Target: Q3 2026)
- [~] GPU vector index CUDA backend: L2, cosine, inner-product kernels (Target: Q4 2026)
- [~] GPU vector index HIP backend: AMD ROCm support with feature-parity (Target: Q4 2026)
- [~] hybrid retrieval rollout Phase B entry: buffer lifecycle RAII + concurrency hardening (Target: Q3 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [ ] Phase B gate: fix 60% of buffer lifecycle RAII gaps (7,712 total → ~4,600 target) (Target: Q3 2026)
- [ ] Phase B gate: ThreadSanitizer clean for Vec KNN insert pipeline (Target: Q3 2026)
- [x] Phase B gate: ANN result validation — output cardinality + range check before tensor layer (2026-08-09: truncation + NaN/negative distance filter added to AnnFrontdoor::search())
- [~] Phase B ctest gate: `test_ann_cpu_parity` for distance and TopK kernels (implemented; environment validation pending) (Target: Q3 2026)
- [~] Phase B benchmark gate: `bench_ann_distance_cpu_vs_flat` (implemented; environment validation pending) (Target: Q3 2026)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high-volume mixed index operation workloads (Target: Q4 2026)
- [ ] extend stress coverage for rebuild/tiering/distributed edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for backend and lifecycle degradation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for core vector and secondary index operations (Target: Q1 2027)
- [ ] broaden benchmark depth for distributed and advanced retrieval workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained multi-tenant index pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract — ANN Frontdoor (issue #5424)
- [x] AnnFrontdoor abstraction API defined (`include/index/ann_frontdoor.h`)
- [x] Decision tree HNSW / ScaNN / DiskANN / Distributed / Flat formalized
- [x] All six artifact scope kinds defined: Document, Chunk, Entity, Adapter, Package, ShardSummary
- [x] Metadata fields for ANN route-aware sharding (ShardMetadata, AnnQueryContext, AnnRetrievalPlan)
- [x] freeze core/acceleration/lifecycle contracts for active major line (2026-08-09: INDEX_CONTRACT.md created; IAnnIndex, AnnFrontdoor, lifecycle contracts frozen)
- [x] define explicit error taxonomy for backend, rebuild, and distribution failure classes (2026-08-09: index_error_codes.h created; IndexErrorCode + IndexError frozen in ranges 1100-1199)

### Phase 2: Core Implementation — ANN Frontdoor (issue #5424)
- [x] AnnFrontdoor::search() / planStrategy() / planRetrieval() implemented
- [x] Routing logic for all six artifact classes with hot/cold tier awareness
- [x] Distributed fan-out with cost-aware shard pruning
- [x] TieredIndexManager integration for hot/cold tier resolution
- [ ] complete hardening for vector/secondary/spatial/graph index internals (Target: Q4 2026)
- [ ] align quantization/compression behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases — ANN Frontdoor (issue #5424)
- [x] Missing backend → FLAT_BRUTE_FORCE fallback with degraded_continue reason code
- [x] Partial shard failures → partial_results flag + configurable fail-closed behavior
- [x] nullptr query / dim==0 guard with std::invalid_argument
- [ ] standardize fail-safe behavior for unsupported/degraded backend scenarios (Target: Q4 2026)
- [ ] unify diagnostics across rebuild/tiering/distributed failure incidents (Target: Q4 2026)

### Phase 4: Tests — ANN Frontdoor (issue #5424)
- [x] Unit tests for all routing strategies and scope kinds (tests/index/test_ann_frontdoor.cpp)
- [x] Tests for Document, Chunk, Entity scope kind routing and candidate return
- [x] Distributed fan-out, flaky shard, and retry tests
- [x] Hot/cold tier demotion tests
- [ ] expand focused regressions for mixed backend/index/lifecycle edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for high-concurrency retrieval and update workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] hot vs cold benchmark for ANN frontdoor routing paths (Target: Q4 2026)
- [ ] lock benchmark-backed release gates for index hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance — ANN Frontdoor (issue #5424)
- [x] ANN frontdoor API documented in include/index/ann_frontdoor.h (Doxygen)
- [x] Artifact class routing table documented in TARGET_ARCHITECTURE.md §2.1
- [x] HNSW vs DiskANN decision tree documented in TARGET_ARCHITECTURE.md §2.1
- [x] core index module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

### Phase 7: Migration / Go-Live — ANN Frontdoor (issue #5424)
- [x] HybridSearch::setAnnFrontdoor() integration point established
- [x] Tensor mid-layer (AdapterRepository, TensorMidLayer) integrates via setAnnFrontdoor()
- [x] Migrate all existing retrieval flows to pass through AnnFrontdoor (2026-08-09: HybridSearch + TensorMidLayer already migrated; ANNFRONTDOOR_ROLLOUT.md documents patterns + remaining callers)
- [x] Rollout guide for existing callers (2026-08-09: ANNFRONTDOOR_ROLLOUT.md created with call patterns, config, backward-compat notes)

## Production Readiness Checklist

- [x] ANN Frontdoor API stable and documented
- [x] HNSW / ScaNN / DiskANN switching validated via unit tests
- [x] All six ANN scope kinds defined and tested
- [x] Shard-aware routing with cost-aware pruning implemented and tested
- [x] core index surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for backend/lifecycle edge paths
- [ ] release benchmark stabilization complete
- [ ] hot vs cold ANN path benchmarks completed

## Known Issues and Limitations

- runtime behavior depends on backend capability, selected index strategy, and operational configuration.
- distributed and advanced acceleration edge scenarios need continued hardening.
- benchmark breadth should continue expanding for specialized index workflows.
- shard-aware routing metadata (ShardMetadata) uses fixed defaults; real cost/freshness injection is future work.

## Breaking Changes

No breaking index contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `index`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
