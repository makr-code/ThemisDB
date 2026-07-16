# EPIC 3.4 Integrity Model

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Integrity verification, Merkle structures, and receipt semantics.

## Scope

- Hash tree or receipt model for artifact verification
- Validation timing on ingest, query path, and background audit
- Planner-facing degradation and block rules on integrity failure

## Planned Repository Surfaces

- `src/distributed_tensor/include/integrity_verification.h`
- `src/distributed_tensor/src/integrity_verification.cc`
- `tests/epic3_distributed_tensor/integrity_verification_test.cc`
- `benchmarks/epic3_distributed_tensor/integrity_verification_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze required integrity metadata and verification states
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 2: Core implementation
- [ ] Document when queries may proceed with cached receipts versus fresh verification
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate corruption, tampering, and partial-receipt edge cases
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 4: Tests
- [ ] Reserve tests and benchmarks for happy-path and degraded verification cost
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 5: Performance and hardening
- [ ] Keep integrity semantics aligned with graph provenance and lifecycle policies
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 6: Documentation and acceptance
- [ ] Keep integrity semantics aligned with graph provenance and lifecycle policies
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.4 Integrity Model`.

### Phase 7: Integration
- [ ] Keep integrity semantics aligned with graph provenance and lifecycle policies
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.4 Integrity Model`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_GRAPH_VALIDATION.md`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
