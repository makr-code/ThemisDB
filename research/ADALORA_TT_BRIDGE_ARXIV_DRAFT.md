# From Adapters to Archives: A Tensor-Train Bridge for AdaLoRA in ThemisDB

**Status:** Review-ready draft
**Last Updated:** 2026-07-27
**Target Venue:** arXiv (cs.DB / cs.LG)
**Repository Artifact:** `research/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md`

---

## Abstract

This draft examines whether AdaLoRA adapter factors can be represented and managed through Tensor-Train (TT) structures in ThemisDB, and what is already implemented versus still planned. The codebase contains a concrete `AdaLoraTTBridge` implementation for export/import, rank-rounding delegation, and similarity-assisted lookup integration, plus targeted tests for injection hooks. However, several publication-level claims from earlier drafts (large latency speedups, storage reduction percentages, and quality gains during live FLARE switching) are not yet backed by reproducible benchmark artifacts in this repository state. This revision therefore narrows claims to repository-verifiable behavior, separates measured evidence from hypotheses, and aligns terminology with current ThemisDB architecture (AQL query layer, multi-model storage model, ACID transaction guarantees, RocksDB-backed tensor persistence interfaces). We provide a claim-to-evidence mapping, an evaluation protocol ready for reproducible experiments, and explicit limitations/known issues to support review readiness.

---

## 1. Introduction / Einleitung

Parameter-efficient fine-tuning (PEFT) methods such as LoRA and AdaLoRA represent model updates with low-rank factors. Tensor-Train decomposition is another low-rank representation family. A bridge between both is relevant for ThemisDB because ThemisDB is positioned as a multi-model database with native AI/LLM integration and a dedicated AQL query layer over shared storage infrastructure [R1].

The central research question of this draft is:

> **RQ1:** Which AdaLoRA-to-TT bridge properties are already implemented and verifiable in the current ThemisDB repository, and which claims still require new benchmark evidence?

A secondary systems question is:

> **RQ2:** Can the current bridge design support future zero-copy serving and cross-adapter deduplication experiments without overstating present production behavior?

### Contributions in this revision

1. **Repository-grounded correction of claims:** unsupported quantitative claims were removed or explicitly marked as hypotheses.
2. **Terminology harmonization:** AQL, multi-model, ACID, TT, and component naming now follow repository usage.
3. **Mandatory section completion:** explicit Methodik/Ansatz, Evaluation/Experimente, and Limitations/Known Issues sections are included.
4. **Evidence traceability:** a direct claim-to-artifact table links statements to code/tests/docs.

---

## 2. Methodik / Ansatz

### 2.1 Scope and verification method

We use a code-and-test grounded review method:

- Inspect bridge API/implementation (`include/training/adalora_tt_bridge.h`, `src/training/adalora_tt_bridge.cpp`).
- Inspect related serving interface status (`include/storage/ggml_tensor_bridge.h`, `src/storage/ggml_tensor_bridge.cpp`).
- Inspect module planning/status artifacts (`src/training/ROADMAP.md`, `src/STUB_INVENTORY.md`).
- Inspect focused tests (`tests/test_adalora_tt_bridge.cpp`).

### 2.2 Current bridge behavior (code-verified)

`AdaLoraTTBridge` currently provides:

- **AdaLoRA -> TT export** via norm-product singular-value approximation and TT-core construction (`exportLayer`, `exportToTT`).
- **TT -> AdaLoRA import** (`importFromTT`).
- **In-process store/load path** through an internal cache keyed by `tenant + adapter_name` (`store`, `loadAdapter`).  `store()` writes to an in-memory `export_cache` (`std::unordered_map`) and optionally to `TensorFingerprintGraph`; it does **not** persist to `TensorNetworkStorageEngine` in the current implementation path.  `storeAdapter()` is a backward-compatible alias that forwards to `store()`.  `loadAdapter()` reads from the same in-memory cache and reconstructs via `importFromTT()`.
- **Optional similarity registration** through `TensorFingerprintGraph` insertion on `store()` when deduplication is enabled.
- **Rank-rounding path** (`roundAndReallocate`) using either:
  - injected training callback (`TrainingStepFn`), or
  - fallback TT rounding (`TensorTrainDecomposer::round`).
- **Adapter mapping injection hook** (`MapAdapterFn`) where `mapAdapter()` returns `false` when no callback is wired.

### 2.3 Boundaries of what is currently proven

The repository currently proves API behavior and hook delegation, but **does not yet provide reproducible benchmark artifacts** in this file's scope for:

- measured hot-load latency improvements,
- measured cross-adapter storage reduction percentages,
- measured FLARE quality uplift from live adapter switching.

Accordingly, those points are treated as future experimental hypotheses, not established results.

---

## 3. Implementation Evidence (Repository-Grounded)

| Evidence ID | Artifact | What it supports |
|---|---|---|
| E1 | `src/training/adalora_tt_bridge.cpp` | Concrete implementation of export/import, store/load, similarity lookup path, rank-rounding fallback, and callback bridges |
| E2 | `include/training/adalora_tt_bridge.h` | Public bridge API and intended semantics for Phase-3/4 integration hooks |
| E3 | `tests/test_adalora_tt_bridge.cpp` | Verified behavior for `mapAdapter()` fallback/delegation (ALTB-P3-01..P3-03) and `roundAndReallocate()` callback routing (ALTB-P4-01..P4-03); thread-safety under concurrent `storeAdapter()` and `findSimilarAdapters()` calls (ALTB-DR-01..DR-02, data-race regression batch) |
| E4 | `include/storage/ggml_tensor_bridge.h` + `src/storage/ggml_tensor_bridge.cpp` + `tests/tensor/test_tensor_phase3.cpp` | GGML bridge is compile-gated (`THEMIS_ENABLE_GGML_BRIDGE`). Injection APIs for all three former stubs are resolved: `GgmlAllocFn` (STUB #263a), `PrefetchFn` (STUB #263b), and `TypeRegistrationFn` (STUB #263c) — all resolved 2026-05-11. Test coverage: GTB-01..GTB-09 in `tests/tensor/test_tensor_phase3.cpp`. The fallback when no fn is injected (nullptr / no-op / placeholder id 9999) is now intentional design, not an unresolved stub. Remaining external gap: upstream ggml type patch for `GGML_TYPE_TT` and llama.cpp `llama_kv_cache_inject()` API (both external to ThemisDB). |
| E5 | `src/training/ROADMAP.md` | Bridge roadmap tracks phases and identifies broader integration as planned work |
| E6 | `src/STUB_INVENTORY.md` | Historical stub context and current callback-based bridge status for AdaLoRA TT and GGML bridge paths |
| E7 | `README.md` | Canonical system positioning and terminology: multi-model architecture, AQL query layer, ACID transaction capabilities |
| E8 | `include/storage/tt_quantizer.h` + `src/storage/tt_quantizer.cpp` | `TTQuantizer` provides post-decomposition INT8 and NF4 quantisation of TT-cores, integrated as a private member of `TensorNetworkStorageEngine`. Directly relevant to any future storage reduction claims: quantised TT-trains reduce per-adapter storage footprint before durability benchmarks are run |

### Claim boundaries

**Supported claims (current repository state):**

- AdaLoRA-to-TT bridge APIs are implemented and test-covered for hook behavior (E1-E3).
- Similarity graph integration points exist in bridge code (E1) and align with fingerprint-graph capabilities (E6).
- GGML injection bridge APIs (`GgmlAllocFn`, `PrefetchFn`, `TypeRegistrationFn`) are resolved in ThemisDB; GTB-01..GTB-09 verify callback delegation (E4).
- `TTQuantizer` (INT8/NF4) is integrated into `TensorNetworkStorageEngine`, enabling compressed TT-core storage (E8).

**Deferred claims (need new experiments/artifacts):**

- specific latency multipliers (e.g., "X-Yx faster"),
- fixed dedup percentage ranges across adapter corpora,
- downstream quality deltas on LegalBench/MedMCQA from live switching.

---

## 4. Evaluation / Experimente

### 4.1 What is already evaluated

Current test evidence in this scope is functional, not performance-centric:

- `mapAdapter()` behavior with/without injected mapping function.
- `roundAndReallocate()` behavior with/without injected training-step function.

These are necessary interface checks but insufficient to claim system-level serving or storage gains.

### 4.2 Reproducible experiment protocol (required for publication claims)

> **A complete, publication-grade benchmark protocol now exists in this repository.**
> See `research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md` for the full specification,
> including exact experiment matrices, hardware/software environment schema, statistical
> minimums, artifact naming conventions, failure modes, and validity threats.
> Lightweight tooling is available under `research/experiments/adalora_tt_bridge/`.

To elevate this draft from design-level evidence to quantitative publication evidence, the following experiment tracks are defined (abbreviated here; see the protocol document for the full matrices):

1. **BT-1: Adapter load path latency**
   - Compare cold/warm load for baseline adapter path vs. bridge path.
   - Report p50/p95/p99, CV(%), 95% bootstrap CI, ≥ 30 runs per cell.
   - Runnable today (in-process bridge path).

2. **BT-2: Storage deduplication efficiency**
   - Domain-homogeneous and domain-heterogeneous adapter sets (10 and 50 adapters).
   - Report bytes before/after, dedup ratio, and false-positive collision count.
   - Runnable today (`TensorFingerprintGraph` path).

3. **BT-3: Rank pruning / reconstruction quality**
   - AdaLoRA greedy vs. TT-rounding under budget-equivalent rank cuts (10%/30%/50%).
   - Report per-layer Frobenius reconstruction error and (when available) downstream task delta.
   - BT-3-A/B runnable today; BT-3-C requires an external downstream eval task.

4. **BT-4: FLARE-style runtime adapter switching**
   - Token latency and task quality with/without live adapter switching.
   - **Blocked:** requires GGML bridge wiring (Stub #271) and FLARE integration.
   - Must not be published before the gate documented in §6.4 of the protocol is cleared.

### 4.3 Reporting requirements

Any future quantitative claim must include all fields defined in
`research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md §7 (Reporting Contract)`:

- hardware + software stack (full environment descriptor per §3.1 of the protocol),
- dataset/task description with split seed,
- exact command and configuration (archived in `results/` directory),
- ≥ 30 independent runs per configuration,
- mean, stddev, CV(%), p50, p95, p99, Cohen's d, 95% bootstrap CI,
- artifact path under `research/experiments/adalora_tt_bridge/results/`.

---

## 5. Limitations / Known Issues

1. **Evidence gap for quantitative claims**
   - Current repository state supports implementation claims, not benchmarked headline numbers.

2. **Hook-based integration status**
   - Core map/round integration points are callback-based; full deployment behavior depends on runtime wiring.

3. **GGML path maturity — injection resolved, external dependencies remain**
   - GGML injection APIs (`GgmlAllocFn`, `PrefetchFn`, `TypeRegistrationFn`) are fully resolved in ThemisDB (STUB #263a/b/c, 2026-05-11, GTB-01..GTB-09).  The intentional fallback behavior when no fn is injected (nullptr / no-op / type-id 9999) is by design.  Remaining external dependencies: upstream `GGML_TYPE_TT` type registration in ggml (targeted Q1 2027) and `llama_kv_cache_inject()` API in llama.cpp (not yet upstream).

4. **Store/load semantics in bridge implementation**
   - `AdaLoraTTBridge::store/loadAdapter` currently operate through bridge-managed in-memory cache semantics (`export_cache`); persistence to `TensorNetworkStorageEngine` is not exercised in this implementation path.  Durable lifecycle guarantees should be validated explicitly per deployment wiring.  Note: `RocksDBTensorBackend` provides the infrastructure for durable TT-core persistence through `TensorNetworkStorageEngine::put()` (annotated as resolving adalora durability in the backend Doxygen), but the bridge's `store()` method does not call through to the engine — it remains an open wiring gap in the bridge code.

5. **Design-target latency estimates not benchmark-backed**
   - The `adalora_tt_bridge.h` source comment contains a design estimate of `~5–15 ms` for FLARE live adapter switching (vs. `300–2000 ms` model reload), and `findSimilarAdapters()` carries a `≤ 15 ms per call` note.  These figures appear as code comments expressing design intent; no reproducible benchmark artifact exists in this repository state to confirm or refute them.  They are treated as unverified design targets until the experiments described in §4.2 are executed.

6. **Threats to validity**
   - External validity is limited until experiments include multiple model sizes, domains, and hardware classes.

---

## 6. Related Work

- **LoRA / AdaLoRA / QLoRA** define the PEFT baseline space [R2, R3, R4].
- **Tensor-Train decomposition** and low-rank approximation theory provide the mathematical compression basis [R5, R6].
- **Active retrieval generation (FLARE)** motivates adaptive context-time specialization scenarios [R7].

This draft contributes a repository-grounded systems review, not yet a completed performance paper.
The benchmark protocol to produce that performance evidence is now defined in
`research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md` with tooling under
`research/experiments/adalora_tt_bridge/`.

---

## 7. Conclusion

The ThemisDB repository already contains meaningful AdaLoRA-TT bridge infrastructure: export/import logic, rank-rounding and mapping hooks, and similarity graph integration points. What is still missing for publication-grade claims is rigorous benchmark evidence and artifact traceability for latency, storage, and quality outcomes. This revised draft is therefore review-ready in structure and factual scope: it documents what is implemented now, what remains hypothesis, and what experiments are required to close the evidence gap.

A complete, implementation-aware benchmark protocol is now defined at
`research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md`. Tracks BT-1 (load latency),
BT-2 (dedup efficiency), and BT-3 (rank pruning quality) are runnable against the
current in-process bridge. Track BT-4 (FLARE runtime switching) is explicitly gated
on GGML bridge wiring and FLARE integration and must not be reported until that gate
is cleared.

---

## 8. Revision Audit

This section records claim corrections made in this revision relative to earlier drafts, the rationale for each change, and outstanding evidence gaps.

### 8.1 Downgraded or Removed Claims

| # | Original Claim | Action | Rationale |
|---|---|---|---|
| A1 | FLARE live adapter switching completes in `~5–15 ms` (presented as a result) | Downgraded to **design estimate** | Appears only as a source comment in `adalora_tt_bridge.h`; no benchmark artifact present (E3 tests are functional, not performance) |
| A2 | `findSimilarAdapters()` latency `≤ 15 ms per call` (presented as measured) | Downgraded to **design target** | Header Doxygen comment, not a measured result; no timing test in `tests/test_adalora_tt_bridge.cpp` |
| A3 | Specific storage reduction percentages across adapter corpora | Maintained as **deferred / requires evidence** | No dedup-ratio artifact available; `auto_deduplicate` path exists (E1) but has no corpus benchmark |
| A4 | `store()`/`loadAdapter()` route through `TensorNetworkStorageEngine` durable storage | Clarified to **in-memory cache path** | `src/training/adalora_tt_bridge.cpp` shows `export_cache` (`std::unordered_map`); `TensorNetworkStorageEngine` member is held but the current `store()`/`loadAdapter()` code does not write to or read from it |
| A5 | R1 reference URL pointed to `blob/main/README.md` | **Fixed** to `blob/develop/README.md` | `main` is a legacy branch name superseded by `community`; `develop` is the canonical integration branch per `BRANCHING_STRATEGY.md` |

### 8.2 Added or Upgraded Evidence

| # | Addition | Reason |
|---|---|---|
| N1 | ALTB-DR-01 and ALTB-DR-02 added to E3 | These data-race regression tests are present in `tests/test_adalora_tt_bridge.cpp` but were absent from the evidence table; they verify thread-safety of `storeAdapter()` + `findSimilarAdapters()` under concurrent access |
| N2 | `storeAdapter()` backward-compatible alias documented in §2.2 | The alias is present in the header but was omitted; tests explicitly call it |
| N3 | E4 updated to reflect resolved GGML injection APIs (STUB #263a/#263b/#263c) + GTB-01..GTB-09 test coverage | All three injection stubs were resolved 2026-05-11; the draft's prior wording implied unresolved stubs when the injection mechanism is fully implemented and test-covered in `tests/tensor/test_tensor_phase3.cpp` |
| N4 | E8 added: `TTQuantizer` (INT8/NF4) as new storage infrastructure evidence | New component (`include/storage/tt_quantizer.h`, `src/storage/tt_quantizer.cpp`) integrated into `TensorNetworkStorageEngine`; directly relevant to future storage reduction claims |

### 8.3 Outstanding Evidence Gaps

| Gap | Required Artifact | Target |
|---|---|---|
| G1 | Cold/warm adapter load latency benchmark | benchmarks/training/ or benchmarks/wave*/; report p50/p95/p99 + hardware spec |
| G2 | Cross-adapter dedup ratio benchmark (homogeneous vs. heterogeneous adapter corpus) | Needs corpus, run script, and result artifact; `TTQuantizer` (E8) should be included to measure compressed storage impact |
| G3 | Rank-pruning quality benchmark (reconstruction error, downstream task delta) | Needs baseline comparator and task dataset |
| G4 | FLARE-style online switch end-to-end quality + latency benchmark | ThemisDB injection APIs resolved (E4 GTB-01..GTB-09); remaining external dependency: `llama_kv_cache_inject()` API in upstream llama.cpp |
| G5 | `TensorNetworkStorageEngine` durable persistence wiring for `store()`/`loadAdapter()` in bridge | `RocksDBTensorBackend` provides the durable backend (E4); the bridge's `store()` does not yet call `engine->put()` — open wiring gap in `adalora_tt_bridge.cpp` |
| G6 | `GGML_TYPE_TT` upstream registration in ggml runtime | `TypeRegistrationFn` injection resolved in ThemisDB (STUB #263c, E4); remaining dependency is the upstream ggml type registration PR (external); targeted Q1 2027 per `ggml_tensor_bridge.h` |

---

## References

- **[R1]** ThemisDB README (project architecture and capability framing).
  https://github.com/makr-code/ThemisDB/blob/develop/README.md

- **[R1b]** ThemisDB AdaLoRA↔TT Bridge Benchmark Protocol.
  `research/ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md` (this repository, 2026)

- **[R1c]** ThemisDB AdaLoRA↔TT Bridge Benchmark Tooling.
  `research/experiments/adalora_tt_bridge/` (this repository, 2026)

- **[R2]** Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models.*
  arXiv: https://arxiv.org/abs/2106.09685

- **[R3]** Zhang, Q. et al. (2023). *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning.*
  arXiv: https://arxiv.org/abs/2303.10512

- **[R4]** Dettmers, T. et al. (2023). *QLoRA: Efficient Finetuning of Quantized LLMs.*
  arXiv: https://arxiv.org/abs/2305.14314

- **[R5]** Oseledets, I. V. (2011). *Tensor-Train Decomposition.* SIAM Journal on Scientific Computing, 33(5), 2295-2317.
  DOI: https://doi.org/10.1137/090752142

- **[R6]** Eckart, C., & Young, G. (1936). *The Approximation of One Matrix by Another of Lower Rank.* Psychometrika, 1(3), 211-218.
  DOI: https://doi.org/10.1007/BF02288367

- **[R7]** Jiang, Z. et al. (2023). *Active Retrieval Augmented Generation.*
  arXiv: https://arxiv.org/abs/2305.06983
