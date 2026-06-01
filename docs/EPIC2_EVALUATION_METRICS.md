# EPIC 2.3 Evaluation Metrics

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Metrics and ablation framework for layered retrieval quality and cost.

## Scope

- Correctness, provenance, compression, latency, and resource metrics
- Ablation plans for removing ANN, tensor, graph, or distributed stages
- Planner- and governance-facing acceptance thresholds

## Planned Repository Surfaces

- `src/evaluation/include/evaluation_metrics.h`
- `src/evaluation/src/evaluation_metrics.cc`
- `tests/epic2_evaluation/evaluation_metrics_test.cc`
- `benchmarks/epic2_evaluation/metrics_computation_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze the core metric vocabulary and required dimensions
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 2: Core implementation
- [ ] Document which metrics require ground truth versus online estimates
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate edge cases for partial evidence, missing labels, or mixed topologies
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 4: Tests
- [ ] Reserve ablation and benchmark hooks for every retrieval stage
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 5: Performance and hardening
- [ ] Keep metric outputs compatible with planner explanations and ADRs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 6: Documentation and acceptance
- [ ] Keep metric outputs compatible with planner explanations and ADRs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.3 Evaluation Metrics`.

### Phase 7: Integration
- [ ] Keep metric outputs compatible with planner explanations and ADRs
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.3 Evaluation Metrics`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `EVALUATION_FRAMEWORK.md`
- `docs/EPIC1_GRAPH_VALIDATION.md`
- `docs/EPIC2_QUERY_PLANNER.md`
