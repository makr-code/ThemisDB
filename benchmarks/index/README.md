---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Index benchmarks

`bench_index_rebuild.cpp` benchmarks secondary-index rebuilds using a RocksDB
fixture with 100,000 entities and regular, composite, range, sparse, geo, TTL,
and full-text indexes. The Google Benchmark cases and iteration settings are
defined in that source.

The root benchmark CMake configuration adds target `bench_index_rebuild` only
when benchmarks are enabled and the source file exists. The repository
`nightly-bench-sweep` configure/build preset enables the benchmark suite:

```sh
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep --target bench_index_rebuild
```

The fixture removes and recreates `bench_rebuild_db` in its working directory;
run it only from an isolated build/work directory where that path contains no
data to preserve. Benchmark results are environment-specific and are not
production latency guarantees.
