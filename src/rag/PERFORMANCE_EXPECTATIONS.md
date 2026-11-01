---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/rag

> These are target definitions, not measured results. The root
> `benchmarks/CMakeLists.txt` conditionally registers
> `bench_rag_evaluation`, `bench_rag_hybrid_retriever`, and `bench_rag_ethics`
> when Google Benchmark and the sources are available. CTest smoke entries are
> also conditional on target creation. Neither registration nor a smoke run
> establishes the performance gates below.

## Scope
- Module: src/rag
- This file defines measurable RAG module performance expectations for release gating.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/rag/bench_rag_evaluation.cpp (`bench_rag_evaluation`)
  - benchmarks/rag/bench_rag_hybrid_retriever.cpp (`bench_rag_hybrid_retriever`)
  - benchmarks/rag/bench_rag_ethics.cpp (`bench_rag_ethics`)
  - benchmarks/rag/bench_delegate_evaluator.cpp (registered target)

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| RAG-1 | recall-at-k behavior remains within release baseline budget | BM_RecallAtK, BM_RecallAt10 |
| RAG-2 | judge latency envelopes remain within release baseline budget | BM_RAGJudge_FAST, BM_RAGJudge_BALANCED, BM_RAGJudge_THOROUGH |
| RAG-3 | batch and distributed evaluator overhead remains bounded | BM_RAGJudge_Batch, BM_DistributedEvaluator_Homogeneous, BM_DistributedEvaluator_FastThorough |
| RAG-4 | hybrid retrieval fusion overhead remains bounded | BM_RRF_Balanced, BM_RRF_Disjoint, BM_Linear_Balanced |
| RAG-5 | retrieval-path baseline comparisons remain bounded | BM_RRF_BM25Only, BM_RRF_VectorOnly, BM_HybridRetriever_BM25Baseline, BM_HybridRetriever_VectorizerPath |
| RAG-6 | prompt-injection detection and sanitization overhead remains bounded | BM_InjectionDetector_Benign, BM_InjectionDetector_Injected, BM_InjectionDetector_Documents, BM_InjectionSanitizer |
| RAG-7 | ethics and bias evaluation overhead remains bounded | BM_EthicalCompliance_Full_Good, BM_BiasDetection_Balanced, BM_EthicalGapDetection_VaryingDocCount |
| RAG-8 | delegate round-trip corruption benchmark runtime remains bounded | BM_DelegateEvaluator_JsonRoundTrip_10k, BM_DelegateEvaluator_JsonEval_100KB |
| RAG-9 | end-to-end RAG pipeline latency remains within release budget | BM_EndToEnd_Pipeline |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| RG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| RG-2 | Judge path p99 <= release threshold | p99 from BM_RAGJudge_FAST/BALANCED/THOROUGH |
| RG-3 | End-to-end pipeline p99 <= release threshold | p99 from BM_EndToEnd_Pipeline |
| RG-4 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation
- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: rag/performance)

- Verified benchmark sources:
  - benchmarks/rag/bench_rag_evaluation.cpp
  - benchmarks/rag/bench_rag_hybrid_retriever.cpp
  - benchmarks/rag/bench_rag_ethics.cpp
  - benchmarks/rag/bench_delegate_evaluator.cpp
- Verified mapping surfaces:
  - recall and judge latency benchmarks
  - hybrid retrieval and path-comparison benchmarks
  - injection/ethics overhead benchmarks
  - delegate and end-to-end pipeline benchmarks
- Result:
  - CMake target registration is conditional on Google Benchmark being found and each source being present.
  - The release-gate mapping still requires reproducible baseline runs and matching cases before it can be treated as measured evidence.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
