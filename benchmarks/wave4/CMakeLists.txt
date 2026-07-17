# ThemisDB Wave 4 Benchmarks — CMakeLists.txt
# Release-Readiness, Performance Governance and Reproducible Long-term Stability
#
# Benchmark suites:
#   bench_w4a_release_gates  — B4-A: release-critical governance workloads
#   bench_w4b_resilience     — B4-B: resilience/degradation scenarios
#   bench_w4c_determinism    — B4-C: determinism, variance control, flake reduction
#   bench_w4d_diagnostics    — B4-D: diagnostics, reporting, maintainability
#
# All targets use themis_add_standard_benchmark() and inherit the standard
# benchmark link libraries (themis_core, Google Benchmark, RocksDB, etc.).
# The wave4_fixtures.h header is made available via a dedicated include
# directory so downstream additions do not need to duplicate the path.

cmake_minimum_required(VERSION 3.20)

# Expose wave4 fixture header to all wave4 benchmark targets.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# B4-A: Release-Critical Benchmark Governance
# ----------------------------------------------------------------------------
themis_add_standard_benchmark(bench_w4a_release_gates bench_w4a_release_gates.cpp)

# ----------------------------------------------------------------------------
# B4-B: Resilience / Degradation Performance Scenarios
# ----------------------------------------------------------------------------
themis_add_standard_benchmark(bench_w4b_resilience bench_w4b_resilience.cpp)

# ----------------------------------------------------------------------------
# B4-C: Determinism, Variance Control & Flake Reduction
# ----------------------------------------------------------------------------
themis_add_standard_benchmark(bench_w4c_determinism bench_w4c_determinism.cpp)

# ----------------------------------------------------------------------------
# B4-D: Diagnostics, Reporting & Maintainability
# ----------------------------------------------------------------------------
themis_add_standard_benchmark(bench_w4d_diagnostics bench_w4d_diagnostics.cpp)
