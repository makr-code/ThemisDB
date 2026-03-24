<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Scheduler Module — Architecture Guide

## Overview

The scheduler module provides distributed task scheduling for ThemisDB: cron-based and event-triggered jobs, priority queues, DAG dependency scheduling, persistent job state, distributed coordination, sandboxed execution (cgroups v2 + seccomp-bpf on Linux), audit logging, anomaly detection, and TLS auth context propagation via `RequestContext`.

## Design Principles

- **Security-first execution** — `TaskScheduler::sandbox_execution` wraps tasks in cgroups v2 + seccomp-bpf on Linux for resource and syscall isolation.
- **Auth context propagation** — `RequestContext` (user_id, client_ip) is stored in TLS (thread-local storage) via `setRequestContext` / `clearRequestContext`.
- **Persistent state** — `TaskResultStore` and `TaskAuditManager` persist job results and audit events to RocksDB.
- **Anomaly detection** — `TaskAnomalyDetector` monitors task duration and error patterns; triggers alerts via `observability`.
- **Hybrid retention** — `HybridRetentionManager` balances hot/cold storage for long-running job histories.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `distributed_task_coordinator.h` | `DistributedTaskCoordinator` | Distributed multi-node task coordination and deduplication |
| `event_trigger.h` | `EventTrigger` | Event-driven task trigger registration |
| `external_scheduler_adapter.h` | `ExternalSchedulerAdapter` | Adapts Kubernetes CronJob / Airflow / Temporal triggers |
| `hybrid_retention_manager.h` | `HybridRetentionManager` | Hot/cold job history retention policy |
| `task_anomaly_detector.h` | `TaskAnomalyDetector` | Duration and error pattern anomaly detection |
| `task_audit_event.h` | `TaskAuditEvent` | Structured audit event type |
| `task_audit_manager.h` | `TaskAuditManager` | Audit event persistence and query |
| `task_result_store.h` | `TaskResultStore` | Persistent task result storage (RocksDB) |
| `task_scheduler.h` | `TaskScheduler`, `RequestContext` | Core scheduler with cron, priority queues, DAG scheduling, sandbox_execution, TLS auth context |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `observability` | `TaskAnomalyDetector` | Task anomaly and SLO alerts |
| `storage` | `TaskResultStore`, `TaskAuditManager` | RocksDB persistent job state |
| `auth` | `RequestContext` (TLS propagation) | User/IP auth context through task execution |
| `network` | `DistributedTaskCoordinator` | Distributed coordination over gRPC |
| External schedulers | `ExternalSchedulerAdapter` | Kubernetes CronJob, Airflow, Temporal |

## Implementation

Implementation in `../../src/scheduler/`.
