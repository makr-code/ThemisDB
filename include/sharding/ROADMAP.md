<!-- Status: current | validated: 2026-04-12 -->

# include/sharding/ — Roadmap

> Public header API evolution roadmap for the ThemisDB Sharding module.  
> Implementation roadmap: [`../../src/sharding/`](../../src/sharding/)

---

## Current Status

The sharding public API is **production-ready** at v1.6.0.  All 86 headers are
stable, versioned, and covered by integration tests.  The v1.6.0 release added
Paxos WAL durability and `ShardRPCClient::writeEntity()` gRPC integration.

---

## Completed ✅

- [x] Core consistent-hash routing (`consistent_hash.h`, `shard_router.h`)
- [x] Raft consensus full API (`raft_consensus.h`, `raft_log.h`, `raft_state.h`)
- [x] Paxos consensus full API (`paxos_consensus.h`, `paxos_snapshot.h`)
- [x] WAL infrastructure (`wal_manager.h`, `wal_applier.h`, `wal_shipper.h`)
- [x] Distributed 2PC transactions
- [x] mTLS transport headers
- [x] Gossip protocol headers
- [x] Locality-aware & adaptive routing
- [x] Cloud backup integration headers
- [x] SLO monitoring interface
- [x] GPU erasure coding interface (CUDA-optional)
- [x] Predictive fault detection interface
- [x] Multi-primary coordinator interface
- [x] TrueTime / HLC clock interfaces
- [x] Prometheus metrics interface
- [x] **Paxos WAL durability** — `handlePrepare`/`handleAccept` log to WAL before returning; `recoverFromWAL()` restores `instances_` on restart (Issue: #4592) (2026-04-12)
- [x] **`ShardRPCClient::writeEntity()`** — gRPC `ReplicateData` cross-shard write; `handleWriteEntityGrpc()` wired in `sendRequestGrpc()` (Issue: #4593) (2026-04-12)
- [x] **10 focused tests PSR-01…PSR-10** — `PaxosPersistenceRecoveryFocusedTests` in `tests/test_paxos_persistence_recovery.cpp` (Issue: #4592)

---

## Planned Features

### Q2 2026

- [ ] `geo_partition_router.h` — Geographic partition-aware routing with
      latency-zone annotations (Target: Q2 2026)
  - Inputs: zone labels, latency matrix, routing hints
  - Outputs: ranked shard list with estimated latency
  - Constraints: must not break existing `ShardRouter` implementations
  - Tests: unit + topology-simulation tests

- [ ] `streaming_replication_v2.h` — Enhanced streaming interface with
      explicit back-pressure negotiation (Target: Q2 2026)
  - Replaces implicit flow control in `stream_protocol.h`
  - Backward-compatible via adapter pattern
  - Tests: chaos / network-fault injection tests

### Q3 2026

- [ ] `adaptive_quorum.h` — Dynamic quorum adjustment based on node health
      (Target: Q3 2026)
  - Inputs: health scores, topology weights
  - Outputs: quorum decision with audit trail
  - Perf: decision latency ≤ 2 ms p99

- [ ] `shard_scheduler.h` — Priority-based shard operation scheduler
      (Target: Q3 2026)
  - Supports FIFO, priority, and deadline-based scheduling
  - Integrates with `shard_resource_manager.h`

- [ ] `federated_shard_connector.h` — Cross-cluster shard federation
      (Target: Q3 2026)
  - Allows read-only federation across ThemisDB clusters
  - Must support mTLS + signed requests end-to-end

### Q4 2026

- [ ] `vector_shard_router.h` — Routing for vector / ANN index shards
      (Target: Q4 2026)
  - Partition strategy: HNSW graph segments
  - Inputs: embedding vector, k-NN hint
  - Perf: routing overhead ≤ 100 µs p99

- [ ] `tiered_consensus.h` — Hierarchical consensus for geo-distributed
      deployments (Target: Q4 2026)
  - Local fast consensus + global slow consensus
  - WAN round-trip tolerance ≥ 200 ms

---

## Implementation Phases (Next Feature Cycle)

### Phase 1 — API Contract Design
- [ ] Draft `geo_partition_router.h` interface with full Doxygen docs
- [ ] Peer review of `streaming_replication_v2.h` API with protocol team
- [ ] ADR (Architecture Decision Record) for adaptive quorum approach

### Phase 2 — Core Implementation
- [ ] Geo partition router implementation file (planned) + unit tests
- [ ] Streaming replication v2 implementation file (planned) with back-pressure logic

### Phase 3 — Error Handling & Edge Cases
- [ ] Timeout and fallback paths for geo-routing failures
- [ ] Quorum degradation handling (partial node failure)

### Phase 4 — Tests
- [ ] Topology-simulation tests for geo-router
- [ ] Chaos/partition tests for streaming v2
- [ ] Fuzz tests for quorum edge cases

### Phase 5 — Performance & Hardening
- [ ] Geo-router latency benchmarks (target: ≤ 100 µs routing overhead)
- [ ] Streaming v2 throughput: ≥ 10 GB/s on 10 GbE network

### Phase 6 — Documentation & Acceptance
- [ ] Update this ARCHITECTURE.md with new headers
- [ ] Update CHANGELOG.md with all new symbols
- [ ] Acceptance review against production-readiness checklist

---

## Production Readiness Checklist

- [x] All headers compile cleanly on GCC 12+, Clang 15+, MSVC 19.35+
- [x] All public symbols documented with Doxygen comments
- [x] No implementation includes in public headers
- [x] Thread-safety guarantees documented per class
- [x] Error return codes defined in `sharding_interfaces.h`
- [x] No STL types with ABI instability in public interface
- [x] Versioning macros present in `sharding_interfaces.h`
- [ ] C-compatible wrapper headers for FFI consumers (Planned Q3 2026)
- [ ] Python / Rust binding stubs generated from headers (Planned Q4 2026)
