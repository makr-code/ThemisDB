### Context

This issue implements the roadmap item '`DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 8 · `DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock

### Goal

Deliver the scoped changes for `DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 8 · `DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/distributed_analytics.cpp` lines 317–325

`getHealthyShardCount()` acquires `mutex_` (line 317) and calls
`e.executor->isHealthy()` for every shard entry (line 321).  `ShardQueryExecutor::isHealthy()`
is a virtual call on a remote executor abstraction — in production implementations this
involves a network ping or gRPC health-check.  Holding `mutex_` for the entire health-check
sweep blocks `addShard()`, `removeShard()`, `getShardIds()`, and the scatter-gather
`executeOnAllShards()` for the full network round-trip multiplied by the shard count.

**Implementation Notes:**
- `[ ]` Introduce a `ShardEntry::cached_healthy` field updated by a background health-monitor thread; `getHealthyShardCount()` reads the cached value under the lock (< 1 µs) instead of doing live checks
- `[ ]` Background health monitor runs at a configurable `health_check_interval` (default 5 s); uses its own dedicated mutex so it does not contend with the main `mutex_`
- `[ ]` Expose `getHealthyShardCountAsync() → std::future<size_t>` for callers that explicitly want live health data without blocking the shard registry
- `[ ]` Add a test: simulate one shard health check that takes 500 ms; assert `addShard()` completes within 5 ms during the health check

**Performance Targets:**
- `getHealthyShardCount()` (cached path): ≤ 2 µs
- Health monitor cycle for 64 shards: ≤ 5 s wall time with per-shard 1 s timeout

---

### Acceptance Criteria

- [ ] Introduce a `ShardEntry::cached_healthy` field updated by a background health-monitor thread; `getHealthyShardCount()` reads the cached value under the lock (< 1 µs) instead of doing live checks
- [ ] Background health monitor runs at a configurable `health_check_interval` (default 5 s); uses its own dedicated mutex so it does not contend with the main `mutex_`
- [ ] Expose `getHealthyShardCountAsync() → std::future<size_t>` for callers that explicitly want live health data without blocking the shard registry
- [ ] Add a test: simulate one shard health check that takes 500 ms; assert `addShard()` completes within 5 ms during the health check
- [ ] `getHealthyShardCount()` (cached path): ≤ 2 µs
- [ ] Health monitor cycle for 64 shards: ≤ 5 s wall time with per-shard 1 s timeout

### Relationships

- Roadmap row: #135 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#8--distributedanalyticsshardingcountgethealthyshardcount--network-io-under-lock
- Source key: roadmap:135:analytics:v1.8.0:8-distributedanalyticsshardingcountgethealthyshardcount-network-io-under-lock

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:135:analytics:v1.8.0:8-distributedanalyticsshardingcountgethealthyshardcount-network-io-under-lock -->
<!-- roadmap-ref: row=135;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#8--distributedanalyticsshardingcountgethealthyshardcount--network-io-under-lock -->
