---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Index test roadmap

## Current Status

Module test targets are generated from `test_*.cpp` sources by
[`CMakeLists.txt`](CMakeLists.txt). This file does not establish that every
source is registered or passing; exclusions and build conditions in that file
are authoritative.

## Open validation boundary

- [ ] Run GPU parity tests on representative supported CUDA/HIP hardware and
  record the build, device, and test evidence in the owning release gate
  (Target: Q4 2026).

The current index-wide implementation and release gates are tracked in
[`../../src/index/ROADMAP.md`](../../src/index/ROADMAP.md). Do not use this
test-folder roadmap to infer implementation readiness.
