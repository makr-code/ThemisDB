---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base benchmarks

This directory contains benchmark sources with different scopes. This README
covers the two targets explicitly registered in `benchmarks/CMakeLists.txt` as
base release-gate executables:

| Source | CMake target | Coverage |
|---|---|---|
| `bench_base_hot_paths.cpp` | `bench_base_hot_paths` | GATE-BASE-01..06: loader queries, reload-manager queries, dependency graph construction, and reload fast-fail. |
| `bench_base_wasm_sandbox.cpp` | `bench_base_wasm_sandbox` | GATE-BASE-07..12: WASM validation/load, ABI checks, error-taxonomy queries, and inactive sandbox statistics. |

`benchmarks/CMakeLists.txt` adds these targets when their source files exist,
links them through `themis_core` and Google Benchmark, and registers them with the
benchmark CTest mechanism when benchmark registration is enabled. Gate labels
and target thresholds are benchmark definitions, not proof of measured
performance or representative-hardware sign-off. The current module roadmap
records hardware measurement/sign-off as pending.

The pool headers' implementation files are not included in the benchmark
coverage stated above, and their inclusion in the explicit `themis_core`
source list is unconfirmed. See [`../../src/base/PERFORMANCE_EXPECTATIONS.md`](../../src/base/PERFORMANCE_EXPECTATIONS.md).
