---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7448"
Milestone: "v2.5.0 / v3.0.0 research and wave plan"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Research and Gap Analysis

## Purpose and scope

This page records the engineering questions and evidence used to evaluate the
`src/rpc_grpc` transport plugin. It is for maintainers deciding whether a
transport behavior is implemented and tested. This module does not define RPC
business methods and does not perform language-model inference or vector
embedding.

## Code and test evidence

| Question | Current evidence | What the evidence establishes |
|---|---|---|
| Does plugin lifecycle reject invalid setup? | [`grpc_plugin.cpp`](grpc_plugin.cpp), [`tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp) | Initialization, start/stop, and contract edge cases have focused test sources; verify the configured test target before relying on a local run. |
| Does the stream adapter preserve handler delivery and finish behavior? | [`bidi_stream_adapter.h`](bidi_stream_adapter.h), [`tests/test_bidi_stream_adapter.cpp`](../../tests/test_bidi_stream_adapter.cpp) | Tests cover null streams, callbacks, finish status, writes, and queue depth using a mock stream; this does not establish network-level behavior. |
| Are method metrics and access logs available? | [`grpc_plugin.cpp`](grpc_plugin.cpp), [`tests/test_grpc_observability.cpp`](../../tests/test_grpc_observability.cpp) | `recordRPC`, `getMetricsText`, and the configured access-log sink are directly exercised. The source does not install an HTTP metrics endpoint. |
| Are transport performance claims reproducible? | [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`bench_rpc_grpc_release_gates.cpp`](../../benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp), [`bench_rpc_grpc_dedicated_gates.cpp`](../../benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp) | Benchmark definitions and thresholds are available; a benchmark source file alone is not evidence that a recent run met its threshold. |
| Does sustained operation remain stable? | [`tests/integration/test_rpc_grpc_soak.cpp`](../../tests/integration/test_rpc_grpc_soak.cpp), [`tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`](../../tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp) | Soak/stress scenarios exist for selected paths; do not generalize them to untested production workloads. |

## State-of-the-art references

The following primary references inform the proposed work. They describe methods
and interoperability conventions; they do not prove that this module complies
with a standard or has achieved a performance result.

| Reference | Relevant finding | Roadmap implication |
|---|---|---|
| [gRPC Performance Best Practices](https://grpc.io/docs/guides/performance/) | Channel reuse and streaming can reduce repeated setup costs, but long-lived streams constrain load balancing and can create HTTP/2 concurrent-stream queueing. Measure the actual workload before selecting a design. | Compare unary and streaming paths across payload sizes and concurrency; do not turn microbenchmark labels into service-level claims. |
| [gRPC Benchmarking](https://grpc.io/docs/guides/benchmarking/) | Representative benchmarking separates client and server resources, uses defined scenarios, and records throughput/latency across configurations. | Use a live listener, fixed hardware and client capacity, declared workload parameters, repeated runs, and retained raw output. |
| [gRPC Deadlines](https://grpc.io/docs/guides/deadlines/) and [Flow Control](https://grpc.io/docs/guides/flow-control/) | Deadlines bound RPC lifetime; flow control coordinates streaming producers and consumers and can otherwise cause blocking. | Make cancellation, deadlines, backpressure, and queue bounds explicit and test slow-reader/blocked-operation cases. |
| [gRPC Graceful Shutdown](https://grpc.io/docs/guides/server-graceful-stop/) and [Health Checking](https://grpc.io/docs/guides/health-checking/) | Graceful shutdown needs a bounded drain period; serving health should reflect readiness instead of merely process existence. | Add a configurable shutdown deadline and externally consumable readiness only after its lifecycle semantics are specified. |
| [OpenTelemetry RPC metrics conventions](https://opentelemetry.io/docs/specs/semconv/rpc/rpc-metrics/) and [gRPC conventions](https://opentelemetry.io/docs/specs/semconv/rpc/grpc/) | Standardized RPC duration instruments and protocol/method attributes improve cross-system interpretation; attribute cardinality still needs control. | Replace ad hoc cumulative-latency-only output with compatible duration histograms and bounded attributes, preserving existing consumers through a reviewed migration. |
| [NIST SP 800-52 Rev. 2](https://csrc.nist.gov/pubs/sp/800/52/r2/final) | Provides TLS configuration and certificate guidance; NIST notes the publication is under review as of 2026-05-07. | Use current security policy and supported gRPC/OpenSSL capabilities to define TLS versions, certificate validation, and negative tests; do not treat this roadmap as a compliance attestation. |

## Code-to-research gap analysis

| Area | Source-backed current state | Gap and consequence |
|---|---|---|
| Performance and scaling | `GRPCServer::start` configures message-size limits and selected keepalive options. The registered benchmarks exercise contract operations or stubs, not the plugin listener; the high-cardinality stress test uses local stubs. | No reproducible live-server latency/throughput baseline exists in the inspected artifacts. The current source does not establish a safe concurrency or throughput envelope. |
| Usability and configuration | The plugin consumes host/port, TLS inputs, `auth_required`, two keepalive values, and `admin_port`. Generic connection, request-timeout, rate-limit, compression, and worker-count fields are not applied. | Callers can configure fields that do not affect runtime behavior; supported versus unsupported configuration needs a validated contract and explicit errors. |
| Streaming and resilience | `BidiStreamAdapter` uses a bounded item queue and synchronous `Write`; `finish()` wakes queue waiters but cannot interrupt an in-flight read/write. `stop()` has no plugin-level deadline. | Blocking, cancellation, queue sizing, write-result propagation, and bounded shutdown lack an end-to-end contract and live failure tests. |
| Security | TLS file presence/readability is checked and TLS reload validates files before a disruptive restart with rollback attempt. `admin_port` uses insecure credentials; TLS file readability does not itself prove certificate/key validity or policy compliance. | The insecure admin listener must be removed or explicitly protected by a secure design; certificate semantics and reload-under-load need negative and integration tests. |
| Observability and governance | Metrics are manually recorded, expose cumulative latency rather than histograms, and have no scrape endpoint or automatically installed request interceptor. Test/benchmark registration is conditional on build configuration. | Standard instrumentation, bounded dimensions, exact build/runtime evidence, and a release support matrix are missing. No test or benchmark result is claimed by this note. |

## Evaluation method

1. Trace the behavior from `grpc_plugin.cpp` or `bidi_stream_adapter.h`.
2. Select the corresponding unit, integration, or stress test; unit mocks are
   not substitutes for a real gRPC listener test.
3. For performance claims, run the benchmark in the repository's configured
   release profile and retain the output and baseline comparison.
4. Record the exact revision, build options, gRPC/Protobuf versions, hardware,
   workload, and raw test/benchmark result. Do not infer a passing result from
   roadmap checkboxes or old benchmark metadata.

The module-focused CMake targets are declared in
[`tests/rpc_grpc/CMakeLists.txt`](../../tests/rpc_grpc/CMakeLists.txt); top-level
gRPC and adapter tests are registered in [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt).
Use the build tree's CTest listing to identify enabled targets: gRPC-dependent
tests are conditional on `THEMIS_ENABLE_GRPC` and the gRPC package being found.

## Assumptions and knowledge gaps

- The plugin wrapper reports streaming and batching capabilities, but the
  capability flags do not prove that every service implements either behavior.
- `GRPCServer::recordRPC` is a call surface; the source does not itself wire an
  interceptor that automatically records every request.
- `BidiStreamAdapter::write` performs synchronous stream writes. Its tests use
  a mock stream and cannot measure network backpressure or cancellation.
- `include/rpc_grpc/rpc_grpc_api_contract.h` describes a normative contract,
  while the implementation is the evidence for present runtime behavior.
  Keep the two aligned when making future changes.
- Performance thresholds in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
  are acceptance targets, not a statement of current measured latency.
- No production workload profile, supported concurrency target, per-release
  hardware baseline, TLS policy profile, compliance target, or operator SLO was
  supplied with issue #7448. Numerical release budgets in
  [`ROADMAP.md`](ROADMAP.md) are therefore proposed validation gates, not
  measured guarantees; owners must approve them against representative workload
  and deployment evidence.
- The standards and vendor guidance above evolve. Recheck their current
  revisions and the project's approved cryptographic policy when implementation
  begins; SP 800-52 Rev. 2 is explicitly under review.

## Related pages

[`MODELING.md`](MODELING.md) describes the runtime model;
[`IMPLEMENTATION.md`](IMPLEMENTATION.md) and [`API.md`](API.md) describe the
current code paths and contracts; [`RUNBOOK.md`](RUNBOOK.md) covers diagnosis.
