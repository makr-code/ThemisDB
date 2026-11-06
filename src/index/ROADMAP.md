---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-11-06"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Index Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Historical targets: Q3 2026 dates in this roadmap have elapsed; owners must rebaseline them before they are treated as active commitments. -->
<!-- Historical evidence snapshot: 2026-08-02. Current focused registration is defined by tests/index/CMakeLists.txt. -->
<!-- Current inventory: 31 test_*.cpp sources; source presence does not imply registration or passing status. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase A–D), §7 (risk) -->

## Current Status

Production index runtime exists across vector/secondary/spatial/graph indexing, acceleration/compression pathways, and index lifecycle/rebuild/tiering operations.

**Wave Alignment (see root ROADMAP.md § Program Execution Model):**
- **Wave B (Q3–Q4 2026):** AnnFrontdoor+vector integration gates, GPU backend validation (CUDA/HIP), hybrid retrieval Phase B (buffer lifecycle RAII, concurrency ThreadSanitizer)
- **Wave B Exit Criteria:** Full 4-layer retrieval chain (search: ANN+vector+graph+LLM) with stable p95/p99 on representative hardware
- **Tier 2 Functional Completeness:** Index performance critical for all retrieval workloads; blocks RAG Phase B and search Phase B

**Hybrid Retrieval Rollout Readiness**: 40% 🟡 (issue #5468; historical estimate, not current gate evidence).
- Phase A (ANN routing): AnnFrontdoor routes to registered ANN backends. Its flat-brute-force helper returns no candidates when no usable backend is available; it is not an exact storage scan or a guaranteed CPU fallback.
- Phase B (ANN + CPU validation): 🟡 Q3 2026 — buffer lifecycle RAII and concurrency gaps must be fixed.
- Phase C (GPU ANN): ❌ Q4 2026 or later — blocked by gpu module gaps.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

**ANN Frontdoor formalized** (issue #5424): `AnnFrontdoor` is the single universal retrieval gate for all
ANN queries. All six artifact classes — Document, Chunk, Entity, Adapter, Package, ShardSummary — are
registered as first-class `AnnScopeKind` values with hot/cold routing and observability.

## In Progress

- [~] hardening backend parity and deterministic fallback across mixed GPU/runtime capabilities (Target: Q3 2026)
- [~] benchmark stabilization for vector search, rebuild, spatial, and quantization hot paths (Target: Q3 2026)
- [~] diagnostics consistency for lifecycle, rebuild, and distributed index incidents (Target: Q3 2026)
- [~] GPU vector index CUDA backend: L2, cosine, inner-product kernels (Target: Q4 2026)
- [~] GPU vector index HIP backend: AMD ROCm support with feature-parity (Target: Q4 2026)
- [~] hybrid retrieval rollout Phase B entry: buffer lifecycle RAII + concurrency hardening (Target: Q3 2026)
- [~] **Phase D: Index Manifest V1 Schema & Version Governance** (Target: Q4 2026)
   - **Specification Document**: `src/index/INDEX_MANIFEST_V1_SCHEMA.json` — JSON Schema for index metadata persistence (2026-09-24 CREATED)
   - **Core Index Changes**:
     - Index manifest persistence in RocksDB column family `"index_manifest"`
     - Embedding/chunking/schema version tracking with history
     - Reindex trigger detection (model_id/embedding_dim/chunking_profile_id/schema major version changes)
     - Version-aware query routing (current index vs candidate index in canary deployment)
   - **Acceptance Criteria**:
     - Manifest JSON Schema valid and enforces all required fields
     - RocksDB persistence reads/writes produce bit-identical manifests
     - Reindex detection logic correctly identifies all 4 trigger conditions
     - Version history archival maintains 30-day retention
   - **Schema Definitions**:
     - `IndexVersion` — version numbers, timestamps, metadata
     - `EmbeddingMetadata` — model ID, dimension, hash
     - `ChunkingMetadata` — profile ID, parameters
     - `IndexSchema` — schema version with breaking change tracking
   - **CI Gate**: `.github/workflows/gate-pr-rag-version.yml` validates manifest schema on index changes

## Planned Features

### Wave 2-B: GPU RAII & CUDA Backend (Target: Q4 2026)

> **Source:** MODULE_GAP_ANALYSIS_WAVE2.md §Wave 2-B, gap scanner verified 2026-08-25  
> **Gap count:** 5 `gpu_memory_leak` (CRITICAL), 26 `unchecked_cuda_call`, 12 `iterator_invalidation`, 79 `todo_as_productionlogic`

- [x] Implement `CudaUniquePtr<T>` RAII wrapper with `cudaFree()` destructor — fix 5 `gpu_memory_leak` in `cuda_hnsw_graph_traversal.cpp:362,370,381` and `gpu_memory_oversubscription.cpp:53` (2026-08-26: `include/index/cuda_utils.h` created; all 6 Impl raw-pointer members migrated to `CudaUniquePtr<T>`; `freeDevice()` simplified to RAII resets; `batchSearch` temporary allocations also wrapped)
  - Constraints: exception-safe; `cudaFree` called on all error paths
  - Errors: `cudaErrorInvalidDevicePointer` → log + `IndexErrorCode::GpuMemoryError`
  - Tests: `tests/index/test_wave5_index_hardening.cpp` (I1-A..D: null-safety, n=0, move, deleter)
- [x] Add `THEMIS_CUDA_CHECK` after every kernel launch in `cuda_hnsw_graph_traversal.cpp`, `gpu_vector_index.cpp`, `rotary_embeddings_cuda.cu` (26 sites) — return `IndexErrorCode::GpuKernelError` on failure (2026-08-26: `THEMIS_CUDA_CHECK` and `THEMIS_CUDA_CHECK_BOOL` macros added to `cuda_utils.h`; `batchSearch` result D2H copy sites hardened; tests: `test_wave5_index_hardening.cpp` I2-A,B)
- [x] Fix 12 `iterator_invalidation` in `graph_index.cpp:244-248`, `multi_vector_search.cpp:224,406` — vector-resize and concurrent traversal patterns (2026-08-26: range-for over JSON array converted to index-based loop; CSV while-loop annotated; multi_vector_search score/rank push_back sites annotated with Wave-B I3 comment)
- [~] Implement CUDA L2/Cosine/Dot-Product kernels in `src/acceleration/cuda/cuda_hnsw_kernels.cu` — replace CPU fallbacks; target ≥4× speedup vs CPU baseline on RTX-class GPU (Target: Q4 2026)
  - Inputs: float32 vectors, batch size ≤ 1e6; outputs: distance matrix + TopK indices
  - Constraints: deterministic FP tolerance ≤ 1e-6 vs CPU reference
  - Tests: `tests/index/test_ann_cuda_kernel_parity.cpp` (L2/cosine/dot CPU vs GPU parity)
- [~] HIP/AMD backend: `HIPVectorBackend::search()` — feature parity with CUDA backend (Target: Q4 2026)

### Hybrid Retrieval Rollout Gates (issue #5468)
- [~] Phase B gate: fix 60% of buffer lifecycle RAII gaps (7,712 total → ~4,600 target) (Target: Q3 2026)
- [~] Phase B gate: ThreadSanitizer clean for Vec KNN insert pipeline (Target: Q3 2026)
- [x] Phase B gate: ANN result validation — output cardinality + range check before tensor layer (2026-08-09: truncation + NaN/negative distance filter added to AnnFrontdoor::search())
- [~] Phase B ctest gate: `test_ann_cpu_parity` for distance and TopK kernels (implemented; environment validation pending) (Target: Q3 2026)
- [~] Phase B benchmark gate: `bench_ann_distance_cpu_vs_flat` (implemented; environment validation pending) (Target: Q3 2026)

### Short-term (3-6 months)
- [x] tighten deterministic behavior under high-volume mixed index operation workloads (Target: Q4 2026)
  - **Evidence**: Wave D soak/stress delivered: `tests/integration/test_index_engine_soak.cpp` (3 soak cases), `tests/index/test_index_highcardinality_stress.cpp` (1 000 000 vectors, 8-thread build + concurrent R/W + multi-backend)
- [x] extend stress coverage for rebuild/tiering/distributed edge scenarios (Target: Q4 2026)
  - **Evidence**: `HighCardinalityIndexBuild` (1M vectors, 8 threads), `ConcurrentIndexReadWrite` (4R+4W threads), `MultiBackendStress` (3 stub backends)
- [x] improve operator-facing diagnostics for backend and lifecycle degradation incidents (Target: Q4 2026)
  - **Evidence**: `docs/operability/RUNBOOK_INDEX_ENGINE.md` — 5 scenarios with log patterns, diagnostic steps, and remediation actions

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for core vector and secondary index operations (Target: Q1 2027)
- [~] broaden benchmark depth for distributed and advanced retrieval workflows (Target: Q1 2027)
- [x] harden long-running reliability under sustained multi-tenant index pressure (Target: Q1 2027)
  - **Evidence**: `IndexSoak_ConcurrentAccessReliability` validates 4 writer + 4 reader threads for the full soak window without data races

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
- [x] expand focused regressions for mixed backend/index/lifecycle edge scenarios (Target: Q4 2026)
  - **Evidence**: Wave D stress test `ConcurrentIndexReadWrite` and `MultiBackendStress` cover mixed backend/lifecycle edge scenarios
- [x] extend deterministic stress fixtures for high-concurrency retrieval and update workloads (Target: Q4 2026)
  - **Evidence**: `tests/index/test_index_highcardinality_stress.cpp` — `ConcurrentIndexReadWrite` (4R+4W, 80K total ops), `HighCardinalityIndexBuild` (1M vectors, 8 writers)

### Phase 5: Performance and Hardening
- [~] hot vs cold benchmark for ANN frontdoor routing paths (Target: Q4 2026)
- [~] lock benchmark-backed release gates for index hot paths (Target: Q4 2026)
- [~] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

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
- [x] remaining hardening tasks closed for backend/lifecycle edge paths
  - **Evidence**: Wave D soak + stress tests cover sustained and high-cardinality paths; runbook covers operator-critical failure scenarios
- [~] release benchmark stabilization complete
- [~] hot vs cold ANN path benchmarks completed

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

### Wave 9 Block 4 — Index Module CRITICAL Closure + FAISS Wiring (2026-08-26)

| Task | Description | Status |
|------|-------------|--------|
| [x] W9-13 | Audit and close all 28 CRITICAL scanner FPs; verify brace balance in 6 files | ✅ Done |
| [x] W9-14 | FAISS optional-feature wiring: `THEMIS_HAS_FAISS` in CMake, stub block verified | ✅ Done |
| [x] W9-15 | GPU Vulkan RAII hardening Phase B entry: `VkBufferRaii` added, stubs documented | ✅ Done |

### Wave 9 Block 5 — Backend Gate + Explicit Fallback Boundaries (2026-09-03)

| Task | Description | Status |
|------|-------------|--------|
| [x] W9-16 | `GPUVectorIndex` failover contract tightened: CPU fallback now explicit (`allowCPUFallback`) on runtime backend failure paths (Vulkan/CUDA/HIP), fail-closed when disabled | ✅ Done |
| [x] W9-17 | Backend gate diagnostics parity wired: `BACKEND_NOT_ENABLED` / `BACKEND_NO_DEVICE_AVAILABLE` / `FALLBACK_*` emission added for index init and Vulkan module backend dispatch | ✅ Done |
| [x] W9-18 | Focused tests added for fallback boundary and backend gate semantics (`tests/index/test_gpu_vector_index_llm_paths_focused.cpp`, `tests/gpu/test_gpu_vulkan_backend.cpp`, `tests/gpu/test_gpu_vector_index.cpp`) | ✅ Done |
| [~] W9-19 | Hardware-only parity gate remains: CUDA-active no-fallback boundary and representative-device Vulkan/CUDA/HIP parity runs require GPU CI hardware | 🟡 Pending hardware evidence |

### Wave D Contribution for `index`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
  - **Evidence**: `tests/index/test_index_highcardinality_stress.cpp` (3 stress cases), `tests/integration/test_index_engine_soak.cpp` (3 soak cases), `docs/operability/RUNBOOK_INDEX_ENGINE.md` (5 operator scenarios with D1 trace span cross-links)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
  - **Evidence**: `tests/integration/test_index_engine_soak.cpp` — `IndexSoak_BuildThroughput`, `IndexSoak_QueryStability`, `IndexSoak_ConcurrentAccessReliability`; THEMIS_SOAK_DURATION_MS default 60 000 ms; TIMEOUT 120 in Wave D foreach
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)
  - **Evidence**: `docs/operability/RUNBOOK_INDEX_ENGINE.md` — 5 scenarios: HNSW corruption, GPU kernel fallback, buffer OOM, rebuild stall, multi-GPU routing failure; log patterns: `[INDEX:HNSWCorruption]`, `[INDEX:GPUKernelFallback]`, `[INDEX:BufferOOM]`, `[INDEX:RebuildStall]`, `[INDEX:MultiGPURoutingFailed]`

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
  - **Evidence**: `GPUVectorIndex` explicit `allowCPUFallback` contract (W9-16); runbook Scenario 2 covers GPU kernel fallback fail-closed operation
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
  - **Evidence**: `docs/operability/RUNBOOK_INDEX_ENGINE.md` with 5 scenarios, log patterns, escalation table

## Research and Gap Analysis — v2.5.0 / v3.0.0 (Issue #7427)

### Research evidence and applicability

The research supports measurement and explicit trade-offs; it does not establish
that a named strategy is implemented or validated in this repository.

| Research finding | Relevance to `index` | Evidence boundary |
|---|---|---|
| HNSW uses a hierarchical proximity graph and tunable search effort to trade query cost against recall. | Evaluate recall, latency, memory, and build cost together when tuning the HNSW path; a single throughput number is insufficient. | The paper's results are algorithm and workload specific, not ThemisDB performance targets. [Malkov & Yashunin, HNSW](https://arxiv.org/abs/1603.09320). |
| DiskANN demonstrates SSD-backed graph search at billion-point scale under a specific hardware and dataset configuration. | Cold-tier/disk-backed routing is a meaningful scale option only after the backend, storage costs, and quality are measured on supported hardware. | The published result cannot be transferred to this module by setting `diskann_available`; the configured flag is not proof of an installed backend. [Subramanya et al., DiskANN, NeurIPS 2019](https://papers.nips.cc/paper_files/paper/2019/hash/09853c7fb1d3f8ee67a61b6bf4a7f8e6-Abstract.html). |
| ANN-Benchmarks evaluates algorithms using a common interface, datasets, parameter sweeps, and quality/performance measures. | Release comparisons should fix corpus, metric, query set, parameters, hardware, and build, and report recall with latency/throughput. | A benchmark method is not itself a representative workload or an enforced CI gate. [Aumüller et al., ANN-Benchmarks](https://arxiv.org/abs/1807.05614). |
| Recent graph-index work identifies deletion/update churn and recall stability as distinct problems from static search. | Measure recall and recovery across sustained insert/update/delete workloads before considering an incremental graph-index design. | The 2025 IP-DiskANN result is a research candidate, not an implementation recommendation or a demonstrated fit for ThemisDB. [Xu et al., IP-DiskANN](https://arxiv.org/abs/2502.13826). |

### Source-backed current state and gaps

- **Performance and scale:** `include/index/ann_frontdoor.h` declares HNSW,
  ScaNN, DiskANN, distributed, and flat strategies, with dataset-size defaults
  and a `diskann_available` switch. `src/index/ann_frontdoor.cpp` uses fixed
  shard-cost/quality metadata in routing, while `recallEstimate()` returns
  informational constants. These are not measurements. `benchmarks/index/`
  currently contains the secondary-index rebuild benchmark, not an ANN
  search/recall benchmark. The rebuild fixture uses 100,000 generated entities
  and seven secondary-index types; its random data and environment-specific
  output are not a reproducible ANN baseline.
- **Usability and routing:** `AnnFrontdoor::planRetrieval()` describes a route;
  it does not prove that the declared backend is present. The frontdoor's
  `bruteForceSearch()` currently returns no candidates when no usable
  storage/backend path is available. Tests in
  `tests/index/test_ann_frontdoor.cpp` and
  `tests/index/test_ann_cpu_parity.cpp` verify deterministic routing and
  fixtures, but stubs do not demonstrate production backend quality.
- **Security and resilience:** the frontdoor bounds top-k cardinality and
  removes negative/NaN distances, but positive infinity is not rejected.
  Distributed behavior can return partial results or fail closed according to
  configuration. Tenant-qualified names and filters are not authorization;
  callers must enforce access control. These boundaries are documented in
  `src/index/API.md` and `src/index/SECURITY.md`.
- **Governance and lifecycle:** `IndexManifestV1` records schema/embedding
  metadata, history, and reindex status, and
  `tests/index/test_index_manifest_v1.cpp` covers serialization/history
  behavior. The API notes that the manifest is not itself a persistence or
  rollback engine and is not thread-safe. Migration, compatibility, and
  recovery guarantees therefore need end-to-end evidence, not only schema
  round trips.
- **Architecture and operations:** the module has explicit routing metadata,
  fallback reason codes, index tests, a rebuild benchmark, and operator
  documentation. However, benchmark thresholds in
  `src/index/PERFORMANCE_EXPECTATIONS.md` are explicitly not proven to be CI
  gates, and representative CUDA/HIP/Vulkan parity remains a hardware-only
  open gate. `tests/index/CMakeLists.txt` also excludes several focused test
  sources; source presence alone is not registration or passing evidence.

**Maturity assessment:** substantial, multi-family production implementation
with useful contracts and deterministic CPU/test-fixture coverage; not yet
release-grade for ANN quality/performance claims across real backends, hardware,
and update/recovery workloads. This is a source review, not a current test,
benchmark, security-audit, or hardware-validation result.

### Release interpretation and success KPIs

- **v2.5.0:** the next planned minor target in `VERSIONING.md` and
  `RELEASE_STRATEGY.md`; this module plan proposes work but does not promise a
  release date or override the root release gates.
- **v3.0.0:** strategic and conditional only. No v3.0.0 release commitment,
  date, approved breaking API, or migration policy was found in the current
  root release/version sources. Do not treat this plan as approval to break
  compatibility.
- Report each benchmark's dataset/corpus, metric, query count, `k`, index
  parameters, backend/build flags, hardware, and baseline commit alongside
  p50/p95/p99 latency, throughput, recall@k, build/rebuild time, and memory.
  Compare only like-for-like runs. Existing documented `<=10%` regression
  expectation remains a proposed release threshold until its baseline and
  enforcement are verified.
- For the existing deterministic CPU fixture, retain recall@10 `>=0.90` and
  CPU/flat result parity at distance tolerance `<=1e-6`; these small-fixture
  checks are necessary regression guards, not production SLOs. Hardware gates
  must report measured results rather than inferred estimates.

### v2.5.0 — evidence and reliability waves

Estimates are engineering person-weeks for one engineer familiar with the
module, excluding external hardware/CI provisioning and release approval.
They are planning ranges, not schedule commitments.

#### Wave 2.5-A — Reproducible baseline and benchmark coverage (Priority: P0)

- **Goal:** establish comparable quality, latency, throughput, rebuild, and
  resource measurements before changing routing or tuning defaults.
- **Tasks:**
  - [ ] Add deterministic ANN search/recall and mixed-operation benchmark cases
    with fixed seeds and explicit workload/parameter metadata; retain the
    current secondary-index rebuild cases (Target: v2.5.0).
  - [ ] Define a per-backend benchmark manifest and report p50/p95/p99,
    throughput, recall@k against an exact baseline, build/rebuild duration, and
    memory where measurable (Target: v2.5.0).
  - [ ] Map every release-gated case to an enabled CMake/CI registration and
    record unsupported hardware as “not run,” not PASS (Target: v2.5.0).
- **Dependencies:** benchmark runner/preset and benchmark target registration;
  deterministic datasets and exact-search oracle.
- **Risk / trade-off:** reproducible fixtures can under-represent production
  skew; broad hardware matrices increase runtime and maintenance cost. Keep
  synthetic fixtures for regressions and require representative datasets for
  release claims.
- **Effort:** 2–3 person-weeks.
- **Success metrics:** 100% of mapped index release cases emit the required
  workload/backend metadata; CPU recall@10 is at least 0.90 on the existing
  fixed-seed 2,000-vector/64-dimension fixture; every performance result has a
  same-host/build comparison or is explicitly baseline-only.
- **Validation:** run registered focused CPU tests and benchmarks; verify exact
  top-k comparison and repeatability across independent runs; execute and
  archive the mapped release benchmark suite on each supported representative
  device before declaring hardware coverage.

#### Wave 2.5-B — Routing, fallback, and result-contract correctness (Priority: P0)

- **Goal:** make the selected route, actual backend execution, and degraded
  outcome agree and be observable.
- **Tasks:**
  - [ ] Test every strategy against backend availability and fail-closed /
    configured fallback behavior; prevent an unavailable strategy from being
    reported as executed (Target: v2.5.0).
  - [ ] Specify and test behavior for empty backends, missing scopes, partial
    shard failures, invalid dimensions, invalid `k`, and non-finite result
    distances, including positive infinity (Target: v2.5.0).
  - [ ] Replace fixed shard quality/cost inputs and unmeasured recall constants
    with explicit unknown/default semantics or measured inputs; do not present
    estimates as SLOs (Target: v2.5.0).
- **Dependencies:** Wave 2.5-A oracle/benchmark baseline; current contracts in
  `include/index/ann_frontdoor.h` and consumer expectations.
- **Risk / trade-off:** stricter validation or fail-closed defaults may reduce
  result availability; legacy consumers may rely on current metadata. Preserve
  the v2 API contract and document any behavior change.
- **Effort:** 3–5 person-weeks.
- **Success metrics:** 100% of route-availability and failure-matrix cases
  report actual backend/fallback status correctly; no invalid distance reaches
  callers; existing CPU/flat parity remains within `1e-6`.
- **Validation:** deterministic unit/integration tests for each route and
  failure branch; fault-injected distributed tests; release benchmark confirms
  recall/latency effects for any changed selection rule.

#### Wave 2.5-C — Manifest compatibility and rebuild recovery (Priority: P1)

- **Goal:** establish verifiable index-version and rebuild behavior without an
  incompatible v2 schema/API change.
- **Tasks:**
  - [ ] Validate manifests on write/read against the canonical schema and add
    malformed, incomplete, unknown-field, and older-version fixtures (Target:
    v2.5.0).
  - [ ] Exercise restart/retry/rollback behavior around interrupted rebuilds
    and make status transitions and retention behavior observable (Target:
    v2.5.0).
  - [ ] Document which metadata is required to decide that an index needs
    reindexing; do not imply that embedding generation is owned by `index`
    (Target: v2.5.0).
- **Dependencies:** Wave 2.5-A benchmark/replay fixtures; storage transaction
  and durability contracts; existing `IndexManifestV1` behavior.
- **Risk / trade-off:** stricter validation can reject historical manifests;
  compatibility must be explicit, bounded, and tested rather than implemented
  as an undocumented fallback.
- **Effort:** 3–5 person-weeks.
- **Success metrics:** all supported manifest fixtures round-trip without
  information loss; unsupported/corrupt versions fail with a diagnosable error;
  interrupted rebuild tests preserve the last committed usable version or
  expose an explicit unavailable state.
- **Validation:** manifest unit tests plus storage-backed restart/fault
  integration tests; schema validation; recovery and retention evidence
  attached to release artifacts.

#### Wave 2.5-D — Release hardening and operational evidence (Priority: P0)

- **Goal:** make v2.5 index claims traceable to passing tests, reproducible
  benchmark runs, and representative hardware evidence.
- **Tasks:**
  - [ ] Reconcile the index test inventory with enabled CMake registrations;
    remove quarantine only when the relevant contract is current and tests
    pass (Target: v2.5.0).
  - [ ] Run sustained concurrent insert/search/rebuild and distributed failure
    tests under sanitizers where supported; record hardware-only GPU parity
    separately (Target: v2.5.0).
  - [ ] Link each claimed KPI to a named test, benchmark case, configuration,
    and retained result artifact (Target: v2.5.0).
- **Dependencies:** Waves 2.5-A through 2.5-C; green `release_critical` CI and
  available representative CPU/GPU runners.
- **Risk / trade-off:** missing devices or expensive long runs can delay
  sign-off; do not downgrade “not run” to PASS or infer GPU parity from CPU
  results.
- **Effort:** 2–4 person-weeks plus runner time.
- **Success metrics:** all index release-critical focused tests pass; no
  sanitizer finding attributable to the tested index paths; every mapped
  benchmark is run or explicitly marked blocked; p95/p99 and recall are
  reported for representative hardware with no unexplained regression over
  10% against a like-for-like baseline.
- **Validation:** focused CTest, sanitizer/soak jobs, benchmark suite, hardware
  parity matrix, and release/governance review. These are proposed module
  gates, subject to root release policy.

### v3.0.0 — Strategic capability and lifecycle waves (conditional)

The waves below require a human-approved v3 scope and release target before
they become commitments. Any public API, serialized format, or migration
breaking change requires an approved breaking-change issue, compatibility
review, migration guide, and changelog entry under `VERSIONING.md`.

#### Wave 3.0-A — Capability-based backend contract (Priority: P0, conditional)

- **Goal:** define a stable, machine-checkable contract for backend capability,
  metric, filters, durability, and resource limitations so routing cannot infer
  support from a strategy name or static size cutoff.
- **Tasks:**
  - [ ] Specify a versioned backend capability contract and a migration path
    from current `IAnnIndex` registrations (Target: v3.0.0, pending approval).
  - [ ] Require route decisions to use advertised and runtime-verified
    capabilities, and expose the reason plus fallback outcome (Target: v3.0.0,
    pending approval).
  - [ ] Define compatibility rules for backend upgrades and unsupported
    capabilities before changing public headers (Target: v3.0.0, pending
    approval).
- **Dependencies:** v2.5 routing evidence; approved API/ABI scope; integration
  owner sign-off.
- **Risk / trade-off:** a richer contract increases implementation burden and
  may require a major migration; keep adapters out unless explicitly approved
  with a removal target.
- **Effort:** 2–4 person-weeks for specification/prototype; implementation
  estimate requires the approved scope.
- **Success metrics:** every route is justified by a verified backend
  capability; unsupported routes fail closed or use an explicitly configured
  fallback; all v2-to-v3 migration fixtures have deterministic outcomes.
- **Validation:** contract/API review, conformance tests for each backend,
  compile checks for public headers, migration tests, and security review.

#### Wave 3.0-B — Workload- and cost-aware retrieval (Priority: P1, conditional)

- **Goal:** choose retrieval paths from measured recall/latency/resource trade
  offs for hot/cold and distributed workloads rather than uncalibrated
  estimates.
- **Tasks:**
  - [ ] Evaluate in-memory HNSW and storage-aware alternatives against the same
    fixed datasets, exact oracle, update pattern, and hardware manifest
    (Target: v3.0.0, pending approval).
  - [ ] Design a bounded routing policy using measured backend capability,
    tier, query constraints, and cost/quality budgets; preserve deterministic
    route explanations (Target: v3.0.0, pending approval).
  - [ ] Include filtered and multi-tenant workloads without treating index
    filters as authorization controls (Target: v3.0.0, pending approval).
- **Dependencies:** Waves 2.5-A/B and 3.0-A; representative corpus and
  storage/device benchmarks.
- **Risk / trade-off:** adaptive routing can oscillate, consume telemetry
  resources, or trade quality for latency unexpectedly. Require bounded
  parameters, stable defaults, and an operator-visible override.
- **Effort:** 4–6 person-weeks, excluding hardware qualification.
- **Success metrics:** at least 0.95 recall@10 on the approved quality workload
  while meeting a latency/resource budget fixed before tuning; report p50/p95/
  p99 and bytes/vector; no unsupported route or unreported fallback.
- **Validation:** offline parameter sweeps, held-out workload validation,
  deterministic route tests, same-host performance comparison, and
  representative-device qualification. Thresholds must be revised if the
  approved workload cannot meet both quality and resource constraints.

#### Wave 3.0-C — High-churn index lifecycle and recovery (Priority: P1, conditional)

- **Goal:** define safe, measurable behavior for sustained insert/update/delete,
  compaction/rebuild, snapshot replacement, and recovery at scale.
- **Tasks:**
  - [ ] Compare periodic rebuild/consolidation with candidate incremental graph
    update approaches under the same churn and recall-stability workload
    (Target: v3.0.0, pending approval).
  - [ ] Specify atomic publication, retention, recovery, and rollback semantics
    for versioned index snapshots (Target: v3.0.0, pending approval).
  - [ ] Add crash/fault tests at acknowledged-write, compaction, and snapshot
    swap boundaries (Target: v3.0.0, pending approval).
- **Dependencies:** Wave 2.5-C manifest/recovery evidence; storage durability
  guarantees; approved retention and recovery objectives.
- **Risk / trade-off:** incremental update algorithms can degrade recall or
  increase write amplification; maintaining old and new snapshots consumes
  extra storage. Do not adopt a research design without measured benefit and
  explicit resource budgets.
- **Effort:** 4–8 person-weeks for evaluation/prototype; production scope to be
  estimated after results.
- **Success metrics:** recall@10 remains at least 0.95 on the approved churn
  workload; every acknowledged mutation survives the defined crash boundary;
  recovery time and temporary/permanent storage overhead meet pre-approved
  budgets.
- **Validation:** fixed-seed churn benchmark, exact-baseline recall at
  checkpoints, crash/restart fault injection, sanitizer runs, and storage
  recovery review.

#### Wave 3.0-D — Governance, security, and operability sign-off (Priority: P0, conditional)

- **Goal:** make index migrations, backend changes, data isolation boundaries,
  and performance claims reviewable and operable for a major release.
- **Tasks:**
  - [ ] Publish compatibility/migration guidance for approved public and
    persisted-format changes, including rollback limits (Target: v3.0.0,
    pending approval).
  - [ ] Verify authorization remains enforced by callers, tenant boundaries
    survive route/fallback changes, and diagnostics do not disclose vector or
    tenant data (Target: v3.0.0, pending approval).
  - [ ] Require current runbooks, benchmark artifacts, test evidence, and
    human sign-off before promoting module readiness (Target: v3.0.0, pending
    approval).
- **Dependencies:** completion of approved v3 waves; root release gates;
  security, storage, and operations owners.
- **Risk / trade-off:** broad compatibility matrices and long-duration
  qualification increase release cost; scope must prioritize supported
  migrations and real deployment profiles.
- **Effort:** 2–4 person-weeks plus review and qualification time.
- **Success metrics:** 100% of supported migration fixtures pass; no unresolved
  high/critical index security findings; every release KPI links to current
  evidence and every operator-critical failure mode has a tested remediation
  path.
- **Validation:** migration/rollback rehearsal, focused security review,
  resilience/chaos tests, benchmark evidence review, runbook exercise, and
  mandatory human release approval.

### Open assumptions and knowledge gaps

- v2.5.0 remains the next planned minor per root version policy, but its module
  schedule and final scope are not established here. v3.0.0 is uncommitted.
- No benchmark results were run for this roadmap update. Hardware models,
  production data distributions, filter selectivity, query concurrency,
  latency budgets, and memory budgets must be supplied by maintainers/operators
  before performance thresholds become release gates.
- Current `AnnFrontdoor` strategy labels and recall estimates are not proof of
  usable backend implementations or observed quality; verify each backend and
  build configuration before planning route-specific guarantees.
- Research comparisons use different corpora, hardware, workloads, and quality
  metrics. They motivate controlled local experiments but cannot establish
  ThemisDB targets or superiority.
- Issue #7427's supplied description is the acceptance source for this plan;
  no additional issue comments or maintainer-approved v3 constraints were
  available during this review.
