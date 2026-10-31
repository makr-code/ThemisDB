---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Index test architecture

`tests/index/` is a module-level verification layer, not a runtime subsystem.
Each C++ test source exercises index APIs or lifecycle behavior; registration,
link dependencies, labels, and exclusions are defined in
[`CMakeLists.txt`](CMakeLists.txt).

## Test boundaries

- Unit and focused integration tests validate public index behavior against
  local test fixtures and configured dependencies.
- Tests that need CUDA linkage or runtime support are conditionally configured;
  other GPU fallback tests do not prove numerical behavior on a GPU.
- Release-critical labels are assigned to selected distributed-vector and
  hardening suites by the module CMake file.
- Suites excluded from registration are not evidence of currently passing
  behavior; check the current CMake conditions before relying on them.

Use the source-level behavior references in
[`../../src/index/API.md`](../../src/index/API.md) and the module
[`README.md`](../../src/index/README.md) to choose tests for a change.
