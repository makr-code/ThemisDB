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

# Evaluation Module Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and scope

`src/evaluation` contains reusable retrieval metrics and the EPIC 2 query-planning
and benchmark-evidence surfaces. This note describes what current source and
tests establish; it does not claim that a planner route improves relevance or
that a performance gate has passed.

## Methods under test

`computeRetrievalQuality()` accepts a ranked result list, a set of relevant IDs,
and cutoff `k`. It computes Recall@k, Precision@k, binary-gain NDCG@k, MRR, and
candidate reduction when total candidate count is supplied. Empty ground truth,
`k=0`, `k` larger than the ranking, and duplicate IDs are rejected with
`MetricError`. Since this contract uses binary relevant/not-relevant IDs, it
does not preserve graded relevance labels.

`DefaultQueryPlanner::selectPath()` evaluates typed execution eligibility,
tensor artifact freshness, resource and category constraints, and distributed
manifest availability. It selects among five paths, from ANN-only through
tensor-assisted and exact-graph validation to distributed summary-first with
exact-on-demand. Decisions carry a `FallbackReason`; unsafe or stale eligibility
can downgrade to the exact path rather than silently selecting an approximate
route.

`BenchmarkMatrix` stores results by scenario and metric dimension. It is a
record/lookup and invalidation contract, not itself a benchmark runner or a
source of measured performance.

## Evidence and limitations

- `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp` checks
  perfect, partial, and zero recall; MRR rank behavior; invalid inputs; and
  candidate-reduction semantics.
- `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp` checks
  record/lookup, overwrite, and invalidation behavior.
- `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp` has dedicated
  microbenchmarks for aggregation, harness invocation, and a stubbed judge
  request. It does not measure retrieval quality or an external judge model.
- Planner selection benchmarks live under `benchmarks/epic2_evaluation/`.
  `src/evaluation/BASELINES.md` contains unfilled baseline tables; declared
  thresholds must not be reported as captured results.
- Focused unit fixtures verify contract behavior, not end-to-end search quality
  on a corpus. Relevance results require a versioned query set and gold labels.

## Reproduction path

After configuring a build with module tests enabled, run:

```bash
cmake --build <build-dir> --target module_evaluation_test_evaluation_retrieval_metrics_focused module_evaluation_test_evaluation_benchmark_matrix_focused
ctest --test-dir <build-dir> -R 'test_evaluation_(retrieval_metrics|benchmark_matrix)_focused_EvaluationFocusedTests' --output-on-failure
```

For microbenchmarks, configure with `THEMIS_BUILD_BENCHMARKS=ON`, build
`bench_evaluation_dedicated_gates`, and retain the full benchmark output,
compiler/build mode, hardware, and run configuration with any result. For
planner measurements use the matching target in
`benchmarks/epic2_evaluation/README.md`. Separate planner-only decision cost
from downstream retrieval latency, and record the selected-path distribution
and fallback reasons as well as percentiles.

## References

- [Modeling notes](./MODELING.md)
- [Architecture](./ARCHITECTURE.md)
- [Roadmap and evidence state](./ROADMAP.md)
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/src/retrieval_metrics.cc`
- `src/evaluation/include/query_planner.h`
- `src/evaluation/src/query_planner.cc`
