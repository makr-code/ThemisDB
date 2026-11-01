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

> Exact model backend not verifiable from final document text alone.

# Plugin focused tests

`tests/CMakeLists.txt` discovers module directories containing a
`CMakeLists.txt`. This directory's `CMakeLists.txt` globs `test_*.cpp` and
creates one focused unit-test executable per source when tests and GTest are
available.

## Target and CTest naming

For source `test_example.cpp`, the target is
`module_plugins_test_example_focused`, and the registered CTest name is
`test_example_plugins_FocusedTests`. The local registration uses tier `unit`,
timeout 120 seconds, and label `plugins`. The boundary-enforcement, manifest
edition-gate, and plugins-contract-hardening targets also receive the
`wave_b` / `wave_b_boundary_policy_control` labels.

The root build must have `THEMIS_BUILD_TESTS` enabled and GTest discoverable.
The CMake registration and source inventory do not prove that the executables
were built or that tests passed.

## Registered source inventory

- Lifecycle, manager, dependencies, and state: `test_plugin_lifecycle.cpp`,
  `test_plugin_lifecycle_state_machine.cpp`, `test_plugin_manager.cpp`,
  `test_plugin_manager_comprehensive.cpp`, `test_plugin_dependency_graph.cpp`,
  `test_plugin_dependency_resolver.cpp`, `test_plugin_error_handling_phase3.cpp`,
  `test_plugin_hot_reload_enhanced.cpp`.
- Capabilities, manifest, and contract: `test_plugin_capability_escalation.cpp`,
  `test_plugin_capability_negotiation.cpp`,
  `test_plugin_manifest_edition_gates_focused.cpp`,
  `test_validation_contract_hardening.cpp`,
  `test_plugins_contract_hardening_focused.cpp`,
  `test_plugin_boundary_enforcement_focused.cpp`.
- Registry and monitoring: `test_registry_concurrency_hardening.cpp`,
  `test_plugin_health_monitor.cpp`, `test_plugin_hot_plug.cpp`,
  `test_plugin_metrics.cpp`, `test_plugin_metrics_integration.cpp`.
- Security and registration: `test_plugin_security_audit.cpp`,
  `test_plugin_security_crl_ocsp.cpp`,
  `test_plugin_security_implementation.cpp`,
  `test_plugin_security_pe_cert_extraction.cpp`,
  `test_plugin_marketplace_manifest.cpp`,
  `test_plugin_registrar_fail_closed.cpp`.

The registrar-fail-closed target adds source files from other modules as
specified in this directory's CMake file. Consult that file before interpreting
it as an isolated plugins-only test.
