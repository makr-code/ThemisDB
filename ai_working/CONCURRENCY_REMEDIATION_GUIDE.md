# Concurrency Remediation Guide — ThemisDB

**Scope:** C++ concurrency safe patterns, CERT CON-compliant fixes, and a
comprehensive false-positive catalog for the concurrency scanner findings in
ThemisDB v1.5.0.

---

## 1. Memory Ordering Reference

### 1.1 Quick Decision Table

| Situation | Use |
|---|---|
| Increment-only stats counter, no reader synchronisation required | `memory_order_relaxed` |
| Release a write so readers see it via `acquire` | `memory_order_release` |
| Observe a prior `release` write | `memory_order_acquire` |
| CAS **success** path: publish data alongside the CAS | `memory_order_acq_rel` |
| CAS **failure** path: only re-read current value | `memory_order_relaxed` |
| Setting a flag that gates other operations | `memory_order_release` (store) + `memory_order_acquire` (load) |
| Need to reason about one global order | `memory_order_seq_cst` (expensive; avoid unless necessary) |

### 1.2 The CAS Ordering Bug (CWE-362)

#### Before (Bug — `changefeed.cpp` pre-fix)

```cpp
// WRONG: relaxed success does not synchronise with readers
uint64_t persisted = persisted_sequence_.load(std::memory_order_relaxed);
while (persisted < seq
       && !persisted_sequence_.compare_exchange_weak(
              persisted, seq, std::memory_order_relaxed)) {
}
```

**What goes wrong:** Thread A writes RocksDB data, then tries to advance
`persisted_sequence_` with a relaxed CAS.  Thread B reads
`persisted_sequence_` with `acquire`.  Because the CAS success is `relaxed`,
there is no happens-before edge between A's write and B's subsequent read —
B could observe the new sequence number while A's writes are still pending in
CPU store buffers (ARM, POWER, RISC-V).

#### After (Fix)

```cpp
// CORRECT: acq_rel success creates happens-before with any acquire-load
uint64_t persisted = persisted_sequence_.load(std::memory_order_relaxed);
while (persisted < seq
       && !persisted_sequence_.compare_exchange_weak(
              persisted, seq,
              std::memory_order_acq_rel,   // success: release data, acquire current
              std::memory_order_relaxed))  // failure: re-read only
{
}
```

**Why relaxed on failure is safe:** When the CAS fails, `persisted` is updated
from the atomic (hardware provides this), and the loop retries.  The next
iteration's successful CAS will carry the required acq_rel fence.  No
additional synchronisation is needed on the failure path.

---

### 1.3 The Flag Store/Load Ordering Bug (CWE-362)

#### Before (Bug — `wire_protocol_server.cpp` pre-fix)

```cpp
// WRONG: both load and store are relaxed
if (overloaded_.load(std::memory_order_relaxed) && ...) {
    overloaded_.store(false, std::memory_order_relaxed);
}
```

**What goes wrong:** Thread A clears `overloaded_` with a relaxed store.
Thread B reads `overloaded_` with a relaxed load and may observe the old
`true` value even after A has already dropped the connection count below the
threshold — causing unnecessary connection rejections.

#### After (Fix)

```cpp
// CORRECT: acquire/release pair establishes happens-before
if (overloaded_.load(std::memory_order_acquire) && ...) {
    // release: all side-effects (connection count decrement) visible to
    // threads that subsequently acquire-load this flag.
    overloaded_.store(false, std::memory_order_release);
}
```

**Rule of thumb:** Any boolean flag used for inter-thread signalling should
use `release` on the write side and `acquire` on the read side.  `relaxed` is
only safe for pure counters where no ordering relationship is required.

---

### 1.4 Atomic Reset Clarity (content_metrics.cpp)

```cpp
// BEFORE: implicit seq_cst from operator=, misleading to readers
total_ingestions_ = 0;

// AFTER: explicit intent documented
total_ingestions_.store(0, std::memory_order_relaxed);
// rationale comment: reset() is called under external coordination;
// relaxed avoids unnecessary pipeline stalls.
```

Although `operator=(0)` on `std::atomic<uint64_t>` is correct (it calls
`store` with `seq_cst`), the explicit form is preferred because:
1. It documents the intended ordering at the call-site.
2. It avoids the unnecessary sequential-consistency fence when batch-resetting
   many counters, reducing CPU pipeline stalls.
3. It is easier to grep for intentional ordering choices during code review.

---

## 2. Safe Patterns Library — `safe_concurrency.h`

See `include/security/safe_concurrency.h` for full API.

### 2.1 Thread-Safe Counter

```cpp
#include "security/safe_concurrency.h"
using namespace themis::security;

// Instead of: std::atomic<uint64_t> errors_{0};
ThreadSafeCounter<uint64_t> errors_;

errors_.increment();          // acq_rel
uint64_t snap = errors_.load(); // acquire
errors_.reset();              // relaxed (call under coordination)
```

### 2.2 Monotonic Sequencer (changefeed pattern)

```cpp
MonotonicSequencer persisted_seq_;

// Producer — replace the bare CAS loop:
persisted_seq_.tryAdvance(new_sequence);

// Consumer:
uint64_t last_seen = persisted_seq_.last(); // acquire
```

### 2.3 Shared Data Guard (replaces bare mutex + field)

```cpp
SharedDataGuard<std::unordered_map<std::string, int>> counts_;

// Writer:
counts_.lock()->emplace("key", 1);

// Reader (with shared_mutex):
SharedDataGuard<std::unordered_map<std::string, int>, std::shared_mutex> counts2_;
auto view = counts2_.lock_shared();
auto it = view->find("key");
```

### 2.4 Safe CAS

```cpp
SafeCAS<uint64_t> hwm_{0};

// Replace compare_exchange_weak:
uint64_t expected = hwm_.load();
while (expected < target) {
    if (hwm_.trySet(expected, target)) break;
}
```

### 2.5 Scoped Flag (overloaded_ pattern)

```cpp
ScopedFlag overloaded_;

// On overload (produces release):
if (overloaded_.setIfClear()) {
    log_overload();
}

// On recovery (produces release):
if (overloaded_.isSet() && conn_count < limit) {  // acquire
    overloaded_.clear();  // release
}
```

---

## 3. CERT C++ Concurrency Rules Reference

| Rule | Description | ThemisDB Pattern |
|---|---|---|
| CON50-CPP | Do not destroy a mutex while it is locked | Use `std::lock_guard` / `std::unique_lock`; never call `delete` on a mutex that may be locked |
| CON51-CPP | Ensure held locks are released on exceptions | `std::lock_guard` is exception-safe by definition; raw `mutex.lock()` / `mutex.unlock()` is not |
| CON52-CPP | Prevent data races when accessing bit-fields | Not applicable (ThemisDB does not use bit-fields for concurrent state) |
| CON53-CPP | Avoid deadlock by locking in a predefined order | Use `LockOrderGuard::lockTwo()` / `lockThree()` for multi-mutex acquisition |
| CON54-CPP | Wrap functions that can spuriously wake up in a loop | All `std::condition_variable::wait()` calls use a predicate lambda |
| CON55-CPP | Preserve thread safety and liveness when using condition variables | ThemisDB uses `wait(lock, predicate)` uniformly |
| CON56-CPP | Do not speculatively lock a non-recursive mutex that is already owned | Plugin manager unlock/relock is intentional and documented |

---

## 4. False Positive Catalog

This section documents scanner categories that generate systematic false
positives in ThemisDB and explains why they are safe.

### 4.1 `deadlock_risk` — Sequential Single-Mutex Blocks

**Pattern detected:**
```cpp
void MyClass::doWork() {
    {
        std::lock_guard<std::mutex> lock(mu_a_);
        // ... work on data_a_ ...
    }
    // Other code ...
    {
        std::lock_guard<std::mutex> lock(mu_b_);
        // ... work on data_b_ ...
    }
}
```

**Why the scanner flags it:** The scanner sees two lock acquisitions in the
same function and assumes they could be nested.

**Why it is safe:** Each `{}` block releases its lock before the next begins.
At no point are both locks held simultaneously.  There is no cycle in the
lock-acquisition order, so there is no deadlock risk.

**Files affected (representative sample):**
- `src/server/plugin_manager.cpp`
- `src/storage/index_manager.cpp`
- `src/cache/lru_cache.cpp`
- `src/scheduler/task_scheduler.cpp`
- `src/replication/replication_manager.cpp`
- Total scanner-reported instances: ~218

**Remediation:** None required.  The pattern is correct.  Add
`THEMIS_GUARDED_BY` annotations where helpful to silence future scanner
runs.

---

### 4.2 `missing_lock` / `double_lock` — Intentional Unlock/Relock

**Pattern detected (`plugin_manager.cpp`):**
```cpp
void PluginManager::loadPlugin(const std::string& name) {
    std::unique_lock<std::mutex> lock(plugins_mutex_);
    if (loading_set_.count(name)) return;  // already loading
    loading_set_.insert(name);
    lock.unlock();  // <<< scanner flags this

    // Recursive load: plugin A may depend on plugin B.
    // Holding the lock here would deadlock.
    doLoad(name);

    lock.lock();    // <<< scanner flags this
    loading_set_.erase(name);
}
```

**Why the scanner flags it:** It detects `unlock()` followed by `lock()` on
the same lock object and reports it as a "double-lock" or "missing-lock"
pattern.

**Why it is safe:** This is a deliberately designed recursive-load pattern.
The lock is released before calling `doLoad()` to allow the recursive call
chain to re-enter `loadPlugin()` without deadlocking.  The `loading_set_`
membership check at the top of the function prevents infinite recursion.

**Files affected:** `src/plugins/plugin_manager.cpp` (4 instances).

**Remediation:** None required.  The pattern is correct.  A comment
(`// Intentional: release lock before recursive load to prevent deadlock`)
is sufficient documentation.

---

### 4.3 `unsafe_singleton` — C++11 Magic Statics

**Pattern detected:**
```cpp
MyRegistry& MyRegistry::instance() {
    static MyRegistry inst{};  // <<< scanner flags as "unsafe singleton"
    return inst;
}
```

**Why the scanner flags it:** Older static-analysis rules (pre-C++11) flagged
function-local statics as unsafe because the C++98 standard did not guarantee
thread-safe initialisation.

**Why it is safe in ThemisDB:** ThemisDB targets C++17 (see `CMakeLists.txt`
`set(CMAKE_CXX_STANDARD 17)`).  ISO C++11 §6.7 paragraph 4 explicitly
states: *"If control enters the declaration concurrently while the variable is
being initialized, the concurrent execution shall wait for completion of the
initialization."*  All major compilers (GCC ≥ 4.3, Clang ≥ 3.0, MSVC ≥ 2015
with `/Zc:threadSafeInit`) implement this guarantee.

**Files affected:** `src/config/config_loader.cpp`, `src/core/registry.cpp`.

**Remediation:** None required.  Use `SingletonHolder<T>` from
`safe_concurrency.h` as self-documenting wrapper if desired.

---

### 4.4 `memory_order` — Relaxed Stats Counters

**Pattern detected:**
```cpp
stats_.total_requests.fetch_add(1, std::memory_order_relaxed);
```

**Why the scanner flags it:** The scanner identifies all `relaxed` accesses as
potential ordering violations.

**Why it is safe:** These are monotonically-increasing telemetry counters.
No correctness-critical operation waits for these values to reach a specific
threshold.  Prometheus / OpenTelemetry scrape them with eventual-consistency
semantics.  Using `relaxed` is the accepted idiom for high-throughput counters
(see Herb Sutter "atomic<> Weapons", cppreference "memory_order" §relaxed).

**Files affected (representative sample):**
`content_metrics.cpp`, `vision_config.cpp`, `wire_stats.cpp`,
`query_stats.cpp`, `cdc_stats.cpp`, `index_stats.cpp`,
`network_audit.cpp`, `grpc_transport.cpp`, `consumer_group.cpp`,
`changefeed_buffer.cpp`, `delivery_tracker.cpp`, `dead_letter_queue.cpp`.

**Remediation:** None required.  Add a comment near the counter declaration:
```cpp
// Telemetry only — relaxed ordering is intentional (no correctness dependency).
std::atomic<uint64_t> total_requests_{0};
```

---

## 5. Checklist for New Concurrent Code

Before submitting a PR that adds or modifies concurrent code, verify:

- [ ] Every `std::atomic` field has a comment explaining the intended ordering.
- [ ] CAS success ordering is at least `acq_rel` if the CAS publishes data.
- [ ] Flag stores use `release`; flag loads use `acquire`.
- [ ] Multi-mutex acquisition uses `LockOrderGuard` or `std::lock`.
- [ ] No `lock_guard` is followed by manual `unlock()` without documenting the pattern.
- [ ] New singletons use `SingletonHolder<T>` or document the C++11 guarantee.
- [ ] Relaxed atomics are labelled "telemetry only" or equivalent.
- [ ] `SharedDataGuard` is used for mutable shared state instead of bare mutex + field.
