# EPIC 3.6 Distributed Retrieval

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Planner-facing integration of distributed tensor retrieval and fragment assembly.

## Scope

- Cross-shard fragment assembly and merge ordering
- Planner handoff from summary-first routing to artifact retrieval
- Fallback behavior when manifests or shards are incomplete

## Planned Repository Surfaces

- `src/distributed_tensor/include/distributed_planner.h`
- `src/distributed_tensor/src/distributed_planner.cc`
- `tests/epic3_distributed_tensor/distributed_planner_test.cc`
- `benchmarks/epic3_distributed_tensor/distributed_retrieval_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze distributed planner inputs, outputs, and merge semantics
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 2: Core implementation
- [ ] Document how federated summaries become shard-local retrieval actions
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate degraded routing for partial shard loss or incompatible artifacts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for fragment assembly and planner cost
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 5: Performance and hardening
- [ ] Keep planner vocabulary aligned with EPIC 1 federated summaries and EPIC 2 query planner docs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 6: Documentation and acceptance
- [ ] Keep planner vocabulary aligned with EPIC 1 federated summaries and EPIC 2 query planner docs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.6 Distributed Retrieval`.

### Phase 7: Integration
- [ ] Keep planner vocabulary aligned with EPIC 1 federated summaries and EPIC 2 query planner docs
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.6 Distributed Retrieval`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_FEDERATED_SUMMARIES.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
