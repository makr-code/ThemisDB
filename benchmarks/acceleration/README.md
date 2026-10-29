---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

# Acceleration benchmarks

Path: `benchmarks/acceleration`

## Purpose

This directory contains the focused benchmark coverage for the acceleration module, with emphasis on backend dispatch, fallback timing, resource contention, and hardware parity checks used to validate the runtime behavior described in `src/acceleration/` and `include/acceleration/`.

## Benchmark files

- `bench_acceleration_dispatch.cpp` — dispatch-path benchmark coverage
- `bench_acceleration_cuda_gates.cpp` — CUDA performance-gate validation
- `bench_acceleration_dedicated_gates.cpp` — dedicated acceleration gate coverage
- `bench_category_b_gpu_cpu_parity.cpp` — GPU/CPU parity benchmark harness

## Validation notes

- Benchmark claims remain environment-specific and must be interpreted against the host hardware, driver stack, and optional backend feature gates in the active build.
- CPU fallback and deterministic fail-closed behavior are the baseline safety guarantee; speedup claims should be treated as benchmark evidence, not universal runtime assertions.
- Benchmark outputs should be read alongside the module roadmap and future enhancements docs when evaluating production readiness.

## Related docs

- `../../src/acceleration/README.md`
- `../../src/acceleration/ARCHITECTURE.md`
- `../../src/acceleration/ROADMAP.md`
- `../../src/acceleration/FUTURE_ENHANCEMENTS.md`
