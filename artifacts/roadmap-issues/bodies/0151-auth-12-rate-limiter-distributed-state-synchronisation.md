### Context

This issue implements the roadmap item 'Rate Limiter: Distributed State Synchronisation' for the auth domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: 12. Rate Limiter: Distributed State Synchronisation

### Goal

Deliver the scoped changes for Rate Limiter: Distributed State Synchronisation in src/auth/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### 12. Rate Limiter: Distributed State Synchronisation

**Priority:** Medium  
**Target Version:** v1.3.0

`auth_rate_limiter.cpp` uses in-process sliding-window counters protected by `std::mutex`. In a multi-node deployment each node tracks independent counters, so an attacker can bypass per-user or per-IP rate limits by spreading requests across nodes (horizontal bypass).

**Implementation Notes:**
- `[ ]` Define `IRateLimiterBackend` interface with `increment(key) -> count` and `reset(key)` operations
- `[ ]` Implement `RedisRateLimiterBackend` using a Lua atomic increment+expire script (avoids TOCTOU) for centrally consistent sliding-window counts across nodes
- `[ ]` Keep `InMemoryRateLimiterBackend` (current implementation) as the default for single-node deployments
- `[ ]` Add integration test: two in-process rate limiter instances sharing a Redis backend both observe the combined request count

---

### Acceptance Criteria

- [ ] Define `IRateLimiterBackend` interface with `increment(key) -> count` and `reset(key)` operations
- [ ] Implement `RedisRateLimiterBackend` using a Lua atomic increment+expire script (avoids TOCTOU) for centrally consistent sliding-window counts across nodes
- [ ] Keep `InMemoryRateLimiterBackend` (current implementation) as the default for single-node deployments
- [ ] Add integration test: two in-process rate limiter instances sharing a Redis backend both observe the combined request count

### Relationships

- Roadmap row: #151 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#12-rate-limiter-distributed-state-synchronisation
- Source key: roadmap:151:auth:v1.3.0:12-rate-limiter-distributed-state-synchronisation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:151:auth:v1.3.0:12-rate-limiter-distributed-state-synchronisation -->
<!-- roadmap-ref: row=151;module=auth;target=v1.3.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#12-rate-limiter-distributed-state-synchronisation -->
