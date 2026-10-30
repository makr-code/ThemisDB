---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Evaluation Notes

AI provenance: Exact model backend not verifiable from final document text alone.

## Purpose and boundary

This document describes how the evaluation module can measure retrieval paths
that use embeddings or tensor summaries. It is for engineers designing evaluation
sets and interpreting results. The module does not generate embeddings, select an
embedding model, or calculate vector similarity; those operations belong to the
retrieval and model components that supply the ranked results and measurements.

## What this module measures

`computeRetrievalQuality()` evaluates an already-ranked list against unique
ground-truth IDs. It treats each ground-truth match as binary relevance and
returns Recall@k, Precision@k, binary-gain NDCG@k, MRR, and (when a valid
candidate count is provided) candidate reduction. It does not evaluate vector
distance calibration, semantic similarity, or graded relevance.

`TensorGraphSnapshot` and `computeTensorGraphRuntimeMetrics()` describe
runtime evidence relevant to embedding-backed paths: artifact age, delta lag,
residual error, rank use, rebuilds, exact fallbacks, summary-first false
negatives, and graph-verified finalization. A metric summary is descriptive; it
does not authorize an approximate result or replace exact graph verification.

`BenchmarkScenario` includes ANN-only, ANN-plus-tensor, ANN-plus-tensor-plus-graph,
distributed summary-first, full-prompt, and tensor-compressed LLM scenarios.
`BenchmarkDimension` includes retrieval quality, latency, throughput, memory,
compression, answer-grounding, and distributed-efficiency dimensions. These
types identify comparison cells; they do not supply a corpus or run the
retrieval/model path.

## Evaluation protocol

1. Freeze a versioned query set and gold labels. Keep query text, relevant IDs,
   cutoff `k`, and candidate-count definition consistent across variants.
2. Record the embedding model and version, preprocessing, index parameters,
   corpus snapshot, hardware, and build configuration with the experiment
   artifacts. `BenchmarkEntry` carries dataset, hardware, and runner tags; it
   has no dedicated embedding-model field, so preserve model metadata alongside
   the matrix.
3. Compare the same query and corpus inputs across ANN, tensor-assisted, and
   exact-graph paths. Keep exact graph results as the correctness reference when
   the query is truth-bearing.
4. Report quality metrics separately from `selectPath()` decision cost and
   end-to-end retrieval latency. Retain the selected-path and fallback-reason
   counts, along with p50/p95/p99 and the environment.
5. Treat empty labels, duplicate IDs, invalid cutoffs, or non-finite/out-of-range
   metric inputs as invalid evaluation data; fix the fixture rather than
   suppressing the validation error.

## Limits and interpretation

- Binary labels do not model graded relevance. If graded judgments are required,
  this API is not sufficient without a separately specified metric contract.
- Synthetic or hand-built fixtures validate API behavior, not embedding quality
  on a representative corpus.
- A tensor summary is advisory; it is not the final truth-bearing result.
- `BASELINES.md` and performance guardrails are not measured results unless
  accompanied by retained benchmark output for the stated environment.

## References

- [Research and reproducibility](./RESEARCH.md)
- [Modeling assumptions and metrics](./MODELING.md)
- [Evaluation API](./API.md)
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/include/benchmark_matrix.h`
- `src/evaluation/include/query_planner.h`
- `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`
- `benchmarks/epic2_evaluation/README.md`
