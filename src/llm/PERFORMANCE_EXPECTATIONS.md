---
Author: "ThemisDB Contributors"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "GitHub Copilot (exact model backend not recorded)"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/llm

## Scope
- Module: src/llm
- This file defines measurable LLM module performance expectations for release gating.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/llm/bench_llm_real_models.cpp (built as `bench_llm_inference_performance`)
  - benchmarks/bench_llm_infrastructure.cpp
  - benchmarks/bench_llm_response_cache.cpp
  - benchmarks/bench_llm_raid_pipeline.cpp
  - benchmarks/bench_rag_hybrid_retriever.cpp

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| LLM-1 | Embedding calls return non-empty finite vectors | RealLLMBench/RealModel_TextEmbedding_Generation, RealLLMBench/RealModel_BatchEmbedding_100Docs |
| LLM-2 | Inference calls succeed and produce non-empty output | RealLLMBench/RealModel_TextGeneration_50Tokens, RealLLMBench/RealModel_ContextScaling |
| LLM-3 | Model load succeeds when a model artifact is configured | BM_RealModel_LoadingTime |
| LLM-5 | Cache hit/miss/mixed workload regressions remain bounded | BM_CacheGetExactHit, BM_CacheGetMiss, BM_CacheMixedWorkload |
| LLM-6 | RAID routing/fan-out overhead remains bounded | BM_DomainRouting_OverheadPerRequest, BM_BatchFanOut_LatencyScaling |
| LLM-7 | Hybrid retriever path remains within baseline budget | BM_HybridRetriever_BM25Baseline, BM_HybridRetriever_VectorizerPath |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| LG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| LG-2 | Model-backed inference returns valid output when the configured artifact is available | RealLLMBench/RealModel_TextGeneration_50Tokens |
| LG-3 | Routing overhead p99 <= release threshold | p99 from BM_DomainRouting_OverheadPerRequest |
| LG-4 | No benchmark case missing in mapped release run | benchmark run manifest completeness |

## P0.3 — Absolute SLO Baselines per VRAM Class

These absolute gates supplement the relative LG-1..LG-4 guards.  Values are
**engineering targets derived from llama.cpp community benchmarks on equivalent
hardware** — marked as **ESTIMATE** until measured on ThemisDB hardware.

*Measurement conditions assumed:* single-user, q4_K_M quantization, no LoRA,
prompt = 512 tokens, generation = 256 tokens, context window = 4096.

### 8 GB VRAM (RTX 3060 / RTX 4060 class)

| SLO ID | Metric | Target | Status |
|--------|--------|--------|--------|
| SLO-8G-01 | TTFT (time-to-first-token), p50 | ≤ 800 ms | ESTIMATE |
| SLO-8G-02 | TTFT, p99 | ≤ 2 500 ms | ESTIMATE |
| SLO-8G-03 | Tokens/s (generation), p50 | ≥ 20 tok/s | ESTIMATE |
| SLO-8G-04 | Tokens/s (generation), p99 | ≥ 10 tok/s | ESTIMATE |
| SLO-8G-05 | End-to-end latency (512+256 tokens), p95 | ≤ 18 s | ESTIMATE |
| SLO-8G-06 | Max sustainable concurrency (no OOM) | 1 request | ESTIMATE |

*Applicable model*: 7B parameter models at Q4_K_M (≈ 4 GB VRAM).

### 12 GB VRAM (RTX 3080 / RTX 4070 class)

| SLO ID | Metric | Target | Status |
|--------|--------|--------|--------|
| SLO-12G-01 | TTFT, p50 | ≤ 500 ms | ESTIMATE |
| SLO-12G-02 | TTFT, p99 | ≤ 1 500 ms | ESTIMATE |
| SLO-12G-03 | Tokens/s (generation), p50 | ≥ 35 tok/s | ESTIMATE |
| SLO-12G-04 | Tokens/s (generation), p99 | ≥ 18 tok/s | ESTIMATE |
| SLO-12G-05 | End-to-end latency (512+256 tokens), p95 | ≤ 10 s | ESTIMATE |
| SLO-12G-06 | Max sustainable concurrency (no OOM) | 2 requests | ESTIMATE |

*Applicable models*: 7B Q8_0 (≈ 8 GB VRAM) or 13B Q4_K_M (≈ 8 GB VRAM).

### 24 GB VRAM (RTX 3090 / RTX 4090 / A5000 class)

| SLO ID | Metric | Target | Status |
|--------|--------|--------|--------|
| SLO-24G-01 | TTFT, p50 | ≤ 250 ms | ESTIMATE |
| SLO-24G-02 | TTFT, p99 | ≤ 800 ms | ESTIMATE |
| SLO-24G-03 | Tokens/s (generation), p50 | ≥ 55 tok/s | ESTIMATE |
| SLO-24G-04 | Tokens/s (generation), p99 | ≥ 30 tok/s | ESTIMATE |
| SLO-24G-05 | End-to-end latency (512+256 tokens), p95 | ≤ 6 s | ESTIMATE |
| SLO-24G-06 | Max sustainable concurrency (no OOM) | 4 requests | ESTIMATE |

*Applicable models*: 13B Q8_0 (≈ 14 GB VRAM) or 34B Q4_K_M (≈ 20 GB VRAM).

### Absolute Regression Gate

| Gate ID | Condition | Action |
|---------|-----------|--------|
| SLO-REG-01 | p99 Tokens/s drops > 20% vs preceding release run on same hardware class | Block release — requires investigation |
| SLO-REG-02 | p99 TTFT exceeds class target by > 50% | Block release — requires investigation |
| SLO-REG-03 | Concurrent-request OOM at rated concurrency | Block release |

### Measurement Methodology

- Run benchmarks with `kCanonicalRngSeed=42` (consistent with `benchmarks/bench_fixtures.h`).
- Use `UseRealTime()` for all latency measurements.
- Record hardware class, driver version, model path, quantization level, and context size in
  each benchmark result artifact.
- Promote ESTIMATE → MEASURED once a reproducible baseline run is captured on ThemisDB CI
  hardware and merged to `RELEASE_GATE_MANIFEST_LLM.json` (to be created).
- Wave-B execution workflow `build-wave-b-llm-benchmarks.yml` now emits `break_even_summary_<scenario>.json` artifacts as baseline-candidate evidence for ANN/Tensor/LLM dispatch decisions; ESTIMATE values remain non-release until promoted to MEASURED with representative hardware proof.

## Validation
- Expectations are considered met when mapped benchmarks run reproducibly in release profile and stay within configured thresholds.
- For proxy-only targets, follow-up benchmark hardening tasks must remain tracked.

## Sourcecode Verification (Module: llm/performance)

- Verified benchmark sources:
  - benchmarks/llm/bench_llm_real_models.cpp
  - benchmarks/bench_llm_infrastructure.cpp
  - benchmarks/bench_llm_response_cache.cpp
  - benchmarks/bench_llm_raid_pipeline.cpp
  - benchmarks/bench_rag_hybrid_retriever.cpp
- Verified mapping surfaces:
  - model load, real model embedding, and inference output validation (RealLLMBench/RealModel_*)
  - cache behavior (BM_Cache*)
  - routing and fan-out overhead (BM_DomainRouting_*, BM_BatchFanOut_*)
  - retrieval path overhead (BM_HybridRetriever_*)
- Result:
  - RealModel cases call `LLMPluginManager`; synthetic token generation and fabricated `llama_context` benchmarks have been removed.
  - This suite does not yet measure request-level TTFT/TPOT percentiles or establish numeric performance thresholds; those remain open until suitable model artifacts and representative hardware evidence are available.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
