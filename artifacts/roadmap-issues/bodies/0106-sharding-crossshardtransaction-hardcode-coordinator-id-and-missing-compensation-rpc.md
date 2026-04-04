### Context

This issue implements the roadmap item '`CrossShardTransaction`: Fix Coordinator ID + Compensation RPC' for the sharding domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `CrossShardTransaction`: Hardcode Coordinator ID and Missing Compensation RPC

### Goal

Deliver the scoped changes for `CrossShardTransaction`: Fix Coordinator ID + Compensation RPC in src/sharding/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `CrossShardTransaction`: Hardcode Coordinator ID and Missing Compensation RPC
**Priority:** High
**Target Version:** v1.8.0

`cross_shard_transaction.cpp` has 3 critical TODOs:
- Line 1898: "TODO: Replace with proper compensation RPC" — SAGA compensation actions call no real RPC.
- Line 2581: `coordinator_id = "coordinator-1"` hardcoded — transaction audit records show a placeholder instead of the actual node.
- Line 2605: same hardcoded coordinator ID.

**Implementation Notes:**
- `[ ]` Inject the actual `DistributedCoordinator::nodeId()` into `CrossShardTransactionManager`; replace all `"coordinator-1"` literals with the real node ID.
- `[ ]` Line 1898: implement the SAGA compensation RPC by dispatching a `ShardRpcClient::compensate(shard_id, txn_id, operation)` call; handle network failures with a bounded retry queue.

---

### Acceptance Criteria

- [ ] Line 1898: "TODO: Replace with proper compensation RPC" — SAGA compensation actions call no real RPC.
- [ ] Line 2581: `coordinator_id = "coordinator-1"` hardcoded — transaction audit records show a placeholder instead of the actual node.
- [ ] Line 2605: same hardcoded coordinator ID.
- [ ] Inject the actual `DistributedCoordinator::nodeId()` into `CrossShardTransactionManager`; replace all `"coordinator-1"` literals with the real node ID.
- [ ] Line 1898: implement the SAGA compensation RPC by dispatching a `ShardRpcClient::compensate(shard_id, txn_id, operation)` call; handle network failures with a bounded retry queue.

### Relationships

- Roadmap row: #106 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#crossshardtransaction-hardcode-coordinator-id-and-missing-compensation-rpc
- Source key: roadmap:106:sharding:v1.8.0:crossshardtransaction-hardcode-coordinator-id-and-missing-compensation-rpc

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:106:sharding:v1.8.0:crossshardtransaction-hardcode-coordinator-id-and-missing-compensation-rpc -->
<!-- roadmap-ref: row=106;module=sharding;target=v1.8.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#crossshardtransaction-hardcode-coordinator-id-and-missing-compensation-rpc -->
