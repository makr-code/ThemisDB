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

# RPC gRPC Future Enhancements

## Scope

This backlog covers the optional plugin runtime in `src/rpc_grpc/`, its public
headers in `include/rpc_grpc/`, focused tests in `tests/rpc_grpc/`, and module
benchmarks in `benchmarks/rpc_grpc/`. The public headers are not generated
service stubs. The built-in API gRPC server in `src/api/` is separate.

The source-backed rationale, external technical references, open assumptions,
wave sequencing, effort ranges, and proposed success criteria are maintained in
the [module roadmap](ROADMAP.md) and [research/gap analysis](RESEARCH.md).

## Design Constraints

- Keep the public contract header distinct from `IRPCServer` and distinguish
  intended contract types from behavior actually enforced by the runtime.
- Make supported configuration, lifecycle errors, ownership, deadlines,
  cancellation, and shutdown behavior explicit. Reject unsupported non-default
  values rather than silently suggesting that they are applied.
- Avoid public signature or ABI changes until plugin callers and migration
  impact have been reviewed. v3.0.0 is a planning target, not blanket approval
  for compatibility shims.
- Never silently fall back to plaintext. The current optional admin listener
  uses insecure credentials; any replacement must satisfy approved security
  policy and be disabled unless explicitly configured.
- Treat all latency, throughput, and soak thresholds as proposed gates until
  validated against a named workload and representative hardware.

## Required Interfaces and Work

| Target | Required work | Primary code/test surface |
|---|---|---|
| v2.5.0 Wave 1 | Live RPC workload and repeatable p50/p95/p99, throughput, error, CPU/RSS baseline with exact host/build/dependency metadata. | `benchmarks/rpc_grpc/`, `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp` |
| v2.5.0 Wave 2 | Validated supported-config contract, programmatic lifecycle/registration outcomes, bounded shutdown, and cancellation/backpressure semantics. | `src/rpc_grpc/grpc_plugin.{h,cpp}`, `src/rpc_grpc/bidi_stream_adapter.h`, `include/plugins/rpc_plugin_interface.h` |
| v2.5.0 Wave 3 | Fail-closed credential tests, secure admin-listener policy, readiness, and automatically wired RPC duration telemetry with bounded attributes. | `src/rpc_grpc/grpc_plugin.cpp`, `tests/rpc_grpc/`, `include/rpc_grpc/` |
| v3.0.0 Wave 4 | Measured architecture comparison and documented synchronous-versus-async decision. Any alternative prototype must remain test/benchmark-only unless separately approved for production. | `benchmarks/rpc_grpc/`, `tests/rpc_grpc/`, `src/rpc_grpc/ROADMAP.md` |
| v3.0.0 Wave 5 | Versioned runtime contract and explicit compatibility/migration mapping for configuration, errors, lifecycle, streams, and credentials. | `include/plugins/rpc_plugin_interface.h`, `include/rpc_grpc/`, module contract tests |
| v3.0.0 Wave 6 | Workload-backed admission/resource limits, sustained/fault tests, and operator/release evidence; compliance evidence only for selected frameworks. | `src/rpc_grpc/`, `tests/rpc_grpc/`, `benchmarks/rpc_grpc/`, module runbooks |

No public-header enhancement in this backlog is an unconditional API commitment.
In particular, `RpcServiceDescriptor` and `CircuitBreaker` must not be described
as runtime-enforced/automatically wired unless their implementation and tests
demonstrate that behavior.

## Test Strategy

- Extend the current focused unit tests for invalid configuration, error
  outcomes, queue limits, cancellation, and finish/write behavior.
- Run real loopback gRPC integration tests for lifecycle, unary/stream traffic,
  TLS/mTLS, credential reload/rollback, readiness, and bounded shutdown.
- Keep stub stress cases clearly labeled as stub-only. They do not substitute
  for live transport, backpressure, client disconnect, or resource-exhaustion
  tests.
- For each release profile, record exact revision, enabled build options,
  gRPC/Protobuf versions, platform, test names/results, and any skipped
  gRPC-dependent cases.
- Run security-negative tests for invalid/mismatched credentials and ensure
  plaintext admin access is not enabled by default.

## Performance Targets

- First establish a representative per-host live-server baseline; do not infer
  it from the current contract and in-process benchmark targets.
- The proposed post-baseline gate is no more than 10% p95/p99 regression at the
  same workload, with no unexplained nominal-load errors. Confirm the budget
  with the workload owner before treating it as release blocking.
- Preserve raw benchmark output and repeat runs. If repeat-to-repeat variance
  exceeds 5%, investigate the environment before accepting the baseline.
- Compare any v3.0.0 transport architecture on identical hardware, build,
  payload, concurrency, and client capacity. Retain the incumbent unless a
  representative scaling objective improves without violating the agreed
  p99/throughput limits.

## Security / Reliability

- Fail closed on invalid TLS material and avoid plaintext fallback; validate
  certificate, key, CA, and reload/rollback behavior with live gRPC tests.
- Bound waiting, in-flight work, queue capacity, and server drain by explicit
  contracts, while documenting which calls can be terminated at a deadline.
- Keep metric attributes bounded and avoid logging credentials, payloads, or
  unbounded caller-supplied values.
- Exercise reload and shutdown during sustained traffic and retain failure
  results with the exact source/build configuration.
- See the [security boundary](SECURITY.md), [operations guide](OPERATIONS.md),
  [module roadmap](ROADMAP.md), [focused test guide](../../tests/rpc_grpc/README.md),
  and [benchmark guide](../../benchmarks/rpc_grpc/README.md).
