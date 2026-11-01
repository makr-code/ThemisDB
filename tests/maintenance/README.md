# tests/maintenance

---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

Mirrored module test folder for `src/maintenance`.

The test subdirectory is discovered by `tests/CMakeLists.txt`. Its CMake file
registers the contract, phase-23, churn, stress, diagnostics, and high-cardinality
targets with CTest. The endurance source is built as an executable there but is
not registered as a CTest test. These are focused unit/stress fixtures; several
test local models or stubs rather than exercising the production orchestrator.
For production schedule-store recovery coverage, use
[`tests/database/test_database_maintenance_orchestrator.cpp`](../database/test_database_maintenance_orchestrator.cpp).

## Focused test commands

The `linux-debug` preset defaults tests off; configure with tests enabled, then
build the registered targets and run their CTest cases. This preset requires its
Linux/vcpkg toolchain dependencies; see the repository setup guide if needed.

```sh
cmake --preset linux-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build build-gcc-linux-debug --target \
  module_maintenance_test_maintenance_contract_hardening_focused_focused \
  module_maintenance_test_maintenance_hardening_phase23_focused \
  module_maintenance_test_maintenance_churn_hardening_focused \
  module_maintenance_test_maintenance_stress_focused \
  module_maintenance_test_maintenance_diagnostics_focused \
  module_maintenance_test_maintenance_highcardinality_stress \
  test_database_maintenance_orchestrator_focused
ctest --test-dir build-gcc-linux-debug \
  -R '^(test_maintenance_.*_FocusedTests|DatabaseMaintenanceOrchestratorFocusedTests)$' \
  --output-on-failure
```

`module_maintenance_test_maintenance_endurance_focused` is a standalone
executable target. Its CMake block uses `add_executable` but does not call
`themis_register_module_focused_test`, so it has no CTest entry and is not run by
the CTest command above. Build it separately with:

```sh
cmake --build build-gcc-linux-debug --target \
  module_maintenance_test_maintenance_endurance_focused
```
