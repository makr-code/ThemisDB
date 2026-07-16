### Context

This issue implements the roadmap item 'Thread-Safety: Add `std::mutex` to `JWTValidator` JWKS Cache' for the auth domain. It is sourced from the consolidated roadmap under 🔴 Critical Priority and targets milestone v1.1.0.

Primary detail section: 1. Thread-Safety: Add Mutex to `JWTValidator` JWKS Cache

### Goal

Deliver the scoped changes for Thread-Safety: Add `std::mutex` to `JWTValidator` JWKS Cache in src/auth/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### 1. Thread-Safety: Add Mutex to `JWTValidator` JWKS Cache

**Priority:** Critical  
**Target Version:** v1.1.0

`include/auth/jwt_validator.h` declares `jwks_cache_` (line 192) and `jwks_cache_time_` (line 193) as plain non-atomic member fields. There is **no `mutable std::mutex`** guarding them in the header or in `jwt_validator.cpp`. When multiple threads call `JWTValidator::validate()` concurrently and the cache expires, they all race into `fetchJWKS()` simultaneously — writing `jwks_cache_` and `jwks_cache_time_` from multiple threads is a data race (undefined behaviour under C++11 and later).

**Implementation Notes:**
- `[ ]` Add `mutable std::shared_mutex jwks_cache_mutex_` to `jwt_validator.h` alongside `jwks_cache_` (`jwt_validator.h:192`)
- `[ ]` Wrap all reads of `jwks_cache_` in `fetchJWKS()` with `std::shared_lock` and all writes with `std::unique_lock` (`jwt_validator.cpp:98-174`)
- `[ ]` Implement "double-checked locking" pattern: acquire shared lock first, check staleness, upgrade to unique lock only if refresh is needed, then re-check to avoid thundering-herd on cache expiry
- `[ ]` Add unit test: spawn 32 threads each calling `validate()` concurrently with cache TTL of 0 — verify no crash under Thread Sanitizer (TSAN)

**Performance Targets:**
- Zero-overhead on warm-cache path (shared_lock is reader-writer; concurrent readers proceed in parallel)
- At most one actual HTTP fetch per cache expiry under concurrent load

---

### Acceptance Criteria

- [ ] Add `mutable std::shared_mutex jwks_cache_mutex_` to `jwt_validator.h` alongside `jwks_cache_` (`jwt_validator.h:192`)
- [ ] Wrap all reads of `jwks_cache_` in `fetchJWKS()` with `std::shared_lock` and all writes with `std::unique_lock` (`jwt_validator.cpp:98-174`)
- [ ] Implement "double-checked locking" pattern: acquire shared lock first, check staleness, upgrade to unique lock only if refresh is needed, then re-check to avoid thundering-herd on cache expiry
- [ ] Add unit test: spawn 32 threads each calling `validate()` concurrently with cache TTL of 0 — verify no crash under Thread Sanitizer (TSAN)
- [ ] Zero-overhead on warm-cache path (shared_lock is reader-writer; concurrent readers proceed in parallel)
- [ ] At most one actual HTTP fetch per cache expiry under concurrent load

### Relationships

- Roadmap row: #1 (🔴 Critical Priority)
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache
- Source key: roadmap:1:auth:v1.1.0:1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:1:auth:v1.1.0:1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache -->
<!-- roadmap-ref: row=1;module=auth;target=v1.1.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache -->
