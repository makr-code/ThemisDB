### Context

This issue implements the roadmap item 'Token Blacklist Persistence and Distributed Support' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.3.0.

Primary detail section: 7. Token Blacklist Persistence and Distributed Support

### Goal

Deliver the scoped changes for Token Blacklist Persistence and Distributed Support in src/auth/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### 7. Token Blacklist Persistence and Distributed Support

**Priority:** High  
**Target Version:** v1.3.0

`token_blacklist.cpp` stores revoked tokens in `std::unordered_set<std::string> blacklist_` (pure in-memory). On process restart all revoked tokens are forgotten — previously revoked JWT tokens become valid again until they expire naturally. In a multi-node deployment each node maintains an independent blacklist with no cross-node synchronisation.

**Implementation Notes:**
- `[ ]` Define abstract interface `ITokenBlacklist` in `include/auth/token_blacklist.h` with `add(jti, expiry)`, `isRevoked(jti)`, `purgeExpired()` methods
- `[ ]` Implement `RedisTokenBlacklist : ITokenBlacklist` backed by Redis `SET jti EX ttl NX` — use hiredis or redis-plus-plus; see `include/auth/token_blacklist.h`
- `[ ]` Implement `RocksDBTokenBlacklist : ITokenBlacklist` for single-node deployments with persistence — write jti+expiry to a dedicated CF, background thread purges expired entries
- `[ ]` Add Bloom filter pre-check (`libbloom` or hand-rolled) in the in-memory path to reduce hash-map lookups on non-revoked tokens (hot path is `isRevoked` returning `false`)
- `[ ]` Bound in-memory blacklist to `max_entries` (configurable, default 1 million) — evict by earliest expiry when capacity is reached, log a warning
- `[ ]` Unit test: revoke a token, restart process, verify token is still rejected (persistence test); revoke on node A, check on node B (distribution test)

**Performance Targets:**
- `isRevoked()` hot path (non-revoked token, warm Bloom filter): ≤ 1 µs
- Redis-backed `isRevoked()`: ≤ 2 ms P99 on local network

---

### Acceptance Criteria

- [ ] Define abstract interface `ITokenBlacklist` in `include/auth/token_blacklist.h` with `add(jti, expiry)`, `isRevoked(jti)`, `purgeExpired()` methods
- [ ] Implement `RedisTokenBlacklist : ITokenBlacklist` backed by Redis `SET jti EX ttl NX` — use hiredis or redis-plus-plus; see `include/auth/token_blacklist.h`
- [ ] Implement `RocksDBTokenBlacklist : ITokenBlacklist` for single-node deployments with persistence — write jti+expiry to a dedicated CF, background thread purges expired entries
- [ ] Add Bloom filter pre-check (`libbloom` or hand-rolled) in the in-memory path to reduce hash-map lookups on non-revoked tokens (hot path is `isRevoked` returning `false`)
- [ ] Bound in-memory blacklist to `max_entries` (configurable, default 1 million) — evict by earliest expiry when capacity is reached, log a warning
- [ ] Unit test: revoke a token, restart process, verify token is still rejected (persistence test); revoke on node A, check on node B (distribution test)
- [ ] `isRevoked()` hot path (non-revoked token, warm Bloom filter): ≤ 1 µs
- [ ] Redis-backed `isRevoked()`: ≤ 2 ms P99 on local network

### Relationships

- Roadmap row: #9 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#7-token-blacklist-persistence-and-distributed-support
- Source key: roadmap:9:auth:v1.3.0:7-token-blacklist-persistence-and-distributed-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:9:auth:v1.3.0:7-token-blacklist-persistence-and-distributed-support -->
<!-- roadmap-ref: row=9;module=auth;target=v1.3.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#7-token-blacklist-persistence-and-distributed-support -->
