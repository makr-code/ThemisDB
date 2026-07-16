### Context

This issue implements the roadmap item '`OrphanDetector`: Wire to `DistributedCoordinator` Transaction List' for the sharding domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `OrphanDetector`: Wire to `DistributedCoordinator` Transaction List

### Goal

Deliver the scoped changes for `OrphanDetector`: Wire to `DistributedCoordinator` Transaction List in src/sharding/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `OrphanDetector`: Wire to `DistributedCoordinator` Transaction List
**Priority:** Medium
**Target Version:** v1.8.0

`orphan_detector.cpp` lines 50 and 67 have: "TODO: Access coordinator's transactions" and "TODO: Get transaction from coordinator". The orphan detector cannot inspect the coordinator's in-flight transaction list, making orphan detection non-functional.

**Implementation Notes:**
- `[ ]` Inject `DistributedCoordinator*` into `OrphanDetector` constructor; call `coordinator->listInFlightTransactions()` at line 50 to get the authoritative transaction list.
- `[ ]` At line 67: call `coordinator->getTransaction(txn_id)` to fetch transaction metadata.

---


**Priority:** High
**Target Version:** v0.9.0

Complete the RPC integration between `shard_rpc_client.cpp` / `shard_rpc_server.cpp` and the mTLS transport layer (`mtls_client.cpp`, `mtls_connection_pool.cpp`). Add automatic retry with exponential backoff and circuit-breaker integration (`circuit_breaker.cpp`) for all cross-shard RPC calls.

**Implementation Notes:**
- Implement `Shard RpcRetryPolicy` in `shard_rpc_client.cpp` using the existing `circuit_breaker.cpp` interface; classify gRPC status codes into retryable (`UNAVAILABLE`, `DEADLINE_EXCEEDED`) and non-retryable (`INVALID_ARGUMENT`, `ALREADY_EXISTS`).
- Wire `mtls_connection_pool.cpp` into `shard_rpc_client.cpp` to reuse TLS sessions; max pool size should be configurable via `gossip_config_manager.cpp`.
- Instrument every RPC call via `operational_metrics.cpp` and `prometheus_metrics.cpp` with labels for `shard_id`, `method`, and `outcome`.
- Certificate rotation events from `utils/pki_client.cpp` must trigger a graceful connection drain in `mtls_connection_pool.cpp` without dropping in-flight requests.

**Performance Targets:**
- Cross-shard RPC P99 latency (LAN): <5 ms excluding consensus overhead.
- Connection pool hit rate: >95% under sustained 10k RPS cross-shard traffic.
- Circuit-breaker open-to-half-open recovery time: configurable, default 5 s.

---

### Acceptance Criteria

- [ ] Inject `DistributedCoordinator*` into `OrphanDetector` constructor; call `coordinator->listInFlightTransactions()` at line 50 to get the authoritative transaction list.
- [ ] At line 67: call `coordinator->getTransaction(txn_id)` to fetch transaction metadata.
- [ ] Implement `Shard RpcRetryPolicy` in `shard_rpc_client.cpp` using the existing `circuit_breaker.cpp` interface; classify gRPC status codes into retryable (`UNAVAILABLE`, `DEADLINE_EXCEEDED`) and non-retryable (`INVALID_ARGUMENT`, `ALREADY_EXISTS`).
- [ ] Wire `mtls_connection_pool.cpp` into `shard_rpc_client.cpp` to reuse TLS sessions; max pool size should be configurable via `gossip_config_manager.cpp`.
- [ ] Instrument every RPC call via `operational_metrics.cpp` and `prometheus_metrics.cpp` with labels for `shard_id`, `method`, and `outcome`.
- [ ] Certificate rotation events from `utils/pki_client.cpp` must trigger a graceful connection drain in `mtls_connection_pool.cpp` without dropping in-flight requests.
- [ ] Cross-shard RPC P99 latency (LAN): <5 ms excluding consensus overhead.
- [ ] Connection pool hit rate: >95% under sustained 10k RPS cross-shard traffic.
- [ ] Circuit-breaker open-to-half-open recovery time: configurable, default 5 s.

### Relationships

- Roadmap row: #202 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#orphandetector-wire-to-distributedcoordinator-transaction-list
- Source key: roadmap:202:sharding:v1.8.0:orphandetector-wire-to-distributedcoordinator-transaction-list

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:202:sharding:v1.8.0:orphandetector-wire-to-distributedcoordinator-transaction-list -->
<!-- roadmap-ref: row=202;module=sharding;target=v1.8.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#orphandetector-wire-to-distributedcoordinator-transaction-list -->
