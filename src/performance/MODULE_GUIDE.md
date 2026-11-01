---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "include/performance/*.h"
  - "src/performance/*.cpp"
  - "tests/performance/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Developer Guide

## Start here

Use this guide when adding or changing a performance subsystem. Read
[README.md](README.md) for module scope,
[ARCHITECTURE.md](ARCHITECTURE.md) for execution boundaries, and
[API.md](API.md) for caller contracts. Check [ROADMAP.md](ROADMAP.md) and
[MODULE_GAPS.md](MODULE_GAPS.md) before treating a follow-up as unplanned.
The public declarations live in `include/performance`; implementation belongs
in `src/performance`; focused tests are under `tests/performance`.

## Change workflow

1. Identify the exact public header, implementation path, downstream caller,
   and focused test. Preserve the v2.x frozen contract in
   `performance_api_contract.h`.
2. Confirm compile-time gates, runtime flags, and hardware or OS prerequisites.
   Keep unsupported paths explicit and retain the fallback behavior promised by
   the interface.
3. Validate input ranges and failure semantics at the boundary. Keep optional
   metrics and background adaptation bounded, and honor each type's
   thread-safety contract.
4. Add or extend focused tests for normal behavior, invalid inputs, capability
   absence, and fallback/error outcomes. For vector algorithms also test
   dimensionality, empty indexes, and result quality against an exact baseline.
5. Run relevant mapped release benchmarks on the same hardware and build
   profile as the baseline. Do not report paper results or estimates as local
   measurements.
6. Update the module docs when behavior or contracts change. Keep API details
   in [API.md](API.md), method assumptions in [MODELING.md](MODELING.md) or
   [EMBEDDING.md](EMBEDDING.md), and incident actions in [RUNBOOK.md](RUNBOOK.md).

## Focused evidence

Useful current test sources include:

- `tests/performance/test_performance_contract_hardening_focused.cpp` for
  contract and edge behavior;
- `tests/performance/test_workload_predictor.cpp` for prediction behavior;
- `tests/performance/test_performance_hardware_accelerator.cpp` for dispatch;
- `tests/performance/test_numa_topology.cpp` for topology behavior;
- `tests/performance/test_rabitq_product_quantizer.cpp` for the RaBitQ path.

`tests/performance/CMakeLists.txt` globs root-level `test_*.cpp` files and
recursively includes phase3/phase4 test files. Root tests use targets
`module_performance_<stem>_focused`; nested names include a stable hash of the
relative source path. A separate root-only glob also creates `_autofocused`
targets for root-level `test_performance_*.cpp` sources. Confirm available tests
in the configured build instead of assuming every source is runnable through
CTest:

```bash
ctest --test-dir <build-dir> -N -R performance
```

The project build configuration determines which optional dependencies and
module flags are available. For docs-only changes, run the metadata gate
command in [IMPLEMENTATION.md](IMPLEMENTATION.md).

## Review checklist

- Is the path genuinely available in this build and on the target host?
- Are fallback and error results distinguishable from successful acceleration?
- Are resource bounds, thread interactions, and data ownership explicit?
- Do tests verify the caller-visible result instead of only internal state?
- Do benchmark results cover the same workload and hardware as the claim?
- Are operator steps and rollback constraints reflected in
  [RUNBOOK.md](RUNBOOK.md) and [OPERATIONS.md](OPERATIONS.md)?

Exact model backend not verifiable from final document text alone.
