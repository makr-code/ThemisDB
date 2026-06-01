# EPIC 2.5 Query Planner

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Hybrid query planner contract for ANN, tensor, graph, and distributed retrieval flows.

## Scope

- Routing decisions across retrieval stages and deployment topologies
- Use of hardware profile, lifecycle state, and approximation rules
- Explainability and observability for planner choices

## Planned Repository Surfaces

- `src/evaluation/include/query_planner.h`
- `src/evaluation/src/query_planner.cc`
- `tests/epic2_evaluation/query_planner_test.cc`
- `benchmarks/epic2_evaluation/planner_decision_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze planner inputs, outputs, and explanation schema
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 2: Core implementation
- [ ] Document deterministic fallback order across layered retrieval
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate degraded routing for missing hardware, stale artifacts, or integrity failures
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 4: Tests
- [ ] Reserve tests for deterministic plans and cross-epic integration
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 5: Performance and hardening
- [ ] Keep planner docs synchronized with approximation and distributed retrieval work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 6: Documentation and acceptance
- [ ] Keep planner docs synchronized with approximation and distributed retrieval work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 7: Integration
- [ ] Keep planner docs synchronized with approximation and distributed retrieval work
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.5 Query Planner`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
