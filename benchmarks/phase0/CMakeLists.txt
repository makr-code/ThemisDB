# ThemisDB Phase-0 Benchmarks — CMakeLists.txt
#
# Phase-0 baseline performance suite for core CRUD operations.
# Establishes initial performance baselines for release-readiness planning.
#
# Benchmark suites:
#   bench_p0_crud_baseline  — P0-D04: Insert-heavy, read-heavy, update-heavy, delete-heavy
#
# All targets use themis_add_standard_benchmark() and inherit the standard
# benchmark link libraries (themis_core, Google Benchmark, RocksDB, etc.).
# The phase0_fixtures.h header is made available via a dedicated include
# directory so downstream additions do not need to duplicate the path.

cmake_minimum_required(VERSION 3.20)

# Expose phase0 fixture header to all phase0 benchmark targets.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# P0-D04: Phase-0 CRUD Baseline Benchmarks
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_p0_crud_baseline.cpp")
    themis_add_standard_benchmark(bench_p0_crud_baseline bench_p0_crud_baseline.cpp)
    message(STATUS "  Phase-0 CRUD Baseline: Insert-heavy, read-heavy, update-heavy, delete-heavy (P0-D04)")
else()
    message(WARNING "  bench_p0_crud_baseline.cpp not found; skipping Phase-0 baseline benchmarks")
endif()
