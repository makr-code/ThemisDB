> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/replication/ARCHITECTURE.md -->

# Replication Module — Public Header Architecture

**Module Path:** `include/replication/`
**Implementation:** `../../src/replication/`
**Canonical architecture doc:** [`../../src/replication/ARCHITECTURE.md`](../../src/replication/ARCHITECTURE.md)

---

## 1. Overview

`include/replication/` defines the **public replication orchestration and data-propagation contract** for ThemisDB. The 13 headers cover leader/follower and multi-writer replication control, conflict resolution, logical replication and CDC, event streams, replication policies, and lag/topology observability.

For runtime composition details — promotion/failover, conflict resolution internals, and observability plumbing — see:
→ [`../../src/replication/ARCHITECTURE.md`](../../src/replication/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Orchestration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `replication_manager.h` | `ReplicationManager` | Primary replication lifecycle control |
| `multi_master_replication.h` / `multi_tier_replication.h` | Multi-writer and tiered types | Alternative topology contracts |
| `raft_v2.h` | `RaftV2` | Consensus-driven replication state |
| `policy.h` | `ReplicationPolicy` | Replication policy assignment and validation |

### 2.2 Propagation and CDC

| Header | Public Type | Purpose |
|--------|------------|---------|
| `logical_replication.h` | `LogicalReplication` | Logical stream publication/consumption |
| `replication_slot.h` | `ReplicationSlot` | Slot lifecycle and persistence |
| `event_stream.h` | `ReplicationEventStream` | Replication event delivery |
| `schema_cdc.h` | `SchemaCDCBridge` | Schema-aware change-data-capture bridge |
| `kafka_change_stream.h` | `KafkaChangeStream` | Kafka-backed CDC integration |

### 2.3 Conflict Resolution and Observability

| Header | Public Type | Purpose |
|--------|------------|---------|
| `conflict_resolution.h` | `ConflictResolver` | Conflict-resolution strategy contract |
| `crdt_types.h` | CRDT value types | Conflict-free merge payloads |
| `observability.h` | `ReplicationObservability` | Lag, topology, and health snapshots |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::replication` | Replication orchestration, stream, and conflict types |

---

## 4. Public Contract Notes

- Replication headers expose explicit promotion, failover, and replication-mode transitions rather than hiding them in storage internals.
- Logical replication, slot, and CDC headers remain public because embedders may integrate external streaming backends.
- Conflict resolution contracts are public to support deployment-specific resolver strategies and CRDT payload types.
- Observability headers provide stable lag/topology diagnostics consumed by operations and admin layers.
