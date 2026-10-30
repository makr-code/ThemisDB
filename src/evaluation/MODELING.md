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

# Evaluation and Planner Modeling

AI provenance note: Exact model backend not verifiable from final document text alone.

## Retrieval quality model

For ranked IDs `r[0..k-1]` and unique ground-truth IDs `G`, the implementation
uses:

- Recall@k = relevant IDs in the first `k` results divided by `|G|`.
- Precision@k = relevant IDs in the first `k` results divided by `k`.
- NDCG@k = DCG normalized against the ideal ordering, with binary gain for
  membership in `G`.
- MRR = reciprocal rank of the first relevant result (zero if none appears).
- Candidate reduction = `1 - k / total_candidates` when a nonzero candidate
  count is supplied.

This metric contract expects a non-empty ground truth, `1 <= k <= ranked.size()`,
and unique IDs in both lists. It rejects invalid data rather than silently
clamping the cutoff or accepting duplicates. Consequently, compare experiments
only when their query set, judgments, cutoff, and candidate-count definition
match.

## Query planner model

The planner is a typed policy selector, not a learned ranking model. It consumes
`ExecutionEligibility`, `TensorArtifactFreshness`, and `PlannerConfig`; applies
eligibility, staleness, rank-cap, delta-lag, exactness, and shard-manifest gates;
then emits an `ExecutionPath` and explicit fallback reason.

The five canonical paths documented in `src/evaluation/src/query_planner.cc` are
ANN-only, ANN plus tensor summary, ANN plus tensor plus exact graph validation,
direct exact graph, and distributed summary-first with exact-on-demand.
Tensor output is advisory; exact graph validation remains on paths that require
truth-bearing results. Category-C work must not be routed to a GPU path. This
policy prioritizes explainable downgrade behavior over choosing a path from
learned latency or quality predictions.

## Benchmark model and interpretation

Benchmark inputs should vary one routing condition at a time (for example
fresh versus stale artifact, force-exact, unavailable hardware, or distributed
manifest state). Measure `selectPath()` independently from downstream
execution; otherwise the result mixes policy overhead with retrieval, network,
and storage costs. Report p50/p95/p99, path counts, and fallback-reason counts
with hardware and build details.

`BenchmarkMatrix` records scenario/dimension values and invalidates by either
key. Its unit tests use hand-constructed values and verify storage semantics;
they are not latency baselines. The repository's baseline tables are not fully
populated, so numerical targets in performance documentation remain guardrails
until backed by retained benchmark output.

## References and verification

- [Research and reproducibility notes](./RESEARCH.md)
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/src/retrieval_metrics.cc`
- `src/evaluation/include/query_planner.h`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/include/benchmark_matrix.h`
- `src/evaluation/src/benchmark_matrix.cc`
- `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`
- `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`
- `benchmarks/epic2_evaluation/planner_decision_bench.cc`
- `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`
