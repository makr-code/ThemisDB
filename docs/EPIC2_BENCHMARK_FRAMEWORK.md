# EPIC 2.2 Benchmark Framework

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Benchmark matrix and scenario definitions for the layered stack.

## Scope

- Standard workloads for ANN, tensor, graph, and distributed retrieval
- Comparable metrics and artifact capture requirements
- Scenario taxonomy for correctness, latency, and cost trade-offs

## Planned Repository Surfaces

- `src/evaluation/include/benchmark_matrix.h`
- `src/evaluation/src/benchmark_matrix.cc`
- `tests/epic2_evaluation/benchmark_matrix_test.cc`
- `benchmarks/epic2_evaluation/benchmark_matrix_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define a stable scenario matrix before numbers are collected
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 2: Core implementation
- [ ] Document required inputs, outputs, and stored benchmark artifacts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate edge cases for incomplete datasets and incomparable hardware
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 4: Tests
- [ ] Reserve performance checks for orchestration overhead itself
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 5: Performance and hardening
- [ ] Keep benchmark ownership aligned with planner and metrics documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 6: Documentation and acceptance
- [ ] Keep benchmark ownership aligned with planner and metrics documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.2 Benchmark Framework`.

### Phase 7: Integration
- [ ] Keep benchmark ownership aligned with planner and metrics documents
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.2 Benchmark Framework`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `EVALUATION_FRAMEWORK.md`
- `docs/TESTING_STRATEGY.md`
- `benchmarks/ROADMAP.md`
