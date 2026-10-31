# Evaluation Module Architecture

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

<!-- Status: source-audited 2026-10-31; build/runtime integration not verified -->
<!-- Last Updated: 2026-10-31 · Issue: #7344 · Milestone: DOC-WEEKLY-2026-44 -->

## Purpose and boundary

`src/evaluation` contains C++ contracts and implementations for metrics,
benchmark-result bookkeeping, hardware profiles, ablation, approximation rules,
query-path selection and artifact lifecycle metadata. The functions consume
caller-supplied values. The planner selects a path; it does not perform
retrieval, hardware discovery, persistence or artifact rebuild work.

## Component and build map

| Component | Header | Implementation | CMake ownership |
|---|---|---|---|
| Benchmark matrix | `include/benchmark_matrix.h` | `src/benchmark_matrix.cc` | `themis_evaluation` |
| Hardware profiles | `include/hardware_profile.h` | `src/hardware_profile.cc` | `themis_evaluation` |
| Retrieval metrics | `include/retrieval_metrics.h` | `src/retrieval_metrics.cc` | `themis_evaluation` |
| Ablation | `include/ablation_framework.h` | `src/ablation_framework.cc` | `themis_evaluation` |
| Artifact lifecycle | `include/artifact_lifecycle.h` | `src/artifact_lifecycle.cc` | `themis_evaluation`; also a conditional EPIC 2.6 library |
| Query planner | `include/query_planner.h` | `src/query_planner.cc` | Separate conditional `epic2_evaluation_lib` |
| Approximation rules | `include/approximation_rules.h` | `src/approximation_rules.cc` | Separate conditional `epic2_approximation_lib` |

`src/evaluation/CMakeLists.txt` lists seven contract headers but only five
sources in `themis_evaluation`. The query planner, approximation, and second
artifact-lifecycle targets have separate conditional declarations. No
`storage_strategy` header or source appears in the module inventory checked for
this update.

The public-path file `include/evaluation/evaluation.h` currently contains no
component includes or declarations. The component headers live under
`src/evaluation/include/`; do not infer API exposure or linkage from the
umbrella-header path alone.

## Data flow and failure boundaries

1. A caller supplies ranked results and labels to metric functions; invalid
   inputs can raise `MetricError`.
2. A caller supplies benchmark values to `BenchmarkMatrix`; absent cells are
   reported as `nullopt`, and a clean zero-sample insert is rejected.
3. A caller supplies eligibility, freshness and planner policy to
   `QueryPlanner::selectPath()`. The returned `PlannerDecision` describes a
   selection and fallback reason, not an executed retrieval result.
4. A caller supplies lifecycle metadata and thresholds to
   `ArtifactLifecycleManager`; methods return calculated state or updated
   metadata. Background work and persistence are outside this component.
5. Approximation policy checks return a typed decision that the caller must
   interpret; they do not enforce authorization on their own.

## Non-goals and integration limits

- No standalone service, runtime configuration endpoint, telemetry exporter or
  database access is defined by these module APIs.
- No learned planner cost model or storage-strategy implementation is present in
  the reviewed source inventory.
- Local test and benchmark registrations do not prove execution or downstream
  production integration.

## Cross-module ownership and aggregate evidence

EPIC 2 item 2.7 is the quantization/mmap/zero-copy assessment defined by
sub-issue #5443 of parent EPIC #5436. Its storage-strategy implementation is
not part of the evaluation module:
`StorageStrategyAssessor` is declared in
`include/distributed_tensor/tensor_storage_strategy.h`, implemented in
`src/distributed_tensor/src/tensor_storage_strategy.cc`, and included in the
`themis_distributed_tensor` library. Its test source is registered through the
focused-test glob in `tests/epic3_distributed_tensor/CMakeLists.txt`.

The EPIC 2 `storage_strategy_bench` is a distinct harness that calls
`DefaultShardPlacementStrategy` with synthetic node inputs; it does not measure
`StorageStrategyAssessor`. Root `ROADMAP.md` records this mapping and the
implementation locations. The CI run attached to merged PR #5606 failed during
configure and skipped build/test; no benchmark result artifact was found.
Source and CMake registrations do not prove 220+ passing tests or completed
benchmark runs.

## References

- [Module overview](./README.md)
- [Implementation and targets](./IMPLEMENTATION.md)
- [API contracts](./API.md)
- [Current roadmap and unresolved evidence](./ROADMAP.md)
- `src/evaluation/CMakeLists.txt`
- `tests/evaluation/CMakeLists.txt`
- `tests/epic2_evaluation/CMakeLists.txt`
- `benchmarks/evaluation/CMakeLists.txt`
- `benchmarks/epic2_evaluation/CMakeLists.txt`
