# EPIC 1.4 LoRA Artifacts

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Planning document for LoRAPackage and PortableAdapterProduct models.

## Scope

- Portable adapter package metadata
- Compatibility between base model, adapter, and retrieval path
- Artifact references consumed by model-switch workflow

## Planned Repository Surfaces

- `src/retrieval/include/lora_package.h`
- `src/retrieval/src/lora_package.cc`
- `tests/epic1_retrieval/lora_package_test.cc`
- `benchmarks/epic1_retrieval/lora_loading_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Separate packaging metadata from runtime loading behavior
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 2: Core implementation
- [ ] Specify integrity, provenance, and compatibility requirements for adapter artifacts
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 3: Error handling and edge cases
- [ ] Document edge cases for incompatible ranks, quantization, or base-model drift
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 4: Tests
- [ ] Reserve performance checks for loading and activation costs
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 5: Performance and hardening
- [ ] Keep relationship to distributed artifact manifests explicit
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 6: Documentation and acceptance
- [ ] Keep relationship to distributed artifact manifests explicit
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.4 LoRA Artifacts`.

### Phase 7: Integration
- [ ] Keep relationship to distributed artifact manifests explicit
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.4 LoRA Artifacts`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_MODEL_SWITCH.md`
- `docs/EPIC3_ARTIFACT_CLASSES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
