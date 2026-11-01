---
Author: "ThemisDB Team"
Created: "2026-08-07"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "build-test"
Canonical_References: "tests/timeseries/test_timeseries_error_path_phase3.cpp; include/timeseries/timeseries_incident_taxonomy.h; tests/timeseries/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries incident-taxonomy test inventory

This file replaces the earlier Phase 3 coverage claims with a source inventory.
It records test declarations and CMake registration, not test execution,
coverage percentage, or runtime performance.

## Current test source

[`test_timeseries_error_path_phase3.cpp`](test_timeseries_error_path_phase3.cpp)
contains tests using the incident types and helpers declared in
[`timeseries_incident_taxonomy.h`](../../include/timeseries/timeseries_incident_taxonomy.h).
The source declares cases for ingest, query, lifecycle, and integration
incident factories; classification helpers; handler registration and
deregistration; concurrent emission; and severity values.

Two latency-named cases in this file are disabled (`DISABLED_...`). They are
not evidence of an active performance test. This suite exercises taxonomy
objects and callbacks; its presence does not establish that each production
component emits those incidents or that every documented incident code is
covered by an implementation-level test.

## Module test registration

[`CMakeLists.txt`](CMakeLists.txt) discovers `test_*.cpp` files, creates
`module_timeseries_<source-stem>_focused` targets, and registers CTest names
`<source-stem>_TimeseriesFocusedTests`. For this source, after configuring the
repository:

```bash
cmake --build <build-dir> --target module_timeseries_test_timeseries_error_path_phase3_focused
ctest --test-dir <build-dir> -R test_timeseries_error_path_phase3_TimeseriesFocusedTests
```

These commands describe registration only. No build or test was run for this
documentation update.

## Coverage boundaries

- Storage-backed behavior should be checked in the focused TSStore tests
  registered elsewhere under `tests/`; taxonomy-only tests do not establish
  RocksDB persistence behavior.
- Do not infer a 40-code/100% coverage figure, a measured latency bound, or a
  production integration guarantee from this test source.
- For current public signatures and error definitions, consult
  `timeseries_incident_taxonomy.h` and
  [`timeseries_api_contract.h`](../../include/timeseries/timeseries_api_contract.h).
