---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Module Developer Guide

## Start with the owning contract

For an integration, begin at
[`include/themis/gpu/gpu_module.h`](../../include/themis/gpu/gpu_module.h)
and this module's [README](README.md) and
[architecture](ARCHITECTURE.md). Use [API.md](API.md) for facade behavior,
[IMPLEMENTATION.md](IMPLEMENTATION.md) for source ownership/invariants, and
[EMBEDDING.md](EMBEDDING.md) when working on vector or embedding consumers.
The headers, not this guide, define signatures and types.

## Build and test workflow

1. Check available CMake presets with `cmake --list-presets` and select the
   repository preset appropriate to the host and intended backend.
2. Configure with `THEMIS_ENABLE_GPU=ON` and `THEMIS_BUILD_TESTS=ON` for the
   GPU-enabled suite. CUDA, HIP, and Vulkan support have separate toolkit/SDK
   requirements; setting a request flag does not guarantee that the backend
   will remain enabled after dependency detection.
3. Build the resulting tree using the selected preset's build directory.
4. Run `ctest --test-dir <build-directory> -L gpu --output-on-failure`.
   The test registration in [`tests/gpu/CMakeLists.txt`](../../tests/gpu/CMakeLists.txt)
   skips GPU tests when `THEMIS_ENABLE_GPU` is off and intentionally excludes
   several suites that target a superseded error-handler API.
5. For a backend-specific change, verify the corresponding test and benchmark
   with the required SDK/device. A CPU-safe or stub-oriented test is not a
   substitute for hardware execution.

For a focused registered test, use its CTest name, which is derived from the
test source stem with `_gpu_FocusedTests`. Confirm registration with
`ctest -N --test-dir <build-directory>` rather than assuming a target exists
in a CPU-only configuration.

## Integration checklist

- Validate configuration and initialization results before submitting work.
- Pass the trusted caller and tenant identifiers used by the policy/quota
  contracts; do not use user-controlled labels as identity.
- Handle policy denial, allocation rejection, timeout, and missing-device
  results distinctly.
- Verify output correctness at the consuming API and expose whether the
  result was produced by a GPU path. `via_gpu == false` alone does not prove a
  CPU implementation ran; see [API.md](API.md).
- Keep diagnostics and operational behavior consistent with
  [RUNBOOK.md](RUNBOOK.md).

## When to update documentation

Change [ARCHITECTURE.md](ARCHITECTURE.md) for ownership/flow changes,
[API.md](API.md) for public contract changes, and
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) only when benchmark
mapping or measured release targets change. Keep future or unvalidated ideas
in [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md), not as current behavior.
