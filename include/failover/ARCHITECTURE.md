> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
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

| Header | Public Type | Purpose |
|--------|------------|---------|
| `auto_failover_manager.h` | `AutoFailoverManager` | Automated leader election and failover |
| `disaster_recovery_manager.h` | `DisasterRecoveryManager` | Disaster recovery orchestration and RTO/RPO tracking |

---

## 3. Namespace Layout

All public types reside in the `themis::failover` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/failover/` expose the **stable public API**; internal types live in `src/failover/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
