# Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-07 -->
<!-- Evidence: 16 test files (406+ tests, 9,025+ LOC), 7 benchmarks (1,937 LOC), 33+ implementation files (8,275+ LOC) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · FUTURE_TENSOR_ROPE.md · THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md -->

## Current Status

Production-usable tensor runtime exists for tensor index management, hybrid bridge operation, and fingerprint graph benchmarked behavior; advanced structural and experimental surfaces continue hardening.

## In Progress

- [x] hardening tensor index/bridge behavior under concurrent workload pressure (Target: Q3 2026) [COMPLETED 2026-08-07]
- [x] improving diagnostics consistency across tensor index, bridge, and graph operations (Target: Q3 2026) [COMPLETED 2026-08-07]
- [x] stabilizing benchmark-backed release guardrails for tensor fingerprint and dedup paths (Target: Q3 2026) [COMPLETED 2026-08-07]
- [x] federated and cross-shard tensor summaries (Completed 2026-07-06, Issue #5427)
- [x] phase-5+ tensor integration closure: durable fingerprint persistence, distributed training coordinator, CUDA compression/routing path, and workflow SLO observability (Completed 2026-07-22)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic behavior for tensor bridge and hybrid routing edge scenarios (Completed 2026-09-16 — test_tensor_highcardinality_stress.cpp, TensorSoak_BridgeRoutingReliability)
- [x] expand stress coverage for fingerprint graph concurrent read/write patterns (Completed 2026-09-16 — test_tensor_highcardinality_stress.cpp, ConcurrentGraphReadWrite/DedupReplayStress)
- [x] improve operator-facing diagnostics for tensor graph export and replay incidents (Completed 2026-09-16 — RUNBOOK_TENSOR_STORE.md)

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for tensor query and graph operation hot paths (Target: Q1 2027 — bench_tensor_dedicated_gates.cpp TN-BM-01..TN-BM-04 in place; hardware run required)
- [~] broaden benchmark depth for tensor index and dedup replay workload diversity (Target: Q1 2027 — bench_tensor_dedicated_gates.cpp TN-BM-01..TN-BM-04 in place)
- [x] harden long-run reliability under sustained tensor graph mutation/query traffic (Completed 2026-09-16 — test_tensor_store_soak.cpp)

### Tensor RoPE Track (Q4 2026 - Q1 2027)
- [ ] TN-ROPE-01 add rotation profile/version metadata and fail-closed mismatch validation across tensor ingestion and query paths (Target: Q4 2026)
- [ ] TN-ROPE-02 implement pre-TT rotation hook in tensor ingestion and kappa-gate pilot path (`TensorIngestionBridge::shouldDecompose`, `TensorIngestionBridge::decompose`) (Target: Q4 2026)
- [ ] TN-ROPE-03 implement query/index parity rotation in hybrid retrieval (`HnswTTBridge::add/search/searchFlat/extractSketch`) with persisted profile/version compatibility checks (Target: Q1 2027)
- [ ] TN-ROPE-04 run transform ablation: pair-rotation vs WHT/Radon/Greens using `TensorButterflyOperator::apply` comparators (Target: Q1 2027)
- [ ] TN-ROPE-05 run llama.cpp benefit study for `generateRAG` tensor modes (`tensor_hybrid`, `tensor_prefix`) with profile-on/off evidence capture (Target: Q1 2027)
- [ ] TN-ROPE-06 run AdaLoRA benefit study (`exportToTT`, `roundAndReallocate`, `findSimilarAdapters`) with fixed-seed rank-efficiency and quality comparison (Target: Q1 2027)
- [ ] TN-ROPE-07 produce reproducible evidence packet (metrics, CI artifacts, decision record) and close/no-close recommendation for production rollout (Target: Q1 2027)

### Neurocognitive Bridge Track (Q4 2026 - Q2 2027)
- [ ] BRAIN-DEC-01 resolve replay scheduling ownership and publish accepted decision record (Target: Q4 2026)
- [ ] BRAIN-DEC-02 resolve homeostasis policy scope (tensor-local first with exported utility signals) and publish accepted decision record (Target: Q4 2026)
- [ ] BRAIN-DEC-03 resolve deterministic arbitration tie-break chain and publish accepted decision record (Target: Q4 2026)
- [ ] BRAIN-DEC-04 resolve edition activation core safety envelope and publish accepted decision record (Target: Q4 2026)
- [ ] WP-BRAIN-01 implement replay consolidation contract with scheduler orchestration and deterministic replay evidence packet (Target: Q1 2027)
- [ ] WP-BRAIN-02 implement homeostasis/forgetting policy with bounded-growth gates and contamination regressions (Target: Q1 2027)
- [ ] WP-BRAIN-03 implement routing arbitration kernel with deterministic tie-break tests and route-thrashing resilience checks (Target: Q2 2027)
- [ ] BRAIN-GATE-01 close BRAIN-REPLAY/BRAIN-HOMEO/BRAIN-ARBITER/BRAIN-SAFETY gate families with go/hold/no-go decision note (Target: Q2 2027)
- [ ] WP-BRAIN-04 evaluate machine-dreaming research mode (offline synthetic gap-filling) with strict isolation and provenance tags (`synthetic=true`, `dream_mode=true`) (Target: Q2 2027)
- [ ] BRAIN-GATE-02 close BRAIN-DREAM-ISOLATION/BRAIN-DREAM-LABEL-INTEGRITY/BRAIN-DREAM-UTILITY/BRAIN-DREAM-NONREGRESSION/BRAIN-DREAM-ETHICS-BOUNDARY gates with explicit research-only vs operational recommendation (Target: Q2 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze tensor index/bridge/graph contracts for current major line (Completed 2026-07-29)
- [x] define explicit error taxonomy for tensor incident classes (Completed 2026-07-29)
- [ ] specify rotation profile contract (`none|fixed|relational|learned`) and metadata schema (`rotation_profile`, `rotation_version`, `rotation_params_hash`, `rotation_seed`) for tensor artifacts (Target: Q4 2026)
- [ ] document compatibility and migration rules for rotated/non-rotated tensor artifacts, including fail-closed conditions (Target: Q4 2026)
- [ ] publish accepted ADR-light records for BRAIN-DEC-01..04 and map each decision to module ownership + gate impact (Target: Q4 2026)

### Phase 2: Core Implementation
- [x] complete hardening for tensor index manager and bridge internals (Completed 2026-08-07)
- [x] align fingerprint and dedup-adjacent behavior to bounded runtime contracts (Completed 2026-08-07)
- [ ] implement ingestion-side rotation hook before pilot/full decomposition in `tensor_ingestion_bridge.cpp` with deterministic profile control (Target: Q4 2026)
- [ ] implement hybrid retrieval parity rotation and profile/version checks in `hnsw_tt_bridge.cpp` (`add`, `addFlat`, `search`, `searchFlat`, `save`, `load`) (Target: Q1 2027)
- [ ] implement replay consolidation lifecycle contract (scheduler orchestration + module-local executors) for tensor/training paths (Target: Q1 2027)
- [ ] implement tensor-local homeostasis policy contract with exported utility signals for cross-module composition (Target: Q1 2027)
- [ ] implement deterministic routing arbitration kernel for equal-confidence route conflicts (Target: Q2 2027)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for bridge faults and graph export/replay errors (Completed 2026-08-07)
- [x] unify diagnostics across index, bridge, and fingerprint incident classes (Completed 2026-08-07)
- [ ] enforce explicit rejection of profile/context mismatch (e.g. relational profile without relation context) with operator-visible diagnostics (Target: Q4 2026)
- [ ] validate deterministic behavior for rotated artifact replay, rebuild, and mixed-version startup scenarios (Target: Q1 2027)
- [ ] enforce explicit BF-01..BF-05 failure taxonomy diagnostics for replay divergence, arbitration instability, over-pruning, and hidden fallback leakage (Target: Q1 2027)
- [ ] enforce machine-dreaming truth-path firewall: synthetic outputs must never flow into production truth planes without evidence-gated promotion (Target: Q2 2027)

### Phase 4: Tests
- [x] expand focused regressions for tensor index/bridge and fingerprint edge scenarios (Completed 2026-07-29 — test_tensor_contract_hardening_focused.cpp, TNCH-01..TNCH-16)
- [x] extend deterministic stress fixtures for concurrent tensor graph workloads (Completed 2026-07-29)
- [ ] add unit tests for profile determinism and metadata round-trip; add integration tests for ingestion->persist->query parity under each profile (Target: Q4 2026)
- [ ] add llama.cpp RAG integration tests for tensor profile on/off in `tensor_hybrid` and `tensor_prefix` modes (Target: Q1 2027)
- [ ] add AdaLoRA bridge integration tests for rotated export/import and rank-budget parity (`exportToTT`, `roundAndReallocate`, `findSimilarAdapters`) (Target: Q1 2027)
- [ ] add BRAIN-REPLAY-DET-01 deterministic replay equivalence focused tests with fixed seed/manifest protocol (Target: Q1 2027)
- [ ] add BRAIN-HOMEO-BOUND-01 bounded-growth + stale-neighbor contamination regression suites (Target: Q1 2027)
- [ ] add BRAIN-ARBITER-DET-01 deterministic tie-break and route-thrashing resilience tests (Target: Q2 2027)
- [ ] add BRAIN-SAFETY-CORE-01 fail-closed + rollback-proof + hidden-fallback-absence acceptance suite (Target: Q2 2027)
- [ ] add BRAIN-DREAM-ISOLATION and BRAIN-DREAM-LABEL-INTEGRITY focused tests for sandbox separation and provenance-tag persistence (Target: Q2 2027)
- [ ] add BRAIN-DREAM-NONREGRESSION and BRAIN-DREAM-ETHICS-BOUNDARY suites to verify no online hallucination increase and policy-constrained synthetic flow behavior (Target: Q2 2027)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for tensor hot paths (Completed 2026-07-29 — bench_tensor_release_gates.cpp, TRNRG-01..TRNRG-06)
- [x] validate p95/p99 and throughput behavior against release baselines (Completed 2026-08-07)
- [ ] run ablation benchmarks for `none` vs `fixed` vs `relational` vs `learned` and pair-rotation vs WHT/Radon/Greens transform families (Target: Q1 2027)
- [ ] meet RoPE rollout gates: Recall@10 gain on >=1 production-like dataset, no required-set nDCG regression worse than -0.5%, latency budget compliance (Target: Q1 2027)
- [ ] run replay/homeostasis/arbitration benchmark families with statistical protocol (minimum run counts, confidence intervals, effect-size reporting) and reproducibility manifests (Target: Q2 2027)
- [ ] run machine-dreaming utility and nonregression benchmark families with fixed-manifest statistical protocol and fail-closed promotion criteria (Target: Q2 2027)

### Phase 6: Documentation and Acceptance
- [x] core tensor module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] tensor_api_contract.h frozen contract header published (Completed 2026-07-29)
- [ ] publish evidence-backed RoPE decision record with explicit go/no-go recommendation and rollback plan (Target: Q1 2027)
- [ ] synchronize tensor/llama_cpp/training roadmap and future-enhancement references for the RoPE study outcomes (Target: Q1 2027)
- [ ] publish neurocognitive bridge decision packet (BRAIN-DEC-01..04 + WP-BRAIN-01..03 + gate outcomes) with edition-aware rollout recommendation (Target: Q2 2027)
- [ ] publish machine-dreaming decision packet with explicit classification (research-only or promotion-candidate), safety rationale, and rollback/deactivation guidance (Target: Q2 2027)

## Production Readiness Checklist

- [x] core tensor surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] tensor_api_contract.h frozen contract header (Phase 1 closure, 2026-07-29)
- [x] test_tensor_contract_hardening_focused.cpp — TNCH-01..TNCH-16 (Phase 4 closure, 2026-07-29)
- [x] bench_tensor_release_gates.cpp — TRNRG-01..TRNRG-06 gate benchmarks (Phase 5 closure, 2026-07-29)
- [x] remaining hardening tasks closed for index/bridge/graph edge paths (Phase 2 closure, 2026-08-07)
- [x] release benchmark stabilization complete (Phase 5 closure, 2026-08-07)

## Known Issues and Limitations

- runtime behavior depends on tensor workload shape and bridge/index configuration.
- selected advanced structural paths remain in active hardening status.
- benchmark depth should continue expanding for broader tensor workload patterns.

## Wave 3 Gap-Closure Tracking (2026-08-31)

- [~] `compression_strategy.cpp` — `TTDecompositionStrategy::compress()` returns
  synthetic 2× ratio; not wired to real TensorTrainDecomposer. STUB #CS-01.
  Target Q2 2027.
- [~] `compression_strategy.cpp` — `CompressionFactory::registerStrategy()` is a
  no-op; strategy map not yet implemented. STUB #CS-02. Target Q2 2027.
- [~] `tensor_routing_strategy.cpp` — Freshness scoring in `RankBasedPrioritization`
  uses constant 1.0; `created_at` timestamp parsing not yet implemented.
  Target Q2 2027.
- [~] `tensor_routing_strategy.cpp` — Adaptive routing in
  `AdaptiveLearningRouter::route()` uses fixed heuristic; metrics-based learning
  loop not yet implemented. Target Q2 2027.

## Breaking Changes

No breaking tensor contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Paper Evidence Baseline (for TN-ROPE Track)

- RoFormer / RoPE positional rotation evidence: Su et al., 2021 (arXiv:2104.09864)
- Relational rotation evidence: RotatE, Sun et al., 2019 (arXiv:1902.10197)
- Rotation-before-compression ANN precedent: Optimized Product Quantization, Ge et al., 2013 (CVPR)
- TT rank/compression tradeoff fundamentals: Oseledets, 2011 (TT decomposition)
- AdaLoRA rank-allocation evidence: Zhang et al., 2023 (arXiv:2303.10512)
- Retrieval-to-generation quality linkage for llama.cpp study framing: Lewis et al., 2020 (RAG, arXiv:2005.11401)

Detailed mapping of hypothesis-to-paper and experiment IDs is maintained in [`FUTURE_TENSOR_ROPE.md`](./FUTURE_TENSOR_ROPE.md).

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `tensor`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Completed 2026-09-16 — test_tensor_highcardinality_stress.cpp, bench_tensor_dedicated_gates.cpp, RUNBOOK_TENSOR_STORE.md)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Completed 2026-09-16 — tests/integration/test_tensor_store_soak.cpp)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Completed 2026-09-16 — docs/operability/RUNBOOK_TENSOR_STORE.md)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Completed 2026-09-16 — bridge routing and dedup error handling validated in soak/stress)
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027 — bench_tensor_dedicated_gates.cpp TN-BM-01..TN-BM-04; hardware run required)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Completed 2026-09-16 — RUNBOOK_TENSOR_STORE.md)
