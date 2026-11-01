# Geo benchmarks

This directory contains geo benchmark sources. They are not a mirror of
`src/geo/`; target registration is described in
`benchmarks/geo/CMakeLists.txt`.

The geo CMake file globs `bench_*.cpp` when benchmarks and Google Benchmark
are available. `bench_spatial_index.cpp` and `bench_spatial_join.cpp` are
excluded there because their existing target registration is retained through
the top-level benchmark configuration.

See `src/geo/PERFORMANCE_EXPECTATIONS.md`
for the benchmark map and interpretation limits. Benchmark presence or
registration is not evidence that a target was built, run, or passed a
performance threshold.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
