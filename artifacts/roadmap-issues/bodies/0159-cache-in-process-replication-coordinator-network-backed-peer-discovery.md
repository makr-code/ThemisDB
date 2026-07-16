### Context

This issue implements the roadmap item 'In-Process Replication Coordinator: Network-Backed Peer Discovery' for the cache domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: In-Process Replication Coordinator: Network-Backed Peer Discovery

### Goal

Deliver the scoped changes for In-Process Replication Coordinator: Network-Backed Peer Discovery in src/cache/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### In-Process Replication Coordinator: Network-Backed Peer Discovery
**Priority:** Medium
**Target Version:** v1.8.0

`cache_replication_coordinator.cpp` uses an in-process `ReplicationBus` where peers are registered via direct pointer sharing (line 73: `for (auto* peer : bus_->peers)`). This only works within a single process. Cross-node cache invalidation (required for clustered deployments) is not implemented.

**Implementation Notes:**
- `[ ]` Define a `IRemoteCachePeer` interface with `invalidate(key)` and `invalidateTenant(tenant_id)` methods.
- `[ ]` Implement `GrpcRemoteCachePeer` backed by the existing gRPC transport in `src/network/grpc_transport.cpp`.
- `[ ]` `CacheReplicationCoordinator` holds a `std::vector<IRemoteCachePeer*>`; populate from cluster membership (Raft or gossip) via a `ClusterView` injection.
- `[ ]` Fanout invalidation to remote peers asynchronously (fire-and-forget with a bounded retry queue); do not block `put()` on remote acknowledgment.

---

### Acceptance Criteria

- [ ] Define a `IRemoteCachePeer` interface with `invalidate(key)` and `invalidateTenant(tenant_id)` methods.
- [ ] Implement `GrpcRemoteCachePeer` backed by the existing gRPC transport in `src/network/grpc_transport.cpp`.
- [ ] `CacheReplicationCoordinator` holds a `std::vector<IRemoteCachePeer*>`; populate from cluster membership (Raft or gossip) via a `ClusterView` injection.
- [ ] Fanout invalidation to remote peers asynchronously (fire-and-forget with a bounded retry queue); do not block `put()` on remote acknowledgment.

### Relationships

- Roadmap row: #159 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#in-process-replication-coordinator-network-backed-peer-discovery
- Source key: roadmap:159:cache:v1.8.0:in-process-replication-coordinator-network-backed-peer-discovery

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:159:cache:v1.8.0:in-process-replication-coordinator-network-backed-peer-discovery -->
<!-- roadmap-ref: row=159;module=cache;target=v1.8.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#in-process-replication-coordinator-network-backed-peer-discovery -->
