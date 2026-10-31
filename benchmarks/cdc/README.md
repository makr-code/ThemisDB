---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "benchmarks/CMakeLists.txt; benchmarks/cdc/bench_cdc_pipeline.cpp; benchmarks/cdc/bench_cdc_delivery_gates.cpp; benchmarks/cdc/bench_changefeed_throughput.cpp"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC benchmarks

The CDC benchmark sources cover consumer-group pipeline operations, delivery
gates, and changefeed throughput:

| Source | CMake target | Coverage |
|---|---|---|
| `bench_cdc_pipeline.cpp` | `bench_cdc_pipeline` | Group lifecycle, fetching, acknowledgement, concurrent fetch |
| `bench_cdc_delivery_gates.cpp` | `bench_cdc_delivery_gates` | Delivery tracking, acknowledgements, group operations, replay gates CDG-01..06 |
| `bench_changefeed_throughput.cpp` | Not registered in `benchmarks/CMakeLists.txt` | Event-type mix, burst writes, replication lag, event record/list latency |

The first two targets are registered conditionally in
[`../CMakeLists.txt`](../CMakeLists.txt) when the corresponding source exists.
Configure a supported repository preset with `THEMIS_BUILD_BENCHMARKS=ON`,
then build the desired target from that preset's binary directory. Presets
that set the option to `OFF` must be overridden at configure time.

Benchmark results depend on the build profile, RocksDB configuration, host
hardware, and run conditions. A source file or declared threshold alone does
not establish a measured baseline or release-gate pass. See
[`../../src/cdc/PERFORMANCE_EXPECTATIONS.md`](../../src/cdc/PERFORMANCE_EXPECTATIONS.md)
for the source-to-expectation mapping.

**Exact model backend not verifiable from final document text alone.**
