# Evaluation Module Roadmap

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

<!-- Source level: level1 · SOT domain: module-behavior -->
<!-- Last Updated: 2026-10-31 · Issue: #7344 · Milestone: DOC-WEEKLY-2026-44 -->

## Current Status

This is a source/CMake status review for issue #7344, not a release acceptance
record. The module source tree contains implementations for benchmark-matrix
handling, hardware profiles, retrieval metrics, ablation, approximation rules,
query-path selection and artifact lifecycle metadata. CMake declares
`themis_evaluation` for five sources, plus conditional query-planner,
approximation and lifecycle targets. There is no `storage_strategy` header,
source or module-library target in the `src/evaluation` inventory checked for
this update. The `StorageStrategyAssessor` implementation is in
`src/distributed_tensor/src/tensor_storage_strategy.cc`, built by
`themis_distributed_tensor`; its focused test source is included by the
distributed-tensor test registration. It is not an evaluation-module target.

The public-path `include/evaluation/evaluation.h` currently contains no
component declarations. Detailed paths and target ownership are in
[ARCHITECTURE.md](./ARCHITECTURE.md) and [IMPLEMENTATION.md](./IMPLEMENTATION.md).

### Source and execution evidence

- `tests/evaluation/CMakeLists.txt` globs five local test sources into focused
  targets when included in a configured build.
- `tests/epic2_evaluation/CMakeLists.txt` contains additional EPIC 2 test
  registrations guarded by library and dependency checks.
- `benchmarks/evaluation/CMakeLists.txt` discovers a local stub-based benchmark
  source when benchmarks are enabled. Separate EPIC 2 registrations are in
  `benchmarks/epic2_evaluation/CMakeLists.txt`: planner decision and error-path
  executables require `epic2_evaluation_lib`; the matrix executable requires
  Google Benchmark and `themis_evaluation`; the storage-strategy source is
  passed to the standard benchmark helper. `artifact_staleness_bench.cc` is not
  registered there. Planner cases measure in-process `selectPath()` calls and
  report latency percentiles; matrix cases include synthetic workload
  simulations; artifact staleness exercises `ArtifactLifecycleManager` through
  Google Benchmark without a registered target; the storage benchmark calls
  `DefaultShardPlacementStrategy` with synthetic node inputs, not
  `StorageStrategyAssessor`.
- Source registrations and benchmark output capabilities do not establish that
  targets were built, tests passed, or benchmark results were captured. No
  build, test or benchmark was run for this documentation-only update.

### Root aggregate status and evidence

Root `ROADMAP.md` now records the related implementation locations without
claiming a verified mapping from these APIs to item 2.7, and does not claim
EPIC 2 acceptance as complete. The storage assessor and its focused-test
registration are owned by `distributed_tensor`; the EPIC 2 benchmark is a
separate synthetic placement harness. Neither registration proves execution.
The aggregate 220+ passing-test count and completed benchmark evidence remain
unverified. This documentation update does not close or reclassify issue #5643.

## In Progress

- [ ] Record build/test/benchmark execution results only after running the
  applicable targets in a configured environment. Keep registration separate
  from execution evidence.
- [x] Identify the storage-strategy implementation and focused-test registration
  as `distributed_tensor`-owned; do not describe them as evaluation-module
  targets.
- [ ] Verify downstream workflow integration only against actual consumers and
  accepted module gates.

## Planned Features

No new runtime feature or delivery commitment is established by this
documentation-only update. The following remain candidates pending ownership,
scope and acceptance decisions:

- Confirm whether the EPIC 2 benchmark is intended to measure
  `StorageStrategyAssessor` or the separate `DefaultShardPlacementStrategy`;
  align its name and evidence with the intended contract.
- Assess downstream workflow integration after verifying consumers and their
  required contracts.

## Implementation Phases

The checked boxes below record documentation/source inventory work only; they
do not mean a release gate passed.

### Phase 1

**Design and API contract**

- [x] Identify component declarations under `src/evaluation/include/`.
- [ ] Define the intended public umbrella contract for the currently empty
  `include/evaluation/evaluation.h` and align its consumers/documentation.

### Phase 2

**Core implementation**

- [x] Inventory the seven implementation files and their CMake target ownership.
- [x] Identify the separate storage-assessor interface, implementation, library
  target and focused-test registration under `distributed_tensor`.

### Phase 3

**Errors and edge cases**

- [x] Confirm source representations for typed metric errors, planner fallback
  reasons, approximation decisions and lifecycle states.
- [ ] Complete any required correctness/security review; this documentation
  inventory is not an exhaustive audit.

### Phase 4

**Tests**

- [x] Distinguish local focused-test registrations from conditional EPIC 2
  registrations.
- [ ] Establish current build and test execution results in a configured
  environment.

### Phase 5

**Performance**

- [x] Distinguish registered planner/matrix/storage targets, the unregistered
  artifact-staleness source, synthetic simulation cases and source-level output
  capability.
- [ ] Capture measured results from applicable implementation benchmarks and
  retain the run environment; do not treat stub or synthetic simulation output
  as production-path evidence.

### Phase 6

**Documentation and acceptance**

- [x] Synchronize the scoped module documentation for issue #7344 while
  preserving dated acceptance and sign-off records as historical.
- [x] Align the root maturity summary with the cross-module source and CMake
  ownership.
- [ ] Obtain current human-reviewed build, test and benchmark acceptance
  evidence before asserting module readiness.

## Production Readiness Checklist

- [x] Resolve the root roadmap versus module-source ownership discrepancy.
- [ ] Establish current aggregate test and benchmark execution evidence.
- [ ] Establish current build and focused-test results in an available,
  configured environment.
- [ ] Capture and review benchmark results with hardware/build context; do not
  use stub or synthetic simulation results as module baselines.
- [ ] Confirm intended ownership and delivery status of storage strategy.
- [ ] Verify downstream integration and acceptance gates before describing
  production readiness.

## Known Issues & Limitations

- Root `ROADMAP.md` now distinguishes cross-module implementation presence from
  aggregate EPIC 2 acceptance; current execution evidence is still missing.
- `include/evaluation/evaluation.h` is an empty umbrella, while component
  declarations are under `src/evaluation/include/`.
- `StorageStrategyAssessor` is owned by `distributed_tensor`, not this module.
  The separately registered EPIC 2 benchmark calls
  `DefaultShardPlacementStrategy` with synthetic nodes and does not establish
  measured `StorageStrategyAssessor` performance.
- Conditional CMake registrations do not prove execution. The local evaluation
  benchmark uses stubs; the EPIC 2 matrix simulations are synthetic, and
  artifact-staleness benchmark source is not registered.

## Breaking Changes

This documentation-only update changes no API or runtime behavior and
introduces no breaking change. Any future contract or source change must be
assessed separately and recorded in the module changelog.

## Historical status records

`MODULE_EVIDENCE.md`, `JUSTIFIED_GAP.md`, the Phase 4-6 checklists and the
Phase 6 sign-off template contain dated records associated with issue #5643.
Their recorded dates, blockers, phase marks and sign-off text are preserved as
historical material. They are not updated by this source audit and must not be
read as current build results or current issue state.

## References

- [Module overview](./README.md)
- [Architecture and source map](./ARCHITECTURE.md)
- [Implementation and CMake mapping](./IMPLEMENTATION.md)
- [Build/test/benchmark evidence](./MODULE_EVIDENCE.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)
- `ROADMAP.md` (root maturity aggregate; reconciliation required)
- `src/evaluation/CMakeLists.txt`
- `tests/evaluation/CMakeLists.txt`
- `tests/epic2_evaluation/CMakeLists.txt`
- `benchmarks/evaluation/CMakeLists.txt`
- `benchmarks/epic2_evaluation/CMakeLists.txt`
