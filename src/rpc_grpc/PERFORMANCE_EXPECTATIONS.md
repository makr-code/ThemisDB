# PERFORMANCE_EXPECTATIONS - src/rpc_grpc

## Scope

- Module: src/rpc_grpc
- This file defines measurable rpc_grpc module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_wal_apply_grpc.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| RPCP-1 | gRPC WAL apply throughput remains bounded for raw payload paths | WalGrpcApplyFixture/ApplyWalBatch (Args: 1-0, 10-0, 50-0) |
| RPCP-2 | gRPC WAL apply throughput remains bounded for compressed payload paths | WalGrpcApplyFixture/ApplyWalBatch (Args: 1-1, 10-1, 50-1) |
| RPCP-3 | batched apply service path remains stable under iteration/reset cycles | WalGrpcApplyFixture/ApplyWalBatch (all registered args) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| RPCG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| RPCG-2 | rpc_grpc hot-path p99 <= release threshold | p99 from mapped wal-apply gRPC benchmark cases |
| RPCG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional rpc_grpc benchmark scenarios are introduced.

## Sourcecode Verification (Module: rpc_grpc/performance)

- Verified benchmark sources:
  - benchmarks/bench_wal_apply_grpc.cpp
- Verified mapping surfaces:
  - WAL apply gRPC raw/compressed and batch stability behavior
- Result:
  - Referenced benchmark case exists in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.