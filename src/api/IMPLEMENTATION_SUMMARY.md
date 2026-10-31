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

# API Module Implementation Summary

This is a source-oriented snapshot of the current API module. It replaces earlier wave reports and
test-count summaries; it does not assert release readiness or report benchmark results.

## Implemented surfaces

- **Shared HTTP policy:** `TransportPolicyMiddleware` decorates an `IHttpHandler`. It validates
  method/path presence, configured path and payload bounds, supported `X-API-Version` values, and
  content-type requirements for non-empty write requests. It only applies where installed in a
  handler chain.
- **Transport contracts and errors:** `ITransportContract` describes adapter capabilities,
  versions, and failure classification. `TransportContractValidator` and `ApiErrorTaxonomy` provide
  shared validation and transport error/status mapping.
- **GraphQL:** `Parser` builds a `Document`; `Executor` evaluates it with an `ExecutionContext` and
  resolver callbacks. Named fragment definitions/spreads and `@include`/`@skip` field directives
  are supported. Inline fragments and other directives are not supported by the current parser.
  `GraphQLAqlResolverFactory` provides AQL-backed resolvers and `GraphQLComplexityEstimator` maps
  document cost to query resource limits.
- **Optional protocols:** `GrpcApiServer` and `ThemisDBGrpcService` provide the gRPC server/service
  integration when built with `THEMIS_ENABLE_GRPC`. `WsChangeHandler` validates CDC WebSocket
  upgrades, while `GraphQLWsHandler` processes the `graphql-transport-ws` protocol when built with
  `THEMIS_ENABLE_WEBSOCKET`.
- **Observability:** `TracingMiddleware` manages correlation/span context; `OtlpExporter` queues and
  exports spans. Telemetry delivery is separate from the success/failure of the database operation.

## Configuration and limits

- The shared transport contract advertises API versions `v1` and `v2`, a 10 MiB payload maximum,
  and a 4096-byte path maximum. `TransportPolicyConfig` can lower its payload/path limits.
- `GrpcServerConfig::max_message_size_bytes` defaults to 100 MiB. The server reads an optional
  `grpc.max_message_size_mb` override from `config/networking/grpc.yaml`.
- `otlp_exporter.h` includes libcurl directly and has no `THEMIS_ENABLE_OTEL` guard. Optional
  Prometheus integration is guarded by `THEMIS_HAS_PROMETHEUS`.

## Focused tests

`tests/api/CMakeLists.txt` discovers `test_*.cpp`, creates a focused target for each source, and
registers each test with the `api` label. Relevant areas include transport contracts/policy,
degraded-mode behavior, gRPC lifecycle, interfaces, observability, security audit, version
handling, and stress/concurrency cases.

For a configured build directory, run an individual generated target, for example:

```sh
cmake --build <build-dir> --target module_api_test_api_contracts_focused
ctest --test-dir <build-dir> -L api --output-on-failure
```

The test source inventory and target-generation rule are maintained in
[`../../tests/api/CMakeLists.txt`](../../tests/api/CMakeLists.txt).

## Benchmarks

- `benchmarks/api/bench_api_transport.cpp` measures mock-handler transport/helper paths.
- `benchmarks/api/bench_api_release_gates.cpp` measures shared policy, contract validation, and
  error-taxonomy paths.
- `benchmarks/server/bench_api_endpoints.cpp` measures GraphQL, correlation, JSON/HTTP, and
  in-process HTTP endpoint cases.

See [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for case coverage, documented
budgets, and the known target-mapping discrepancy. No measured performance result is asserted here.

## Known limitations and open questions

- `QueryAllowList::isAllowed()` checks membership independently of the `enabled_` flag. The
  production request-enforcement call site has not been verified; do not treat the flag alone as
  an enforcement control.
- `IGRPCBridge` is an interface; no concrete implementation was found in the API module.
- A generated Doxygen snapshot exists, but its source paths and snapshot date are stale. See
  [`DOXYGEN.md`](DOXYGEN.md) before relying on its symbol inventory.
