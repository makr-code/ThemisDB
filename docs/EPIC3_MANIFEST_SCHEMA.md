# EPIC 3.2 Manifest Schema

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Manifest coordination and metadata schema for distributed tensor artifacts.

## Scope

- Artifact identity, versioning, and placement metadata
- Freshness, integrity, and compatibility receipts
- Planner and retrieval-facing lookup fields

## Planned Repository Surfaces

- `src/distributed_tensor/include/artifact_manifest.h`
- `src/distributed_tensor/src/artifact_manifest.cc`
- `tests/epic3_distributed_tensor/artifact_manifest_test.cc`
- `benchmarks/epic3_distributed_tensor/manifest_coordination_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze manifest field names and required invariants early
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 2: Core implementation
- [ ] Document merge and conflict resolution behavior across shards
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate partial-manifest and stale-manifest failure cases
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for parsing, sync, and merge cost
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 5: Performance and hardening
- [ ] Keep manifest fields planner-friendly and integrity-aware
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 6: Documentation and acceptance
- [ ] Keep manifest fields planner-friendly and integrity-aware
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 7: Integration
- [ ] Keep manifest fields planner-friendly and integrity-aware
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.2 Manifest Schema`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
