# EPIC 1.5 Model Switch Workflow

<!-- Status: complete | implementation delivered 2026-07-16 | closes #5419 -->

## Summary

Model-switch orchestration for retrieval-selected adapters and compatible base
models.  Implements a ratchet compatibility matrix, rebuild-first logic, and
policy-safe migration gates.

## Repository Surfaces

| File | Description |
|---|---|
| `include/llm/model_switch_workflow.h` | Public API: `SemVer`, `RatchetCompatibilityMatrix`, `RebuildPolicy`, `ModelSwitchWorkflow` |
| `src/llm/model_switch_workflow.cpp`   | Production implementation |
| `tests/model/test_model_switch_workflow.cpp` | Phase 6 QA test suite (35+ cases) |
| `docs/de/llm/MODEL_SWITCH_WORKFLOW_SOP.md`   | Operator SOP / Runbook |
| `docs/adr/adr-e1-005-model-switch-compatibility.md` | Architecture decision record |

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Freeze request-to-model compatibility metadata (`ModelSwitchRequest`, `SemVer`)
- [x] Define `RatchetCompatibilityMatrix` schema and ratchet semantics

### Phase 2: Core implementation
- [x] Implement `ModelSwitchWorkflow::executeSwitch()` with six gate categories
- [x] Implement `RatchetCompatibilityMatrix` with advance-only semantics and JSON serialization
- [x] Implement `RebuildPolicy` with configurable triggers and fail-closed mode

### Phase 3: Error handling and edge cases
- [x] No-op switch detection (same model name + version)
- [x] Unknown package → `INCOMPATIBLE` with descriptive error
- [x] Ratchet floor violation → immediate `INCOMPATIBLE` (no further checks)
- [x] Orchestrator promotion failure surfaced as warning (non-blocking)
- [x] Draft-adapter incompatibilities surfaced as warnings

### Phase 4: Tests
- [x] 35+ GTest cases in `tests/model/test_model_switch_workflow.cpp`
- [x] Ratchet semantics tests (advance floor, reject downgrade, operator override)
- [x] Fail-closed / fail-open rebuild policy tests
- [x] JSON round-trip tests for matrix, entry, policy, and result
- [x] Outcome-state tests (COMPATIBLE, REBUILD_REQUIRED, BLOCKED, INCOMPATIBLE)

### Phase 5: Performance and hardening
- [x] All checks run via existing `AdapterRegistry::validateCompatibility` (no new I/O)
- [x] Matrix lookups are O(n) with entries; acceptable for typical < 100-entry matrices
- [x] `[[nodiscard]]` applied to all result-bearing methods

### Phase 6: Documentation and acceptance
- [x] Doxygen comments on all public API surfaces
- [x] Operator SOP runbook: `docs/de/llm/MODEL_SWITCH_WORKFLOW_SOP.md`
- [x] ADR E1-005 completed: `docs/adr/adr-e1-005-model-switch-compatibility.md`
- [x] Issue #5419 deliverables satisfied

### Phase 7: Integration
- [x] `model_switch_workflow.h` includes `final_layer_orchestrator.h` and `adapter_registry.h`
- [x] Tests use the same CMakeLists.txt glob as other model-module tests
- [x] JSON audit output compatible with package-event log schema

## Acceptance Signals

- [x] `ModelSwitchWorkflow::executeSwitch()` returns a fully auditable `ModelSwitchResult`
- [x] Ratchet matrix prevents silent floor lowering without operator override
- [x] Rebuild policy is data-driven; no switch logic embedded in callers
- [x] All six gate categories covered: ratchet, architecture, tokenizer, layer dimensions, quantization, prompt format
- [x] SOP runbook documents rebuild runbook, rollback procedure, and ratchet governance

## References

- `docs/de/llm/MODEL_SWITCH_WORKFLOW_SOP.md`
- `docs/adr/adr-e1-005-model-switch-compatibility.md`
- `docs/EPIC1_LORA_ARTIFACTS.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
- Issue #5419
