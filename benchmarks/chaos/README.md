---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos benchmark sources

This directory contains benchmark source files for `src/chaos`. Source presence
does not establish that a benchmark was built, run, or passed its emitted
thresholds.

| Source | CMake target | Build wiring status |
|---|---|---|
| `bench_chaos_stress.cpp` | `bench_chaos_stress` | **TODO:** `benchmarks/CMakeLists.txt` tests for `bench_chaos_stress.cpp` at the benchmark root, but the file is under `chaos/`; the condition is false for this checkout, so this target is not added. |
| `bench_chaos_release_gates.cpp` | `bench_chaos_release_gates` | Added when this source exists; CMake also registers the target with benchmark CTest registration. |
| `bench_chaos_microbenchmarks.cpp` | `bench_chaos_microbenchmarks` | Added when this source exists; CMake also registers the target with benchmark CTest registration. |

The benchmark source files declare scenarios and counter thresholds. Successful
measurements, baseline comparisons, and threshold enforcement remain TODO until
verified from actual benchmark runs and CI configuration.

Canonical evidence: [`../../benchmarks/CMakeLists.txt`](../CMakeLists.txt),
[`../../src/chaos/chaos_framework.cpp`](../../src/chaos/chaos_framework.cpp),
and the three source files in this directory.
