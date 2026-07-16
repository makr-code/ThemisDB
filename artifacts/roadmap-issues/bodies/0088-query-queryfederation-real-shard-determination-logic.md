### Context

This issue implements the roadmap item '`QueryFederation`: Real Shard Determination Logic' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: `QueryFederation`: Real Shard Determination Logic

### Goal

Deliver the scoped changes for `QueryFederation`: Real Shard Determination Logic in src/query/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### `QueryFederation`: Real Shard Determination Logic
**Priority:** High
**Target Version:** v1.6.0

`query_federation.cpp` line 348: "TODO: Implement actual shard determination logic". All federated queries currently default to broadcasting to all shards, making federation performance O(N shards) regardless of the query's key range.

**Implementation Notes:**
- `[ ]` Implement shard key routing: use `ShardingManager::getShardsForKeyRange(collection, min_key, max_key)` to route range queries to only the relevant shards.
- `[ ]` For point lookups, route to the single shard owning the key via `ShardingManager::getShardForKey(collection, key)`.
- `[ ]` Retain broadcast for queries without a shard key predicate (full-collection scans); log a `WARN` when broadcasting to > 10 shards.
- `[ ]` Add unit tests: 3-shard setup, point lookup routes to 1 shard; range query routes to 2 shards; full scan broadcasts to all 3.

---

### Acceptance Criteria

- [ ] Implement shard key routing: use `ShardingManager::getShardsForKeyRange(collection, min_key, max_key)` to route range queries to only the relevant shards.
- [ ] For point lookups, route to the single shard owning the key via `ShardingManager::getShardForKey(collection, key)`.
- [ ] Retain broadcast for queries without a shard key predicate (full-collection scans); log a `WARN` when broadcasting to > 10 shards.
- [ ] Add unit tests: 3-shard setup, point lookup routes to 1 shard; range query routes to 2 shards; full scan broadcasts to all 3.

### Relationships

- Roadmap row: #88 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#queryfederation-real-shard-determination-logic
- Source key: roadmap:88:query:v1.6.0:queryfederation-real-shard-determination-logic

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:88:query:v1.6.0:queryfederation-real-shard-determination-logic -->
<!-- roadmap-ref: row=88;module=query;target=v1.6.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#queryfederation-real-shard-determination-logic -->
