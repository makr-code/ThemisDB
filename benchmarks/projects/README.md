---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Projects Benchmarks

`bench_projects_release_gates` is registered by this directory's
`CMakeLists.txt` when `THEMIS_BUILD_BENCHMARKS=ON`. Its current cases measure
the `ProjError` enum only:

- cast enum values to `int32_t`;
- switch dispatch over the four defined error values;
- check the numeric range;
- batch-cast 1,000 values.

Each case uses five repetitions. These microbenchmarks do not invoke project
storage or runtime APIs and are not project lifecycle, versioning, diff,
template, collaboration, or audit performance evidence.

To build the target, configure with `THEMIS_BUILD_BENCHMARKS=ON` and build
`bench_projects_release_gates`. No module-native runtime benchmark is currently
registered in this directory.

## Installation

This is an in-tree target; no separate benchmark package or installation step
is provided.

## Usage

Run the built executable with the desired Google Benchmark arguments, for
example `--benchmark_filter=BM_ProjError_Cast`.
