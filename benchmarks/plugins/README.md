---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Plugin benchmarks

When `THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is available,
`benchmarks/plugins/CMakeLists.txt` registers one target:
`bench_plugins_release_gates`, built from `bench_plugins_release_gates.cpp`.
Its four cases measure `PluginsError` enum casting, switch dispatch,
`PluginRegistrationDescriptor` allocation, and a 1,000-iteration batch cast.
The source labels the cases GATE-PLG-01 through GATE-PLG-04 with thresholds;
those labels are not evidence of a passing run or a configured regression
baseline.

`bench_plugin_system.cpp` and `bench_plugin_hot_plug.cpp` are registered by the
parent `benchmarks/CMakeLists.txt` as separate benchmark targets. They are not
part of `bench_plugins_release_gates`, which is registered by the local
`benchmarks/plugins/CMakeLists.txt`. Check the active CMake configuration
before interpreting any target as built or executed.

No build or benchmark was run for this documentation update. See
[`../../src/plugins/PERFORMANCE_EXPECTATIONS.md`](../../src/plugins/PERFORMANCE_EXPECTATIONS.md)
for the case mapping and evidence limits.
