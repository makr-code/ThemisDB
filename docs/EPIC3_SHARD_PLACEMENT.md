# EPIC 3.3 Shard Placement

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Factorization-aware placement strategy for distributed tensor artifacts.

## Scope

- Placement heuristics by artifact size, factorization, and access pattern
- Interaction with hardware profile and network topology
- Planner hints for cross-shard retrieval cost

## Planned Repository Surfaces

- `src/distributed_tensor/include/shard_placement.h`
- `src/distributed_tensor/src/shard_placement.cc`
- `tests/epic3_distributed_tensor/shard_placement_test.cc`
- `benchmarks/epic3_distributed_tensor/placement_strategy_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze placement input metadata and expected outputs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 2: Core implementation
- [ ] Document cost model inputs from hardware and workload shape
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate hot-spot, skew, and under-replicated placement edge cases
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for deterministic placement and rebalance cost
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 5: Performance and hardening
- [ ] Keep placement policy synchronized with hardware profiles and planner rules
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 6: Documentation and acceptance
- [ ] Keep placement policy synchronized with hardware profiles and planner rules
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.3 Shard Placement`.

### Phase 7: Integration
- [ ] Keep placement policy synchronized with hardware profiles and planner rules
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.3 Shard Placement`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC2_HARDWARE_PROFILES.md`
- `docs/EPIC2_QUERY_PLANNER.md`
