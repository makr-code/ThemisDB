---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# index

Path: `tests/index/`

## Purpose

This directory contains focused C++ tests for ANN routing, vector indexes,
index management and manifests, GPU fallback/memory behavior, compression,
spatial correctness, tier migration, and hardening. The test sources are not a
single uniform suite: CMake registers `test_*.cpp` files as focused tests and
also creates a second autofocused registration for eligible `test_index_*.cpp`
files.

## Registration and limits

- Registration logic and target names are maintained in
  [`CMakeLists.txt`](CMakeLists.txt); root test configuration discovers this
  module file when `THEMIS_BUILD_TESTS=ON`. Many standard presets disable tests.
- Several suites are explicitly skipped in the focused loops because they
  target a superseded contract. A source file's presence therefore does not
  guarantee that it is built or registered in every configuration.
- GPU-related tests do not establish hardware parity unless run on the
  corresponding supported device and build.

For current index behavior, see [`../../src/index/API.md`](../../src/index/API.md)
and [`../../src/index/MODULE_GUIDE.md`](../../src/index/MODULE_GUIDE.md).
