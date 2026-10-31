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

# rpc_grpc Performance Expectations

## Registered module benchmarks

`benchmarks/rpc_grpc/CMakeLists.txt` registers two targets when
`THEMIS_BUILD_BENCHMARKS` is enabled:

| Target | Workload | Evidence boundary |
|---|---|---|
| `bench_rpc_grpc_release_gates` | Error-enum conversion/dispatch, descriptor operations, and fail-closed predicate microbenchmarks (GATE-RPC-01..08) | Exercises contract types and helper logic, not a live RPC request path. Thresholds appear as source labels/comments; the benchmark does not assert them as pass/fail gates. |
| `bench_rpc_grpc_dedicated_gates` | Registration, stream-send, credential-reload, and service-call in-process stubs (GRPC-BM-01..04) | Does not invoke `GRPCServer`, load credentials, or connect to a gRPC service; measurements are simulations only. |

These benchmark definitions are not measurement results. No current run,
hardware baseline, or threshold comparison is recorded here. Do not describe a
target as passing based only on its source definition or a roadmap checkbox.
For a release claim, retain the exact revision, build options, host
configuration, raw benchmark output, and a comparison with an approved
representative-hardware baseline.

## Other gRPC benchmark sources

`benchmarks/server/bench_wal_apply_grpc.cpp` is a separate server benchmark;
it is not registered by `benchmarks/rpc_grpc/CMakeLists.txt`. Its workload must
not be attributed to the rpc_grpc module targets without verifying the source
and target relationship for the specific change.

## Validation

Use the benchmark-enabled repository CMake configuration and the targets above.
The expectations and source labels do not establish p95/p99 behavior for the
plugin listener, TLS reload, request dispatch, or stream adapter. Add measured
workloads and an automated baseline gate before making those claims.
