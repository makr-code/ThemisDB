# Distributed Tensor Module - Future Enhancements

**Last Updated:** 2026-11-23
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `include/distributed_tensor/graph_summary_projection.h`, `include/distributed_tensor/shard_rpc_fragment_fetcher.h`, `include/sharding/shard_artifact_fragment_store.h`, `include/sharding/shard_rpc_server.h`, `src/main_server.cpp`, `proto/sharding/shard_rpc.proto`, `tests/sharding/CMakeLists.txt`, `tests/sharding/test_shard_transfer_store.cpp`, `tests/sharding/test_shard_transfer_grpc.cpp`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `PERFORMANCE_EXPECTATIONS.md`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

> The EPIC 3 phase-5 source paths below exist and are configured by
> `benchmarks/epic3_distributed_tensor/CMakeLists.txt` when benchmarks are
> enabled. Their numeric targets are criteria, not measured outcomes. The
> separate `bench_distributed_tensor_dedicated_gates.cpp` uses in-process
> stubs and is not production performance evidence. See
> [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Scope

- evolve EPIC 3 contracts into production distributed tensor behavior
- harden resiliency and integrity guarantees across distributed lifecycle paths
- promote only from deterministic benchmark and fault-test evidence

## Design Constraints

- preserve compatibility across EPIC 1/2/3 dependency boundaries
- maintain explicit integrity/recovery semantics under failure conditions
- keep distributed behavior diagnosable and bounded under degraded states

## Required Interfaces

- placement, planner, integrity, recovery, and infrastructure runtime APIs must emit benchmarkable metrics
- planned EPIC 3 benchmark targets should remain mapped to their workload sources in `benchmarks/epic3_distributed_tensor/`
- result bundles must satisfy the gate schema referenced by `release_gate_manifest_epic3.json`
- exact fragment fetches must bind peer, shard, artifact, manifest SHA-256, deadline, and byte ceilings; the optional ShardRPC adapter provides this transport contract when gRPC and sharding are enabled
- the host now has a versioned `(principal_id, shard_id, artifact_id)` RocksDB mapping and opt-in mTLS server startup; publishers must persist bytes before publishing the matching manifest and emit the documented adapter-similarity projection when graph validation is required

## Implementation Notes

- phase-5 metadata now defines canonical seed `42`, representative topologies, and gate identities
- runtime benchmark sources should be added only alongside the corresponding distributed tensor implementation sources
- phase-6 acceptance documentation must consume recorded pass/fail outputs instead of narrative-only status claims
- `ShardArtifactFragmentStore` owns immutable, principal-scoped payload persistence with synchronous WAL commits, and `main_server.cpp` wires it to `ShardRPCServer` only when explicitly enabled. The adapter verifies fetched bytes against the manifest digest; automatic producer/manifest publication remains unimplemented, and this source has not been built or run
- `GraphSummaryProjectionCodec` defines the bounded `TSPJ` version-1 adapter-similarity payload and the planner merges decoded candidates before `GraphTruthValidator`. Unknown or invalid formats remain fail-closed; arbitrary tensor weights are not projectable through this contract

## Test Strategy

- extend the configured tests in `tests/epic3_distributed_tensor/` for contract, degraded-mode, and fault-path scenarios
- keep integrity and recovery tests aligned with the benchmark profile topology assumptions
- require isolated reruns for any failed benchmark gate before thresholds are reconsidered

## Performance Targets

- planner latency p99 `<= 750 µs`
- placement throughput `>= 25,000 ops/s`
- integrity verification success ratio `>= 0.999`
- degraded rebuild convergence `<= 30,000 ms`
- control-plane degraded availability ratio `>= 0.99`
- variance ceiling `<= 5.0%`

## Security / Reliability

- integrity regressions must be visible in both test and benchmark evidence
- degraded-mode behavior must never be promoted without explicit gate outcomes
- acceptance remains blocked until runtime evidence is attached for every Phase 5 profile
