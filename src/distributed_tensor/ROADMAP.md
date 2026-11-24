# Distributed Tensor Module Roadmap

**Last Updated:** 2026-11-24
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `include/distributed_tensor/local_projection_publisher.h`, `include/distributed_tensor/shard_rpc_fragment_fetcher.h`, `proto/sharding/shard_rpc.proto`, `src/main_server.cpp`, `cmake/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/sharding/CMakeLists.txt`, `tests/sharding/test_shard_transfer_grpc.cpp`, `benchmarks/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
**Related:** Issues #7341, #7414

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Source inventory reconciled: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phases A–D) -->

## Current Status

The current module target is `themis_distributed_tensor`, built from the source
list in `src/distributed_tensor/CMakeLists.txt`. It declares ten public headers.
The target publicly requests `cxx_std_20`, matching the `std::span` API in
`tensor_storage_strategy.h`; CMake consumers linking the target inherit the
requirement, and direct header consumers must compile as C++20. Local CMake
files register three focused test executables and
one in-process benchmark under `tests/distributed_tensor/` and
`benchmarks/distributed_tensor/`. Separately, `tests/CMakeLists.txt` adds
`tests/epic3_distributed_tensor/`, whose CMake registers the EPIC 3
phase/contract sources. No distributed-tensor soak is registered under
`tests/integration`: the former in-process stub-only source was removed because
it exercised no production transport. A focused ShardRPC gRPC/mTLS
fragment-fetch test is conditionally registered as `ShardTransferGrpcTests`
under `tests/sharding/`; its original case uses a test reader. An additional
source case stores tenant adapters through `AdapterRepository`, closes and
reopens RocksDB, reloads the records to build a fixture fingerprint graph,
then publishes and retrieves its summary through mTLS and the manifest-bound
adapter; graph validation is included when the query and LLM module targets
are available. The two `ShardFragmentGrpc.*` tests passed 2/2, the complete
test executable passed 8/8, and `ShardTransferGrpcTests` passed 1/1 in CTest
on 2026-11-24. This fixture path does not exercise production host
configuration or a distributed-tensor soak.
A versioned principal/shard/artifact RocksDB mapping
and opt-in `main_server.cpp` startup are now implemented in source. A local
single-shard publisher persists a remote-readable manifest record; callers can
fetch it through mTLS when they already have a trusted artifact ID. An opt-in
`main_server.cpp` startup publisher now loads and validates one configured
tenant's adapter records, builds the fingerprint graph, and publishes one
configured query projection when the ShardRPC adapter target is linked. The
server link is independent of test-target registration, so production wiring
does not disappear when focused sharding tests are disabled.
Continuous producer updates, discovery and freshness remain open. A versioned adapter-similarity graph
projection and fail-closed planner validation are source-implemented and were
built and exercised by the focused mTLS test. The RocksDB fragment-store
regressions also built and passed in `ShardTransferStoreTests` on 2026-11-24.
`themis_server` also built with
the ShardRPC adapter linked while `THEMIS_ENABLE_HTTP_SERVER=OFF`; production
configuration and live startup remain untested. The benchmark parent configuration adds
`benchmarks/epic3_distributed_tensor/` and its placement, integrity, recovery,
retrieval, partial-refit, and summary-first sources. The separate private
integrity test and benchmark are registered only when OpenSSL Crypto is found.

Historical validation note (2026-10-31): a diagnostic configure stopped at
`cmake/Dependencies.cmake:727` because `fmt` was unavailable. That attempt
generated no targets and is superseded for the focused paths by the
2026-11-24 Linux validation:

- `cmake --build build-linux-raft-grpc --target test_shard_transfer_store --parallel 6` — passed.
- `ctest --test-dir build-linux-raft-grpc -R "^ShardTransferStoreTests$" --output-on-failure` — passed, 1/1 CTest test.
- `cmake --build build-linux-raft-grpc --target themis_server --parallel 6` — passed; compile/link evidence only, not a production startup test.
- `./build-linux-raft-grpc/bin/tests_by_module/sharding/Debug/test_shard_transfer_grpc --gtest_filter="ShardFragmentGrpc.*"` — passed, 2/2 tests.
- `./build-linux-raft-grpc/bin/tests_by_module/sharding/Debug/test_shard_transfer_grpc` — passed, 8/8 tests.
- `THEMIS_CTEST_EMIT_TEST_OUTPUT=1 ctest --test-dir build-linux-raft-grpc -R "^ShardTransferGrpcTests$" --output-on-failure` — passed, 1/1 CTest test on retry; the first attempt timed out at 90 seconds.
- The changed test translation unit compiled from its existing `compile_commands.json` entry and linked using the generated Ninja link command. A standard CMake rebuild was stopped after regeneration expanded the target to 731 actions (8 had started).

No full module suite, benchmark, production-configured startup, or soak test
was run. Source registration and this focused evidence do not establish
production enablement or release readiness.

The source inventory establishes implementation surfaces, but not production
enablement, release readiness, or measured runtime behavior. Phase labels and
gate targets below remain planning/status records unless backed by current
source and separately verified evidence.

**Integrity/training source caveat:** The active library target now uses
OpenSSL EVP SHA-256 when OpenSSL Crypto is found at configure time and fails
closed otherwise. It accepts only SHA-256 and uses versioned, length-prefixed
receipt inputs with sorted shard IDs; hashing does not authenticate a producer.
`ArtifactManifest::compute_manifest_hash()` remains a separate
non-cryptographic checksum, and the nested integrity implementation is still
outside the target. No build or test was run for this change by request.
`TensorTrainingCoordinator` executes registered worker callbacks synchronously
in-process; it does not provide networked or distributed worker coordination.

**Tensor-Update Rollout Track**: dynamic tensor-update infrastructure is staged across four
phases (A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Phase A
(manifest schema + advisory-only policy) is complete. Phase B (delta log + partial refit)
targets Q3 2026. Phase C (shard summary coordination) targets Q4 2026. Phase D (optional
acceleration) targets 2027+.

**Module readiness**: the module implementation and test/benchmark source
registrations are present. Phase 3/4 pass status, benchmark evidence,
acceptance, and production enablement require execution evidence and remain
unverified. Representative-hardware performance and production sign-off remain
pending.

## In Progress

- [~] EPIC 3 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for distributed test/benchmark readiness (Target: Q3 2026)
- [~] Phase 5: Adapter-Distribution & Sharding-Kopplung — LoRA artifact distribution interfaces,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, and recovery ordering (Issue #5418, Target: Q3 2026)
- [~] tensor artifact manifest schema: ArtifactManifest + ManifestStore wiring (Target: Q3 2026)
- [x] derived-artifact lifecycle and staleness management: LifecycleState FSM, ArtifactClassifier, manifest fields, invalidation manager (issue #5442, Target: Q3 2026)
- [x] phase-4 distributed contract and fault-path coverage expansion beyond the focused regression suite (Target: Q4 2026)

## Planned Features

### Tensor-Update Infrastructure Track (issue #5468)

#### Phase A — Exact-First Infrastructure (Q3 2026, start)
- [x] tensor delta log schema (DeltaLogEntry, DeltaWindow, TensorDeltaLog in src/distributed_tensor/include/tensor_delta_log.h; 309 LOC)
- [x] manifest schema committed with advisory-only artifact policy documented (Target: Q3 2026)
- [x] Prometheus freshness metrics wired (`tensor_freshness_age_seconds`, `tensor_delta_log_entries_total`) (Target: Q3 2026)
- [x] snapshot-based rebuild worker (src/distributed_tensor/src/snapshot_update_worker.cc; 524 LOC; patch/refit/rebuild thresholds)
- [x] exact graph fallback guarantee documented and tested (src/distributed_tensor/include/exact_graph_fallback.h; tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)

#### Phase B — Local Tensor Maintenance (Q3 2026, weeks 6–9)
- [x] patch path for small tensor deltas (bounded delta window) (patch_threshold_pct_ in snapshot_update_worker.cc)
- [x] partial refit for bounded windows (refit_threshold_pct_ in snapshot_update_worker.cc)
- [x] rebuild fallback for unstable or large update sets (fallback rebuild in snapshot_update_worker.cc:139-146)
- [x] planner freshness gates (`tensor_freshness_age_seconds` threshold checks) (Prometheus metric wired)
- [x] ctest gate: `test_tensor_delta_log` (tests/epic3_distributed_tensor/test_tensor_delta_log.cpp)
- [x] ctest gate: `test_tensor_rebuild_fallback` (tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)
- [x] ctest gate: `test_phase_b_edge_cases` (tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp) — Phase B comprehensive edge case testing
- [x] benchmark gate: `bench_tensor_partial_refit` (benchmarks/epic3_distributed_tensor/bench_tensor_partial_refit.cc) — Performance targets: p99 <= 30ms for bounded windows

**Phase B Implementation (2026-08-17 Completion)**:
- [x] Enhanced patch path with delta window bounds checking (sequence continuity, age validation, structural mutation detection)
- [x] Partial refit with state machine transitions (PARTIAL_REFITTED state, rank cap enforcement, residual thresholds)
- [x] Rebuild fallback triggering on:
  - [x] Delta log overflow detection (95% threshold on max entries)
  - [x] Instability detection heuristics (high mutation frequency, residual thresholds)
  - [x] Fail-closed behavior on manifest validation failure
- [x] Comprehensive edge case test suite (PBE-01..24): 24 tests covering patch bounds, overflow, instability, state transitions
- [x] Full benchmark suite with performance targets:
  - Patch path: p99 <= 5ms (10-50 entry windows)
  - Partial refit: p99 <= 30ms (100-3200 entry windows)
  - Rebuild fallback: p99 <= 100ms
  - Decision logic: < 100µs per call
- [x] Production-ready error handling: manifest validation, rank cap checks, residual enforcement
- [x] State machine transitions for all three paths (PATCHED, PARTIAL_REFITTED, REBUILT)

#### Phase C — Distributed Tensor Artifact Coordination (Q4 2026) ✅ COMPLETE
- [x] shard summary refresh with freshness timestamps (✅ Implemented 2026-08-17)
- [x] summary-first routing with escalation to exact-on-demand (✅ Implemented 2026-08-17)
- [x] exact-on-demand tensor fetch (replaces summary when accuracy required) (✅ Implemented 2026-08-17)
- [x] multi-shard freshness consensus (✅ Implemented 2026-08-17)
- [x] ctest gate: `module_epic3_distributed_tensor_tensor_shard_summary_focused` (✅ 41 tests 2026-08-17)
- [x] benchmark gate: `module_epic3_distributed_tensor_bench_summary_first_focused` (✅ 12 benchmarks 2026-08-17)

#### Phase D — Optional Acceleration (2027+)
- [~] CPU/GPU break-even validation for tensor operations (Target: 2027)
- [~] bounded GPU refinement after break-even proven (Target: 2027)
- [ ] no unconditional GPU dependency on any tensor path (Target: 2027)

### Short-term (3-6 months)
- [x] implement phase-3 runtime failure and degraded-mode semantics (Target: Q4 2026)
- [x] deliver phase-4 distributed contract and fault-path tests (Target: Q4 2026)
- [x] establish phase-5 distributed benchmark and hardening baselines (Wave D: bench_distributed_tensor_dedicated_gates.cpp — DT-BM-01..04 delivered 2026-09-16)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 3.1 (Artifact Classes) contract ownership mapped and documented
- [x] EPIC 3.2 (Manifest Schema) contract ownership mapped and documented
- [x] EPIC 3.3 (Shard Placement) contract ownership mapped and documented
- [x] EPIC 3.4 (Integrity Model) contract ownership mapped and documented
- [x] tensor-update rollout track staged (Phases A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md`
- [x] advisory-only artifact policy documented with invariant: tensor artifacts never replace graph-verified results (Target: Q3 2026)
- [x] derived-artifact lifecycle model: `LifecycleState` (READY/STALE/INVALIDATED/REBUILDING/FAILED), `TruthSemantic`, `ArtifactClass`, `RebuildState`, `UpdateMode`, `InvalidationReason` defined in `artifact_manifest.h` (issue #5442, Target: Q3 2026)
- [x] manifest lifecycle fields defined: `source_seq_start`, `source_seq_end`, `delta_lag`, `artifact_age_ms`, `residual`, `rank_cap`, `lifecycle_state`, `invalidation_reason` (issue #5442, Target: Q3 2026)
- [x] advisory-only enforcement: `ArtifactClassifier::isValidCombination` prevents DERIVED/EPHEMERAL from acquiring GROUND_TRUTH semantic (issue #5442, Target: Q3 2026)

### Phase 2: Core Implementation
- [x] production translation units and public headers implemented
- [x] `themis_distributed_tensor` wired into the active community build graph
- [x] integrity translation units created for EPIC 3.4
- [x] deterministic JSON hashing and manifest-facing proof/receipt serialization implemented
- [x] cached-receipt versus fresh-verification rules documented for query-path consumers
- [x] 3.4 runtime distributed failure semantics completed across recovery/planner integration
- [x] lifecycle transition helpers (`transitionToRebuilding`, `transitionToReadyAfterRebuild`, `transitionToFailed`) and planner freshness gate helper (`shouldRejectForPlanner`) implemented in `ArtifactInvalidationManager` (issue #5442, Target: Q3 2026)
- [x] Phase A: tensor delta log + manifest store wired (Target: Q3 2026) ✅ **TDL-01..18 in tests/epic3_distributed_tensor/test_tensor_delta_log.cpp**
- [x] Phase A: snapshot-based rebuild worker implemented (Target: Q3 2026) ✅ **RFB-01..10 in tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp**
- [x] Phase B: patch path + partial refit + rebuild fallback implemented (Target: Q3 2026) ✅ **2026-08-17 complete: tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp (24 tests), benchmarks/epic3_distributed_tensor/bench_tensor_partial_refit.cc**
- [x] Phase C: shard summary refresh + summary-first routing + exact-on-demand fetch implemented (Target: Q4 2026) ✅ **2026-08-17 complete: tests/epic3_distributed_tensor/test_tensor_shard_summary.cpp (41 tests), benchmarks/epic3_distributed_tensor/bench_tensor_summary_first.cc (12 benchmarks)**

### Phase 3: Error Handling and Edge Cases
- [x] runtime distributed failure semantics implemented and verified
- [~] Active receipt hashing uses OpenSSL EVP SHA-256 when available, rejects unsupported/malformed inputs, and fails closed without OpenSSL; hashing does not authenticate the producer. The focused test and implementation translation units compile in the current WSL configuration, but the target has not been linked or run.
- [x] partial-receipt and stale-receipt edge case handling implemented
- [x] recovery coordination hooks implemented (EPIC 3.5 integration ready)
- [x] Phase B: rebuild fallback triggered on delta log overflow and instability (Target: Q3 2026) ✅ **2026-08-17: delta log overflow detection (95% threshold), instability detection heuristics, fail-closed behavior**
- [x] Phase C: exact-on-demand fallback when summary freshness threshold exceeded (Target: Q4 2026) ✅ **2026-08-17: multi-shard freshness consensus, exact-on-demand escalation logic**

### Phase 4: Tests
- [x] focused Phase 3 regression suite implemented in `tests/epic3_distributed_tensor/`
- [x] broaden contract and fault-path coverage across all EPIC 3 components — `test_phase4_contract_coverage.cpp` (58 tests, sub-issues 3.1–3.7)
- [~] RocksDB fragment-store regressions and opt-in host startup publication are implemented in source. The Linux mTLS fixture case built and passed through publisher, RocksDB reopen, transport, and manifest-bound retrieval; `themis_server` compiled and linked with the adapter. Production configuration and live startup remain untested.
- [?] Historical test-source/pass claims remain unvalidated; they do not establish current behavior. New SHA-256 regressions are registered in focused test sources but have not been run.
- [?] Integrity test and benchmark sources are configured, but their results and cryptographic behavior are unverified.
- [x] lifecycle & staleness management unit tests — `test_lifecycle_staleness_management.cpp` (31 tests, LSM-01..LSM-31, issue #5442)
- [x] `test_tensor_delta_log` — Phase A ctest gate ✅ **Done; TDL-01..TDL-18, TDL-A1..A3 (tests/epic3_distributed_tensor/test_tensor_delta_log.cpp)**
- [x] `test_tensor_rebuild_fallback` — Phase A/B ctest gate ✅ **Done; RFB-01..RFB-10, RFB-A1..A5 (tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)**
- [x] `test_phase_b_edge_cases` — Phase B edge case ctest gate ✅ **Done; PBE-01..PBE-24 (24 tests, tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp)**
- [x] `test_tensor_shard_summary` — Phase C ctest gate ✅ **Done; 41 tests (tests/epic3_distributed_tensor/test_tensor_shard_summary.cpp), 2026-08-17 completion**

### Phase 5: Performance and Hardening
- [~] Adapter-Distribution & Sharding-Kopplung: LoRA artifact distribution APIs,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, recovery ordering (Issue #5418)
- [x] benchmark suite implemented in `benchmarks/epic3_distributed_tensor/`
- [x] benchmark suite implemented in `tests/epic3_distributed_tensor/integrity_verification_bench.cc`
- [~] performance optimization with hardware acceleration (SHA-NI, AVX-512)
- [ ] alignment with graph provenance performance targets
- [x] `bench_tensor_partial_refit` — Phase B benchmark gate ✅ **Done; 8 benchmarks with performance targets: p99 <= 30ms for 100-3200 entry windows, 2026-08-17 completion**
- [x] `bench_tensor_summary_first` — Phase C benchmark gate ✅ **Done; 12 benchmarks validating summary-first routing and exact-on-demand escalation, 2026-08-17 completion**

### Phase 6: Documentation and Acceptance
- [x] core distributed tensor docs aligned to source-verifiable implementation state
- [x] tensor-update rollout track documented in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468)
- [~] acceptance docs tied to measured runtime behavior ⏳ **Phase 6 Acceptance Checklist updated 2026-08-17; benchmark evidence collection pending**

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met
- [~] Phase A production enable: manifest schema + advisory policy validation gates pending (Target: Q3 2026)
- [~] Phase B production enable: rebuild fallback & instability detection gates pending (Target: Q3 2026)
- [~] Phase C production enable: shard summary coordination gates pending (after sharding Phase C gates at 70% ready) (Target: Q4 2026)

## Production Readiness Checklist

- Current source registration is summarized below; test-pass and benchmark-pass
  status is not established by source presence.
- Earlier checklist entries for Phase A/B/C tests, benchmark outcomes, and
  dependency gap percentages are dated roadmap assertions. Their cited
  `epic3_distributed_tensor` paths/targets are absent from the current local
  module CMake inventories, so those status values are unresolved as current
  readiness evidence.

## Current Test and Benchmark Registration

The current configured inventory is maintained in
[`../../tests/distributed_tensor/CMakeLists.txt`](../../tests/distributed_tensor/CMakeLists.txt)
and [`../../benchmarks/distributed_tensor/CMakeLists.txt`](../../benchmarks/distributed_tensor/CMakeLists.txt):

- `module_distributed_tensor_test_tensor_allocation_focused`
- `module_distributed_tensor_test_tensor_operations_focused`
- `module_distributed_tensor_test_tensor_performance_focused`
- `bench_distributed_tensor_dedicated_gates` (when
  `THEMIS_BUILD_BENCHMARKS` is enabled; source uses in-process stubs)

`tests/epic3_distributed_tensor/` additionally configures phase/contract tests
and an integrity-verification benchmark test; see its `CMakeLists.txt` for
guards and exact targets. A real ShardRPC fragment-transport integration test
is conditionally configured under `tests/sharding/`; one case uses the
local publisher and RocksDB reader with a fixture summary, but does not exercise
production startup or automatic publication. `ShardTransferStoreTests` also
contains RocksDB-backed regressions for the dedicated mapping; its focused
target built and passed 1/1 in the Linux WSL build tree on 2026-11-24.
`benchmarks/epic3_distributed_tensor/`
configures four standard benchmark sources and the partial-refit and
summary-first targets when benchmarks are enabled.

The corresponding CTest registration names and evidence requirements are in
[`TEST_EVIDENCE_COLLECTION.md`](TEST_EVIDENCE_COLLECTION.md). Registration is
not evidence of build or test success. The prior EPIC 3 source/target paths are
present in current CMake; their dated pass-status claims have not been
revalidated in this source-only review.

## Known Issues & Limitations

- Benchmark evidence and final acceptance documentation are still pending before default pipeline integration can be enabled.
- Phase A/B/C enablement and the Phase D GPU plan are described in dated
  planning material; their current gate status and cross-module percentage
  dependencies were not verified in this source review.
- Advisory artifact handling is specified in the relevant public headers; its
  enforcement across all consumers was not validated in this review.

## Research-Based Release Roadmap (Issue #7414)

### Short assessment

**Maturity: component prototype; not yet a production distributed-tensor runtime.**
The module has public contracts for manifests, placement, retrieval planning,
integrity, and recovery, but the reviewed code does not establish a complete
networked execution path or production performance.
The plausible current use is caller-managed placement and advisory retrieval of
derived tensor artifacts; general remote shard fetching and distributed training
are not established module capabilities.

| Area | Source evidence | Gap and consequence |
|---|---|---|
| Application and API | `include/distributed_tensor/distributed_planner.h` exposes summary-first planning and an injectable `IFragmentFetcher`; `ShardRPCFragmentFetcher` supplies manifest-verifying gRPC/mTLS transport. `GraphSummaryProjectionCodec` accepts only the versioned adapter-similarity payload and the planner requires graph-backed evidence. `ShardArtifactFragmentStore` maps `(principal_id, shard_id, artifact_id)` into a versioned RocksDB keyspace. `main_server.cpp` can serve fragments and, when explicitly enabled and linked, publish one validated tenant adapter snapshot at startup. `TensorTrainingCoordinator` invokes worker callbacks in-process (`tensor_training_coordinator.cc`). | Startup publication is one-shot and configuration-driven; it does not refresh on later writes. Trusted artifact discovery, revocation, freshness, multi-shard publication and distributed manifest consensus remain open. The mapping is not a `TensorNetworkStorageEngine` field mapping, arbitrary tensor bytes are not projectable, and none of the new paths has runtime evidence. |
| Integrity and security | The active target includes `src/distributed_tensor/integrity_verification.cc`, which now uses OpenSSL EVP SHA-256 conditionally and fails closed without it. The nested implementation remains outside the target. See `CMakeLists.txt` and `SECURITY.md`. | Hashing does not authenticate a producer; the manifest's separate `compute_manifest_hash()` remains non-cryptographic. New code/tests are unbuilt and untested; no production trust claim is established. |
| Recovery and resilience | `recovery_manager.cc` creates in-memory jobs with timestamp-based IDs and heuristic recovery-time estimates; `recovery_manager.h` describes retries and degraded modes. | Durable job state, real shard transfer/reconstruction, idempotency, and observed recovery objectives are not established by this module source. |
| Tests | `tests/distributed_tensor/CMakeLists.txt` registers allocation, operations, and performance tests; a separate EPIC 3 suite is registered from `tests/epic3_distributed_tensor/`. | Registration is not execution evidence. The latest recorded configure attempt in `TEST_EVIDENCE_COLLECTION.md` stopped before generating targets because `fmt` was unavailable; current pass status is unverified. |
| Performance and operations | `benchmarks/distributed_tensor/bench_distributed_tensor_dedicated_gates.cpp` benchmarks in-process stubs. Separate EPIC 3 benchmark sources are configured independently. | Stub thresholds are not network/GPU measurements. No representative-hardware p95/p99 baseline, SLO, or production runbook validation is established. |

### Research basis and design implications

Research results motivate experiments; they are not ThemisDB results or
guaranteed speedups. Any performance target below is a proposed acceptance
criterion to be checked against a recorded baseline.

| Research / technical source | Finding relevant to this module | Design implication |
|---|---|---|
| Kandadi et al., [*NCCL M2N: A Layout- and Topology-Aware Collective for Distributed Tensor Resharding*](https://arxiv.org/abs/2610.07516) (2026) | Describes avoiding replica-multiplied network traffic through layout-aware transfer scheduling and hierarchical topology routes; reports workload-specific speedups on a 256-GPU system. | Measure bytes moved and latency by topology; compare against an explicitly defined baseline. Do not transfer the paper’s speedup to ThemisDB or assume GPU/NCCL availability. |
| Lian et al., [*Universal Checkpointing: Efficient and Flexible Checkpointing for Large Scale Distributed Training*](https://arxiv.org/abs/2406.18820) (arXiv:2406.18820, revised 2025) | Separates distributed checkpoint saving from representation used to load/restart under a different parallelism configuration. | Version checkpoint metadata and test restore compatibility across worker counts and layouts before promising elastic recovery. |
| Gandhi and Kozyrakis, [*Sparse Checkpointing for Fast and Reliable MoE Training*](https://arxiv.org/abs/2412.15411) (MoEvement; NSDI 2026) | Evaluates incremental checkpointing and localized recovery, reporting trade-offs and gains for its MoE workloads. | Evaluate checkpoint cost, recovery scope, and consistency together; use results only to select experiments, not as module acceptance results. |
| NIST [FIPS 180-4, *Secure Hash Standard*](https://doi.org/10.6028/NIST.FIPS.180-4) | Specifies secure hash algorithms whose digests detect message changes; a digest alone does not authenticate its producer. | Use a vetted cryptographic implementation and known-answer tests for integrity. Design authentication/key ownership separately; do not describe an unauthenticated digest as proof of origin. |

### Release boundary and estimates

- **v2.5.0:** make the existing component contracts safe to evaluate and
  integrate; close cryptographic-integrity and bounded-failure gaps; produce
  repeatable integration and performance evidence. This milestone does not
  imply a built-in distributed training service.
- **v3.0.0:** add topology-aware distributed execution and portable recovery
  only after a concrete transport/ownership boundary is agreed and v2.5.0
  evidence is available.
- Estimates are engineering weeks (five working days per engineer-week), exclude
  review and external hardware/cluster provisioning, and are planning ranges,
  not commitments.

### v2.5.0 — Component hardening and evidence

#### Wave 1 — Evidence and contract closure (P0; 1–2 engineering weeks)

- **Goal:** establish one verified contract and baseline for every implemented
  path before expanding the API.
- **Dependencies:** none.
- **Tasks:**
  - [ ] Reconcile the active CMake target, public headers, and registered local/EPIC 3 test and benchmark targets; label source registration separately from executed evidence (Target: v2.5.0).
  - [~] Specify manifest/receipt versioning, shard identity and ordering, freshness, graph-truth fallback, and failure semantics in the module contracts; the adapter-similarity projection and fail-closed planner path are now specified in source/docs, while runtime evidence remains open (Target: v2.5.0).
  - [x] Select ShardRPC gRPC/mTLS as the optional fragment transport and add the manifest-bound adapter; host storage and opt-in startup are source-implemented, while durable-job-state ownership remains unresolved (Target: v2.5.0).
  - [~] Integrate fragment writes into the owner-side artifact publication path and validate the production startup/storage composition; source registration alone is not acceptance evidence (Target: v2.5.0).
- **Risk / trade-off:** premature API commitments can create compatibility debt; limit changes to additive/versioned contracts and record unresolved integration ownership.
- **Exit metrics:** 100% of public distributed operations have a documented owner, input/validation rule, failure outcome, and test target; every current performance claim is either linked to a raw result or marked as a target/unverified.
- **Validation:** review CMake inventories and call paths; run focused tests after the required dependencies are available; retain exact configure/build/CTest output and mark unavailable runs as blocked, not passed.

#### Wave 2 — Cryptographic integrity and manifest compatibility (P0; 2–4 engineering weeks)

- **Goal:** remove the active demonstration-hash ambiguity and make receipts
  verifiable across supported builds and manifest versions.
- **Dependencies:** Wave 1; selection of an already-approved cryptographic implementation available in the repository’s supported build matrix.
- **Tasks:**
  - [~] Route the active `themis_distributed_tensor` target through OpenSSL EVP SHA-256 when available; reject unsupported algorithms and fail closed without OpenSSL. Build/security validation is pending (Target: v2.5.0).
  - [~] Define canonical, unambiguous receipt inputs with a format tag, length-prefixed fields, manifest version, artifact identity, and sorted shard IDs/content hashes; document that hashing alone does not authenticate a producer (Target: v2.5.0).
  - [~] Add known-answer, empty/malformed input, missing-shard-hash, reordered-shard, bit-flip, and legacy-receipt regression tests to configured focused suites; execution remains pending (Target: v2.5.0).
- **Risk / trade-off:** a secure hash costs more CPU than the demonstration hash and changing digest inputs invalidates old receipts; version receipts and require explicit re-verification/migration rather than silently accepting old digests.
- **Exit metrics:** all selected standard known-answer vectors pass; every tested content or identity mutation is rejected; unsupported algorithms never produce a verified receipt; record hashing throughput by payload size and compare it to the pre-change baseline.
- **Validation:** focused unit tests, sanitizer-enabled integrity tests where available, and a dependency/license/security review; do not claim cryptographic security from source names or test registration alone.

#### Wave 3 — Bounded execution and recovery contracts (P0; 3–5 engineering weeks)

- **Goal:** make delegated fetch and recovery behavior observable, bounded, and
  safe under partial failure without introducing an implicit transport.
- **Dependencies:** Wave 1 interface ownership; Wave 2 integrity verification; publisher-side fragment writes, recovery-persistence ownership, and runtime validation before end-to-end production acceptance.
- **Tasks:**
  - [~] Define the manifest-bound adapter contract for deadlines, byte ceilings, shard/artifact identity, digest verification, and structured failures; caller-driven cancellation remains unsupported (Target: v2.5.0).
  - [ ] Bound retries by both count and elapsed time, make retry eligibility/idempotency explicit, and prevent incomplete or unverified shards from becoming successful results (Target: v2.5.0).
  - [ ] Define persistence and restart semantics for recovery jobs; keep the in-memory `DefaultRecoveryManager` explicitly non-durable until a compatible persistence backend is selected (Target: v2.5.0).
  - [ ] Add contract tests for missing fetcher, timeout, cancellation, duplicate/unknown shard, partial response, corruption, exhausted retries, and primary-artifact degraded-mode rejection (Target: v2.5.0).
- **Risk / trade-off:** retries and degraded reads can amplify load or weaken correctness; default to bounded fail-closed behavior for primary/ground-truth artifacts and make any advisory path explicit.
- **Exit metrics:** 100% of injected failure cases terminate within the configured deadline plus documented test tolerance; retry count never exceeds the configured cap; zero incomplete, wrong-identity, or invalid-integrity results are accepted as success.
- **Validation:** deterministic fault-injection tests; concurrency/race checks for recovery state; the Linux ShardRPC mTLS fixture case is built and passed, and the Windows RocksDB store test passed. Production startup/storage runtime evidence and caller-driven cancellation remain unverified.

#### Wave 4 — v2.5.0 integration and performance evidence (P0; 2–4 engineering weeks)

- **Goal:** establish release evidence for correctness, latency, throughput, and
  recovery on an explicitly described deployment profile.
- **Dependencies:** Waves 1–3 and a provisioned representative multi-process or multi-node environment.
- **Tasks:**
  - [~] Add end-to-end tests that exercise placement → fetch → integrity check → fallback/recovery with the selected adapter, including a failed node and stale summary; the passing fixture case composes local graph-summary publication, RocksDB reopen, mTLS, manifest-bound fetch and conditional graph validation, but does not cover distributed recovery (Target: v2.5.0).
  - [ ] Replace any release decision based on DT-BM-01..04 stub thresholds with measurements from the real execution path; retain the stubs only as isolated microbenchmarks (Target: v2.5.0).
  - [ ] Capture p50/p95/p99 latency, throughput, bytes transferred, resource use, and recovery time for 1/2/4-node profiles and 1 MiB/64 MiB/1 GiB artifacts; run each profile at least three times and archive raw outputs with topology/build metadata (Target: v2.5.0).
- **Risk / trade-off:** representative hardware and network variability may block or dominate results; publish environment and variance, and do not generalize a single topology’s result.
- **Exit metrics:** all defined profiles have reproducible raw evidence; zero silent corruption or unsafe fallback across the failure matrix; p95/p99 thresholds are declared before the gate run and pass on the target profile. No throughput uplift is claimed without a measured baseline.
- **Validation:** focused tests plus end-to-end fault runs; benchmark rerun for any failed gate; release review checks source, test, security, and benchmark evidence together.

### v3.0.0 — Distributed execution and elastic operation

#### Wave 5 — Topology-aware placement and resharding (P1; 4–6 engineering weeks)

- **Goal:** reduce avoidable network movement for validated tensor layouts while
  retaining deterministic placement and correctness.
- **Dependencies:** v2.5.0 gates; selected transport; topology metadata and workload ownership supplied by the relevant cluster/accelerator modules.
- **Tasks:**
  - [ ] Extend placement and retrieval cost models to include source/destination layout, topology tiers, link bandwidth/latency, replication, and data volume (Target: v3.0.0).
  - [ ] Implement a deterministic transfer schedule that avoids duplicate inter-domain egress where replicas can be created locally; expose plan rationale and estimated versus observed cost (Target: v3.0.0).
  - [ ] Compare the schedule against the current supported baseline for at least three disclosed topology shapes and include exact-result parity tests (Target: v3.0.0).
- **Risk / trade-off:** topology-aware planning adds metadata and schedule complexity and may overfit a single fabric; retain a deterministic baseline strategy and avoid unconditional GPU/NCCL dependencies.
- **Exit metrics:** on preselected eligible reshard workloads, reduce median network bytes by at least 15% versus the recorded baseline with no correctness failures; p95 latency must not regress by more than 10% on any declared topology. Revisit these proposed thresholds if Wave 4 baselines show they are infeasible.
- **Validation:** deterministic planner/property tests, randomized layout parity, and representative-hardware benchmarks; report bytes, latency, throughput, and variance separately.

#### Wave 6 — Portable checkpoint and restart (P1; 5–8 engineering weeks)

- **Goal:** restore a consistent artifact after worker loss or topology changes
  without relying on process-local recovery state.
- **Dependencies:** versioned manifests and integrity from Wave 2; durable recovery ownership from Wave 3; topology-aware placement from Wave 5.
- **Tasks:**
  - [ ] Define a versioned checkpoint format separating shard payload representation from the metadata needed to map shards onto a new worker layout (Target: v3.0.0).
  - [ ] Implement checkpoint publication as an atomic, integrity-verified state transition; incomplete checkpoints must remain unavailable for restore (Target: v3.0.0).
  - [ ] Test restart after process/node loss and restore across at least two worker counts and two supported topology classes, including interrupted writes and incompatible versions (Target: v3.0.0).
- **Risk / trade-off:** layout-independent restore can increase metadata or conversion cost; benchmark save and restore separately and retain explicit compatibility windows.
- **Exit metrics:** restore succeeds for every supported matrix case; RPO is no greater than the configured checkpoint interval; p99 restore time is no greater than 2× the measured clean-restart baseline; corrupted or incomplete checkpoints are rejected.
- **Validation:** crash/fault injection at each publication boundary, compatibility tests across format versions, and repeated save/restore benchmarks with raw artifact and resource measurements.

#### Wave 7 — Operational, security, and governance readiness (P1; 3–5 engineering weeks)

- **Goal:** make distributed tensor behavior operable and auditable without
  overstating regulatory certification.
- **Dependencies:** Waves 4–6 and integration contracts for authorization, key management, metrics, and audit ownership.
- **Tasks:**
  - [ ] Define authorization checks for artifact/tenant access and authenticated provenance for manifests/receipts using the repository’s approved security services (Target: v3.0.0).
  - [ ] Emit bounded-cardinality metrics and structured audit events for plan, fetch, integrity, fallback, retry, checkpoint, and recovery outcomes; redact payloads and secrets (Target: v3.0.0).
  - [ ] Add operator guidance for degraded mode, capacity exhaustion, integrity failure, restore, rollback, and evidence retention; document supported deployment profiles (Target: v3.0.0).
  - [ ] Run threat-model review, dependency/license review, sustained-load and chaos tests, and record human security/operations sign-off (Target: v3.0.0).
- **Risk / trade-off:** signatures and audit retention add key-management and storage burden; delegate key custody and retention to approved shared services, and fail closed when trust cannot be established.
- **Exit metrics:** every external artifact operation is authorized before data release; all security/fault scenarios have an explicit test and outcome; all required metrics and audit fields are present without payload/secret leakage; no open critical/high finding is accepted without the repository’s formal exception process.
- **Validation:** security review and focused authorization/integrity tests; sustained-load and node-failure drills; audit redaction tests; runbook exercise and named human sign-off. Passing these checks is not a compliance certification.

### Open assumptions and decision points

- The root roadmap currently identifies v2.4.0-alpha as the current release; v2.5.0 and v3.0.0 are planning targets in this issue, not release dates or approved compatibility commitments.
- The ShardRPC transport and manifest-verifying adapter, a canonical principal/shard/artifact RocksDB mapping, and opt-in host startup are implemented in source but unbuilt/unvalidated. Owner-side payload publication, durable recovery store, authorization/key-management ownership beyond configured principals, and representative cluster evidence remain open. Waves depending on those gaps remain blocked.
- The target hash library, receipt migration policy, supported topology matrix, workload mix, and deployment SLOs require maintainer decisions. Numeric performance targets above are proposed gates, not measured baselines.
- Research papers report results for their own systems and hardware. Reproduce relevant comparisons using ThemisDB’s real execution path before adopting algorithmic or performance claims.

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
- Any change to the ArtifactManifest schema must be backward-compatible or versioned.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `distributed_tensor`
- A 2026-09-16 entry reported a high-cardinality test and runbook; their
  current source/artifact availability was not verified here.
- The previous `tests/integration/test_distributed_tensor_soak.cpp` used only
  in-process queue, sync, and routing stubs; it has been removed and is not
  integration evidence. A focused ShardRPC mTLS integration test now
  exercises real fragment transport with a test-only storage reader; its
  fixture-graph case composing the production fragment store and mTLS adapter
  passed in the Linux focused CTest run on 2026-11-24. The
  production storage mapping, optional mTLS serving, and an opt-in one-shot
  startup publisher now build successfully; only the fixture path has runtime
  test evidence, not the configured production startup.
  Trusted artifact discovery and refresh remain open. A bounded adapter-similarity projection
  and planner validation path are exercised by that fixture case. No
  consumer discovery path or distributed-tensor soak exists; those ownership
  and deployment gates remain open.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [?] Operator-critical runbook coverage was reported in a 2026-09-16 snapshot; current artifact availability and validation are unresolved.
