---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Updates benchmarks

The update benchmark sources are in this directory. Five benchmark targets are
registered by its `CMakeLists.txt`; the cluster-stress target is registered
separately by the repository-level [`benchmarks/CMakeLists.txt`](../CMakeLists.txt).
Both registrations are conditional on `THEMIS_BUILD_BENCHMARKS`.

Registered benchmark groups:

- Release/API contract microbenchmarks: `bench_updates_release_gates.cpp`
- Coordinated update path: `bench_updates_coordinated_hardening.cpp`
- Canary failure/rollback paths: `bench_updates_canary_resilience.cpp`
- Schema operations: `bench_updates_schema_diversity.cpp`
- Mixed long-run update workload: `bench_updates_long_run_stability.cpp`

`bench_updates_cluster_stress_q4.cpp` is registered by the repository-level
benchmark CMake file, which also adds a CTest smoke entry when the target is
available. It is not listed in this directory's `CMakeLists.txt`. Check the
configured build tree for target availability. Source-commented thresholds are
not approved release gates, and registration is not evidence of a completed run;
see
[`../../src/updates/PERFORMANCE_EXPECTATIONS.md`](../../src/updates/PERFORMANCE_EXPECTATIONS.md).

## Usage

Enable `THEMIS_BUILD_BENCHMARKS` in the repository's CMake configuration,
build the registered target, and record its output with the host and build
profile. Exact generated target availability depends on the configured build
tree.

## Installation

These are in-tree benchmark sources, not separately installed executables.
