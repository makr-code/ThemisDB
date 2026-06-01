# EPIC 1.1 ANN Frontdoor

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

ANN abstraction and routing contract for the first retrieval stage.

## Scope

- HNSW and DiskANN capability model
- Index selection and fallback rules
- Candidate budget handoff to tensor and graph stages

## Planned Repository Surfaces

- `src/retrieval/include/ann_frontdoor.h`
- `src/retrieval/src/ann_frontdoor.cc`
- `tests/epic1_retrieval/ann_frontdoor_test.cc`
- `benchmarks/epic1_retrieval/ann_retrieval_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze the provider-neutral ANN interface and capability flags
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 2: Core implementation
- [ ] Define routing decisions that depend on hardware profile and corpus shape
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 3: Error handling and edge cases
- [ ] Document error modes such as index unavailable, stale index, or recall floor miss
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 4: Tests
- [ ] Reserve unit and benchmark coverage for backend comparison
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 5: Performance and hardening
- [ ] Keep integration with planner and observability explicit
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 6: Documentation and acceptance
- [ ] Keep integration with planner and observability explicit
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.1 ANN Frontdoor`.

### Phase 7: Integration
- [ ] Keep integration with planner and observability explicit
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.1 ANN Frontdoor`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC2_HARDWARE_PROFILES.md`
- `docs/EPIC2_QUERY_PLANNER.md`
