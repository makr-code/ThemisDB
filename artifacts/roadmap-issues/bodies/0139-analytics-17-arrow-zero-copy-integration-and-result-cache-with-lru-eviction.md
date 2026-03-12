### Context

This issue implements the roadmap item 'Arrow Zero-Copy Integration and Result Cache with LRU Eviction' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 17 · Arrow Zero-Copy Integration and Result Cache with LRU Eviction

### Goal

Deliver the scoped changes for Arrow Zero-Copy Integration and Result Cache with LRU Eviction in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 17 · Arrow Zero-Copy Integration and Result Cache with LRU Eviction
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/analytics_export.cpp`, `src/analytics/olap.cpp`, `src/analytics/arrow_export.cpp`

`analytics_export.cpp` line 341 allocates a `std::vector<uint8_t> chunk(data.begin()+offset, …)` for every chunk during Arrow IPC streaming — unnecessary copy when the source buffer is already contiguous.  The OLAP result cache in `olap.cpp` can grow unbounded (no eviction policy).

**Implementation Notes:**
- `[ ]` Use `arrow::Buffer::Wrap()` or `arrow::MutableBuffer` zero-copy wrappers instead of copying bytes into `std::vector<uint8_t>` during Arrow IPC serialization in `analytics_export.cpp` line 341
- `[ ]` Implement `LRUCache<std::string, OLAPResult>` (doubly-linked list + `unordered_map`, max 1 000 entries configurable) for OLAP query result caching — current implementation has no eviction
- `[ ]` Cache key for OLAP must be computed from a normalized query representation (sorted dimensions, canonical filter order) so semantically equivalent queries hit the same entry
- `[ ]` Add TTL-based invalidation: cached entries older than `cache_ttl_ms` (configurable, default 60 s) are evicted on next access or by a background cleanup thread

**Performance Targets:**
- Arrow IPC export copy overhead: ≤ 1 % of total export time (zero-copy path)
- OLAP cache hit rate for repeated identical queries: ≥ 80 % in typical dashboard workloads

---

### Acceptance Criteria

- [ ] Use `arrow::Buffer::Wrap()` or `arrow::MutableBuffer` zero-copy wrappers instead of copying bytes into `std::vector<uint8_t>` during Arrow IPC serialization in `analytics_export.cpp` line 341
- [ ] Implement `LRUCache<std::string, OLAPResult>` (doubly-linked list + `unordered_map`, max 1 000 entries configurable) for OLAP query result caching — current implementation has no eviction
- [ ] Cache key for OLAP must be computed from a normalized query representation (sorted dimensions, canonical filter order) so semantically equivalent queries hit the same entry
- [ ] Add TTL-based invalidation: cached entries older than `cache_ttl_ms` (configurable, default 60 s) are evicted on next access or by a background cleanup thread
- [ ] Arrow IPC export copy overhead: ≤ 1 % of total export time (zero-copy path)
- [ ] OLAP cache hit rate for repeated identical queries: ≥ 80 % in typical dashboard workloads

### Relationships

- Roadmap row: #139 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#17--arrow-zero-copy-integration-and-result-cache-with-lru-eviction
- Source key: roadmap:139:analytics:v1.8.0:17-arrow-zero-copy-integration-and-result-cache-with-lru-eviction

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:139:analytics:v1.8.0:17-arrow-zero-copy-integration-and-result-cache-with-lru-eviction -->
<!-- roadmap-ref: row=139;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#17--arrow-zero-copy-integration-and-result-cache-with-lru-eviction -->
