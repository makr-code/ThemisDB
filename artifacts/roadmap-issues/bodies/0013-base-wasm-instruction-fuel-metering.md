### Context

This issue implements the roadmap item 'WASM Instruction Fuel Metering' for the base domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: WASM Instruction Fuel Metering

### Goal

Deliver the scoped changes for WASM Instruction Fuel Metering in src/base/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### WASM Instruction Fuel Metering
**Priority:** High
**Target Version:** v1.2.0

`wasm_plugin_sandbox.cpp` allocates linear memory and validates imports/exports but has no instruction-counting / fuel mechanism. A malicious or buggy WASM plugin can spin indefinitely without triggering any timeout.

**Implementation Notes:**
- `[ ]` Add `WasmSandboxConfig::max_instructions` (default: 1 billion) and `WasmSandboxConfig::fuel_check_interval` fields in `wasm_plugin_sandbox.h`.
- `[ ]` Implement a fuel counter decremented per basic block in the WASM interpreter dispatch loop in `wasm_plugin_sandbox.cpp`; when fuel reaches zero, set `last_error_` and return an error code instead of the host function result.
- `[ ]` Expose remaining fuel via `WasmPluginSandbox::remainingFuel()` for observability.
- `[ ]` Add unit test: WASM module with infinite loop terminates within `max_instructions` cycles and returns a structured error.

**Performance Targets:**
- Fuel check overhead: ≤ 3 % CPU overhead vs. unchecked execution on a tight compute loop.

---

### Acceptance Criteria

- [ ] Add `WasmSandboxConfig::max_instructions` (default: 1 billion) and `WasmSandboxConfig::fuel_check_interval` fields in `wasm_plugin_sandbox.h`.
- [ ] Implement a fuel counter decremented per basic block in the WASM interpreter dispatch loop in `wasm_plugin_sandbox.cpp`; when fuel reaches zero, set `last_error_` and return an error code instead of the host function result.
- [ ] Expose remaining fuel via `WasmPluginSandbox::remainingFuel()` for observability.
- [ ] Add unit test: WASM module with infinite loop terminates within `max_instructions` cycles and returns a structured error.
- [ ] Fuel check overhead: ≤ 3 % CPU overhead vs. unchecked execution on a tight compute loop.

### Relationships

- Roadmap row: #13 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#wasm-instruction-fuel-metering
- Source key: roadmap:13:base:v1.2.0:wasm-instruction-fuel-metering

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:13:base:v1.2.0:wasm-instruction-fuel-metering -->
<!-- roadmap-ref: row=13;module=base;target=v1.2.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#wasm-instruction-fuel-metering -->
