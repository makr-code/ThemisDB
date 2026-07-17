> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/sharding/ROADMAP.md -->

# Sharding Module — Public Header Roadmap

**Module Path:** `include/sharding/`
**Canonical implementation roadmap:** [`../../src/sharding/ROADMAP.md`](../../src/sharding/ROADMAP.md)

---

## Overview

Tracks public sharding API contract stability, distributed-operations header coverage, and future public entry points. Runtime hardening remains in:

→ [`../../src/sharding/ROADMAP.md`](../../src/sharding/ROADMAP.md)

---

## Current Status

All 99 sharding headers are present and cover routing, consensus, quorum, cross-shard transactions,
WAL durability, rebalance/repair operations, secure transport, adapter distribution, and operational
metrics.

---

## Completed ✅

- [x] `shard_router.h`, `adaptive_shard_router.h`, `consistent_hash.h`, `locality_aware_router.h` — routing and placement surfaces
- [x] Raft/Paxos/quorum headers — consensus and topology contract layer
- [x] `cross_shard_transaction.h`, `two_phase_commit_coordinator.h`, `distributed_transaction.h` — distributed transaction surfaces
- [x] `wal_manager.h`, `wal_applier.h`, `wal_shipper.h`, `metadata_wal.h` — durability and replication plumbing headers
- [x] `auto_rebalancer.h`, `data_migrator.h`, `shard_repair_engine.h`, `auto_recovery_manager.h` — migration and repair surfaces
- [x] `mtls_client.h`, `secure_transport_client.h`, `health_monitor.h`, `operational_metrics.h` — secure transport and observability contracts
- [x] `lora_artifact_distribution.h` — adapter distribution store, Merkle proof, receipt, and recovery contracts

---

## In Progress

- [ ] Document expected compatibility boundaries between consensus-specific headers and generic coordinator abstractions (Target: 2026-Q3)
- [ ] Add clearer degraded-quorum and migration-failure guidance across public repair/operations headers (Target: 2026-Q3)

---

## Planned

- [ ] `topology_change_event.h` — shared event contract for routing/repair subscribers (Target: 2026-Q4)
- [ ] `distributed_deadlock_snapshot.h` — explicit deadlock-diagnostics DTO for cross-shard transaction consumers (Target: 2026-Q4)
- [ ] Deprecation notes for legacy single-path router helpers once adaptive/locality-aware routing becomes default (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public sharding headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
