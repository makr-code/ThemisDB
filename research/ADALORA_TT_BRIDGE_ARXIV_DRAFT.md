# From Adapters to Archives: A Tensor-Train Bridge for AdaLoRA in ThemisDB

**Status:** Review-ready draft
**Last Updated:** 2026-05-13
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
- **In-process store/load path** through an internal cache keyed by `tenant + adapter_name` (`store`, `loadAdapter`).
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
| E3 | `tests/test_adalora_tt_bridge.cpp` | Verified behavior for `mapAdapter()` fallback/delegation and `roundAndReallocate()` callback routing |
| E4 | `include/storage/ggml_tensor_bridge.h` + `src/storage/ggml_tensor_bridge.cpp` | GGML bridge is compile-gated and still contains simulation/fallback behavior unless real alloc/registration callbacks are wired |
| E5 | `src/training/ROADMAP.md` | Bridge roadmap tracks phases and identifies broader integration as planned work |
| E6 | `src/STUB_INVENTORY.md` | Historical stub context and current callback-based bridge status for AdaLoRA TT and GGML bridge paths |
| E7 | `README.md` | Canonical system positioning and terminology: multi-model architecture, AQL query layer, ACID transaction capabilities |

### Claim boundaries

**Supported claims (current repository state):**

- AdaLoRA-to-TT bridge APIs are implemented and test-covered for hook behavior (E1-E3).
- Similarity graph integration points exist in bridge code (E1) and align with fingerprint-graph capabilities (E6).
- GGML integration is not a fully unconditional production path yet; it depends on integration callbacks/build gating (E4, E6).

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

3. **GGML path maturity constraints**
   - GGML bridge code is compile-gated and includes fallback/simulation paths; production behavior depends on real allocator/type-registration integration.

4. **Store/load semantics in bridge implementation**
   - `AdaLoraTTBridge::store/loadAdapter` currently operate through bridge-managed cache semantics in this implementation path; durable lifecycle guarantees should be validated explicitly per deployment wiring.

5. **Threats to validity**
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

## References

- **[R1]** ThemisDB README (project architecture and capability framing).
  https://github.com/makr-code/ThemisDB/blob/main/README.md

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
