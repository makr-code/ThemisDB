### Context

This issue implements the roadmap item 'Lock-Free L1 Read Path' for the cache domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Lock-Free L1 Read Path

### Goal

Deliver the scoped changes for Lock-Free L1 Read Path in src/cache/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Lock-Free L1 Read Path
**Priority:** High
**Target Version:** v1.7.0

`AdaptiveQueryCache::get()` currently takes an exclusive `std::lock_guard<std::mutex>` on `l1_mutex_` (line 206 of `adaptive_query_cache.cpp`) on every read, including cache hits. Under high read concurrency all reader threads serialize, defeating the purpose of the in-memory hot tier. The design constraint explicitly forbids new mutex acquisitions on `get()`.

**Implementation Notes:**
- `[ ]` Replace `l1_cache_` (`std::unordered_map` + `std::mutex`) with a concurrent hash map (e.g., `tbb::concurrent_hash_map` or a custom open-addressing map with per-bucket spinlocks) in `adaptive_query_cache.cpp`.
- `[ ]` For expiry-on-read (lines 213–215), use a compare-exchange on an atomic `expired` flag so only one thread performs the erase while others proceed to the L2 path.
- `[ ]` Update `l1_eviction_strategy_->onAccess()` to use a lock-free counter (std::atomic) per entry rather than calling into the eviction strategy under the lock.
- `[ ]` Benchmark: measure `get()` throughput with 16 reader threads before and after; target ≥ 3× improvement on L1 hit path.

**Performance Targets:**
- L1 hit path throughput: ≥ 5 M ops/s per core under 16-thread contention.

---

### Acceptance Criteria

- [ ] Replace `l1_cache_` (`std::unordered_map` + `std::mutex`) with a concurrent hash map (e.g., `tbb::concurrent_hash_map` or a custom open-addressing map with per-bucket spinlocks) in `adaptive_query_cache.cpp`.
- [ ] For expiry-on-read (lines 213–215), use a compare-exchange on an atomic `expired` flag so only one thread performs the erase while others proceed to the L2 path.
- [ ] Update `l1_eviction_strategy_->onAccess()` to use a lock-free counter (std::atomic) per entry rather than calling into the eviction strategy under the lock.
- [ ] Benchmark: measure `get()` throughput with 16 reader threads before and after; target ≥ 3× improvement on L1 hit path.
- [ ] L1 hit path throughput: ≥ 5 M ops/s per core under 16-thread contention.

### Relationships

- Roadmap row: #53 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#lock-free-l1-read-path
- Source key: roadmap:53:cache:v1.7.0:lock-free-l1-read-path

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:53:cache:v1.7.0:lock-free-l1-read-path -->
<!-- roadmap-ref: row=53;module=cache;target=v1.7.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#lock-free-l1-read-path -->
