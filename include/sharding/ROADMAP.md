---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../src/sharding/ROADMAP.md
  - sharding_api_contract.h
  - tests/sharding/CMakeLists.txt
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

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

The current tree contains 107 `.h` files under `include/sharding/`. This inventory confirms files,
not contract completeness or runtime maturity. The target files and future items below are retained
as planning records; their dates have elapsed and their present priority/commitment has not been
revalidated. **TODO:** reconcile each proposal with the active module roadmap before treating it as
an active delivery commitment.

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

## Planned (proposal; not a current API commitment)

- [ ] `topology_change_event.h` — shared event contract for routing/repair subscribers (Target: 2026-Q4)
- [ ] `distributed_deadlock_snapshot.h` — explicit deadlock-diagnostics DTO for cross-shard transaction consumers (Target: 2026-Q4)
- [ ] Deprecation notes for legacy single-path router helpers once adaptive/locality-aware routing becomes default (Target: 2026-Q4)

The proposed headers above are not present in the current `include/sharding/` inventory. No
implementation or compatibility schedule is implied by listing them here.

---

## Breaking Change History

None in v1.x. Public sharding headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
