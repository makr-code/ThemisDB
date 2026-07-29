# Evaluation Module - Build and Test Evidence

<!-- Status: current | validated: 2026-07-29 -->
<!-- Issue: #5643 (Development Status 2026-07-18) -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · README.md · PRODUCTION_REQUIREMENTS.md -->

## Evidence Summary

This document tracks the current evidence state for the evaluation module status issue
and records both the canonical issue snapshot and the current-environment validation
attempt performed during this refresh.

## Canonical Snapshot (from issue context, validated 2026-07-18)

- Preset: `windows-release`
- Focused target pattern: `module_evaluation_test_*_focused.exe`
- Result: focused module binary was not found in `build-msvc-windows-release/bin_out`
- Status: evidence gap documented in issue #5643

## Source-Verifiable Coverage (validated 2026-07-29)

### Runtime sources present

- `src/evaluation/src/hardware_profile.cc`
- `src/evaluation/src/benchmark_matrix.cc`
- `src/evaluation/src/retrieval_metrics.cc`
- `src/evaluation/src/ablation_framework.cc`
- `src/evaluation/src/approximation_rules.cc`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/src/artifact_lifecycle.cc`

### Focused / contract test surfaces present

- `tests/epic2_evaluation/hardware_profile_test.cc`
- `tests/epic2_evaluation/query_planner_test.cc`
- `tests/epic2_evaluation/benchmark_matrix_test.cc`
- `tests/epic2_evaluation/retrieval_metrics_test.cc`
- `tests/epic2_evaluation/ablation_framework_test.cc`
- `tests/epic2_evaluation/approximation_rules_test.cc`
- `tests/epic2_evaluation/artifact_lifecycle_test.cc`
- `tests/epic2_evaluation/test_query_planner_cache.cc`

Note: not every source file listed above is currently registered as a local executable
test target in `tests/epic2_evaluation/CMakeLists.txt`.

### Benchmark surfaces present

- `benchmarks/epic2_evaluation/planner_decision_bench.cc`
- `benchmarks/epic2_evaluation/benchmark_matrix_bench.cc`
- `benchmarks/epic2_evaluation/artifact_staleness_bench.cc`
- `benchmarks/epic2_evaluation/storage_strategy_bench.cc`

Note: current benchmark wiring is narrower than the full source list above; see the
registration section for the targets that are presently declared in CMake.

## Focused Test Registration Evidence (source-verifiable, validated 2026-07-29)

- Registration file: `tests/epic2_evaluation/CMakeLists.txt`
- Focused target naming rule:
  - `module_epic2_evaluation_${_stem}_focused`
- Currently registered focused source:
  - `hardware_profile_test.cc`
- Current standalone GTest executables registered from source:
  - `query_planner_test`
  - `approximation_rules_test`
  - `benchmark_matrix_test`
  - `retrieval_metrics_test`
  - `ablation_framework_test`
  - `test_query_planner_cache`

## Benchmark Registration Evidence (source-verifiable, validated 2026-07-29)

- Registration file: `benchmarks/epic2_evaluation/CMakeLists.txt`
- Current benchmark targets are gated by `THEMIS_BUILD_BENCHMARKS` and dependency availability
- Directly declared benchmark executables or wrappers:
  - `planner_decision_bench`
  - `benchmark_matrix_bench`
  - `bench_epic2_evaluation_storage_strategy_bench`

## Current Local Build/Test Attempt (2026-07-29)

- Command: `cmake --preset community-release`
- Result: failed during configure
  - vcpkg warning: local checkout not present
  - hard failure: RocksDB not found; install `librocksdb-dev` or provide vcpkg `rocksdb`
- Impact: no local evaluation-focused binaries could be generated in this environment during this validation pass

## CI / Workflow Context (2026-07-29)

- Recent non-module workflow failure reviewed via GitHub Actions logs:
  - workflow: `Supply Chain — SBOM Generation & Signing`
  - failure cause: unable to resolve `aquasecurity/trivy-action@0.28.0`
- Assessment: this failure is independent of the evaluation module status work and does not change the module evidence gap recorded above

## Status Assessment

- [x] Roadmap/future/audit/security/performance docs refreshed for issue #5643
- [x] Source/test/benchmark registration paths verified in source
- [~] Fresh executable build/test evidence remains blocked by local dependency setup
- [ ] New executable run evidence for focused module targets captured for this cycle
- [ ] Benchmark-backed guardrail evidence captured for this cycle

## Next Evidence Actions

1. Restore build prerequisites in the validation environment (`librocksdb-dev` or vcpkg `rocksdb`).
2. Configure and build evaluation-focused targets.
3. Execute focused tests and append pass/fail evidence with timestamp.
4. Run the relevant EPIC 2 benchmarks and record measured guardrails before Phase 6 closure.
