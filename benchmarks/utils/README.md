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

# Utils Benchmarks

`CMakeLists.txt` currently registers only `bench_utils_release_gates` from
`bench_utils_release_gates.cpp`, and only when `THEMIS_BUILD_BENCHMARKS` is
enabled. That Google Benchmark target measures configuration-object
construction/copy and `UtilsError` cast/switch dispatch.

The directory also contains `bench_pii_stream_scanner.cpp`,
`benchmark_runner.cpp`, and `phase_a3_benchmark_runner.cpp`; none is registered
by the current directory CMake file. The Phase A.3 runner simulates workload
operations and is not evidence for production-path performance. Its old
measurements are documented as historical, not as release gates, in
[`BENCHMARK_BASELINE.md`](BENCHMARK_BASELINE.md).

For current scope and remaining measurement gaps, see
[`../../src/utils/PERFORMANCE_EXPECTATIONS.md`](../../src/utils/PERFORMANCE_EXPECTATIONS.md).
