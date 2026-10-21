---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Replication API Contract Summary

## Scope

This file documents the public-facing contract of the replication subsystem as it is represented in `include/replication/*.h` and implemented in `src/replication/*.cpp`.

## Primary API surfaces

### `ReplicationConfig`

`ReplicationConfig` defines the cluster-level runtime policy for enablement, failure detection, quorum, WAL durability, and read preference. It is the first configuration object callers use when creating a replication coordinator.

Important fields include:

- `mode`: `SYNC`, `SEMI_SYNC`, or `ASYNC`
- `heartbeat_interval_ms` and election timeout values
- `wal_directory` and retention policy
- `min_sync_replicas` and quorum settings
- `enable_auto_failover`, `failure_detection_timeout_ms`, and failover thresholds
- `max_replication_lag_ms` and timeout controls

### `ReplicationManager`

`ReplicationManager` is the leader/follower and failover control point for the module. It exposes initialization, replica membership management, read routing, WAL replication operations, and health/failover behavior.

The manager is expected to handle:

- add/remove replica membership,
- enable/disable placement constraints,
- trigger failover and leader promotion,
- route reads based on lag and preference,
- collect health and cluster status,
- export metrics to observability surfaces.

### `WALEntry`

`WALEntry` is the canonical replication payload. It carries operation metadata plus a durable byte representation. `serialize()` and `deserialize()` encode and validate the binary form used by the WAL pipeline.

### `ReplicationSlot` and `ReplicationSlotManager`

These types provide the persistence and lifecycle model for logical or streaming slots. They expose pause, resume, drop, and advance operations and track LSN-style state.

### `AsyncWalShipper`

`AsyncWalShipper` exposes a bounded asynchronous queue for shipping serialized WAL data to remote datacenters. It accepts `WalSegment` objects, triggers alert callbacks on lag excess, and exports Prometheus-compatible metrics.

### `LagAlertManager`

`LagAlertManager` tracks the lag of each replica and emits `AlertEvent` notifications when the configured thresholds are crossed. It is the main operator-facing signal for overload or replication staleness.

## Contract invariants

This API surface assumes the following invariants:

- state changes are explicit, not implicit,
- lag and health are surfaced through metrics and events,
- duplicate or invalid identity inputs are rejected or fail closed,
- blocking I/O is isolated away from the hot lock paths,
- sequence ordering remains the authoritative replication correctness primitive.

## Error expectations

The module does not treat partial or stale state as success. A replica that cannot be validated, fails health checks, or misses quorum is treated as degraded, failed, or read-ineligible depending on the operation. This is consistent with the fail-closed design in the code.

## API usage shape

A normal use flow looks like this:

1. create a `ReplicationConfig`,
2. initialize a `ReplicationManager`,
3. add peer replicas and any placement constraints,
4. use `WALEntry` to serialize writes and replicate them,
5. monitor lag and health with `LagAlertManager`,
6. use `LogicalReplicationManager` or `AsyncWalShipper` where logical or remote propagation is needed.

## Summary

The public replication API is explicit and conservative. The module exposes enough control for operators and developers to tune durability and performance, but it does not hide failure, lag, or quorum uncertainty behind a permissive default.
