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

`benchmarks/utils/CMakeLists.txt` adds `bench_utils_release_gates` from
`bench_utils_release_gates.cpp` when `THEMIS_BUILD_BENCHMARKS` is enabled. The
root benchmark CMake also adds and registers `bench_utils_benchmark_runner`.
The release-gates target measures configuration-object construction/copy and
`UtilsError` cast/switch dispatch.

`benchmark_runner.cpp` is separately built as `bench_utils_benchmark_runner`.
It now uses Google Benchmark registration and JSON output, and benchmarks the
actual `BloomFilterConfig`, `RetryPolicy`, and `UtilsError` declarations from
`include/utils/utils_api_contract.h`. These are narrow contract-value
microbenchmarks, not end-to-end utility-service measurements. Run it with
`--benchmark_out=<file>.json --benchmark_out_format=json`; do not treat these
results as subsystem release evidence.

The directory also contains `bench_pii_stream_scanner.cpp` and
`phase_a3_benchmark_runner.cpp`; neither is registered by the current directory
CMake file. The Phase A.3 runner simulates workload operations and is not
evidence for production-path performance. Its old measurements are documented
as historical, not as release gates, in
[`BENCHMARK_BASELINE.md`](BENCHMARK_BASELINE.md).

For current scope and remaining measurement gaps, see
[`../../src/utils/PERFORMANCE_EXPECTATIONS.md`](../../src/utils/PERFORMANCE_EXPECTATIONS.md).
