---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/failover/ARCHITECTURE.md -->

# Failover Module — Public Header Architecture

**Module Path:** `include/failover/`  
**Implementation:** `../../src/failover/`  
**Canonical architecture doc:** [`../../src/failover/ARCHITECTURE.md`](../../src/failover/ARCHITECTURE.md)

---

## 1. Overview

`include/failover/` defines the **public automatic failover management and disaster recovery coordination API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/failover/ARCHITECTURE.md`](../../src/failover/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Failover and Recovery

| Header | Public types | Purpose |
|--------|------------|---------|
| `auto_failover_manager.h` | `AutoFailoverManager`, `AutoFailoverConfig`, `FailoverResult`, event/state/statistics types | Health/failure monitoring, bounded request queue, quorum/fencing checks, and promotion requests |
| `disaster_recovery_manager.h` | `DisasterRecoveryManager`, `DisasterRecoveryConfig`, plan/result/step types | Plan validation and ordered seven-step DR coordination |
| `failover_api_contract.h` | `NodeRole`, `FailoverErrorCode`, contract helpers | Shared failover/retry contract declarations |
| `quorum_log.h` | `QuorumLog`, `QuorumEntry`, `QuorumState` | Optional local append/recovery log |
| `topology_snapshot.h` | `TopologySnapshot` | Snapshot and node-set comparison helpers |

---

## 3. Namespace Layout

These declarations use the `themis::failover` namespace.

---

## 4. Contract Notes

- Headers in `include/failover/` expose the public declarations; runtime implementation lives in `src/failover/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- The headers do not implement authentication or provide storage-restore/traffic-management adapters. See the [implementation architecture](../../src/failover/ARCHITECTURE.md).
