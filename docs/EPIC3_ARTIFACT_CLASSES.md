# EPIC 3.1 Artifact Classes

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Tensor artifact classes, packaging boundaries, and lifecycle roles.

## Scope

- Shard-local tensor, summary, adapter, and derived artifact classes
- Ownership of source-of-truth, cache, and exported artifacts
- Compatibility metadata shared with model-switch and planner logic

## Planned Repository Surfaces

- `src/distributed_tensor/include/tensor_artifact_classes.h`
- `src/distributed_tensor/src/tensor_artifact_classes.cc`
- `tests/epic3_distributed_tensor/tensor_artifact_classes_test.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze the artifact taxonomy and lifecycle roles
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 2: Core implementation
- [ ] Document relationships between portable and shard-local artifacts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate invalid combinations of class, freshness, and compatibility state
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 4: Tests
- [ ] Reserve tests for classification and lifecycle transitions
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 5: Performance and hardening
- [ ] Keep artifact vocabulary aligned with lifecycle and manifest documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 6: Documentation and acceptance
- [ ] Keep artifact vocabulary aligned with lifecycle and manifest documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.1 Artifact Classes`.

### Phase 7: Integration
- [ ] Keep artifact vocabulary aligned with lifecycle and manifest documents
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.1 Artifact Classes`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_LORA_ARTIFACTS.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
