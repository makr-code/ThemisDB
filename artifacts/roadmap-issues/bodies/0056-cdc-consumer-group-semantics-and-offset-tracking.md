### Context

This issue implements the roadmap item 'Consumer Group Semantics and Offset Tracking' for the cdc domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Consumer Group Semantics and Offset Tracking

### Goal

Deliver the scoped changes for Consumer Group Semantics and Offset Tracking in src/cdc/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Consumer Group Semantics and Offset Tracking
**Priority:** High
**Target Version:** v1.8.0

Currently, multiple consumers of the same changefeed each receive all events independently (fan-out). Add consumer group support so a group of consumers cooperatively processes a partition of the change log, with durable offset tracking so replay can resume after disconnect without full log scan.

**Implementation Notes:**
- `[ ]` Create `consumer_group_manager.cpp`; introduce `ConsumerGroup` with a durable `group_id` and per-group `committed_sequence` stored in RocksDB (key: `cdc_group:{group_id}:offset`).
- `[ ]` `Changefeed` assigns partitions by key-hash modulo `group.consumer_count`; each consumer receives only its assigned partition.
- `[ ]` Consumer connects with `{"action":"subscribe","group_id":"etl-workers","consumer_id":"worker-3","collection":"orders"}`.
- `[ ]` Consumer acknowledges processed events with `{"action":"ack","group_id":"etl-workers","sequence":10042}`; server advances committed offset.
- `[ ]` On reconnect, resume from `committed_sequence + 1` without scanning the full log.
- `[?]` Decision needed: how to rebalance partitions when consumers join/leave mid-session (static assignment vs. cooperative rebalance protocol).

**Performance Targets:**
- Consumer group offset commit (RocksDB write) < 1 ms p99.
- Resume-from-offset for a group that was offline for 24 h (≤ 10M buffered events) begins delivering in < 5 s.

---

### Acceptance Criteria

- [ ] Create `consumer_group_manager.cpp`; introduce `ConsumerGroup` with a durable `group_id` and per-group `committed_sequence` stored in RocksDB (key: `cdc_group:{group_id}:offset`).
- [ ] `Changefeed` assigns partitions by key-hash modulo `group.consumer_count`; each consumer receives only its assigned partition.
- [ ] Consumer connects with `{"action":"subscribe","group_id":"etl-workers","consumer_id":"worker-3","collection":"orders"}`.
- [ ] Consumer acknowledges processed events with `{"action":"ack","group_id":"etl-workers","sequence":10042}`; server advances committed offset.
- [ ] On reconnect, resume from `committed_sequence + 1` without scanning the full log.
- [ ] Decision needed: how to rebalance partitions when consumers join/leave mid-session (static assignment vs. cooperative rebalance protocol).
- [ ] Consumer group offset commit (RocksDB write) < 1 ms p99.
- [ ] Resume-from-offset for a group that was offline for 24 h (≤ 10M buffered events) begins delivering in < 5 s.

### Relationships

- Roadmap row: #56 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cdc/FUTURE_ENHANCEMENTS.md#consumer-group-semantics-and-offset-tracking
- Source key: roadmap:56:cdc:v1.8.0:consumer-group-semantics-and-offset-tracking

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:56:cdc:v1.8.0:consumer-group-semantics-and-offset-tracking -->
<!-- roadmap-ref: row=56;module=cdc;target=v1.8.0 -->
<!-- roadmap-detail: src/cdc/FUTURE_ENHANCEMENTS.md#consumer-group-semantics-and-offset-tracking -->
