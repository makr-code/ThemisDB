# EPIC 2.1 Hardware Profiles

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Hardware profile definitions and sizing guidance for layered retrieval.

## Scope

- Memory, storage, accelerator, and network capability tiers
- Single-node, shard-local, and federated deployment profiles
- Planner and benchmark consumption of hardware metadata

## Planned Repository Surfaces

- `src/evaluation/include/hardware_profile.h`
- `src/evaluation/src/hardware_profile.cc`
- `tests/epic2_evaluation/hardware_profile_test.cc`
- `benchmarks/epic2_evaluation/hardware_profile_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze profile schema before any planner heuristics depend on it
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 2: Core implementation
- [ ] Document minimum supported and recommended sizing bands
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate invalid or partially-known hardware profile states
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 4: Tests
- [ ] Reserve test coverage for parsing, validation, and planner interaction
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 5: Performance and hardening
- [ ] Keep storage and accelerator hints aligned with EPIC 3 placement rules
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 6: Documentation and acceptance
- [ ] Keep storage and accelerator hints aligned with EPIC 3 placement rules
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.1 Hardware Profiles`.

### Phase 7: Integration
- [ ] Keep storage and accelerator hints aligned with EPIC 3 placement rules
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.1 Hardware Profiles`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
