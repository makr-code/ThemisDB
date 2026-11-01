---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI ChatGPT"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

AI provenance: Exact model backend not verifiable from final document text alone.

# Evaluation module tests

These are module-local focused test sources, not an end-to-end retrieval test
suite. `CMakeLists.txt` globs `test_*.cpp` in this directory and creates a
focused executable and CTest registration for each source when this directory is
included in a configured build. For source stem `<stem>`, the generated names
are `module_evaluation_<stem>_focused` and
`<stem>_EvaluationFocusedTests`, respectively.

| Test source | What the source exercises |
|---|---|
| `test_evaluation_retrieval_metrics_focused.cpp` | Retrieval-quality metrics and `MetricCollector` behavior, including invalid input cases |
| `test_evaluation_ablation_framework_focused.cpp` | Ablation registration, execution and report behavior |
| `test_evaluation_benchmark_matrix_focused.cpp` | Matrix record, lookup, replacement, invalidation and clearing |
| `test_evaluation_artifact_lifecycle_focused.cpp` | Lifecycle state and transition behavior |
| `test_evaluation_highcardinality_stress.cpp` | In-process stub stress cases; it does not exercise the evaluation implementation |

The local CMake file adds implementation sources explicitly for the retrieval
metrics, ablation, benchmark-matrix and artifact-lifecycle focused tests. The
other globbed test sources have no additional implementation source in the
CMake mapping; the high-cardinality stress file uses local stubs. These
registrations do not prove that targets were built or tests passed.

The separate `tests/epic2_evaluation/CMakeLists.txt` registers additional
EPIC 2 contract tests behind target and dependency checks. See
[`../../src/evaluation/IMPLEMENTATION.md`](../../src/evaluation/IMPLEMENTATION.md)
for the source/build map. No tests were run as part of the documentation-only
refresh.
