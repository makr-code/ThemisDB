---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Temporal test inventory and follow-up

This file describes the test subtree; it is not an independent release
readiness roadmap. The temporal implementation plan is
[`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md), and aggregate
maturity is governed by the root [`../../ROADMAP.md`](../../ROADMAP.md).

## Current inventory

- 20 test sources are present in `tests/temporal/`.
- The CMake file registers all 20 as focused tests and registers the 15
  `test_temporal_*.cpp` files a second time with `_autofocused` targets.
- Both registration loops set a 120-second timeout. The second loop labels its
  CTest entries `temporal` and `autogen`.

These are source/configuration facts only; they do not establish that tests
were built or passed.

## Verification TODOs

- **TODO:** record the configured build preset and focused CTest execution
  results.
- **TODO:** identify tests that use test-local models versus production runtime
  classes before making contract-coverage claims.
- **TODO:** link current coverage or soak results before assigning coverage
  targets or declaring reliability criteria complete.

See [`README.md`](README.md) for source groupings and CMake behavior.
