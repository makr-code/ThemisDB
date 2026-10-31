# Updates benchmarks

The update benchmark sources and their current CMake registration are in this
directory. Configuration is conditional on `THEMIS_BUILD_BENCHMARKS`.

Registered benchmark groups:

- Release/API contract microbenchmarks: `bench_updates_release_gates.cpp`
- Coordinated update path: `bench_updates_coordinated_hardening.cpp`
- Canary failure/rollback paths: `bench_updates_canary_resilience.cpp`
- Schema operations: `bench_updates_schema_diversity.cpp`
- Mixed long-run update workload: `bench_updates_long_run_stability.cpp`

`bench_updates_cluster_stress_q4.cpp` exists in the source tree but is not
registered in [`CMakeLists.txt`](CMakeLists.txt). Check the configured build
tree for available targets. A source file or declared counter is not evidence
of a completed run or a release threshold; see
[`../../src/updates/PERFORMANCE_EXPECTATIONS.md`](../../src/updates/PERFORMANCE_EXPECTATIONS.md).

## Usage

Enable `THEMIS_BUILD_BENCHMARKS` in the repository's CMake configuration,
build the registered target, and record its output with the host and build
profile. Exact generated target availability depends on the configured build
tree.

## Installation

These are in-tree benchmark sources, not separately installed executables.
