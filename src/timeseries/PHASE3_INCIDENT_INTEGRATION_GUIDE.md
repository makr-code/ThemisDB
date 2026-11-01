---
Author: "ThemisDB Team"
Created: "2026-07-29"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/timeseries/timeseries_incident_taxonomy.h; include/timeseries/timeseries_api_contract.h; src/timeseries/timeseries_incident_taxonomy.cpp; tests/timeseries/test_timeseries_error_path_phase3.cpp"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries incident taxonomy: implementation boundary

The previous version presented proposed code snippets as integrations already
implemented in the flush controller, retention, remote-write, encrypted
storage, and query optimizer. Those snippets do not match the current
interfaces and are withdrawn. This page documents the verified boundary only.

## What is declared and tested

- [`timeseries_incident_taxonomy.h`](../../include/timeseries/timeseries_incident_taxonomy.h)
  declares incident classes for ingest, query, lifecycle, and integration,
  along with severity, context, factories, handlers, and classification
  helpers.
- [`timeseries_api_contract.h`](../../include/timeseries/timeseries_api_contract.h)
  declares the timeseries error-code contract. It is not the return type of
  every API in the module; inspect the specific public declaration.
- [`test_timeseries_error_path_phase3.cpp`](../../tests/timeseries/test_timeseries_error_path_phase3.cpp)
  exercises taxonomy construction/classification and handler behavior. Two
  latency-named cases are disabled. The test does not establish that each
  production component emits an incident for every listed condition.
- The taxonomy implementation is
  [`timeseries_incident_taxonomy.cpp`](timeseries_incident_taxonomy.cpp).
  Check individual component source files before claiming an integration.

## Validation and registration

`tests/timeseries/CMakeLists.txt` discovers `test_*.cpp` and creates focused
targets from their filenames. The incident-test target is
`module_timeseries_test_timeseries_error_path_phase3_focused`; its CTest name
is `test_timeseries_error_path_phase3_TimeseriesFocusedTests`.

```bash
cmake --build <build-dir> --target module_timeseries_test_timeseries_error_path_phase3_focused
ctest --test-dir <build-dir> -R test_timeseries_error_path_phase3_TimeseriesFocusedTests
```

These are registration-derived commands, not evidence that the suite was
built or run. The issue #7387 documentation pass did not run builds or tests.

## Integration guidance

When adding an incident emission to a component, first verify that the
component has a suitable error/failure path and use its actual return types.
Do not replace an existing error return with a taxonomy event unless the
component contract explicitly calls for that behavior. Preserve safe diagnostic
context; avoid secrets and raw remote-write payloads. Add a focused test of the
real component path before documenting the integration as implemented.
