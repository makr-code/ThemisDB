# ThemisDB - MVCC Concurrency & Thread-Safety Guide

## MVCC Fundamentals

### What is MVCC?

Multi-Version Concurrency Control (MVCC) allows readers and writers to operate
on different versions of the same data simultaneously, avoiding read-write
contention that plagues traditional locking schemes.

```text
Timeline:
  T=0  T=1  T=2  T=3  T=4
  ─────────────────────────
  [v1] ──── read ─────────   ← Reader sees v1 snapshot
       [v2] write ──────────  ← Writer creates v2 without blocking readers
```

### Version Numbering Schema

```cpp
// Each transaction gets a monotonically increasing version number
using VersionId = uint64_t;

struct VersionedRecord {
    VersionId created_at;   // Transaction that created this version
    VersionId deleted_at;   // UINT64_MAX if still live (not deleted)
    std::vector<uint8_t> data;
};

// Version visibility rule:
// A record version V is visible to transaction T if:
//   V.created_at <= T.snapshot_version  AND
//   V.deleted_at  >  T.snapshot_version
bool isVisible(const VersionedRecord& v, VersionId snapshot) {
    return v.created_at <= snapshot && v.deleted_at > snapshot;
}
```

### Snapshot Isolation

```cpp
class Transaction {
public:
    explicit Transaction(VersionStore& store)
        : store_(store),
          snapshot_version_(store.currentVersion()),
          txn_id_(store.beginTransaction()) {}

    // All reads use the snapshot taken at BEGIN, not the current state
    std::optional<Record> read(const Key& key) const {
        return store_.readAtVersion(key, snapshot_version_);
    }

    bool commit() {
        return store_.commitTransaction(txn_id_, write_set_);
    }

private:
    VersionStore& store_;             // Reference to the owning store
    VersionId snapshot_version_;      // Frozen at BEGIN
    TransactionId txn_id_;
    WriteSet write_set_;
};
```

---

## Lock Strategies in ThemisDB

### Read-Write Lock (RWLock)

```cpp
// ✅ Good: Use shared_mutex for read-heavy workloads
class MVCCIndex {
    mutable std::shared_mutex rw_mutex_;
    std::unordered_map<Key, VersionChain> index_;

public:
    // Multiple concurrent readers allowed
    std::optional<Record> read(const Key& key) const {
        std::shared_lock lock(rw_mutex_);  // Shared (read) lock
        auto it = index_.find(key);
        return it != index_.end() ? it->second.latestVisible() : std::nullopt;
    }

    // Exclusive writer access
    void write(const Key& key, Record record) {
        std::unique_lock lock(rw_mutex_);  // Exclusive (write) lock
        index_[key].addVersion(std::move(record));
    }
};
```

### Optimistic Locking

```cpp
// ✅ Good: Optimistic concurrency—validate at commit, not at read
class OptimisticTransaction {
public:
    explicit OptimisticTransaction(VersionStore& store) : store_(store) {}

    Record read(const Key& key) {
        auto [record, version] = store_.readWithVersion(key);
        read_set_[key] = version;  // Record version for validation
        return record;
    }

    bool commit() {
        std::unique_lock lock(store_.commitMutex());

        // Validate: check no read item has changed since we read it
        for (const auto& [key, expected_version] : read_set_) {
            if (store_.currentVersion(key) != expected_version) {
                return false;  // Conflict detected → abort
            }
        }

        // Apply writes
        store_.applyWrites(write_set_);
        return true;
    }

private:
    VersionStore& store_;
    std::unordered_map<Key, VersionId> read_set_;
    WriteSet write_set_;
};
```

### Timestamp-based Ordering

```cpp
// ✅ Good: Use logical clocks for distributed/multi-threaded ordering
class LogicalClock {
    std::atomic<uint64_t> counter_{0};
public:
    uint64_t tick() {
        return counter_.fetch_add(1, std::memory_order_acq_rel);
    }
    uint64_t current() const {
        return counter_.load(std::memory_order_acquire);
    }
};

// Global clock used by all transactions
inline LogicalClock g_txn_clock;
```

### MVCC vs. Pessimistic Locking

| Aspect | MVCC | Pessimistic Locking |
|--------|------|---------------------|
| Read-write contention | None (readers never block writers) | Readers block writers |
| Write-write contention | Detect at commit (abort one) | Block until lock released |
| Throughput (read-heavy) | ✅ High | ❌ Lower |
| Throughput (write-heavy) | Depends on abort rate | Predictable |
| Memory overhead | Higher (multiple versions) | Lower |
| Deadlock risk | None from reads | Must manage |

---

## Thread-Safety Patterns

### Copy-on-Write (CoW)

```cpp
// ✅ Good: Readers get stable snapshot, writers create new copies
// Uses C++20 std::atomic<shared_ptr> (atomic_load/store are deprecated in C++20)
class CoWConfig {
    std::atomic<std::shared_ptr<const ConfigData>> data_;
    mutable std::mutex write_mutex_;

public:
    // Readers: O(1), wait-free after atomic load
    std::shared_ptr<const ConfigData> get() const {
        return data_.load(std::memory_order_acquire);
    }

    // Writer: Creates new copy, atomically swaps
    void update(std::function<void(ConfigData&)> mutator) {
        std::unique_lock lock(write_mutex_);
        auto current = data_.load(std::memory_order_relaxed);
        auto new_data = std::make_shared<ConfigData>(*current);  // Copy
        mutator(*new_data);
        data_.store(std::move(new_data), std::memory_order_release);  // Atomic swap
    }
};
```

### Double-Checked Locking (Safe with C++11 atomics)

```cpp
// ✅ Good: Safe DCL pattern (requires std::atomic or std::call_once)
class Singleton {
    static std::atomic<Singleton*> instance_;
    static std::mutex init_mutex_;

public:
    static Singleton* getInstance() {
        Singleton* p = instance_.load(std::memory_order_acquire);
        if (!p) {
            std::unique_lock lock(init_mutex_);
            p = instance_.load(std::memory_order_relaxed);
            if (!p) {
                p = new Singleton();
                instance_.store(p, std::memory_order_release);
            }
        }
        return p;
    }
};

// ✅ Better: Prefer std::call_once or Meyers singleton instead
Singleton& getInstance() {
    static Singleton instance;  // Thread-safe initialization since C++11
    return instance;
}
```

### Lock-Free Queue (Michael-Scott, MPMC)

```cpp
// ✅ Good: Michael-Scott lock-free queue (MPMC — multiple producer, multiple consumer)
// Suitable for producer-consumer pipelines with multiple threads on each end.
template <typename T>
class LockFreeQueue {
    struct Node {
        std::atomic<Node*> next{nullptr};
        T data;
    };

    alignas(64) std::atomic<Node*> head_;  // Separate cache lines
    alignas(64) std::atomic<Node*> tail_;

public:
    LockFreeQueue() {
        Node* dummy = new Node{};
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        // Drain remaining items and free all nodes including the dummy
        while (dequeue()) {}
        delete head_.load();
    }

    void enqueue(T value) {
        Node* node = new Node{};
        node->data = std::move(value);
        Node* prev_tail = tail_.exchange(node, std::memory_order_acq_rel);
        prev_tail->next.store(node, std::memory_order_release);
    }

    std::optional<T> dequeue() {
        Node* head = head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        if (!next) return std::nullopt;

        T value = std::move(next->data);
        head_.store(next, std::memory_order_release);
        delete head;
        return value;
    }
};
```

### Atomic Operations for Counters

```cpp
// ✅ Good: Use std::atomic for metrics/counters
class TransactionMetrics {
    std::atomic<uint64_t> committed_{0};
    std::atomic<uint64_t> aborted_{0};
    std::atomic<uint64_t> active_{0};

public:
    void onBegin()   { active_.fetch_add(1, std::memory_order_relaxed); }
    void onCommit()  {
        committed_.fetch_add(1, std::memory_order_relaxed);
        active_.fetch_sub(1, std::memory_order_relaxed);
    }
    void onAbort()   {
        aborted_.fetch_add(1, std::memory_order_relaxed);
        active_.fetch_sub(1, std::memory_order_relaxed);
    }

    uint64_t committed() const { return committed_.load(std::memory_order_relaxed); }
};
```

---

## Deadlock Prevention

### Lock Ordering Discipline

```cpp
// ✅ Rule: Always acquire multiple locks in a fixed global order
// Define a canonical order (e.g., by address or by enum value)

// ❌ Bad: Locks acquired in different order across threads → deadlock
// Thread A: lock(mutex_a) → lock(mutex_b)
// Thread B: lock(mutex_b) → lock(mutex_a)  ← Deadlock!

// ✅ Good: Always lock in the same order
enum class LockOrder : int {
    IndexLock  = 0,
    StorageLock = 1,
    MetaLock   = 2,
};

void operationRequiringMultipleLocks() {
    // Lock in enum order: Index → Storage → Meta
    std::unique_lock index_lock(index_mutex_);
    std::unique_lock storage_lock(storage_mutex_);
    std::unique_lock meta_lock(meta_mutex_);
    // ...
}
```

### std::lock for Unordered Acquisition

```cpp
// ✅ Good: Use std::lock when order cannot be predetermined
void mergeRecords(Record& a, Record& b) {
    // std::lock acquires both without deadlock (using try-lock internally)
    std::unique_lock lock_a(a.mutex_, std::defer_lock);
    std::unique_lock lock_b(b.mutex_, std::defer_lock);
    std::lock(lock_a, lock_b);  // Deadlock-free acquisition
    
    a.mergeFrom(b);
}
```

### Timeout Strategies

```cpp
// ✅ Good: Use try_lock_for to avoid indefinite blocking
bool tryAcquireWithTimeout(std::timed_mutex& mtx,
                            std::chrono::milliseconds timeout) {
    if (mtx.try_lock_for(timeout)) {
        return true;  // Lock acquired
    }
    // Log potential contention
    LOG_WARN("Lock acquisition timed out after {}ms", timeout.count());
    return false;
}
```

---

## Testing MVCC Code

### Thread Sanitizer Integration

```bash
# Build with ThreadSanitizer enabled
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
      -B build_tsan
cmake --build build_tsan

# Run tests; TSan will report any detected data races
./build_tsan/tests/storage_tests
```

### CMake TSan Preset

```cmake
# In CMakePresets.json or CMakeLists.txt:
option(THEMIS_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(THEMIS_ENABLE_TSAN)
    target_compile_options(themis_core PUBLIC -fsanitize=thread -g)
    target_link_options(themis_core PUBLIC -fsanitize=thread)
endif()
```

### Stress Testing with Multiple Threads

```cpp
// ✅ Good: Concurrent stress test pattern for MVCC validation
TEST(MVCCStress, ConcurrentReadWrite) {
    constexpr int NUM_THREADS = 8;
    constexpr int OPS_PER_THREAD = 10000;

    MVCCStore store;
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                if (i % 3 == 0) {
                    // Writer
                    auto txn = store.beginTransaction();
                    txn.write("key_" + std::to_string(t), "value_" + std::to_string(i));
                    if (!txn.commit()) {
                        // Commit conflict is expected; count unexpected errors
                    }
                } else {
                    // Reader
                    auto txn = store.beginTransaction();
                    auto val = txn.read("key_" + std::to_string(t % NUM_THREADS));
                    (void)val;  // Read-only; always succeeds
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}
```

### Detecting Data Races with Assertions

```cpp
// ✅ Good: Debug-only access guards (detect misuse in tests)
class GuardedState {
#ifndef NDEBUG
    mutable std::atomic<std::thread::id> owner_{std::thread::id{}};
#endif

public:
    void exclusiveAccess() {
#ifndef NDEBUG
        auto expected = std::thread::id{};
        auto current = std::this_thread::get_id();
        assert(owner_.compare_exchange_strong(expected, current) &&
               "Concurrent exclusive access detected!");
#endif
        // ... actual work
#ifndef NDEBUG
        owner_.store(std::thread::id{});
#endif
    }
};
```

---

## Common Pitfalls

### 1. Use-After-Free

```cpp
// ❌ Bad: Pointer to version that may be garbage collected
VersionedRecord* ptr = store.getLatestVersion(key);
// ... time passes, GC runs ...
auto data = ptr->data;  // ❌ Dangling pointer!

// ✅ Good: Hold shared_ptr to keep version alive
auto version = store.getLatestVersionShared(key);  // Returns shared_ptr
auto data = version->data;  // ✅ Safe: refcount prevents GC
```

### 2. Data Races

```cpp
// ❌ Bad: Non-atomic read of value modified by other threads
bool running_ = true;  // Plain bool
// Thread A:
while (running_) { /* work */ }  // ❌ Possible torn read
// Thread B:
running_ = false;  // ❌ Data race!

// ✅ Good: Atomic flag
std::atomic<bool> running_{true};
while (running_.load(std::memory_order_acquire)) { /* work */ }
running_.store(false, std::memory_order_release);
```

### 3. Deadlocks

```cpp
// ❌ Bad: Recursive lock on non-recursive mutex
void outer() {
    std::lock_guard lock(mutex_);
    inner();  // ❌ Deadlock: tries to lock mutex_ again
}
void inner() {
    std::lock_guard lock(mutex_);  // ❌ Already held by outer()!
}

// ✅ Good: Use recursive_mutex or refactor to avoid nested locking
std::recursive_mutex mutex_;  // If re-entrancy is genuinely needed
```

### 4. Livelock

```cpp
// ❌ Bad: Two transactions each backing off and retrying simultaneously
// → Neither makes progress

// ✅ Good: Add randomized backoff to break livelock
void retryWithBackoff(std::function<bool()> op) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 50);

    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (op()) return;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(dist(rng)));  // Randomized backoff
    }
    throw TransactionException("Max retries exceeded");
}
```

### 5. Priority Inversion

```cpp
// Problem: Low-priority thread holds lock needed by high-priority thread
// Solution: Use priority-aware mutexes or lock-free structures
// On Linux: pthread_mutexattr_setprotocol with PTHREAD_PRIO_INHERIT
```

---

## Performance Under Concurrency

### Lock Contention Measurement

```bash
# Measure lock contention with perf
perf stat -e lock:contention_begin,lock:contention_end \
    ./build/tests/storage_tests

# Or use lockdep analysis (Linux kernel module—for development only)
```

### Read-Heavy vs. Write-Heavy Workloads

```cpp
// ✅ Good: Separate read and write paths with different synchronization
class HybridStore {
    // Read-heavy path: uses shared_mutex + MVCC
    std::shared_mutex read_mutex_;
    MVCCVersionStore versions_;

    // Write path: uses a dedicated write lock + queue
    std::mutex write_mutex_;
    std::deque<WriteOp> write_queue_;

public:
    Record read(const Key& key) const {
        std::shared_lock lock(read_mutex_);  // Concurrent reads
        return versions_.readLatest(key);
    }

    void write(const Key& key, Record record) {
        std::unique_lock lock(write_mutex_);  // Serialized writes
        write_queue_.push_back({key, std::move(record)});
        flushWrites();
    }
};
```

### NUMA-Aware Thread Affinity

```cpp
// ✅ Good: Pin threads to NUMA nodes to reduce cross-socket memory latency
#ifdef __linux__
#include <numa.h>

void pinThreadToNUMANode(int node) {
    if (numa_available() < 0) return;
    struct bitmask* mask = numa_allocate_cpumask();
    numa_node_to_cpus(node, mask);
    // Use numa_sched_setaffinity — accepts struct bitmask* directly,
    // avoiding the undefined-behaviour reinterpret_cast to cpu_set_t*.
    numa_sched_setaffinity(0, mask);
    numa_free_cpumask(mask);
}
#endif
```

---

## Debugging MVCC Issues

### GDB Thread Debugging

```bash
# Attach GDB to running process
gdb -p <pid>

# GDB commands for thread analysis
(gdb) info threads            # List all threads
(gdb) thread 3                # Switch to thread 3
(gdb) bt                      # Show backtrace
(gdb) thread apply all bt     # Show backtrace for all threads
(gdb) watch -l var            # Hardware watchpoint on variable
```

### Logging Best Practices

```cpp
// ✅ Good: Include thread ID and transaction version in all log messages
LOG_DEBUG("[txn={}][thread={}] Read key={} at version={}",
    txn_id_,
    std::this_thread::get_id(),
    key,
    snapshot_version_);

// ✅ Good: Use structured logging for machine-readable traces
spdlog::info({
    {"event", "txn_commit"},
    {"txn_id", txn_id_},
    {"version", committed_version},
    {"write_set_size", write_set_.size()},
});
```

### Reproducing Race Conditions

```bash
# ThreadSanitizer: most reliable race detector
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" -B build_tsan
cmake --build build_tsan && ./build_tsan/tests/storage_tests

# Helgrind (Valgrind): detailed lock analysis, higher overhead
valgrind --tool=helgrind \
    --history-level=full \
    ./build/tests/storage_tests 2> helgrind.log
cat helgrind.log | grep -A 20 "Possible data race"
```

---

## Code Examples

### Safe Counter Implementation

```cpp
class SafeCounter {
    std::atomic<int64_t> value_{0};
public:
    void increment() { value_.fetch_add(1, std::memory_order_relaxed); }
    void decrement() { value_.fetch_sub(1, std::memory_order_relaxed); }
    int64_t load() const { return value_.load(std::memory_order_acquire); }
};
```

### MVCC-Compliant Transaction Example

```cpp
// Full transaction lifecycle with conflict detection
bool runTransaction(MVCCStore& store, const Key& key, const Value& new_val) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        auto txn = store.beginTransaction();

        // Read current value (uses snapshot)
        auto current = txn.read(key);

        // Business logic on the snapshot data
        Value updated = transform(current.value_or(Value{}), new_val);

        // Write to transaction's private write buffer
        txn.write(key, updated);

        // Commit: validates no conflict since snapshot was taken
        if (txn.commit()) return true;  // Success

        // Conflict: retry with new snapshot
        std::this_thread::yield();
    }
    return false;  // Max retries exceeded
}
```

---

## Additional Resources

- [CODE_STANDARDS.md](CODE_STANDARDS.md) - C++ thread-safety patterns
- [TESTING_GUIDE.md](TESTING_GUIDE.md) - ThreadSanitizer test integration
- [PERFORMANCE_PROFILING.md](PERFORMANCE_PROFILING.md) - Lock contention profiling
- C++ Concurrency in Action (Anthony Williams) - Comprehensive reference
- Linux `man 7 pthreads` - POSIX thread semantics
- ThreadSanitizer docs: https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual
