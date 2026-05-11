# LLM Processing Optimization Patterns in ThemisDB: Evidence-Bound arXiv Draft

**Status**: In Review
**Version**: 0.3
**Last Updated**: 2026-05-11
**Target Venue**: arXiv (cs.DB / cs.DC / cs.LG)

---

## Abstract

Large Language Model (LLM) serving optimizations such as continuous batching, speculative decoding, and KV-cache strategies are typically evaluated in serving-only stacks. ThemisDB requires a different evaluation lens because inference is co-located with retrieval, query execution, and transactional components in a multi-model database runtime. This paper reviews optimization patterns and maps them to verifiable ThemisDB artifacts. Our contribution is an evidence-bounded synthesis: we separate implementation-supported claims from deferred performance claims that still require mixed-load experiments. The repository already exposes concrete integration surfaces (AQL LLM commands, RAG execution paths, observability outputs, benchmark harnesses), but some end-to-end superiority claims remain open pending dedicated measurements. We provide a reproducible experimental design, explicit claim boundaries, and a traceability table from claims to repository evidence.

## I. Introduction

### A. Problem context

Database-native LLM serving differs from standalone model serving. In ThemisDB, LLM execution shares resources with AQL processing, retrieval paths, indexing, and transaction-related infrastructure. Therefore, optimization claims from isolated inference environments cannot be transferred without qualification.

### B. Gap

Earlier document versions described technically plausible optimization gains but mixed them with unverified quantitative forecasts. For submission readiness, all central claims must be either (1) evidence-backed in the repository or (2) explicitly deferred.

### C. Contributions

1. A repository-grounded mapping of LLM optimization patterns to concrete ThemisDB components.
2. A claim-bounded evaluation framing that distinguishes measured/supportable statements from pending hypotheses.
3. A minimal reproducibility and traceability scaffold aligned to arXiv draft expectations.

### D. Research questions and hypotheses

- **RQ1:** Which optimization patterns are already supported by concrete ThemisDB implementation artifacts?
- **RQ2:** Which performance claims are currently measurable from available benchmark outputs, and which remain open?
- **RQ3:** What mixed-load evaluation protocol is required to close deferred claims safely?

- **H1:** ThemisDB already contains implementation-level anchors for continuous batching, speculative/lookup decoding pathways, and cache-centric optimizations.
- **H2:** Existing benchmark artifacts support baseline-oriented conclusions but are insufficient for final mixed-load superiority claims without additional targeted runs.

## II. Related Work

Core prior work includes speculative decoding [1], paged-attention-oriented memory management [2], and IO-aware attention acceleration [3]. Practical serving systems (e.g., Orca, Sarathi-Serve) emphasize throughput/latency trade-offs in serving-centric environments [4,5].

Our novelty is not a new decoding algorithm. It is an integration review for a **database-native** system, with explicit claim-to-artifact mapping and conservative boundaries on unresolved performance claims.

## III. System Model / Architecture

ThemisDB exposes LLM functionality through AQL-facing command handlers and plugin-driven inference paths. Relevant interfaces include `LLM RAG`, `LLM EMBED`, `LLM MODEL`, `LLM LORA`, and `LLM STATS`, implemented through `LlmAqlHandler` and associated runtime components.

Assumed operational model:

- Multi-model DB workload mix (query/retrieval/inference coexistence)
- Runtime observability and failure handling via circuit breakers and statistics outputs
- Capability declarations for async inference and memory/serving optimizations in architecture docs

Failure/threat model considered in this draft:

- Overstating gains from synthetic or API-surface-only benchmark paths
- Tail-latency regressions under contention despite average-speed improvements
- Hardware/profile sensitivity of KV and speculation policies

## IV. Method / Design

This work uses a three-stage evidence method:

1. **Artifact audit:** verify statements against source files and benchmark harnesses.
2. **Claim classification:** mark each statement as supported vs deferred.
3. **Evaluation design:** define a mixed-load experiment matrix for unresolved hypotheses.

Design principle: no quantitative superiority claim is asserted without either direct measured evidence or explicit deferred status.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `src/aql/llm_aql_handler.cpp` | LLM command execution and error paths | Concrete AQL LLM operations (`RAG`, `EMBED`, model/LoRA lifecycle, stats/cache stats) | ready |
| E2 | `src/aql/ARCHITECTURE.md` | Component and threading model | `LlmAqlHandler` role and LLM command interface surface | ready |
| E3 | `ARCHITECTURE.md` | LLM capability declarations | Async inference, continuous batching, paged KV-cache, RAG module anchors | ready |
| E4 | `include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md` | Pattern mapping document | Internal mapping from llama.cpp optimization patterns to ThemisDB components | ready |
| E5 | `benchmarks/bench_v1_3_0_features.cpp` | LLM/RAG-adjacent benchmarks | Existing harnesses (`BM_EmbeddingCache_Query_Hit`, `BM_HybridSearch_RRF`, `BM_Combined_LLM_RAG_Pipeline`) | ready |
| E6 | `PERFORMANCE_EXPECTATIONS.md` | Baseline and measurement scope | Current measured baseline context plus pending AI/ML measurement scope | ready |

Rule used in this document: all major claims in Sections III-VIII map to at least one evidence ID.

## VI. Experimental Methodology

### A. Setup

- **Software:** current ThemisDB repository state with pinned commit for final submission snapshot.
- **Execution mode:** serving-only and mixed-load profiles.
- **Controls:** fixed seeds, warm-up phase, repeated runs, and percentile reporting (p50/p95/p99).

### B. Workloads

- **W1 (Serving-only):** inference-centric baseline for throughput and TTFT.
- **W2 (Retrieval + inference):** retrieval contention influence on serving behavior.
- **W3 (Mixed DB load):** query/retrieval/inference concurrency to test tail-latency stability.

### C. Metrics

- Latency: p50/p95/p99
- Throughput: requests/s and tokens/s
- Stability: fallback rate, timeout/queue pressure indicators
- Quality guardrails: response consistency checks under fixed prompt replay

## VII. Results

### A. Primary evidence-backed findings

1. **Implementation readiness exists** for AQL-facing LLM command handling and runtime observability (E1, E2).
2. **Optimization mapping is documented** at architecture and inference-pattern level (E3, E4).
3. **Benchmark harnesses exist** but include both production-near and synthetic/API-surface cases (E5).

### B. Quantitative context

`PERFORMANCE_EXPECTATIONS.md` reports strong core-system baselines and also states pending AI/ML measurement coverage for specific areas (E6). Therefore, this draft currently supports baseline feasibility claims, not final cross-workload optimization superiority.

### C. Reporting tables (submission scaffold)

Table R1. Policy comparison plan.

| Policy | Workload | TTFT p50 | Tokens/s | p99 latency | Fallback rate |
|--------|----------|----------|----------|-------------|---------------|
| Baseline | W1/W2/W3 | pending | pending | pending | pending |
| Batch+KV | W1/W2/W3 | pending | pending | pending | pending |
| Batch+KV+Spec | W1/W2/W3 | pending | pending | pending | pending |

Table R2. Claim closure status.

| Claim Class | Status | Evidence |
|------------|--------|----------|
| Implementation-level support | closed | E1-E5 |
| Mixed-load superiority | open | requires dedicated runs beyond E6 |

## VIII. Discussion

### A. Practical implications

ThemisDB has concrete integration groundwork for LLM optimization features, but deployment decisions still require mixed-load measurements to avoid overgeneralizing from partial or synthetic evidence.

### B. Threats to validity

- **Internal validity:** synthetic benchmark paths can overestimate real deployment behavior.
- **Construct validity:** throughput gains alone may hide p99 degradation.
- **External validity:** hardware and workload composition can change optimal policy selection.

### C. Claim boundaries

**Supported claims:**

- The repository contains concrete AQL-integrated LLM control paths and observability hooks (E1, E2).
- The architecture and internal LLM insight docs encode continuous batching/speculation/KV-related integration intent (E3, E4).
- Relevant benchmark harnesses are present (E5).

**Deferred claims:**

- Final policy-superiority rankings across mixed-load production scenarios remain pending dedicated experimental runs (E6 + VI).

## IX. Reproducibility & Artifact

Recommended baseline execution flow:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure -j 1 --timeout 60
```

Artifact requirements for submission snapshot:

- freeze commit hash in this section,
- publish workload configs and policy settings used for W1-W3,
- include raw metric exports plus aggregation scripts.

Known pitfalls:

- conflating API-surface microbenchmarks with full E2E serving,
- insufficient repetition for stable p99 estimates,
- hardware-dependent variance without normalized environment metadata.

## X. Limitations, Risk, Ethics

- This draft is evidence-bounded and does not claim closed optimization superiority without dedicated mixed-load runs.
- Throughput-oriented tuning can degrade reliability/quality if not bounded by tail-latency and fallback metrics.
- Deployment in sensitive environments should require rollback controls and explicit observability thresholds.

## XI. Conclusion

ThemisDB already provides verifiable implementation anchors for LLM processing optimizations in a database-native context. The current evidence supports architectural and implementation-readiness claims, while final mixed-load performance claims are intentionally deferred pending targeted experiments. This version meets arXiv-style minimum structure and establishes a traceable path from claims to repository artifacts.

## References

1. Leviathan, Y., Kalman, M., & Matias, Y. (2023). *Fast Inference from Transformers via Speculative Decoding*. ICML. URL: https://proceedings.mlr.press/v202/leviathan23a.html
2. Kwon, W., et al. (2023). *Efficient Memory Management for Large Language Model Serving with PagedAttention*. SOSP. URL: https://arxiv.org/abs/2309.06180
3. Dao, T., et al. (2022). *FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness*. NeurIPS. URL: https://arxiv.org/abs/2205.14135
4. Yu, G., et al. (2022). *Orca: A Distributed Serving System for Transformer-Based Generative Models*. OSDI. URL: https://www.usenix.org/conference/osdi22/presentation/yu
5. Agrawal, A., et al. (2024). *Taming Throughput-Latency Tradeoff in LLM Inference with Sarathi-Serve*. OSDI. URL: https://www.usenix.org/conference/osdi24/presentation/agrawal
6. Fu, Z., et al. (2023). *Break the Sequential Dependency of LLM Inference Using Lookahead Decoding*. arXiv. URL: https://arxiv.org/abs/2312.11462
7. ThemisDB Contributors. (2026). *ThemisDB Repository*. URL: https://github.com/makr-code/ThemisDB
8. ThemisDB. `src/aql/llm_aql_handler.cpp`. URL: https://github.com/makr-code/ThemisDB/blob/main/src/aql/llm_aql_handler.cpp
9. ThemisDB. `ARCHITECTURE.md`. URL: https://github.com/makr-code/ThemisDB/blob/main/ARCHITECTURE.md
10. ThemisDB. `PERFORMANCE_EXPECTATIONS.md`. URL: https://github.com/makr-code/ThemisDB/blob/main/PERFORMANCE_EXPECTATIONS.md
11. ThemisDB. `benchmarks/bench_v1_3_0_features.cpp`. URL: https://github.com/makr-code/ThemisDB/blob/main/benchmarks/bench_v1_3_0_features.cpp
12. ThemisDB. `include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md`. URL: https://github.com/makr-code/ThemisDB/blob/main/include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states contribution and boundaries
- [x] Introduction defines problem, gap, and contributions
- [x] Related work section with novelty delta
- [x] Method and assumptions are explicit
- [x] Implementation-evidence traceability table included
- [x] Experimental methodology section included
- [x] Results section distinguishes supported vs deferred claims
- [x] Threats/limitations/ethics documented
- [x] References are consistent and resolvable
- [ ] Final submission commit hash frozen in text

## Appendix B. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | AQL-integrated LLM command and observability paths exist in code. | E1, E2 |
| C2 | Architecture/docs map optimization patterns to ThemisDB runtime components. | E3, E4 |
| C3 | LLM/RAG-adjacent benchmark harnesses exist. | E5 |
| C4 | Mixed-load superiority claims remain open pending dedicated runs. | E6 |
