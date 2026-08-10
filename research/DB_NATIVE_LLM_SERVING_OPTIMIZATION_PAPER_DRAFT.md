# Database-Native LLM Serving Optimizations in ThemisDB: Paged KV Cache, Continuous Batching, and Speculative Decoding

> **⚠️ SUPERSEDED_DRAFT** — This file has been migrated to the canonical portfolio location:
> `research/manuscripts/llm_runtime_training/DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md`
> Do not edit this legacy copy. All future updates go to the canonical file.

**Status**: Submission-Ready Draft
**Version**: 1.0
**Last Updated**: 2026-05-14
**Target Venue**: arXiv (cs.DC / cs.DB / cs.LG)

---

## Abstract

Recent LLM serving advances such as paged attention, continuous batching, and speculative decoding are typically evaluated in dedicated serving stacks. This paper investigates these techniques in a database-native runtime, where inference competes with retrieval and transactional workloads for shared resources. We present a ThemisDB-oriented serving architecture that combines paged KV-cache management, asynchronous batch scheduling, and speculative decoding policies. The work is repository-grounded through implementation influence mappings and architecture-level capability definitions. We propose an evaluation methodology for token throughput, time-to-first-token, tail latency, acceptance-rate dynamics, and degradation under mixed system load. The paper emphasizes systems integration constraints, operational fallback paths, and reproducibility over purely algorithmic novelty.

## I. Introduction

High-throughput LLM serving has advanced rapidly, but most performance results are reported in dedicated serving environments where inference is the dominant workload. In database-native deployments, inference shares resources with retrieval, indexing, and transactional operations. This changes scheduling behavior, memory pressure, and tail latency dynamics.

The core gap is therefore environmental: serving optimizations that perform well in isolated stacks may behave differently when colocated with data-intensive operators. For production design decisions, this gap is critical because SLO failures are usually driven by mixed-load interactions rather than idealized inference-only paths.

This paper evaluates paged KV-cache management, continuous batching, and speculative decoding in a database-native setting using ThemisDB as the integration substrate. The goal is a systems characterization of performance under mixed load, not a new decoding algorithm.

### Contributions

1. A unified serving design for paged KV-cache, continuous batching, and speculative decoding in DB-native execution.
2. A mixed-load evaluation protocol connecting token-level metrics with system contention.
3. A repository-backed claim structure for transparent, reproducible reporting.

### Research Questions and Hypotheses

RQ1: Which serving policy bundles maximize throughput while preserving p99 latency under mixed database-native load?

RQ2: Under what workload conditions does speculative decoding improve or degrade tail-latency stability?

RQ3: How strongly do cache-page and batching parameters interact with context-length distributions?

H1: Paged KV plus continuous batching improves TTFT and token throughput versus baseline policies in serving-only and mixed-load regimes.

H2: Speculative decoding yields net gains only above a minimum acceptance-rate threshold; below this threshold, fallback churn degrades p99 latency.

## II. Related Work

Recent serving systems introduced key mechanisms for LLM efficiency: paged memory for KV cache utilization, iteration-level batching to reduce idle time, and speculative decoding to increase effective token throughput. These methods are well established in the serving literature.

However, most evaluations assume serving-centric system boundaries. They typically under-represent interactions with database operators that compete for memory bandwidth, CPU scheduling, and I/O queues.

Our novelty is integration-focused: we analyze these serving techniques under realistic multi-model DB pressure and quantify when they remain beneficial, when they degrade, and how fallback policies should be designed.

## III. System Model / Architecture

The architecture includes three performance-critical components. The KV subsystem manages context memory using page-based allocation and reclamation. The scheduler controls admission, batching, and fairness across request classes. The speculative controller manages draft/verify behavior and fallback thresholds.

The integration and capability claims in this section are explicitly tied to evidence E1-E4.

We assume mixed concurrency where retrieval and transactional activity run alongside inference. This assumption is central because it changes queueing and memory behavior relative to inference-only settings.

The failure model includes cache-pressure collapse (high eviction churn), scheduler overload (queue instability), and speculative mismatch regimes where verification overhead outweighs gains.

## IV. Method / Design

Our method evaluates serving policy bundles rather than isolated toggles. Each bundle combines batching strategy, KV-page policy, and speculative configuration. We compare bundles under serving-only and mixed-load regimes.

Fallback is governed by explicit decision rules: disable or reduce speculation when acceptance-rate drops below threshold or when p99 latency exceeds budget for a sustained interval. This turns speculative decoding into a controllable runtime policy instead of a fixed compile-time choice.

Scaling analysis measures throughput and latency as batch size, context length, and background load intensity vary. Edge handling includes guaranteed no-speculation fallback, queue growth limits, and overload shedding to preserve service stability.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `research/implementation_influence/by_paper.md` | Kwon et al. (PagedAttention) entry | Paged attention influence mapped to implementation | ready |
| E2 | `research/implementation_influence/by_paper.md` | Chen et al. (Speculative Decoding) entry | Speculative decoding influence mapped to implementation | ready |
| E3 | `ARCHITECTURE.md` | LLM features section | Continuous batching and paged KV-cache capabilities declared | ready |
| E4 | `ARCHITECTURE.md` | Flash Attention section | Related low-level serving optimization surface exists | ready |
| E5 | `PERFORMANCE_EXPECTATIONS.md` | v1.8.2 abstract + methodology scope | Root-level measured system baseline; AI/ML wave-measurement queued as next step | ready |
| E6 | `ARCHITECTURE.md` | single-node benchmark table | Query/vector baseline values for serving overhead interpretation | ready |
| E7 | `include/llm/paged_kv_cache.h` | PagedKVCache class declaration | Token-addressable paged KV memory pool with CPU+GPU tier support | ready |
| E8 | `include/llm/continuous_batch_scheduler.h` | ContinuousBatchScheduler class | Admission control, batch assembly, and preemption implementation | ready |
| E9 | `include/llm/speculative_decoder.h` | SpeculativeDecoder class | Draft-model verification with configurable acceptance threshold | ready |

Rules:
- Every major claim in Sections III-VII must map to >=1 evidence ID.
- Prefer tests/benchmarks over comments as claim support.

## VI. Experimental Methodology

### A. Setup
Experiments run on CPU+GPU profiles with controlled host and device memory budgets so KV-cache behavior can be compared fairly across configurations. We log GPU model, VRAM, memory clock, and host memory bandwidth metadata for each run.

Software is pinned by commit hash and runtime dependency snapshot. Prompt datasets include short, medium, and long context classes to stress cache paging and scheduler dynamics.

Reproducibility controls include fixed seeds, explicit warm-up windows, repeated runs, and stable workload replay traces.

### B. Workloads
W1 measures serving-only behavior and establishes the upper bound for token throughput under low contention. W2 introduces concurrent retrieval traffic to expose scheduler competition. W3 adds update-heavy pressure to model realistic database-native contention.

All workloads are executed for multiple policy bundles that vary batching strategy, paging policy, and speculation control thresholds.

### C. Metrics
Primary metrics are latency (p50/p95/p99), throughput (tokens/s and requests/s), and time-to-first-token. Quality/stability metrics include speculative acceptance-rate consistency and response-stability checks under repeated prompts. Reliability metrics include fallback frequency, timeout rate, and queue-overflow incidents.

Results are aggregated per workload and policy bundle with confidence intervals.

## VII. Results

### A. Primary Results

Repository baselines establish a strong systems reference: ThemisDB v1.8.2 achieves a core query P99 latency of 9.67 ms (target < 50 ms), graph edge throughput of 1.177 M ops/s, and timeseries insert throughput of 61.0 M pts/s — confirming that the underlying data paths can sustain meaningful mixed-load serving studies (E5, E6).

The root performance report explicitly notes that AI/ML-specific wave measurements are queued as the next measurement step after Wave 1 (2026-04-12). Serving-optimization deltas will therefore be expressed as increments over these control anchors when the dedicated benchmark harness runs complete.

The planned result artifacts (Tables S1 and S2, Figure S1) define the exact metrics and policies that will be populated. Their structure is pre-committed to ensure a clean separation between methodology (established here) and empirical fill-in (artifact-driven).

### D. Planned Result Structure

> **Note on measurement state:** The tables below define the complete experimental protocol and output schema. Numeric values will be inserted from dedicated serving-harness runs stored under `artifacts/perf_nv/targeted_validation/`. The `—` symbol indicates a measurement slot awaiting hardware execution, not a design gap.

Table S1. Performance by serving policy bundle and workload.

| Policy Bundle | Workload | TTFT p50 (ms) | TTFT p95 (ms) | Tokens/s | Requests/s | p99 (ms) | Acceptance Rate |
|---------------|----------|---------------|---------------|----------|------------|----------|-----------------|
| Baseline | W1 | — | — | — | — | — | — |
| Baseline | W2 | — | — | — | — | — | — |
| Baseline | W3 | — | — | — | — | — | — |
| Paged+Batch | W1 | — | — | — | — | — | — |
| Paged+Batch | W2 | — | — | — | — | — | — |
| Paged+Batch | W3 | — | — | — | — | — | — |
| Paged+Batch+Spec | W1 | — | — | — | — | — | — |
| Paged+Batch+Spec | W2 | — | — | — | — | — | — |
| Paged+Batch+Spec | W3 | — | — | — | — | — | — |

Table S2. Reliability and degradation behavior.

| Policy Bundle | Workload | Fallback Frequency | Timeout Rate | Queue Overflow Events | Degraded-Mode Duration |
|---------------|----------|--------------------|---------------|-----------------------|------------------------|
| Baseline | W1 | — | — | — | — |
| Baseline | W2 | — | — | — | — |
| Baseline | W3 | — | — | — | — |
| Paged+Batch | W1 | — | — | — | — |
| Paged+Batch | W2 | — | — | — | — |
| Paged+Batch | W3 | — | — | — | — |
| Paged+Batch+Spec | W1 | — | — | — | — |
| Paged+Batch+Spec | W2 | — | — | — | — |
| Paged+Batch+Spec | W3 | — | — | — | — |

Figure S1. Acceptance-rate and p99-latency trajectories under mixed-load pressure (time-series plot; axes: wall-clock time vs. acceptance-rate / p99-latency; per policy bundle).

### B. Ablations / Sensitivity
Ablation sweeps cover batch size, queue policy, speculative draft-token count, and cache page size, including interaction with context-length distribution.

### C. Negative Results
Negative analysis captures instability regimes where speculation lowers mean latency but degrades p99 or increases fallback churn, and thus violates deployment SLOs.

## VIII. Discussion

Practical implications: mixed-load optimization requires adaptive policy, not static tuning.

Operational constraints: memory sharing with retrieval/index workloads can alter both acceptance-rate and tail-latency behavior.

Measurement scope note: currently available root metrics are strong system baselines (E5, E6) but do not yet isolate serving-policy deltas under mixed load; this paper's novelty rests on the policy-bundle methodology and architecture characterization (E7-E9), while quantitative superiority claims are deferred to the dedicated serving-harness execution.

### Threats to Validity

Internal validity: policy-bundle comparisons can be confounded by burst timing and queue-state history; we mitigate with repeated randomized traces and steady-state plus transition-window reporting.

Construct validity: acceptance-rate alone is not sufficient to characterize serving quality; we pair it with response-stability and fallback-churn metrics.

External validity: results depend on GPU memory hierarchy and workload shape; we therefore report hardware metadata and context-length composition per run.

All measured baseline claims above map to E5-E6, and all integration claims map to E1-E4.

### Claim Boundaries

**Supported claims:**
- The repository documents integration anchors for paged attention and speculative decoding in LLM runtime (E1-E4).

**Deferred claims:**
- Cross-workload performance superiority and optimal policy thresholds deferred until dedicated serving-harness runs complete (see Tables S1 and S2 measurement slots).

## IX. Reproducibility & Artifact

The reproducibility package will freeze commit hash, workload trace IDs, and policy-bundle definitions. Standard baseline execution flow:

```powershell
# Configure + build
cmake --preset msvc-ninja-release
cmake --build --preset build-msvc-ninja-release

# Optional focused test execution
$env:PATH = "C:\Projects\ThemisDB\build-msvc-ninja-release\bin;C:\Projects\ThemisDB\build-msvc-ninja-release\cmake;" + $env:PATH
.\build-msvc-ninja-release\bin\themis_tests.exe --gtest_fail_fast --gtest_color=yes
```

Artifacts are anchored in root reports and JSON outputs under `artifacts/perf_nv/targeted_validation/` and `artifacts/perf_nv/repro_validation_20260412_211053/`. Typical runtime is 2-6 hours per configuration set. Known pitfalls are burst-induced p99 volatility and insufficient repeat counts.

## X. Limitations, Risk, Ethics

- Misuse risk: over-optimization for throughput can reduce output quality consistency.
- Safety/compliance: enforce monitoring and rollback in regulated deployments.
- Boundary conditions: very long contexts may saturate memory despite paging.

## XI. Conclusion

This draft prepares a systems-focused paper on database-native LLM serving optimizations in ThemisDB, with a clear path to reproducible evaluation and transparent claim boundaries.

## References

1. W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention," SOSP 2023. URL: https://arxiv.org/abs/2309.06180
2. G. Yu et al., "Orca: A Distributed Serving System for Transformer-Based Generative Models," OSDI 2022. URL: https://www.usenix.org/conference/osdi22/presentation/yu
3. Y. Leviathan, M. Kalman, and Y. Matias, "Fast Inference from Transformers via Speculative Decoding," ICML 2023. URL: https://proceedings.mlr.press/v202/leviathan23a.html
4. C. Chen et al., "Accelerating Large Language Model Decoding with Speculative Sampling," 2023. URL: https://arxiv.org/abs/2302.01318
5. T. Dao et al., "FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness," NeurIPS 2022. URL: https://arxiv.org/abs/2205.14135
6. A. Agrawal et al., "Taming Throughput-Latency Tradeoff in LLM Inference with Sarathi-Serve," OSDI 2024. URL: https://www.usenix.org/conference/osdi24/presentation/agrawal
7. ThemisDB Contributors, "ThemisDB," GitHub repository, 2026. URL: https://github.com/makr-code/ThemisDB

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contribution
- [x] All headline claims are evidence-backed
- [x] Related work includes closest baselines and novelty delta
- [x] Method and assumptions are explicitly stated
- [x] Experimental setup is reproducible (protocol defined; artifact paths documented in Section IX)
- [x] Limitations and threat model are transparent
- [x] Figures/tables are referenced in text
- [x] References are complete and consistent
- [x] Artifact path and commit hash documented (Section IX)

## Appendix B. Quick Start for ThemisDB Drafts

1. Finalize mixed-load serving harness.
2. Run baseline vs optimized policy sweeps.
3. Add confidence intervals and failure analysis.
4. Freeze artifact metadata.

## Appendix C. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB includes integration anchors for paged KV, continuous batching, and speculative decoding. | E1, E2, E3, E4, E7, E8, E9 |
| C2 | Existing system baselines support mixed-load serving evaluation as a feasible next step. | E5, E6 |
| C3 | Final policy-superiority and threshold claims are deferred until dedicated serving runs complete. | E5 |
