### Context

This issue implements the roadmap item 'Rate Limiter — Stale Bucket Eviction and Nested Lock Contention' for the api domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v2.0.0.

Primary detail section: Rate Limiter — Stale Bucket Eviction and Nested Lock Contention

### Goal

Deliver the scoped changes for Rate Limiter — Stale Bucket Eviction and Nested Lock Contention in src/api/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### Rate Limiter — Stale Bucket Eviction and Nested Lock Contention
**Priority:** Medium
**Target Version:** v2.0.0

`include/api/rate_limiter.h` implements a token-bucket rate limiter. Two structural issues limit correctness and scalability in long-running deployments.

**Implementation Notes:**
- `[ ]` **`buckets_` map grows unbounded** (`rate_limiter.h::RateLimiter::allow()`): every unique key passed to `allow()` creates a `Bucket` entry that is never removed. In production, keys are typically tenant IDs or IP addresses; a deployment running for weeks will accumulate thousands of stale buckets. Add a TTL-based eviction pass: in `allow()` (or a dedicated background sweep), remove buckets whose `last_refill` is older than `2 × window` and whose `tokens >= capacity` (fully recharged means no active traffic).
- `[ ]` **`OperationRateLimiter::allow()` holds outer mutex while calling inner `RateLimiter::allow()`** (`rate_limiter.h`): `OperationRateLimiter::allow()` takes `mutex_` with a `std::lock_guard`, then calls `it->second->allow(key, cost)`, which in turn takes `RateLimiter::mutex_`. This is a two-mutex lock chain on every allowed request. Under high concurrency (e.g., 5,000 GraphQL requests/sec), this creates a mutex bottleneck on the outer lock. Replace the outer `std::mutex` with `std::shared_mutex` (shared lock for `allow()`/`remaining()`; exclusive lock only for `setLimit()`).
- `[ ]` **`RateLimiter::allow()` calls `steady_clock::now()` inside the lock** (`rate_limiter.h::Bucket::consume()`): `Bucket::refill()` calls `std::chrono::steady_clock::now()` while the outer `mutex_` is held. A clock syscall under a mutex adds unnecessary critical-section time. Compute `now` before acquiring the lock and pass it to `consume()`.

**Performance Targets:**
- `RateLimiter::allow()` throughput ≥ 1,000,000 calls/sec single-thread (vs. ~200,000 with current nested locks).
- Stale bucket count bounded to ≤ 2× the number of active clients at any time.

---

### Acceptance Criteria

- [ ] **`buckets_` map grows unbounded** (`rate_limiter.h::RateLimiter::allow()`): every unique key passed to `allow()` creates a `Bucket` entry that is never removed. In production, keys are typically tenant IDs or IP addresses; a deployment running for weeks will accumulate thousands of stale buckets. Add a TTL-based eviction pass: in `allow()` (or a dedicated background sweep), remove buckets whose `last_refill` is older than `2 × window` and whose `tokens >= capacity` (fully recharged means no active traffic).
- [ ] **`OperationRateLimiter::allow()` holds outer mutex while calling inner `RateLimiter::allow()`** (`rate_limiter.h`): `OperationRateLimiter::allow()` takes `mutex_` with a `std::lock_guard`, then calls `it->second->allow(key, cost)`, which in turn takes `RateLimiter::mutex_`. This is a two-mutex lock chain on every allowed request. Under high concurrency (e.g., 5,000 GraphQL requests/sec), this creates a mutex bottleneck on the outer lock. Replace the outer `std::mutex` with `std::shared_mutex` (shared lock for `allow()`/`remaining()`; exclusive lock only for `setLimit()`).
- [ ] **`RateLimiter::allow()` calls `steady_clock::now()` inside the lock** (`rate_limiter.h::Bucket::consume()`): `Bucket::refill()` calls `std::chrono::steady_clock::now()` while the outer `mutex_` is held. A clock syscall under a mutex adds unnecessary critical-section time. Compute `now` before acquiring the lock and pass it to `consume()`.
- [ ] `RateLimiter::allow()` throughput ≥ 1,000,000 calls/sec single-thread (vs. ~200,000 with current nested locks).
- [ ] Stale bucket count bounded to ≤ 2× the number of active clients at any time.

### Relationships

- Roadmap row: #141 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#rate-limiter--stale-bucket-eviction-and-nested-lock-contention
- Source key: roadmap:141:api:v2.0.0:rate-limiter-stale-bucket-eviction-and-nested-lock-contention

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:141:api:v2.0.0:rate-limiter-stale-bucket-eviction-and-nested-lock-contention -->
<!-- roadmap-ref: row=141;module=api;target=v2.0.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#rate-limiter--stale-bucket-eviction-and-nested-lock-contention -->
