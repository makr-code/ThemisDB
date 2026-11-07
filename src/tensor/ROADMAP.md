---
Author: "ThemisDB Contributors"
Created: "2026-08-07"
Last Updated: "2026-11-07"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-07 -->
<!-- Current target, focused-test, and benchmark source lists are defined by the module CMake files; registration is not execution evidence. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · FUTURE_TENSOR_ROPE.md · THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md -->

## Current Status

Implemented runtime surfaces include tensor index management, bridges, adapter persistence, and fingerprint similarity. Benchmark targets are registered, but current source-specific performance evidence is not established by the historical baseline reports; advanced structural and experimental surfaces continue hardening.

## In Progress

- [x] hardening tensor index/bridge behavior under concurrent workload pressure (Target: Q3 2026) [COMPLETED 2026-08-07]
- [x] improving diagnostics consistency across tensor index, bridge, and graph operations (Target: Q3 2026) [COMPLETED 2026-08-07]
- [~] define and validate source-specific release guardrails for tensor fingerprint and dedup paths (Target: Q1 2027 — registered targets exist; representative evidence remains required)
- [x] federated and cross-shard tensor summaries (Completed 2026-07-06, Issue #5427)
- [x] phase-5+ tensor integration closure: durable fingerprint persistence, distributed training coordinator, CUDA compression/routing path, and workflow SLO observability (Completed 2026-07-22)

## Planned Features

### Research-driven v2.5.0 → v3.0.0 plan

This is a **proposed module plan**, not a release commitment; version allocation,
capacity, hardware and thresholds require maintainer approval. Its numbered waves
are tensor-local and do not replace the project Wave A–D or release gates in
the [root roadmap](../../ROADMAP.md) and
[release strategy](../../RELEASE_STRATEGY.md).

**Current maturity: HARDENING, not performance or release sign-off.** The
`themis_tensor` library contains indexing, TT ingestion, fingerprint and adapter
operations ([build](./CMakeLists.txt), [public contracts](../../include/tensor/README.md)).
`routeFor()` uses dimension-derived estimates while `createIndex()` creates a
flat TT index without consuming that route
([source](./tensor_index_manager.cpp)); fingerprint lookup scans entries
([source](./tensor_fingerprint_graph.cpp)). The default core bridge can be
in-memory rather than durable ([module README](./README.md)). CUDA dispatch can
fall back to CPU ([source](./tensor_compression_routing_accelerator.cpp)).
These behaviors constrain the promises made to users and operators.

Focused, concurrency and stress test sources are registered by
[tests/tensor/CMakeLists.txt](../../tests/tensor/CMakeLists.txt); no execution
result is established here. [Tensor benchmarks](../../benchmarks/tensor/CMakeLists.txt)
are conditional on `THEMIS_BUILD_BENCHMARKS`. The dedicated gate benchmark uses
in-process stubs, and the release-gate matmul uses a local helper; neither is
proof of production tensor performance
([dedicated](../../benchmarks/tensor/bench_tensor_dedicated_gates.cpp),
[release](../../benchmarks/tensor/bench_tensor_release_gates.cpp)). The
[integration baseline](../../benchmarks/tensor/bench_tensor_integration_baseline.cpp)
does call module classes, but also has synthetic and empty-train fixtures; case
validity must be checked before interpreting results.

**Research-to-gap mapping.**

| Evidence | Implication for this module | Planned response |
|---|---|---|
| Oseledets, [Tensor-Train Decomposition](https://doi.org/10.1137/090752286) (2011): low-rank TT approximation and rounding | Rank, storage and reconstruction error trade off; dimension alone does not prove compressibility. `compress()` caps reported error and `compressTTTrain()` reports a fixed 2× ratio ([source](./compression_strategy.cpp)). | Wave 1 measures actual error/bytes; Wave 2 makes results truthful and evaluates routing. |
| Williams et al., [Roofline](https://doi.org/10.1145/1498765.1498785) (2009); Yang et al., [hierarchical Roofline](https://arxiv.org/abs/2009.05257) (2020) | Arithmetic intensity and measured bandwidth help distinguish memory and compute limits; neither predicts end-to-end latency. | Waves 1/3 measure actual operations, memory traffic and latency before optimization. |
| Micikevicius et al., [Mixed Precision Training](https://arxiv.org/abs/1710.03740) (2018); [PyTorch numerical accuracy](https://pytorch.org/docs/stable/notes/numerical_accuracy.html) | Reduced precision can change results and reduction order is not generally bitwise portable. Training evidence does not establish TT-retrieval quality. | Waves 2/4 compare reconstruction and retrieval against higher-precision references, including non-finite inputs. |
| [PyTorch benchmark guidance](https://github.com/pytorch/pytorch/blob/main/torch/utils/benchmark/README.md) and [reproducibility](https://pytorch.org/docs/stable/notes/randomness.html) | Warmup, synchronization, repeated runs and recorded environment matter; a seed alone does not guarantee cross-platform identity. | Wave 1 establishes a repeatable manifest and distributions, not a single headline result. |
| [Safetensors format](https://github.com/huggingface/safetensors#format); [PyTorch serialization security](https://pytorch.org/docs/stable/notes/serialization.html#weights-only-security) | Non-executable formats still need size and content checks; these sources do not establish that ThemisDB uses either format. | Wave 2 validates existing artifact readers, quotas and integrity; no format adoption is presumed. |

**Priorities:** v2.5.0: P0 evidence and safety (W1–W2), then P1 measured
performance/operability (W3). v3.0.0: P1 validated scalable retrieval and
interoperability (W4), then P1 deployment and governance closure (W5). Existing
TN-ROPE and BRAIN tracks remain separate hypotheses: only promote after their
own gates, not merely because a wave mentions them.

**Evidence protocol for all waves:** record commit, target and actual implementation,
compiler/build flags, CPU/GPU/driver, thread count, dataset/shape/rank/dtype,
seed, tenant distribution, cache state, warmup and timing scope. Use at least
20 independent runs per performance condition, reporting median and 95% confidence
interval of run-level effects; collect at least 10,000 individual operations per
condition for p95/p99, not percentiles of benchmark averages. Synchronize GPU
timings and report transfer-inclusive and kernel-only results separately. Archive
raw samples, commands, correctness checks and manifests. Compare paired runs on
the same hardware; baseline and proposed limits below are not measured results.

#### W1 — v2.5.0: trustworthy baseline (P0; 3–4 engineer-weeks)

- [ ] **Goal/tasks:** build an inventory mapping each existing benchmark and
  focused test to its real implementation; add valid, bounded fixtures for TT
  ingest, exact and fingerprint top-10 search, adapter overwrite, and routing.
  Include 1k/10k/50k candidates, 1/10 tenants, warm/cold cache and rank/shape
  variation; compare exact search with a dense reference.
- **Dependencies:** storage/benchmark owners and representative CPU hardware;
  GPU hardware only for separately labeled GPU measurements.
- **Risk/trade-off:** synthetic data or stub paths can make a fast but irrelevant
  baseline; 50k exact scans may exceed test-resource budgets.
- **Validation/exit:** build `themis_tensor` and relevant focused targets, run
  tensor CTest cases and production-path benchmarks; archive manifests and raw
  distributions per protocol. 100% of reported cases identify the actual symbol
  and success/result count, and no stub-only case is a release-performance gate.
  Establish measured baseline B before approving numerical performance targets.

#### W2 — v2.5.0: correctness and safety contracts (P0; 5–7 engineer-weeks)

- [ ] **Goal/tasks:** fix `CompressionFactory` ownership/registration lifetime,
  replace or explicitly reject unsupported `compressTTTrain()` behavior, and
  report actual compression/error statistics without hiding tolerance violations.
  Validate dimensions, multiplication overflow, rank, dtype, non-finite values and
  resource limits at artifact/ingestion boundaries. Define explicit durability,
  routing, CPU fallback, and failure diagnostics without changing the existing
  minor-version public contract; align docs and operator runbooks.
- **Dependencies:** W1; storage/security owners for artifact formats, quotas and
  persistence; public API review for any contract change.
- **Risk/trade-off:** correcting misleading metrics can expose existing caller
  assumptions; stricter validation can reject previously accepted artifacts.
- **Validation/exit:** focused unit/property tests and round-trip/corrupt/truncated,
  overflow, NaN/Inf, cross-tenant, concurrent register/create and restart/fault
  tests; ASan/UBSan and TSan where supported. 100% of the enumerated invalid cases
  fail explicitly without partial publication; factory lifecycle has zero sanitizer
  findings. Dense-reference relative Frobenius error and actual bytes must match
  reported values within documented floating-point tolerance, including zero norm.

#### W3 — v2.5.0: measured performance and operations (P1; 4–6 engineer-weeks)

- [ ] **Goal/tasks:** profile the W1 graph/ingest/adapter hot paths, use Roofline
  only where counters are available, and address the measured bottleneck. Add
  bounded concurrency/backpressure and observable CPU/GPU/fallback and persistence
  state; document recovery, rollback and supported configuration.
- **Dependencies:** W1 baseline and W2 correctness/security; observability and
  storage owners, representative hardware.
- **Risk/trade-off:** throughput optimizations can worsen tail latency, memory,
  recall or deterministic behavior; GPU availability is not universal.
- **Validation/exit:** concurrent read/write and fault-injection/soak suites plus
  CPU/GPU parity when hardware exists. On each agreed W1 workload, p95/p99 and
  throughput regress by no more than 10% versus B, with paired 95% intervals
  reported; peak RSS and error/fallback counts are recorded and no unbounded
  growth is observed in a 1-hour steady-state soak. Alert and runbook paths are
  exercised; missing GPU evidence blocks GPU claims, not CPU measurements.

#### W4 — v3.0.0: scalable retrieval and interoperability (P1; 8–12 engineer-weeks)

- [ ] **Goal/tasks:** prototype candidate generation for fingerprint scans and
  data-driven routing against the W1 exact/dense oracle; evaluate TT rank and
  quality, profile rotation using TN-ROPE-01..07, and versioned artifact/API
  migration with explicit read/write compatibility. Keep experimental paths
  disabled until their acceptance gates are met.
- **Dependencies:** W1–W3, search/index/storage and optional accelerator owners;
  dataset rights and release-owner approval for any major-version contract.
- **Risk/trade-off:** ANN pruning and quantization can lose recall; new formats
  can strand existing artifacts; RoPE evidence is a hypothesis, not a guarantee.
- **Validation/exit:** paired fixed-manifest evaluation on at least two datasets,
  including a production-like set when approved: Recall@10 >= 0.95 relative to
  exact top-10 and nDCG@10 loss <= 0.5 percentage points; compare p95/p99, memory
  and throughput to W1 at 10k/50k. Promote a new path only if its 50k p95 is at
  least 20% lower than the exact baseline with the 95% interval excluding zero,
  without breaching quality gates. Test old/new artifact migration, mismatch
  rejection and rollback; TN-ROPE additionally retains its existing stricter gates.

#### W5 — v3.0.0: resilience, governance and release handoff (P1; 4–6 engineer-weeks)

- [ ] **Goal/tasks:** finalize threat model, tenant isolation, quota and corruption
  controls, provenance and compatibility policy; rehearse recovery, upgrade and
  rollback; deliver evidence bundle, ownership and on-call runbook. Evaluate
  BRAIN tracks only through their separately defined safety gates.
- **Dependencies:** W2–W4 and security, storage, operations and release sign-off;
  project-level gates in root release strategy remain authoritative.
- **Risk/trade-off:** major migration and distributed dependencies increase
  operational complexity; without an agreed dataset or deployment budget, hold.
- **Validation/exit:** deterministic fault/restart and mixed-version migration
  tests, tenant-isolation and malformed-input fuzz corpus, sanitizer and security
  review; zero unresolved critical/high findings in the changed tensor paths or
  an approved exception. 100% of planned scenarios have evidence and a named
  owner; `release_critical` on `develop`, required project resilience/security
  gates, retained benchmark artifacts and human release approval are prerequisites.

**Open assumptions and gaps:** estimates are engineering effort, not elapsed
calendar time, and exclude cross-team dependencies. Dataset selection, baseline B,
hardware, memory/latency budgets and release ownership are unapproved; targets
are proposed and must be revisited after W1, not presented as measured results.
The developer wiki has no dedicated `tensor` page (its distributed-tensor page
describes another module); its 2026-11-02 status reports 1,450 removed sources,
so freshness needs review. The root roadmap's open `themis_tensor` target item
conflicts with its declaration in the module CMake file; declaration alone does
not prove a successful build. Reconcile this aggregate with release owners.

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
- [~] validate p95/p99 and throughput behavior for the actual tensor implementation against release baselines (Target: Q1 2027 — historical measurements do not identify the tensor implementation)
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
- [~] bench_tensor_release_gates.cpp and bench_tensor_dedicated_gates.cpp are registered; source-specific representative measurements and review remain required
- [x] remaining hardening tasks closed for index/bridge/graph edge paths (Phase 2 closure, 2026-08-07)
- [~] release benchmark validation remains open until reproducible, source-specific evidence is recorded

## Known Issues and Limitations

- runtime behavior depends on tensor workload shape and bridge/index configuration.
- selected advanced structural paths remain in active hardening status.
- benchmark depth should continue expanding for broader tensor workload patterns.

## Wave 3 Gap-Closure Tracking (2026-08-31)

- [~] `compression_strategy.cpp` — `compress()` calls the decomposer but caps
  reported error; `compressTTTrain()` still reports a fixed 2× ratio. Validate
  actual output/error semantics (W2; Target: v2.5.0).
- [~] `compression_strategy.cpp` — `registerStrategy()` populates a registry,
  but `create()` constructs a `unique_ptr` from its `shared_ptr`'s raw pointer.
  Correct ownership and concurrency semantics (W2; Target: v2.5.0).
- [~] `tensor_routing_strategy.cpp` — freshness decay and `AdaptiveRouting`
  outcome metrics exist; validate timestamps, concurrency and selection behavior
  rather than assuming these paths are unimplemented (W2–W4).

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
