### Context

This issue implements the roadmap item 'WASM Non-Function Import Parsing Completeness' for the base domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: WASM Non-Function Import Parsing Completeness

### Goal

Deliver the scoped changes for WASM Non-Function Import Parsing Completeness in src/base/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### WASM Non-Function Import Parsing Completeness
**Priority:** Medium
**Target Version:** v1.2.0

In `wasm_plugin_sandbox.cpp` (lines 192–203), parsing of the imports section stops accumulating entries when a non-function import (table, memory, global) is encountered before all function imports have been listed. The comment acknowledges this limitation: "only the imports before the first non-function entry will appear in `info.imports`." This means capability-model enforcement is incomplete for WASM modules that declare memory/table imports before their function imports.

**Implementation Notes:**
- `[ ]` Fix the import-section parser in `wasm_plugin_sandbox.cpp` to correctly skip non-function import descriptors (table: `0x01`, memory: `0x02`, global: `0x03`) and continue accumulating function imports regardless of ordering.
- `[ ]` Add unit tests with WASM binaries that interleave memory and function imports; verify all function imports appear in `info.imports`.

---

### Acceptance Criteria

- [ ] Fix the import-section parser in `wasm_plugin_sandbox.cpp` to correctly skip non-function import descriptors (table: `0x01`, memory: `0x02`, global: `0x03`) and continue accumulating function imports regardless of ordering.
- [ ] Add unit tests with WASM binaries that interleave memory and function imports; verify all function imports appear in `info.imports`.

### Relationships

- Roadmap row: #153 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#wasm-non-function-import-parsing-completeness
- Source key: roadmap:153:base:v1.2.0:wasm-non-function-import-parsing-completeness

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:153:base:v1.2.0:wasm-non-function-import-parsing-completeness -->
<!-- roadmap-ref: row=153;module=base;target=v1.2.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#wasm-non-function-import-parsing-completeness -->
