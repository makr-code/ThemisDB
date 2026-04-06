# Lock-Free L1 Cache Read Path with std::shared_mutex + std::atomic

**Metadaten:**
- Source: Martin Thompson — "Mechanical Sympathy" blog + CPU cache-line sharing best practices
- URL: https://mechanical-sympathy.blogspot.com/
- Tags: performance, concurrency
- ThemisDB-Versionen: v1.9.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

High-frequency read workloads in database caches suffer from excessive locking overhead when a single `std::mutex` guards both reads and writes. Martin Thompson's "Mechanical Sympathy" series documents how false sharing of cache lines and lock contention can nullify gains from L1/L2 hardware caches. The solution is a split-path design: reads use a reader-writer lock (`std::shared_mutex`) so multiple threads share the lock concurrently, while metadata fields that are polled on every access (hit count, last-used timestamp) are maintained as `std::atomic` members to avoid any lock acquisition on the hot path.

In ThemisDB, the adaptive query cache (`src/cache/adaptive_query_cache.cpp`) applies this pattern via its `L1Entry` struct, where the result payload is protected by `shared_mutex` but per-entry counters (`hits`, `insert_epoch`) are atomics updated without any lock.

## 🎯 Core Principles

- **Reader-writer separation**: Allow many concurrent readers by distinguishing `shared_lock` (read) from `unique_lock` (write).
- **Atomic metadata fields**: Fields read on every cache probe (hit counter, epoch, flags) must be `std::atomic` to avoid taking any lock at all on the truly hot path.
- **Cache-line alignment**: Hot atomic fields should be placed at the beginning of the struct (or padded) to prevent false sharing with adjacent non-atomic data.
- **Minimal critical sections**: Write locks are held only for the microseconds needed to swap the result pointer; result construction happens before the lock is taken.
- **Epoch-based invalidation**: A single `std::atomic<uint64_t>` epoch counter lets readers detect stale entries without holding a write lock.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/cache/adaptive_query_cache.cpp` — Primary implementation of L1Entry with `std::shared_mutex` + `std::atomic` fields; upgraded in v1.9.0 as part of the cache performance overhaul.
- `src/cache/` (headers) — `L1Entry`, `CacheStats`, and `AdaptiveQueryCache` class definitions.

### What Was Adopted?

- `L1Entry` carries `std::atomic<uint64_t> hits` and `std::atomic<uint64_t> insert_epoch` that are incremented/read without any lock.
- The result payload (`std::shared_ptr<QueryResult>`) is guarded by a per-entry `mutable std::shared_mutex entry_mutex`; readers call `std::shared_lock<std::shared_mutex>` and writers call `std::unique_lock<std::shared_mutex>`.
- Cache-wide eviction uses a separate `std::shared_mutex cache_mutex` to protect the hash-map of entries; reads acquire a shared lock on the map, then a shared lock on the individual entry — two-level reader-writer locking.
- Writes to the map (insert/evict) acquire a `unique_lock` on `cache_mutex` only; individual entry write locks are nested inside.
- Result objects are constructed outside any lock; only the pointer swap is performed under `unique_lock`.

### Deviations & Rationale

- **No hazard pointers / RCU**: Full lock-free reads with RCU or hazard pointers were evaluated but rejected for v1.9.0 due to implementation complexity and the fact that the existing `shared_mutex` path already saturated the benchmark target. RCU is tracked for a future phase.
- **No padding to 64-byte cache lines**: The `L1Entry` struct does not currently carry explicit `alignas(64)` padding. This is a known gap; profiling did not show false-sharing symptoms under typical workloads but may be revisited.
- **Global statistics via atomic counters**: Global hit/miss counters are additional `std::atomic` fields on the cache object, consistent with the principle but not aligned to a dedicated cache line.

## ⚠️ Trade-offs & Limitations

- **Reader-writer lock overhead**: `std::shared_mutex` on many platforms has non-trivial overhead compared to a plain spinlock when contention is very low; the benefit materialises only under moderate-to-high read concurrency (≥4 concurrent reader threads).
- **Priority inversion risk**: Writers can starve under continuous reader pressure on some `shared_mutex` implementations (POSIX `pthread_rwlock` writer-preference vs. reader-preference varies by OS). ThemisDB relies on the platform default.
- **Two-level locking increases complexity**: Nested `cache_mutex` (shared) + `entry_mutex` (shared or exclusive) must always be acquired in a fixed order to prevent deadlock. Any future change to locking order must be carefully reviewed.
- **Atomic epoch requires wrapping awareness**: `uint64_t` epoch counters overflow after 2^64 increments. For the current insert rate this is effectively infinite, but code must not assume monotonic order after wrap.

## 🔬 Validation

- [x] Code reviewed against source material (Mechanical Sympathy, C++17 shared_mutex spec)
- [x] Unit or integration tests written (`tests/cache/adaptive_query_cache_test.cpp`)
- [x] Performance measured — cache read throughput benchmarks in `benchmarks/cache/`
- [x] Module README linked (`src/cache/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Shared Mutex Read-Write Locks](shared_mutex_read_write_locks.md)
- [OpenTelemetry Tracing](opentelemetry_tracing.md)

---
**Last Updated:** 2026-04-06
