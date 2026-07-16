### Context

This issue implements the roadmap item 'Configurable LRU Cache Size and TTL via Environment Variables' for the config domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Configurable LRU Cache Size and TTL via Environment Variables

### Goal

Deliver the scoped changes for Configurable LRU Cache Size and TTL via Environment Variables in src/config/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Configurable LRU Cache Size and TTL via Environment Variables
**Priority:** Medium
**Target Version:** v1.7.0

`LRUCacheWithTTL` in `config_path_resolver.cpp` is constructed with hardcoded capacity=1000 and TTL=5 minutes. Allow overriding via environment variables to support deployments with many more config paths or where aggressive caching causes stale-path issues.

**Implementation Notes:**
- `[x]` Read `THEMIS_CONFIG_CACHE_SIZE` (integer, default 1000) and `THEMIS_CONFIG_CACHE_TTL` (integer, default 300) at static initialisation time.
- `[x]` Validate ranges: capacity in [10, 100000], TTL in [1, 86400]; fall back to defaults with a `fprintf(stderr, ...)` warning if out of range (spdlog not yet initialised at static-init time).
- `[x]` Update `cache_` initialisation in the static initializer block to use env-var-aware helpers.
- `[x]` Add `ConfigPathResolver::currentCacheConfig()` method returning `CacheConfig{capacity, ttl_seconds}` for observability.
- `[x]` Document environment variables in `src/config/README.md`.

**Performance Targets:**
- Zero performance regression on `resolve()` for default config values.
- `currentCacheConfig()` is a pure read with no locking overhead.

---

### Acceptance Criteria

- [ ] Read `THEMIS_CONFIG_CACHE_SIZE` (integer, default 1000) and `THEMIS_CONFIG_CACHE_TTL` (integer, default 300) at static initialisation time.
- [ ] Validate ranges: capacity in [10, 100000], TTL in [1, 86400]; fall back to defaults with a `fprintf(stderr, ...)` warning if out of range (spdlog not yet initialised at static-init time).
- [ ] Update `cache_` initialisation in the static initializer block to use env-var-aware helpers.
- [ ] Add `ConfigPathResolver::currentCacheConfig()` method returning `CacheConfig{capacity, ttl_seconds}` for observability.
- [ ] Document environment variables in `src/config/README.md`.
- [ ] Zero performance regression on `resolve()` for default config values.
- [ ] `currentCacheConfig()` is a pure read with no locking overhead.

### Relationships

- Roadmap row: #165 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#configurable-lru-cache-size-and-ttl-via-environment-variables
- Source key: roadmap:165:config:v1.7.0:configurable-lru-cache-size-and-ttl-via-environment-variables

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:165:config:v1.7.0:configurable-lru-cache-size-and-ttl-via-environment-variables -->
<!-- roadmap-ref: row=165;module=config;target=v1.7.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#configurable-lru-cache-size-and-ttl-via-environment-variables -->
