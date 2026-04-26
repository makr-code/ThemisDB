# std::shared_mutex for Concurrent Read / Exclusive Write Locks

**Metadaten:**
- Source: ISO C++17 Standard §30.6.5 (shared_mutex); "The Art of Multiprocessor Programming" — Herlihy & Shavit (2008)
- URL: https://en.cppreference.com/w/cpp/thread/shared_mutex | https://doi.org/10.1016/C2011-0-07738-9
- Tags: concurrency, performance
- ThemisDB-Versionen: v1.8.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Many shared data structures in a database server have a strongly skewed read/write ratio: configuration is read thousands of times per second but updated rarely; cache entries are looked up frequently but invalidated infrequently. Using a plain `std::mutex` serialises all readers against each other unnecessarily, leaving CPU cores idle that could be serving concurrent read requests. C++17's `std::shared_mutex` (reader-writer lock) allows unlimited concurrent readers (via `std::shared_lock`) while requiring exclusive access for writers (via `std::unique_lock`), matching the access pattern precisely.

Herlihy & Shavit's "The Art of Multiprocessor Programming" Chapter 8 provides the theoretical foundation for reader-writer lock correctness (progress conditions, fairness, starvation freedom) that guided ThemisDB's choice of implementation and usage patterns.

## 🎯 Core Principles

- **shared_lock for reads**: All read-only accesses to the guarded data use `std::shared_lock<std::shared_mutex>`, allowing concurrent reads from multiple threads.
- **unique_lock for writes**: All mutations use `std::unique_lock<std::shared_mutex>`, excluding all other readers and writers.
- **Lock acquisition order is fixed**: Where multiple `shared_mutex` instances must be locked together, acquisition order is always consistent (documented per data structure) to prevent deadlock.
- **No lock escalation**: A `shared_lock` must never be upgraded to `unique_lock` on the same mutex without releasing the shared lock first; lock escalation is not supported by `std::shared_mutex` and leads to deadlock.
- **Minimise write lock scope**: Write-lock critical sections are kept as short as possible; expensive computation (e.g., schema validation, cache eviction selection) happens before acquiring the write lock, with only the pointer/value swap inside the lock.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/config/config_schema_validator.h` — `ConfigEncryptedStore` uses `mutable std::shared_mutex mutex_`; `getConfig()` (read) takes `std::shared_lock`; `updateConfig()` (write) takes `std::unique_lock`.
- `src/cache/adaptive_query_cache.cpp` — Two-level locking: outer `cache_mutex_` (shared for map reads, exclusive for map modifications) + per-entry `entry_mutex_` (shared for result reads, exclusive for result updates). See `lock_free_cache_reads.md`.
- `src/server/cluster_membership.cpp` — Cluster node set is protected by `shared_mutex`; gossip readers use `shared_lock`; membership update uses `unique_lock`.
- `src/index/schema_registry.cpp` — Schema registry reads use `shared_lock`; schema registration uses `unique_lock`.

### What Was Adopted?

- `mutable std::shared_mutex mutex_` declared in each class that manages shared state.
- Read path: `std::shared_lock<std::shared_mutex> lock(mutex_); return data_;`
- Write path: `std::unique_lock<std::shared_mutex> lock(mutex_); data_ = std::move(new_data);`
- RAII lock objects (`std::shared_lock`, `std::unique_lock`) used exclusively; no manual `lock()`/`unlock()` calls.
- Critical sections documented with inline comments indicating what data they protect.
- Thread-safety guarantees documented in class-level doxygen comments (`@threadsafe`).

### Deviations & Rationale

- **No upgrade mutex (boost::upgrade_mutex)**: Boost provides a three-state upgrade mutex that allows read → upgrade → write without releasing. ThemisDB does not use this because the upgrade-to-write pattern is rare and the simpler release-and-reacquire approach avoids the additional complexity and potential for priority inversion.
- **std::shared_mutex over pthread_rwlock**: On Linux, `std::shared_mutex` is implemented with `pthread_rwlock_t` under the hood. The C++ standard type was chosen for portability and RAII integration. Where profiling showed `std::shared_mutex` contention, the data was restructured to reduce write frequency rather than switching to a spinlock.
- **Fairness not tunable**: `std::shared_mutex` fairness (reader-preference vs. writer-preference) is implementation-defined. On glibc, `pthread_rwlock` defaults to reader-preference, which can starve writers under heavy reader load. This is acceptable for ThemisDB's workloads (writes are infrequent); if starvation is observed, a write-preference mutex can be configured via `pthread_rwlockattr_setkind_np`.

## ⚠️ Trade-offs & Limitations

- **High-frequency write paths should not use shared_mutex**: If writes are as frequent as reads, `shared_mutex` offers no benefit over `std::mutex` (due to the bookkeeping overhead) and may be slower. For write-heavy paths (e.g., high-throughput log append), a plain mutex or lock-free queue is preferred.
- **False sharing with mutex storage**: The `shared_mutex` object itself can occupy a cache line shared with the data it protects. Placing the mutex and the data it protects in separate cache lines (via `alignas(64)`) avoids this; ThemisDB applies this only for the most contended cases.
- **No condition variable support with shared_lock**: `std::condition_variable_any` must be used (rather than `std::condition_variable`) when waiting on a `shared_mutex`. This is documented in the concurrency guide.
- **Recursive locking not supported**: `std::shared_mutex` does not support recursive locking; attempting to acquire a `shared_lock` from a thread that already holds a `unique_lock` on the same mutex is undefined behaviour.

## 🔬 Validation

- [x] Code reviewed against C++17 §30.6.5 and Herlihy & Shavit Chapter 8
- [x] Thread-safety verified with TSan (`-fsanitize=thread`) in CI for all shared_mutex-guarded paths
- [x] Unit tests in `tests/config/config_schema_validator_test.cpp` use multiple reader threads + occasional writer threads under `std::latch` synchronisation
- [x] Performance benchmark confirms read throughput scales linearly with thread count up to 16 reader threads
- [x] Module README linked (`src/config/README.md`, `src/cache/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Lock-Free Cache Reads](lock_free_cache_reads.md)
- [Boost.Asio Async I/O](boost_asio_async_io.md)

---
**Last Updated:** 2026-04-06
