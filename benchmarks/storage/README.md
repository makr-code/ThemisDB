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
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "CMakeLists.txt"
  - "../../CMakePresets.json"
  - "bench_storage_performance.cpp"
  - "bench_storage_release_gates.cpp"
  - "bench_storage_dedicated_gates.cpp"
  - "../../src/storage/PERFORMANCE_EXPECTATIONS.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage benchmarks

This directory contains storage-related `bench_*.cpp` sources. See
[`CMakeLists.txt`](CMakeLists.txt) for build conditions and registration; it
does not register every local source through the same path.

The local CMake file requires `THEMIS_BUILD_BENCHMARKS` and
`benchmark::benchmark`. It excludes `bench_crud.cpp`, `bench_mvcc.cpp`, and
`bench_wal_stress.cpp` from its local target loop to preserve their existing
top-level benchmark target names. `bench_storage_performance.cpp` contains the
allocator/memory/RCU cases; `bench_storage_release_gates.cpp` and
`bench_storage_dedicated_gates.cpp` contain the in-process gate harnesses.

The release and dedicated gate harnesses use in-process mock/stub components
and do not measure real RocksDB or disk I/O. Their declared targets are not
evidence of measured results or production thresholds. See
[`../../src/storage/PERFORMANCE_EXPECTATIONS.md`](../../src/storage/PERFORMANCE_EXPECTATIONS.md)
for the source-to-case mapping and remaining validation.

## Preset caveat

`linux-release` is the defined Linux release preset; it requires Linux,
GCC 11+, Ninja, and a bootstrapped repository-local vcpkg checkout, and it
sets `THEMIS_BUILD_BENCHMARKS=OFF`. `community-debug` is a defined
system-package preset that enables benchmarks and requires the platform
development packages. Google Benchmark must also be available for these
targets; if it is missing, this directory's CMake configuration skips them.
Do not use `linux-ninja-release` or a generic `release` preset name; neither
is defined in `CMakePresets.json`.
