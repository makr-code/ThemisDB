### Context

This issue implements the roadmap item 'Adaptive Shard Rebalancer with Load-Based Splitting' for the sharding domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: [ ] Adaptive Shard Rebalancer with Load-Based Splitting

### Goal

Deliver the scoped changes for Adaptive Shard Rebalancer with Load-Based Splitting in src/sharding/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### [ ] Adaptive Shard Rebalancer with Load-Based Splitting
**Priority:** Medium
**Target Version:** v0.10.0

Extend `auto_rebalancer.cpp` and `shard_load_detector.cpp` to automatically split hot shards when CPU or storage utilization exceeds configurable thresholds. The rebalancer uses `predictive_detector.cpp` ML-based load forecasting to initiate splits before saturation occurs.

**Implementation Notes:**
- Add a `HotShardSplitPolicy` class to `auto_rebalancer.cpp` that consumes `shard_load_detector.cpp` metrics and emits split proposals to `rebalance_operation.cpp`.
- Integrate with `predictive_detector.cpp` to forecast load 5 minutes ahead; trigger pre-emptive split when predicted load exceeds 80% of capacity.
- `data_migrator.cpp` must support live migration with dual-write semantics: old shard accepts writes during migration, new shard catches up via `wal_shipper.cpp`, then atomic cutover via `shard_topology.cpp`.
- Emit split/merge events to `utils/audit_logger.cpp` for compliance audit trail.

**Performance Targets:**
- Shard split migration downtime (read unavailability): 0 ms (dual-write protocol).
- Write latency increase during live migration: <20% above baseline P99.
- Rebalancer decision cycle: <10 s from load threshold breach to split proposal.

---

### Acceptance Criteria

- [ ] Add a `HotShardSplitPolicy` class to `auto_rebalancer.cpp` that consumes `shard_load_detector.cpp` metrics and emits split proposals to `rebalance_operation.cpp`.
- [ ] Integrate with `predictive_detector.cpp` to forecast load 5 minutes ahead; trigger pre-emptive split when predicted load exceeds 80% of capacity.
- [ ] `data_migrator.cpp` must support live migration with dual-write semantics: old shard accepts writes during migration, new shard catches up via `wal_shipper.cpp`, then atomic cutover via `shard_topology.cpp`.
- [ ] Emit split/merge events to `utils/audit_logger.cpp` for compliance audit trail.
- [ ] Shard split migration downtime (read unavailability): 0 ms (dual-write protocol).
- [ ] Write latency increase during live migration: <20% above baseline P99.
- [ ] Rebalancer decision cycle: <10 s from load threshold breach to split proposal.

### Relationships

- Roadmap row: #203 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#-adaptive-shard-rebalancer-with-load-based-splitting
- Source key: roadmap:203:sharding:v1.7.0:adaptive-shard-rebalancer-with-load-based-splitting

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:203:sharding:v1.7.0:adaptive-shard-rebalancer-with-load-based-splitting -->
<!-- roadmap-ref: row=203;module=sharding;target=v1.7.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#-adaptive-shard-rebalancer-with-load-based-splitting -->
