---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Benchmarks

This directory contains microbenchmarks for API transport and shared policy code. It does not
contain the API endpoint roundtrip benchmark; that source is
[`../server/bench_api_endpoints.cpp`](../server/bench_api_endpoints.cpp).

| Source | Target | Coverage |
|---|---|---|
| `bench_api_transport.cpp` | `bench_api_transport` | Mock-handler request handling, response/header construction, malformed-request validation, correlation-ID propagation, header lookup, and sequential requests |
| `bench_api_release_gates.cpp` | `bench_api_release_gates` | Shared transport-policy checks, error-taxonomy mapping, contract validation, and sustained/mixed request loops |

Both targets are registered in [`../CMakeLists.txt`](../CMakeLists.txt) when
`THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is available. The gate case targets and
the benchmark-to-target mapping caveat are documented in
[`../../src/api/PERFORMANCE_EXPECTATIONS.md`](../../src/api/PERFORMANCE_EXPECTATIONS.md).
