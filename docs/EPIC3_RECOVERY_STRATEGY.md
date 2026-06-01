# EPIC 3.5 Recovery Strategy

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Recovery, rebuild, and erasure coding strategy for distributed tensor artifacts.

## Scope

- Recovery levels for summary, adapter, and full tensor artifacts
- Rebuild ordering, erasure coding, and cold-start behavior
- Interaction with lifecycle staleness and integrity failures

## Planned Repository Surfaces

- `src/distributed_tensor/include/recovery_manager.h`
- `src/distributed_tensor/src/recovery_manager.cc`
- `tests/epic3_distributed_tensor/recovery_manager_test.cc`
- `benchmarks/epic3_distributed_tensor/recovery_rebuild_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze the recovery state machine and rebuild priorities
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 2: Core implementation
- [ ] Document which failures require immediate rebuild versus degraded service
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate edge cases for multi-shard loss, partial corruption, and missing receipts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for rebuild cost and survivability
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 5: Performance and hardening
- [ ] Keep recovery compatible with lifecycle, placement, and integrity documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 6: Documentation and acceptance
- [ ] Keep recovery compatible with lifecycle, placement, and integrity documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.5 Recovery Strategy`.

### Phase 7: Integration
- [ ] Keep recovery compatible with lifecycle, placement, and integrity documents
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.5 Recovery Strategy`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
