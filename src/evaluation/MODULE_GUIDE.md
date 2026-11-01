---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; OpenAI ChatGPT/themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Evaluation Module Developer Guide

AI provenance: Exact model backend not verifiable from final document text alone.

## Purpose and entry points

The evaluation module contains contracts and implementations for retrieval
quality metrics, benchmark result organization, hardware profiles, approximation
policy, query-path decisions, and derived-artifact lifecycle state. Use this
guide to find the implementation and test surface for a change; use the headers
for API details and `ROADMAP.md` / `MODULE_EVIDENCE.md` for current delivery and
validation status.

## Where to start

| Change area | Read first | Registered test source/target |
|---|---|---|
| Retrieval quality and evidence metrics | `src/evaluation/include/retrieval_metrics.h`, `src/evaluation/src/retrieval_metrics.cc` | `tests/evaluation/test_evaluation_retrieval_metrics_focused.cpp`; local focused target generated from filename |
| Benchmark matrix | `src/evaluation/include/benchmark_matrix.h`, `src/evaluation/src/benchmark_matrix.cc` | `tests/evaluation/test_evaluation_benchmark_matrix_focused.cpp`; also `tests/epic2_evaluation/benchmark_matrix_test.cc` |
| Hardware profiles | `src/evaluation/include/hardware_profile.h`, `src/evaluation/src/hardware_profile.cc` | `tests/epic2_evaluation/hardware_profile_test.cc`; conditional module-focused target |
| Ablation metrics | `src/evaluation/include/ablation_framework.h`, `src/evaluation/src/ablation_framework.cc` | `tests/evaluation/test_evaluation_ablation_framework_focused.cpp`; also `tests/epic2_evaluation/ablation_framework_test.cc` |
| Planner routing | `src/evaluation/include/query_planner.h`, `src/evaluation/src/query_planner.cc` | `tests/epic2_evaluation/query_planner_test.cc`; conditional on planner library and GTest |
| Approximation boundaries | `src/evaluation/include/approximation_rules.h`, `src/evaluation/src/approximation_rules.cc` | `tests/epic2_evaluation/approximation_rules_test.cc`; conditional on the approximation library and GTest |
| Artifact freshness/lifecycle | `src/evaluation/include/artifact_lifecycle.h`, `src/evaluation/src/artifact_lifecycle.cc` | `tests/evaluation/test_evaluation_artifact_lifecycle_focused.cpp`; also `tests/epic2_evaluation/artifact_lifecycle_test.cc` |

Target and CTest names can differ slightly from source filenames; consult the
two test CMake files before assuming an executable is registered. Optional
targets may be absent when GTest or the relevant module option is unavailable.
The files in `tests/evaluation/` are discovered by a filename glob; the
EPIC 2 tests have individual dependency and target checks. Neither registration
proves that a test was built or executed. The public-path
`include/evaluation/evaluation.h` currently does not include component headers.

## Safe integration sequence

1. Define the evaluation input, reference labels, and failure semantics before
   changing a metric or policy.
2. Update the smallest relevant public contract and implementation. Keep API
   ownership clear: headers are in `include/`, implementation is in `src/`.
3. Add deterministic tests for valid behavior and boundary failures. Metric
   errors, planner downgrade reasons, and lifecycle states are part of the
   contract and should be asserted directly.
4. Build and run the focused target. Record unavailable targets or environment
   blockers as blockers; do not report source presence as a passing test.
5. For performance claims, run the matching benchmark under a recorded build,
   hardware, data, and configuration. Store results and current limitations in
   `MODULE_EVIDENCE.md`.
6. Preserve the governance rule that default-workflow integration is not
   enabled until required tests, benchmark evidence, and acceptance gates are
   satisfied.

## Common edge cases

- Reject empty reference labels, duplicate identifiers, invalid `k`, and
  non-finite or out-of-range metric inputs according to `MetricError`.
- Keep stale, missing, or rebuilding tensor artifacts distinguishable through
  fallback reasons; do not claim a tensor summary is ground truth.
- Treat missing shard manifests as an exact-path fallback condition.
- Do not route Category C work to GPU or an approximate-only path.
- Handle empty/missing benchmark cells as unavailable evidence rather than
  fabricating a baseline.

## References

- [API contracts](./API.md)
- [Implementation guide](./IMPLEMENTATION.md)
- [Architecture boundaries](./ARCHITECTURE.md)
- [Testing and evidence](./MODULE_EVIDENCE.md)
- [Benchmark methodology](./MODELING.md)
- `src/evaluation/include/README.md`
- `src/evaluation/src/README.md`
