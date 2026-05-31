# Replication Readiness Plan (RAID 1/10)

> Alignment note (2026-05-31): This plan is a secondary implementation guide.
> Authoritative current workload and target behavior are defined in:
> - `src/replication/FUTURE_ENHANCEMENTS.md`
> - `src/replication/MODULE_GAPS.md`
> - `src/replication/ROADMAP.md`
> Newer planning docs take precedence over historical status summaries.

> **Related Documentation:**
> - **[replication-ha-guide.md](./replication-ha-guide.md)** - Complete HA/replication guide with deployment topologies
> - **[REPLICATION_IMPLEMENTATION_STATUS.md](../../reports/REPLICATION_IMPLEMENTATION_STATUS.md)** - Historical component status snapshot

## Module Organization

This plan covers the WAL-based replication infrastructure implemented in the **`sharding/` module**:
- `include/sharding/` - Headers for WAL components (wal_manager.h, wal_shipper.h, wal_applier.h, replication_coordinator.h, replica_topology.h)
- `src/sharding/` - Implementation files for WAL infrastructure
- The high-level `replication/` module (ReplicationManager) orchestrates these low-level components

## Current Findings
- WAL pipeline exists: WALManager (persist), WALShipper (async sender, needs mTLS), WALApplier (apply with LSN checks/retries).
- Receive endpoints in place: HTTP `/api/v1/wal/apply` implemented with auth/HMAC + latency metrics; gRPC ApplyWalBatch service implemented and server lifecycle wired (when gRPC enabled).
- Shipper is still not started in the server lifecycle; RaftWALIntegration present but not wired.
- ShardRPCClient remains in-process simulation (no real network IO for shipper path).
- Redundancy modes are defined (`redundancy_strategy.h`), but no runtime write/read routing uses them.

## Missing Pieces for RAID1/10 Replication
- Wire WALShipper lifecycle (leader-only) with real network client; provide config for peers, batch, compression, retries, TLS/mTLS.
- Write concern in write API (ONE/MAJORITY/ALL) plus replication wait logic; surface quorum failures to clients.
- Prometheus metrics for ship/apply lag/backlog and failure counters; ensure gRPC/HTTP paths expose apply stats.
- Replica topology mapping for RAID1/10 (stripe + mirrors) and routing hooks.
- Multi-node tests (MAJORITY) and endurance/lag convergence runs; ensure no 404 sync misses.

## Proposed Implementation Steps
1) ✅ Ship path: WALShipper lifecycle wired in main_server.cpp; config-driven with replicas, batch size, compression, retries, TLS paths; starts when shipper_enabled=true.
2) ✅ Write concern: WriteConcern enum (ONE/MAJORITY/ALL), ReplicationCoordinator waits for quorum with timeout, returns WriteResult with ack count/errors.
3) ✅ Metrics: Prometheus counters/gauges for ship/apply throughput, failures, lag/backlog exposed across HTTP/gRPC; PrometheusMetrics wired into WALShipper.
4) Topology: define per-collection/shard replica sets for RAID1/10 (stripe+mirrors) and connect to routing + shipper config.
5) Testing: unit (Applier idempotency/LSN already covered), loopback shipper→apply, multi-node MAJORITY flow, recovery/backlog drain, endurance/lag convergence benchmarks.

## Acceptance Criteria
- ✅ Apply endpoints (HTTP + gRPC) functional and idempotent by LSN (done).
- ✅ WALShipper wired in server lifecycle with config-driven replicas, compression, retries (done).
- ✅ Prometheus metrics for ship batches, bytes, failures, lag, backlog exported per replica.
- Leader writes with MAJORITY wait for quorum or timeout; client sees failure on missing quorum.
- RAID1/10 endurance tests show zero 404 sync misses; lag falls toward zero after load.

## Next Actions
- ✅ Wire WALShipper lifecycle (leader-only) with real ShardService client + TLS/config (done: config/distributed/replication/basic.example.yaml shows usage).
- ✅ Write concern handling: ReplicationCoordinator tracks LSN acks, waits for quorum with timeout, returns success/failure to write API.
- ✅ Wire ReplicationCoordinator into HttpServer POST /entities: append to WAL, call coordinator.waitForReplication(), return 503 on quorum timeout.
- ✅ Expose Prometheus metrics: ship_batches_total, ship_bytes_total, ship_failures_total, replication_lag_seconds, backlog_bytes per replica.
- Test shipper→HTTP apply in loopback: start two instances (primary on 8765, replica on 8766), write to primary with MAJORITY, verify applied on replica.
- Stand up multi-node integration test (MAJORITY) to verify no 404 and lag convergence.
- Re-run RAID endurance test with replicas enabled; capture logs for any remaining failures.

## See Also

- **[replication-ha-guide.md](./replication-ha-guide.md)** - Complete HA/replication guide with deployment topologies and operational procedures
- **[REPLICATION_IMPLEMENTATION_STATUS.md](../../reports/REPLICATION_IMPLEMENTATION_STATUS.md)** - Detailed implementation status with component breakdown
