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

# Plugin benchmark mapping and evidence limits

## Registered target

With `THEMIS_BUILD_BENCHMARKS` enabled, `benchmarks/plugins/CMakeLists.txt`
registers only `bench_plugins_release_gates` from
`bench_plugins_release_gates.cpp`. The benchmark implementation contains:

| Case | Work measured | Source label/target |
|---|---|---|
| `BM_PLG01_ErrorEnumCast` | Cast a `PluginsError` value | GATE-PLG-01, p99 label ≤ 5 ns |
| `BM_PLG02_SwitchDispatch` | Switch dispatch over error values | GATE-PLG-02, p99 label ≤ 10 ns |
| `BM_PLG03_StructAlloc` | `PluginRegistrationDescriptor` allocation | GATE-PLG-03, p99 label ≤ 500 ns |
| `BM_PLG04_BatchCast` | Batch error casts (1,000 iterations) | GATE-PLG-04, p99 label ≤ 5 μs/batch |

These are source comments/benchmark labels, not evidence that the thresholds
are enforced by the benchmark runner or currently met. No benchmark was run as
part of this documentation update; no baseline or observed latency is reported.

## Other source files

`benchmarks/plugins/bench_plugin_system.cpp` and
`benchmarks/plugins/bench_plugin_hot_plug.cpp` are registered as separate
targets by the parent `benchmarks/CMakeLists.txt`. They are not part of
`bench_plugins_release_gates`, which is registered by
`benchmarks/plugins/CMakeLists.txt`. Source and target registration do not
establish that a target was built or executed. The ethics-AI benchmarks are in
a different benchmark directory and are not plugin runtime measurements.

## Validation requirements

Before using any benchmark as a release gate, identify its built target,
benchmark cases, build profile, baseline artifact, comparison rule, and
machine/environment. Record actual results separately from source labels.

**TODO:** No verified end-to-end plugin load/unload/reload or hot-plug
performance threshold and no current release baseline were found in the
registered plugin benchmark configuration. The dedicated system and hot-plug
benchmark targets require inspection and execution before their results can
support a release claim.
