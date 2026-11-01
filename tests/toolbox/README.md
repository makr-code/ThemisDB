---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox test targets

`tests/toolbox/CMakeLists.txt` uses a CMake glob for direct `test_*.cpp` files. For each file stem it defines executable target `module_toolbox_<stem>_focused` and CTest name `<stem>_toolbox_FocusedTests`, with timeout 120 seconds and label `toolbox`.

## Current source files and registrations

| Source file | Build target | CTest name |
|---|---|---|
| `test_content_fingerprinter_binary_focused.cpp` | `module_toolbox_test_content_fingerprinter_binary_focused_focused` | `test_content_fingerprinter_binary_focused_toolbox_FocusedTests` |
| `test_toolbox_contract_hardening_focused.cpp` | `module_toolbox_test_toolbox_contract_hardening_focused_focused` | `test_toolbox_contract_hardening_focused_toolbox_FocusedTests` |
| `test_toolbox_highcardinality_stress.cpp` | `module_toolbox_test_toolbox_highcardinality_stress_focused` | `test_toolbox_highcardinality_stress_toolbox_FocusedTests` |
| `test_toolbox_ingestion.cpp` | `module_toolbox_test_toolbox_ingestion_focused` | `test_toolbox_ingestion_toolbox_FocusedTests` |
| `test_toolbox_phase5.cpp` | `module_toolbox_test_toolbox_phase5_focused` | `test_toolbox_phase5_toolbox_FocusedTests` |
| `test_toolbox_primitives.cpp` | `module_toolbox_test_toolbox_primitives_focused` | `test_toolbox_primitives_toolbox_FocusedTests` |

The CMake registration is the authority for target names, CTest names, timeout, and label. Test source files describe their own cases; their presence does not establish that every test is a production or release gate. In particular, `test_toolbox_highcardinality_stress.cpp` explicitly uses stubs/simulations and does not exercise production routing or bridge paths.

## Running the registered tests

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target module_toolbox_test_toolbox_primitives_focused
ctest --preset linux-release -R test_toolbox_primitives_toolbox_FocusedTests --output-on-failure
```

To select all registered toolbox tests, use:

```bash
ctest --preset linux-release -L toolbox --output-on-failure
```

These are documented commands, not a claim that the targets were built or tests executed during this documentation update. No current total case count or expected runtime is asserted here.

## Sources

- [`CMakeLists.txt`](./CMakeLists.txt)
- Test source files listed above
- [`src/toolbox/IMPLEMENTATION.md`](../../src/toolbox/IMPLEMENTATION.md)
- [`src/toolbox/PERFORMANCE_EXPECTATIONS.md`](../../src/toolbox/PERFORMANCE_EXPECTATIONS.md)

> Exact model backend not verifiable from final document text alone.
