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

| Change area | Read first | Focused verification |
|---|---|---|
| Retrieval quality or evidence metrics | `include/retrieval_metrics.h`, `src/retrieval_metrics.cc` | `test_evaluation_retrieval_metrics_focused` |
| Benchmark matrix | `include/benchmark_matrix.h`, `src/benchmark_matrix.cc` | `test_evaluation_benchmark_matrix_focused` |
| Hardware profiles | `include/hardware_profile.h`, `src/hardware_profile.cc` | `hardware_profile_test` |
| Ablation metrics | `include/ablation_framework.h`, `src/ablation_framework.cc` | `test_evaluation_ablation_framework_focused` |
| Planner routing | `include/query_planner.h`, `src/query_planner.cc` | `query_planner_test` |
| Approximation boundaries | `include/approximation_rules.h`, `src/approximation_rules.cc` | `approximation_rules_test` |
| Artifact freshness/lifecycle | `include/artifact_lifecycle.h`, `src/artifact_lifecycle.cc` | `test_evaluation_artifact_lifecycle_focused` |

Target and CTest names can differ slightly from source filenames; consult the
two test CMake files before assuming an executable is registered. Optional
targets may be absent when GTest or the relevant module option is unavailable.

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
