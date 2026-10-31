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

# Evaluation Implementation Guide

AI provenance: Exact model backend not verifiable from final document text alone.

## Scope and build boundaries

The evaluation sources and their build ownership are defined by
`src/evaluation/CMakeLists.txt`. The `themis_evaluation` static library compiles
the benchmark matrix, hardware profile, retrieval metrics, ablation, and artifact
lifecycle sources. Query planning and approximation rules are separate libraries,
`epic2_evaluation_lib` and `epic2_approximation_lib`, and are conditionally
declared when GTest or `THEMIS_BUILD_EPIC2` is available. Do not assume one
target exposes every header in the module.

The library target advertises C++17 and declares no external library dependency.
Test executables have additional build prerequisites; their registration is
conditional. Confirm that the target exists in the configured build before
relying on a test or optional library.

The public-path `include/evaluation/evaluation.h` currently includes no
component declarations. Component headers listed by the module CMake file are
installed to `include/evaluation` only as part of the relevant install step;
header-path presence alone does not establish that the matching implementation
is linked.

## Implementation responsibilities

| Surface | Implementation responsibility | Contract to preserve |
|---|---|---|
| `hardware_profile.cc` | Built-in profiles, validation, lookup, activation, tier-transition checks | Return validation errors/warnings; do not silently accept an invalid profile or tier move |
| `benchmark_matrix.cc` | Scenario/dimension results and comparison operations | Preserve result edge flags and sample counts; distinguish absent cells from recorded values |
| `retrieval_metrics.cc` | Stateless metric calculations | Reject invalid data with `MetricError`; avoid silent NaN or division-by-zero behavior |
| `ablation_framework.cc` | Ablation evaluation support | Keep its input and result contracts aligned with focused tests |
| `query_planner.cc` | Policy-based execution-path selection | Return a typed path and reason; exact graph is the safe fallback; do not treat tensor output as truth |
| `approximation_rules.cc` | Zone, exactness, and policy-boundary decisions | Deny or escalate unsafe truth-bearing and Category C requests; retain policy-version context |
| `artifact_lifecycle.cc` | Lifecycle state computation and metadata transitions | Make stale, invalidated, rebuilding, and failed states distinguishable |

`src/evaluation/CMakeLists.txt` has no storage-strategy source or target. The
separately located `benchmarks/epic2_evaluation/artifact_staleness_bench.cc`
exists, but `benchmarks/epic2_evaluation/CMakeLists.txt` does not register it.
That directory declares planner-decision and planner-error-path executables
when `epic2_evaluation_lib` is available, a matrix target when Google Benchmark
and `themis_evaluation` are available, and passes the storage-strategy source
to the standard benchmark helper (with its CTest wrapper if the target exists).
The unregistered artifact-staleness source exercises `ArtifactLifecycleManager`
through Google Benchmark. The planner harnesses time in-process `selectPath()`
calls and report latency percentiles; the matrix source includes explicitly
synthetic workload simulations; and the storage harness calls
`DefaultShardPlacementStrategy` with synthetic node inputs. These target
declarations and output capabilities are not execution evidence.

`query_planner.h` documents five path identifiers: ANN-only, ANN with tensor
summary, ANN with tensor and exact graph validation, direct exact graph, and
distributed summary-first with exact-on-demand. `selectPath()` itself chooses
among paths from its eligibility, freshness, and configuration inputs; it does
not execute retrieval or rebuild artifacts.

## Invariants for changes

- Keep exactness and fallback outcomes explicit through `ExecutionPath`,
  `FallbackReason`, `GovernanceDecision`, and `ExactnessViolation`; do not
  introduce a silent approximate fallback.
- Category C work (including policy, provenance, and transactions) must not use
  a GPU path. Tensor artifacts remain advisory and graph validation remains
  required for truth-bearing results.
- Validate metric inputs at the public computation boundary. Preserve
  `MetricErrorKind` so callers can distinguish invalid input classes.
- `BenchmarkMatrix` is keyed by scenario and dimension. A scenario/dimension
  value is not a benchmark run or proof of a performance target.
- The `PlannerObserver*` passed to the planner factory is non-owning. Keep it
  alive for the planner lifetime; its callback must be non-throwing and
  non-reentrant. Concurrent planner calls also require a thread-safe observer.
- `MetricCollector` is not thread-safe; synchronize concurrent access in the
  caller.

## Validation workflow

Build and run the focused target for the changed contract. For example, with
module tests enabled:

```bash
cmake --build <build-dir> --target module_evaluation_test_evaluation_retrieval_metrics_focused
ctest --test-dir <build-dir> -R '^test_evaluation_retrieval_metrics_focused_EvaluationFocusedTests$' --output-on-failure
```

For planner changes, use the `query_planner_test` target when the EPIC 2 build
and GTest prerequisites are available. For approximation changes, run
`approximation_rules_test`. Check `tests/evaluation/CMakeLists.txt` and
`tests/epic2_evaluation/CMakeLists.txt` for the actual registration conditions
and names; source files alone do not prove a test ran.

Before asserting performance, run the matching benchmark target, retain its
output and environment details, and update the evidence in
`MODULE_EVIDENCE.md`. Keep this document subordinate to the module headers and
implementation when descriptions diverge.

## References

- [API contracts](./API.md)
- [Module guide](./MODULE_GUIDE.md)
- [Architecture](./ARCHITECTURE.md)
- [Roadmap and evidence state](./ROADMAP.md)
- `src/evaluation/CMakeLists.txt`
- `tests/evaluation/CMakeLists.txt`
- `tests/epic2_evaluation/CMakeLists.txt`
