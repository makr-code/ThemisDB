> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Replication Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/replication/`

---

## 1. Overview

The Replication module provides ThemisDB's high-availability and data durability
infrastructure. It implements leader-follower replication with Raft-like consensus,
multi-master replication for geo-distributed deployments, WAL shipping, automatic failover,
point-in-time recovery (PITR), and conflict resolution for concurrent writes.

---

## 2. Design Principles

- **Raft Consensus** – leader election and log replication follow the Raft protocol for
  strong consistency guarantees in leader-follower mode.
- **Multiple Replication Modes** – SYNC (strong consistency), SEMI_SYNC (quorum), and
  ASYNC (maximum throughput) modes are configurable per collection.
- **WAL-Based** – all replication uses Write-Ahead Log entries; this enables PITR and
  guarantees durability even under partial failures.
- **Automatic Failover** – health monitoring detects leader failure and triggers automatic
  leader promotion within configurable timeout.
- **Conflict Resolution** – multi-master mode uses vector clocks and hybrid logical clocks
  (HLC) for causality tracking; conflicts resolved via LWW, CRDT, or custom strategies.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `replication_manager.cpp` | Main orchestrator: leader-follower, WAL shipping, failover, `BidirectionalReplicationManager`, `GeoReplicationManager` |
| `raft_v2.cpp` | Full Raft v2 implementation: `MembershipChangeManager`, joint consensus, log replication |
| `conflict_resolution.cpp` | `ThreeWayMergeResolver` (git-style) and `FieldLevelMergeResolver` (UNION/INTERSECT/LEFT_BIAS/RIGHT_BIAS) |
| `logical_replication.cpp` | `LogicalReplicationManager` — schema-aware slots, include/exclude filters, DDL streaming, cross-version transforms |
| `event_stream.cpp` | `ReplicationEventStream` — RAII subscription handles for CDC consumers |
| `policy.cpp` | `ReplicationPolicy` — per-collection policy assignment and topology validation |
| `replication_slot.cpp` | `ReplicationSlot` / `ReplicationSlotManager` — pause/resume slot management |
| `schema_cdc.cpp` | Schema-aware Change Data Capture with Avro/Protobuf schema registry integration |
| `multi_tier_replication.cpp` | Multi-tier (cascading) replication topology management |
| `observability.cpp` | `ReplicationObserver` — lag snapshots, topology graph, bottleneck detection, health scores |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                   Write Path (src/storage/)                     │
│   storage.write(key, value) → WAL entry → replicate            │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  ReplicationManager (leader)                     │
│                                                                  │
│  WAL → ship to replicas                                         │
│  Modes: SYNC / SEMI_SYNC / ASYNC                                │
│  Health check: heartbeat to all replicas                        │
└──────────────────────────┬──────────────────────────────────────┘
                           │ WAL entries
         ┌─────────────────┴──────────────────────────┐
         │                                            │
┌────────▼──────────────┐              ┌──────────────▼──────────┐
│   Replica 1           │              │   Replica 2             │
│   apply WAL entries   │              │   apply WAL entries     │
│   ack to leader       │              │   ack to leader         │
└───────────────────────┘              └─────────────────────────┘
         │
         └─ Raft: LeaderElection → RaftNode (consensus)
```

---

## 4. Data Flow

### 4.1 Leader-Follower Write (SEMI_SYNC)

```
Client write → leader WAL entry
    │
    ├─ ship WAL entry to all replicas
    ├─ wait for ack from quorum (min_sync_replicas)
    │       ├─ quorum acked → commit → return success
    │       └─ timeout → apply locally; log warning
    │
    └─ remaining replicas catch up asynchronously
```

### 4.2 Leader Failover

```
Heartbeat timeout detected for leader
    │
    ├─ LeaderElection: initiate Raft election
    ├─ candidates increment term, send RequestVote to peers
    ├─ majority votes → new leader elected
    ├─ new leader resumes WAL shipping
    └─ clients re-discover leader via cluster metadata
```

### 4.3 Point-in-Time Recovery

```
SnapshotManager::restore(timestamp: "2026-01-15T12:00:00Z")
    │
    ├─ load latest snapshot before timestamp
    ├─ replay WAL entries from snapshot to timestamp
    └─ database state restored to target time
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Receives from** | `src/storage/` | WAL entries on every write |
| **Uses** | `src/network/` | Wire protocol for inter-node WAL shipping |
| **Uses** | `src/cdc/` | Change notification for replica subscribers |
| **Provides to** | `src/server/` | Replication status API |
| **Uses** | `src/observability/` | Replication lag and health metrics |

---

## 6. Threading & Concurrency Model

- WAL shipping runs on a dedicated background thread per replica.
- Leader election is event-driven (heartbeat timeout triggers election).
- `ReplicationManager` is thread-safe; all public methods use internal locks.
- Snapshot creation runs on a background thread to avoid blocking writes.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| WAL batching | Multiple WAL entries batched into a single network write |
| ASYNC mode | Zero replication overhead on write path (eventual consistency) |
| Read routing | Read requests routed to nearest/fastest replica |
| Cascading replication | Reduce leader fan-out by cascading through replica chains |

---

## 8. Security Considerations

- WAL entries are transmitted over the authenticated wire protocol (TLS).
- Replica nodes are authenticated via mTLS before accepting replication connections.
- WAL entries include checksums to detect corruption in transit.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `replication.mode` | "async" | SYNC / SEMI_SYNC / ASYNC |
| `replication.min_sync_replicas` | 1 | Min acks for SEMI_SYNC |
| `replication.heartbeat_interval_ms` | 1000 | Leader heartbeat interval |
| `replication.election_timeout_ms` | 5000 | Follower election timeout |
| `replication.max_lag_bytes` | 10485760 | Max allowed replication lag (10 MB) |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Replica connection failure | Mark replica as unhealthy; continue ASYNC |
| Quorum unavailable (SEMI_SYNC) | Degrade to ASYNC; alert operator |
| Leader failure | Trigger Raft election; promote new leader |
| WAL corruption on replica | Full re-sync from snapshot |
| Network partition | Leader continues; partitioned replicas wait and re-sync on reconnect |

---

## 11. Known Limitations & Future Work

- Cross-datacenter replication with WAN optimization is <!-- TODO: verify --> planned.
- CDC integration for downstream consumers via Kafka bridge is partial.

---

## 12. References

- `src/replication/README.md` — module overview
- `docs/replication/` — replication guide
- `docs/high_availability.md` — HA deployment guide
- `ARCHITECTURE.md` (root) — full system architecture
