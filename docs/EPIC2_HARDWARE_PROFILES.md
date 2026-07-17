# EPIC 2.1 Hardware Profiles

<!-- Status: current | implementation-active | validated: 2026-07-15 -->

## Summary

Hardware profile definitions and sizing guidance for layered retrieval.

## Scope

- Memory, storage, accelerator, and network capability tiers
- Single-node, shard-local, and federated deployment profiles
- Planner and benchmark consumption of hardware metadata

## Repository Surfaces

- `src/evaluation/include/hardware_profile.h`
- `src/evaluation/src/hardware_profile.cc`
- `tests/epic2_evaluation/hardware_profile_test.cc`
- `tests/epic2_evaluation/CMakeLists.txt`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Freeze the initial profile schema for CPU, RAM, NVMe, GPU, network, and tiering metadata
- [x] Expose a queryable registry contract before planner heuristics depend on it

### Phase 2: Core implementation
- [x] Document and codify minimum/recommended sizing bands for development, production, and federated profiles
- [x] Implement profile lookup, activation, and initialization in `hardware_profile.cc`

### Phase 3: Error handling and edge cases
- [x] Reject incomplete or internally inconsistent profile definitions
- [x] Guard dynamic hot/warm/cold tier switches with profile-transition validation

### Phase 4: Tests
- [x] Cover parsing, validation, break-even guidance, and profile/tier switching in `hardware_profile_test.cc`
- [x] Wire focused test coverage into `tests/epic2_evaluation/CMakeLists.txt`

### Phase 5: Performance and hardening
- [x] Record initial DiskANN break-even thresholds per profile in the ANN layer rules
- [x] Keep storage and accelerator hints aligned with EPIC 3 placement expectations

### Phase 6: Documentation and acceptance
- [x] Keep profile naming, sizing bands, and tiering guidance synchronized with `HARDWARE_REQUIREMENTS.md`
- [x] Require every built-in profile to cover ANN, tensor, graph, and LLM layers

### Phase 7: Integration
- [x] Keep storage and accelerator hints aligned with EPIC 3 placement and planner expectations
- [x] Wire focused validation into the local EPIC 2 evaluation test CMake integration

## Acceptance Signals

- The implementation exposes stable built-in profile IDs: `development`, `production`, and `high_performance_federated`.
- Validation rejects incomplete/incompatible profiles before they can become active.
- Tests verify all four layered retrieval stages remain covered across all built-in profiles.

## References

- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
