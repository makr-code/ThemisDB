---
Author: "ThemisDB Contributors"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "CMakeLists.txt"
  - "../../CMakePresets.json"
  - "../../include/storage/storage_engine.h"
  - "../../include/storage/mvcc_store.h"
  - "../../include/storage/wal_storage.h"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage tests

This directory contains flat `test_*.cpp` sources for storage components,
including the engine, MVCC/WAL, blob backends, diagnostics, tensor storage,
layout advice, and focused hardening cases. At this audit, 21 matching
`test_*.cpp` files are present.

[`CMakeLists.txt`](CMakeLists.txt) discovers `test_*.cpp` files and registers
focused unit-test targets. It also has a `test_storage_*.cpp` auto-focused
pass that skips targets already created by the first pass. Registration does
not mean a test was built or passed.

## Build preset caveat

The defined Linux release preset is `linux-release`; `linux-ninja-release` and
a generic `release` preset are not defined. `linux-release` requires Linux,
GCC 11+, Ninja, and the repository-local bootstrapped vcpkg checkout. It sets
`THEMIS_BUILD_TESTS=OFF` and `THEMIS_BUILD_BENCHMARKS=OFF`, so it does not
configure the focused test targets described above.

For a system-package development build, `community-debug` is defined, but it
inherits `THEMIS_BUILD_TESTS=OFF` from the base preset. Enable tests explicitly
when configuring, for example:

```bash
cmake --preset community-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build --preset community-debug --target module_storage_test_mvcc_chain_pruner_focused
ctest --test-dir build-community-debug -R '^test_mvcc_chain_pruner_StorageFocusedTests$' --output-on-failure
```

This preset requires Linux/macOS, GCC or Clang, Ninja, and the system
development packages listed by the preset/`SETUP.md`; it does not use vcpkg.
The commands configure/build one target only and do not claim that it passes.

For test/benchmark source maps, see
[`../../src/storage/AUDIT.md`](../../src/storage/AUDIT.md). Run the relevant
CTest target in the intended build configuration and record the result before
claiming validation.
