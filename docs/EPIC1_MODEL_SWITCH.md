# EPIC 1.5 Model Switch Workflow

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Model-switch orchestration for retrieval-selected adapters and compatible base models.

## Scope

- Compatibility matrix between retrieval outcome and executable model path
- Planner and artifact validation prerequisites
- Safe fallback when a requested switch is unsupported

## Planned Repository Surfaces

- `src/retrieval/include/model_switch.h`
- `src/retrieval/src/model_switch.cc`
- `tests/epic1_retrieval/model_switch_test.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Freeze request-to-model compatibility metadata
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 2: Core implementation
- [ ] Document safe fallback order for unavailable or incompatible adapters
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 3: Error handling and edge cases
- [ ] Define failure reporting for unsupported switch requests
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 4: Tests
- [ ] Link compatibility validation to lifecycle and integrity data
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 5: Performance and hardening
- [ ] Reserve targeted tests for matrix coverage and degraded fallbacks
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 6: Documentation and acceptance
- [ ] Reserve targeted tests for matrix coverage and degraded fallbacks
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.5 Model Switch Workflow`.

### Phase 7: Integration
- [ ] Reserve targeted tests for matrix coverage and degraded fallbacks
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.5 Model Switch Workflow`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_LORA_ARTIFACTS.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
