# EPIC 2.6 Artifact Lifecycle

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Derived artifact lifecycle and staleness policy for retrieval and tensor artifacts.

## Scope

- Source-of-truth versus derived artifact classes
- Refresh windows, invalidation triggers, and rebuild priorities
- Lifecycle signals consumed by planner and recovery flows

## Planned Repository Surfaces

- `src/evaluation/include/artifact_lifecycle.h`
- `src/evaluation/src/artifact_lifecycle.cc`
- `tests/epic2_evaluation/artifact_lifecycle_test.cc`
- `benchmarks/epic2_evaluation/artifact_staleness_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define freshness and staleness metadata shared across all epics
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 2: Core implementation
- [ ] Document lifecycle ownership and background rebuild expectations
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate failure modes for stale, missing, or incompatible derived artifacts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for invalidation and rebuild overhead
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 5: Performance and hardening
- [ ] Align rebuild semantics with EPIC 3 recovery and integrity documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 6: Documentation and acceptance
- [ ] Align rebuild semantics with EPIC 3 recovery and integrity documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.6 Artifact Lifecycle`.

### Phase 7: Integration
- [ ] Align rebuild semantics with EPIC 3 recovery and integrity documents
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.6 Artifact Lifecycle`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_LORA_ARTIFACTS.md`
- `docs/EPIC3_RECOVERY_STRATEGY.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
