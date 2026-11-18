---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-18"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Performance Expectations

## Benchmark sources

| Source | Coverage | Limitations |
|---|---|---|
| [`../../benchmarks/api/bench_api_transport.cpp`](../../benchmarks/api/bench_api_transport.cpp) | Mock-handler transport, response construction, validation, correlation-ID, and sequential-request microbenchmarks | Measures the adapter/helper path, not a running HTTP server |
| [`../../benchmarks/api/bench_api_release_gates.cpp`](../../benchmarks/api/bench_api_release_gates.cpp) | `TransportPolicyMiddleware`, `ApiErrorTaxonomy`, and `TransportContractValidator` cases (`GATE-API-01..06`) | Source comments state target budgets; benchmark execution alone does not establish that a CI baseline passed |
| [`../../benchmarks/server/bench_api_endpoints.cpp`](../../benchmarks/server/bench_api_endpoints.cpp) | GraphQL parse/execute, correlation IDs, JSON serialization/deserialization, HTTP message construction, and in-process endpoint roundtrips | Server endpoint cases require their linked runtime dependencies and a usable RocksDB environment |

The endpoint benchmark target is registered in `benchmarks/server/CMakeLists.txt`.
The two API-local targets are registered by `benchmarks/CMakeLists.txt`.
All are available only when `THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is found.

## Current cases

The endpoint benchmark source currently defines the following families:

- GraphQL parse: simple uncached, simple cached, complex uncached, and invalid input.
- GraphQL execution: parse plus execution with a mock resolver.
- Correlation ID: UUIDv4 generation and existing-header check.
- JSON and HTTP: single-document/paginated serialization, PUT deserialization, and GET/POST message construction.
- HTTP endpoints: health check and persistent document-list roundtrips.
- Shared transport: GET/POST handling, malformed-request validation, response/header construction, and version negotiation.
- Transport release gates: valid/rejected policy requests, error-taxonomy conversion, contract validation, and sustained/mixed request loops.

## Mapping caveat

`benchmarks/benchmark_target_mapping.json` currently assigns several API target labels to
benchmark cases that do not measure those labels (for example, the WebSocket-subscription,
concurrent-WebSocket, bulk-insert, and OTLP-flush labels map to JSON serialization or GraphQL
parsing cases). Do not interpret those entries as direct measurements of the labeled behavior.

TODO: Reconcile the API target labels and primary benchmark cases in the mapping file before using
those IDs as release evidence. That JSON file is outside this documentation update's requested
Markdown scope.

## Documented target budgets

`bench_api_release_gates.cpp` documents the following per-operation targets:

| Gate | Case family | Commented target |
|---|---|---:|
| GATE-API-01 | Valid GET policy handling | ≤ 5 µs/op |
| GATE-API-02 | Valid POST policy handling | ≤ 10 µs/op |
| GATE-API-03 | Oversized-payload rejection | ≤ 5 µs/op |
| GATE-API-04 | Unsupported-version rejection | ≤ 5 µs/op |
| GATE-API-05 | Error-code/HTTP-status mapping | ≤ 1 µs/op |
| GATE-API-06 | Contract validation | ≤ 5 µs/op |

These are targets documented beside the benchmark cases, not a report of measured results. Compare
benchmark output against the configured release baseline and environment-specific acceptance gate
before claiming a performance pass.

## Release validation protocol

The v2.5.0/v3.0.0 roadmap does not treat the table above as a pass. Before release sign-off:

1. Reconcile every API entry in `benchmarks/benchmark_target_mapping.json` with the benchmark case
   it measures. Do not use mismatched labels as evidence.
2. Configure a test-enabled release build with `THEMIS_BUILD_BENCHMARKS=ON`, then build and run the
   focused API tests:

   ```sh
   cmake --build <build-dir> --target \
     module_api_test_api_contracts_focused \
     module_api_test_api_phase4_concurrency_focused \
     module_api_test_api_degraded_mode_focused \
     module_api_test_api_otlp_exporter_config_focused
   ctest --test-dir <build-dir> \
     -R '^test_api_(contracts|phase4_concurrency|degraded_mode|otlp_exporter_config)_api_FocusedTests$' \
     --output-on-failure
   ```

3. Run mapped microbenchmarks and representative endpoint tests on pinned, recorded hardware.
   For offered-load tests, use at least three rates up to measured saturation and repeat each
   profile five times. Preserve the command, commit, compiler/build flags, host, raw output,
   offered load, p50/p95/p99 latency, throughput, and error rate.
4. Compare p95 and p99 with a dated baseline for the same workload and environment; flag a
   regression greater than 5%. A benchmark target's success or source-comment budget is not
   sufficient without the retained result and reviewed baseline.
5. Run correctness, security, and route-wiring checks separately. These microbenchmarks do not
   establish production authentication, middleware installation, optional-transport availability,
   overload recovery, or live collector behavior.

No measured API baseline or passing release result is recorded by this document. Keep the
performance gate `[!] unverified` until the evidence bundle is attached to the release review.
