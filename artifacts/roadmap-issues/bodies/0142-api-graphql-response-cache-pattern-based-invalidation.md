### Context

This issue implements the roadmap item 'GraphQL Response Cache — Pattern-Based Invalidation' for the api domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v2.0.0.

Primary detail section: GraphQL Response Cache — Pattern-Based Invalidation

### Goal

Deliver the scoped changes for GraphQL Response Cache — Pattern-Based Invalidation in src/api/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### GraphQL Response Cache — Pattern-Based Invalidation
**Priority:** Medium
**Target Version:** v2.0.0

`include/api/graphql_cache.h::ResponseCache::invalidatePattern()` contains a `TODO: Implement pattern-based invalidation` comment. The current implementation nukes the entire cache on any collection change, causing unnecessary cache misses for queries targeting unrelated collections.

**Implementation Notes:**
- `[ ]` **`ResponseCache::invalidatePattern()` always clears entire cache** (`graphql_cache.h:290`): the method receives a `pattern` argument (e.g., the collection name `"orders"`) but ignores it and calls `cache_.clear()`, invalidating all cached responses regardless of which collection they reference. Implement selective eviction: at cache insertion time, tag each `CachedResponse` with the set of collections it reads (extracted from the resolved query fields). In `invalidatePattern(collection)`, iterate the cache and evict only entries whose tag set contains `collection`. This requires extending `CachedResponse` with a `std::unordered_set<std::string> collections` field.

**Performance Targets:**
- Targeted invalidation of a single collection evicts ≤ 10% of cached entries when 10 distinct collections are active.

---

### Acceptance Criteria

- [ ] **`ResponseCache::invalidatePattern()` always clears entire cache** (`graphql_cache.h:290`): the method receives a `pattern` argument (e.g., the collection name `"orders"`) but ignores it and calls `cache_.clear()`, invalidating all cached responses regardless of which collection they reference. Implement selective eviction: at cache insertion time, tag each `CachedResponse` with the set of collections it reads (extracted from the resolved query fields). In `invalidatePattern(collection)`, iterate the cache and evict only entries whose tag set contains `collection`. This requires extending `CachedResponse` with a `std::unordered_set<std::string> collections` field.
- [ ] Targeted invalidation of a single collection evicts ≤ 10% of cached entries when 10 distinct collections are active.

### Relationships

- Roadmap row: #142 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#graphql-response-cache--pattern-based-invalidation
- Source key: roadmap:142:api:v2.0.0:graphql-response-cache-pattern-based-invalidation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:142:api:v2.0.0:graphql-response-cache-pattern-based-invalidation -->
<!-- roadmap-ref: row=142;module=api;target=v2.0.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#graphql-response-cache--pattern-based-invalidation -->
