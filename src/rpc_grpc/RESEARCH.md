---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Research Notes

## Purpose and scope

This page records the engineering questions and evidence used to evaluate the
`src/rpc_grpc` transport plugin. It is for maintainers deciding whether a
transport behavior is implemented and tested. This module does not define RPC
business methods and does not perform language-model inference or vector
embedding.

## Questions and evidence

| Question | Current evidence | What the evidence establishes |
|---|---|---|
| Does plugin lifecycle reject invalid setup? | [`grpc_plugin.cpp`](grpc_plugin.cpp), [`tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp) | Initialization, start/stop, and contract edge cases have focused test sources; verify the configured test target before relying on a local run. |
| Does the stream adapter preserve handler delivery and finish behavior? | [`bidi_stream_adapter.h`](bidi_stream_adapter.h), [`tests/test_bidi_stream_adapter.cpp`](../../tests/test_bidi_stream_adapter.cpp) | Tests cover null streams, callbacks, finish status, writes, and queue depth using a mock stream; this does not establish network-level behavior. |
| Are method metrics and access logs available? | [`grpc_plugin.cpp`](grpc_plugin.cpp), [`tests/test_grpc_observability.cpp`](../../tests/test_grpc_observability.cpp) | `recordRPC`, `getMetricsText`, and the configured access-log sink are directly exercised. The source does not install an HTTP metrics endpoint. |
| Are transport performance claims reproducible? | [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`bench_rpc_grpc_release_gates.cpp`](../../benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp), [`bench_rpc_grpc_dedicated_gates.cpp`](../../benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp) | Benchmark definitions and thresholds are available; a benchmark source file alone is not evidence that a recent run met its threshold. |
| Does sustained operation remain stable? | [`tests/integration/test_rpc_grpc_soak.cpp`](../../tests/integration/test_rpc_grpc_soak.cpp), [`tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`](../../tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp) | Soak/stress scenarios exist for selected paths; do not generalize them to untested production workloads. |

## Evaluation method

1. Trace the behavior from `grpc_plugin.cpp` or `bidi_stream_adapter.h`.
2. Select the corresponding unit, integration, or stress test; unit mocks are
   not substitutes for a real gRPC listener test.
3. For performance claims, run the benchmark in the repository's configured
   release profile and retain the output and baseline comparison.
4. Record the exact revision, build options, gRPC/protobuf versions, and test
   result. Do not infer a passing result from roadmap checkboxes or old
   benchmark metadata.

The module-focused CMake targets are declared in
[`tests/rpc_grpc/CMakeLists.txt`](../../tests/rpc_grpc/CMakeLists.txt); top-level
gRPC and adapter tests are registered in [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt).
Use the build tree's CTest listing to identify enabled targets: gRPC-dependent
tests are conditional on `THEMIS_ENABLE_GRPC` and the gRPC package being found.

## Findings and limits

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

## Related pages

[`MODELING.md`](MODELING.md) describes the runtime model;
[`IMPLEMENTATION.md`](IMPLEMENTATION.md) and [`API.md`](API.md) describe the
current code paths and contracts; [`RUNBOOK.md`](RUNBOOK.md) covers diagnosis.
