# LLM Processing Optimization Patterns for ThemisDB: Codebase Review and Integration Guidance

**Status**: In Review
**Version**: 0.2
**Last Updated**: 2026-05-11

---

## Abstract

This document reviews LLM inference optimization patterns (continuous batching, speculative decoding, KV-cache management, and embedding/prefix caching) and verifies which claims are currently supported by the ThemisDB repository. The review is grounded in code and benchmark artifacts, with explicit claim boundaries. The main finding is that ThemisDB already exposes relevant integration surfaces (AQL LLM commands, RAG execution paths, LLM statistics/circuit-breaker telemetry, and benchmark harnesses), but end-to-end mixed-load validation of serving optimizations remains partially pending. We therefore separate (1) supported implementation-level claims from (2) deferred performance claims that require dedicated experiments. This structure is intended to make the article review-ready and consistent with the current repository state.

## Introduction

LLM-serving optimization patterns are often reported in serving-only environments. In ThemisDB, the LLM path is integrated into a **multi-model database** stack and interacts with AQL execution, retrieval, and transactional infrastructure. That integration context changes what can be claimed safely.

### Problem

Earlier versions of this document mixed concrete implementation observations with unverified performance forecasts (for example, unconditional 2-4x speedup claims). This revision removes unsupported claims and ties every central statement to repository artifacts.

### Scope

This review focuses on:

- AQL-facing LLM operations (`LLM RAG`, `LLM EMBED`, `LLM MODEL`, `LLM LORA`, `LLM STATS`)
- LLM/RAG runtime integration points and observability
- Benchmark artifacts relevant to optimization patterns
- Terminology alignment (AQL, multi-model, consistency model, component names)

## Methodology

We used a repository-grounded verification method with three steps:

1. **Artifact inspection**: verify LLM/RAG capability claims against concrete files in `src/`, `include/`, `benchmarks/`, and top-level architecture/performance documents.
2. **Claim classification**:
   - **Supported claim**: directly backed by implementation or measured artifact.
   - **Deferred claim**: plausible but not yet validated by dedicated mixed-load experiment data.
3. **Terminology normalization**: enforce consistent use of ThemisDB terms:
   - **AQL** (Advanced Query Language)
   - **Multi-model database**
   - **Consistency model**: strong consistency via Raft and eventual consistency via Gossip (deployment-selectable), plus MVCC/OCC/2PC/SAGA references in system documentation.

### Evidence anchors used in this review

| ID | Artifact | What it supports |
|---|---|---|
| E1 | `src/aql/llm_aql_handler.cpp` | Concrete AQL LLM command execution paths (`RAG`, `EMBED`, model/LoRA lifecycle, stats, cache stats) and error handling |
| E2 | `src/aql/ARCHITECTURE.md` | `LlmAqlHandler` role, AQL LLM command surface, integration/threading model |
| E3 | `ARCHITECTURE.md` | System-level capability declarations including async inference, continuous batching, paged KV-cache, RAG evaluation components |
| E4 | `include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md` | Canonical internal mapping from llama.cpp example patterns to ThemisDB component design |
| E5 | `benchmarks/bench_v1_3_0_features.cpp` | Existing benchmark functions for embedding cache, hybrid search, and combined LLM-RAG API-surface path |
| E6 | `PERFORMANCE_EXPECTATIONS.md` | Current measured baseline context, SLO framing, and explicit statement that parts of AI/ML measurement wave are pending |

## Evaluation / Experiments

### A. What is currently supported by repository evidence

1. **AQL LLM command surface is implemented and observable** (E1, E2).
   The code includes execution paths and explicit failures for `LLM RAG`, `LLM EMBED`, `LLM MODEL`, `LLM LORA`, `LLM STATS`, plus circuit-breaker and cache statistics output.

2. **Optimization patterns are documented as integration targets** (E3, E4).
   The architecture and internal insight documents describe continuous batching, paged KV-cache, and speculation-related mappings.

3. **Relevant benchmark harnesses exist** (E5).
   Benchmarks such as `BM_EmbeddingCache_Query_Hit`, `BM_HybridSearch_RRF`, and `BM_Combined_LLM_RAG_Pipeline` are present.

4. **Important caveat for benchmark interpretation** (E5, E6).
   `BM_Combined_LLM_RAG_Pipeline` explicitly benchmarks API-surface simulation (including null manager pointers in setup), so it must not be interpreted as production E2E serving proof without additional runs.

### B. Current quantitative context (verified)

From `PERFORMANCE_EXPECTATIONS.md` (E6), ThemisDB reports strong core-system baselines (for example Graph and time-series SLO hits and query P99 target achievement), while also documenting pending measurement work for parts of distributed/AI-ML modules. This supports a conservative conclusion:

- The optimization direction is technically plausible and partially scaffolded.
- Final superiority claims for mixed-load LLM serving are **not yet fully closed**.

### C. Experiment plan required to close deferred claims

To validate optimization impact rigorously, run a focused matrix:

- **Workloads**: serving-only, retrieval+inference, mixed transactional + retrieval + inference
- **Policies**: baseline, batching-only, batching+KV policy, batching+KV+speculation
- **Metrics**: TTFT, p50/p95/p99 latency, req/s, tokens/s, fallback rate, cache hit rate
- **Acceptance**: improvement must hold under mixed load, not only in isolated runs

## Terminology and Consistency Alignment

This revision standardizes terminology as follows:

- Use **AQL** consistently for language/runtime command references.
- Use **multi-model database** for system positioning.
- Use **consistency model** with explicit distinction: Raft (strong) vs Gossip (eventual), and transaction/concurrency terms MVCC, OCC, 2PC, SAGA where documented.
- Use component names as they appear in repository artifacts (`LlmAqlHandler`, `RagJudge`, `CoherenceEvaluator`, `InferenceEngineEnhanced`, etc.).

## Limitations / Known Issues

1. Some optimization descriptions are currently architecture/design level rather than fully benchmark-validated production claims.
2. AI/ML measurement coverage is explicitly staged in project performance reporting; not every optimization has closed empirical deltas yet.
3. Existing benchmark files include both production-near and synthetic/API-surface scenarios; interpretation must distinguish these classes.
4. Hardware-sensitive optimizations (e.g., KV strategies, speculation policy) require environment-specific validation before broad generalization.

## Conclusion

ThemisDB already contains meaningful implementation and architectural groundwork for LLM processing optimizations, especially in AQL-integrated LLM/RAG flows and instrumentation. However, only a subset of performance claims can currently be stated as measured outcomes. The correct review-ready position is:

- keep supported implementation claims,
- keep measured baseline claims,
- defer mixed-load optimization superiority claims until dedicated experiments are completed and reported.

## References

1. ThemisDB repository. https://github.com/makr-code/ThemisDB
2. `src/aql/llm_aql_handler.cpp` (AQL LLM execution and telemetry paths). https://github.com/makr-code/ThemisDB/blob/main/src/aql/llm_aql_handler.cpp
3. `ARCHITECTURE.md` (system architecture and capability declarations). https://github.com/makr-code/ThemisDB/blob/main/ARCHITECTURE.md
4. `PERFORMANCE_EXPECTATIONS.md` (benchmark/SLO report and measurement status). https://github.com/makr-code/ThemisDB/blob/main/PERFORMANCE_EXPECTATIONS.md
5. `benchmarks/bench_v1_3_0_features.cpp` (LLM/RAG-related benchmark functions). https://github.com/makr-code/ThemisDB/blob/main/benchmarks/bench_v1_3_0_features.cpp
6. `include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md` (internal mapping of llama.cpp optimization patterns). https://github.com/makr-code/ThemisDB/blob/main/include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md
7. Y. Leviathan, M. Kalman, and Y. Matias. *Fast Inference from Transformers via Speculative Decoding*. ICML 2023. https://proceedings.mlr.press/v202/leviathan23a.html
8. W. Kwon et al. *Efficient Memory Management for Large Language Model Serving with PagedAttention*. SOSP 2023. https://arxiv.org/abs/2309.06180
9. T. Dao et al. *FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness*. NeurIPS 2022. https://arxiv.org/abs/2205.14135
10. Z. Fu et al. *Break the Sequential Dependency of LLM Inference Using Lookahead Decoding*. arXiv 2023. https://arxiv.org/abs/2312.11462
