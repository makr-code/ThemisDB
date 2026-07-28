# Core Module - Build and Test Evidence

<!-- Status: current | validated: 2026-07-28 -->
<!-- Issue: #5638 (Development Status 2026-07-18) -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · ARCHITECTURE.md -->

## Evidence Summary

This document tracks the current evidence state for the core module status issue and records both canonical historical evidence and current-environment validation gaps.

## Canonical Snapshot (from issue context, validated 2026-07-18)

- Preset: `windows-release`
- Focused target pattern: `module_core_test_*_focused.exe`
- Result: focused module binary was not found in `build-msvc-windows-release/bin_out`
- Status: evidence gap documented in issue #5638

## Focused Test Registration Evidence (source-verifiable, validated 2026-07-28)

- Test registration file: `tests/core/CMakeLists.txt`
- Focused target naming rule:
  - `module_core_${_stem}_focused`
- Current focused test source present:
  - `tests/core/test_core_smoke.cpp`
- Expected focused target from current source:
  - `module_core_test_core_smoke_focused`

## Current Local Build/Test Attempt (2026-07-28)

- Command: `cmake --preset linux-release`
- Result: failed before generation
  - missing toolchain file `vcpkg/scripts/buildsystems/vcpkg.cmake`
  - Ninja build program not found
- Impact: no local focused core binary could be built in this environment during this validation pass

## Status Assessment

- [x] Roadmap/future/architecture synchronization refreshed for issue #5638
- [x] Focused test registration path verified in source
- [~] Fresh executable-level focused build/test evidence remains blocked by local toolchain/generator setup
- [ ] New executable run evidence for `module_core_test_*_focused` captured for this cycle

## Next Evidence Actions

1. Restore functional build prerequisites (toolchain + Ninja) in validation environment.
2. Configure and build focused core test target(s).
3. Execute focused core test binary and append pass/fail evidence with timestamp.
