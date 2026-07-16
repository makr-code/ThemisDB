### Context

This issue implements the roadmap item 'Distributed Maintenance Coordination via Raft' for the maintenance domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: Distributed Maintenance Coordination via Raft

### Goal

Deliver the scoped changes for Distributed Maintenance Coordination via Raft in src/maintenance/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### Distributed Maintenance Coordination via Raft
**Priority:** Low
**Target Version:** v2.0.0

In a multi-node cluster, each node independently schedules and fires maintenance jobs. Two nodes may run the same schedule concurrently, causing compaction storms or double maintenance.

**Implementation Notes:**
- `[x]` Integrate with the existing Raft-based distributed lock (`src/replication/raft_v2.cpp` or a dedicated distributed lock service) to elect a single maintenance leader per schedule.
- `[x]` Before firing a scheduled job, the orchestrator calls `DistributedLock::tryAcquire(schedule_id, ttl=window_duration_ms)`; only the node that acquires the lock runs the job.
- `[x]` Non-leader nodes log "schedule {id} skipped — lock held by peer {node_id}" at DEBUG level.
- `[x]` Lock TTL must be ≥ estimated task duration + 30 s safety margin; configurable per schedule.

---

### Acceptance Criteria

- [x] Integrate with the existing Raft-based distributed lock (`src/replication/raft_v2.cpp` or a dedicated distributed lock service) to elect a single maintenance leader per schedule.
- [x] Before firing a scheduled job, the orchestrator calls `DistributedLock::tryAcquire(schedule_id, ttl=window_duration_ms)`; only the node that acquires the lock runs the job.
- [x] Non-leader nodes log "schedule {id} skipped — lock held by peer {node_id}" at DEBUG level.
- [x] Lock TTL must be ≥ estimated task duration + 30 s safety margin; configurable per schedule.

### Relationships

- Roadmap row: #252 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#distributed-maintenance-coordination-via-raft
- Source key: roadmap:252:maintenance:v2.0.0:distributed-maintenance-coordination-via-raft

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:252:maintenance:v2.0.0:distributed-maintenance-coordination-via-raft -->
<!-- roadmap-ref: row=252;module=maintenance;target=v2.0.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#distributed-maintenance-coordination-via-raft -->
