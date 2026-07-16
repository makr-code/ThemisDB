### Context

This issue implements the roadmap item '`RedisCacheCoordinator` Async Pub/Sub Subscription Loop' for the cache domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: `RedisCacheCoordinator` Async Pub/Sub Subscription Loop

### Goal

Deliver the scoped changes for `RedisCacheCoordinator` Async Pub/Sub Subscription Loop in src/cache/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### `RedisCacheCoordinator` Async Pub/Sub Subscription Loop
**Priority:** High
**Target Version:** v1.7.0

`redis_cache_coordinator.cpp` uses synchronous blocking `hiredis` calls (`redisCommand`) for both PUBLISH and SUBSCRIBE. The subscription thread blocks indefinitely on `redisGetReply`. On Redis disconnect, the thread silently exits without notifying the coordinator of the failure; reconnection is only triggered on the next PUBLISH call.

**Implementation Notes:**
- `[ ]` Replace synchronous `hiredis` calls with `hiredis-async` + `libuv` or a dedicated async event loop thread to avoid blocking the coordinator's callers.
- `[ ]` Implement a reconnection health loop: if the subscription thread exits, schedule reconnect with exponential back-off (1 s, 2 s, 4 s, max 30 s) and emit a `cache.redis.reconnect` metric.
- `[ ]` Expose `RedisCacheCoordinator::isConnected()` observable via `GET /v1/admin/cache/health`.
- `[ ]` The Windows stub (line 80 of `distributed_cache_coordinator.cpp`) should be replaced with a proper compile-time feature flag; the warning log on every construction is noisy in tests.

---

### Acceptance Criteria

- [ ] Replace synchronous `hiredis` calls with `hiredis-async` + `libuv` or a dedicated async event loop thread to avoid blocking the coordinator's callers.
- [ ] Implement a reconnection health loop: if the subscription thread exits, schedule reconnect with exponential back-off (1 s, 2 s, 4 s, max 30 s) and emit a `cache.redis.reconnect` metric.
- [ ] Expose `RedisCacheCoordinator::isConnected()` observable via `GET /v1/admin/cache/health`.
- [ ] The Windows stub (line 80 of `distributed_cache_coordinator.cpp`) should be replaced with a proper compile-time feature flag; the warning log on every construction is noisy in tests.

### Relationships

- Roadmap row: #54 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#rediscachecoordinator-async-pubsub-subscription-loop
- Source key: roadmap:54:cache:v1.7.0:rediscachecoordinator-async-pubsub-subscription-loop

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:54:cache:v1.7.0:rediscachecoordinator-async-pubsub-subscription-loop -->
<!-- roadmap-ref: row=54;module=cache;target=v1.7.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#rediscachecoordinator-async-pubsub-subscription-loop -->
