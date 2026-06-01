> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/scheduler/ARCHITECTURE.md -->

# Scheduler Module — Public Header Architecture

**Module Path:** `include/scheduler/`  
**Implementation:** `../../src/scheduler/`  
**Canonical architecture doc:** [`../../src/scheduler/ARCHITECTURE.md`](../../src/scheduler/ARCHITECTURE.md)

---

## 1. Overview

`include/scheduler/` defines the **public distributed task scheduling, event-triggered execution, anomaly detection, audit management, and external scheduler integration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/scheduler/ARCHITECTURE.md`](../../src/scheduler/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Scheduling

| Header | Public Type | Purpose |
|--------|------------|---------|
| `task_scheduler.h` | `TaskScheduler` | Cron/interval/event-driven task scheduler |
| `distributed_task_coordinator.h` | `DistributedTaskCoordinator` | Cross-node distributed task coordination |
| `event_trigger.h` | `EventTrigger` | Event-driven task trigger definitions |
| `external_scheduler_adapter.h` | `ExternalSchedulerAdapter` | Integration adapter for external schedulers |
### 2.2 Retention and Anomalies

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hybrid_retention_manager.h` | `HybridRetentionManager` | Hot/warm/cold data retention scheduling |
| `task_anomaly_detector.h` | `TaskAnomalyDetector` | Scheduled task anomaly detection |
### 2.3 Auditing and Storage

| Header | Public Type | Purpose |
|--------|------------|---------|
| `task_audit_event.h` | `TaskAuditEvent` | Immutable task execution audit event |
| `task_audit_manager.h` | `TaskAuditManager` | Task audit trail management |
| `task_result_store.h` | `TaskResultStore` | Persistent task result storage |

---

## 3. Namespace Layout

All public types reside in the `themis::scheduler` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/scheduler/` expose the **stable public API**; internal types live in `src/scheduler/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
