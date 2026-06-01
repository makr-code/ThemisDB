# EPIC 2 Architecture

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Overall design for evaluation, hardware-aware planning, and governance.

## Scope

- Hardware profile and benchmark matrix ownership
- Evaluation metrics and approximation governance boundaries
- Query planner and artifact lifecycle coordination

## Planned Repository Surfaces

- `src/evaluation/include/hardware_profile.h`
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/artifact_lifecycle.h`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define how online planner rules and offline evaluation share vocabulary
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 2: Core implementation
- [ ] Document which metrics are enforcement-grade versus advisory
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 3: Error handling and edge cases
- [ ] Reserve planner hooks for approximation governance and hardware constraints
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 4: Tests
- [ ] Describe lifecycle ownership for derived artifacts and summaries
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 5: Performance and hardening
- [ ] Keep storage strategy guidance tied to artifact classes rather than implementations
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 6: Documentation and acceptance
- [ ] Keep storage strategy guidance tied to artifact classes rather than implementations
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2 Architecture`.

### Phase 7: Integration
- [ ] Keep storage strategy guidance tied to artifact classes rather than implementations
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2 Architecture`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `EVALUATION_FRAMEWORK.md`
- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC3_ARCHITECTURE.md`
