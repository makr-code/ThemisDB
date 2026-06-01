# EPIC 1.3 Graph Validation

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Graph truth layer and evidence assembly contract.

## Scope

- Evidence bundle model for retrieval verification
- Provenance and confidence propagation
- Failure-close rules when graph checks reject ANN or tensor candidates

## Planned Repository Surfaces

- `src/retrieval/include/graph_validator.h`
- `src/retrieval/src/graph_validator.cc`
- `tests/epic1_retrieval/graph_validator_test.cc`
- `benchmarks/epic1_retrieval/graph_validation_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define evidence bundle inputs and outputs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 2: Core implementation
- [ ] Specify provenance retention across graph traversals and merges
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 3: Error handling and edge cases
- [ ] Document boundary between validation and ranking
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 4: Tests
- [ ] Enumerate edge cases for contradictory evidence and incomplete graph state
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 5: Performance and hardening
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 6: Documentation and acceptance
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 7: Integration
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.3 Graph Validation`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `TARGET_ARCHITECTURE.md`
- `docs/EPIC2_EVALUATION_METRICS.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
