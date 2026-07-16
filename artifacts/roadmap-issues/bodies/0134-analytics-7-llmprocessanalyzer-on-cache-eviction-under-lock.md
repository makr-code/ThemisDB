### Context

This issue implements the roadmap item '`LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 7 · `LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock

### Goal

Deliver the scoped changes for `LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 7 · `LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/llm_process_analyzer.cpp` lines 93–115, 515–530

**7a – O(N) eviction:** `putInCache()` (line 93) holds `cache_mutex` and scans all 1 000
entries linearly to find the one with the earliest expiry (lines 105–112).  Under high LLM
call rates this becomes a serialization bottleneck.  The hard-coded limit `1000` (line 105)
is not configurable from `LLMConfig`.

**7b – Expensive cache-key serialization:** `getCacheKey()` (line 515) calls
`request.process_trace.dump()` which serializes the full `nlohmann::json` object to a string
on every call — even for cache hits.  For large process traces (hundreds of events) this can
take several milliseconds in the hot request path.

**Implementation Notes:**
- `[ ]` Replace `std::unordered_map<string, CacheEntry>` + manual linear eviction with an `LRUCache<string, nlohmann::json>` backed by a doubly-linked list and hash map, giving O(1) get/put/evict — the pattern already proposed in the OLAP section above, or a simple `boost::compute::detail::lru_cache` adapter
- `[ ]` Expose `max_cache_entries` in `LLMConfig` (default 1 000) so operators can tune without recompiling
- `[ ]` In `getCacheKey()`, compute a `xxHash`/`SHA256` of `request.process_trace` rather than a full `dump()` — reduces key-build time from O(trace_size) to O(1) for the cache lookup fast path; store the full JSON only on cache miss
- `[ ]` Add a microbenchmark: `putInCache()` with 1 000 existing entries must complete in ≤ 1 µs

**Performance Targets:**
- `putInCache()` / `getFromCache()`: O(1) amortised, ≤ 1 µs P99 under 16 concurrent callers
- `getCacheKey()` for a 500-event trace: ≤ 50 µs (hash-based, not JSON dump)

---

### Acceptance Criteria

- [ ] Replace `std::unordered_map<string, CacheEntry>` + manual linear eviction with an `LRUCache<string, nlohmann::json>` backed by a doubly-linked list and hash map, giving O(1) get/put/evict — the pattern already proposed in the OLAP section above, or a simple `boost::compute::detail::lru_cache` adapter
- [ ] Expose `max_cache_entries` in `LLMConfig` (default 1 000) so operators can tune without recompiling
- [ ] In `getCacheKey()`, compute a `xxHash`/`SHA256` of `request.process_trace` rather than a full `dump()` — reduces key-build time from O(trace_size) to O(1) for the cache lookup fast path; store the full JSON only on cache miss
- [ ] Add a microbenchmark: `putInCache()` with 1 000 existing entries must complete in ≤ 1 µs
- [ ] `putInCache()` / `getFromCache()`: O(1) amortised, ≤ 1 µs P99 under 16 concurrent callers
- [ ] `getCacheKey()` for a 500-event trace: ≤ 50 µs (hash-based, not JSON dump)

### Relationships

- Roadmap row: #134 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#7--llmprocessanalyzer--on-cache-eviction-under-lock
- Source key: roadmap:134:analytics:v1.8.0:7-llmprocessanalyzer-on-cache-eviction-under-lock

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:134:analytics:v1.8.0:7-llmprocessanalyzer-on-cache-eviction-under-lock -->
<!-- roadmap-ref: row=134;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#7--llmprocessanalyzer--on-cache-eviction-under-lock -->
