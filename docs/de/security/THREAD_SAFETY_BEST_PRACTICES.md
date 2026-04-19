# Thread-Safety Best Practices for ThemisDB

**Date:** 2026-01-05  
**Purpose:** Guidelines for thread-safe development in ThemisDB

---

## Overview

This document provides best practices for thread-safe code development in ThemisDB. Following these guidelines helps prevent race conditions and ensures robust concurrent operation.

---

## Core Principles

### 1. RAII for Resource Management

**Always use RAII** (Resource Acquisition Is Initialization) for managing resources:

```cpp
// ✅ Good: RAII with std::lock_guard
void safeOperation() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Critical section - lock automatically released
}

// ❌ Bad: Manual locking
void unsafeOperation() {
    mutex_.lock();
    // If exception thrown, mutex never unlocked!
    mutex_.unlock();
}
```

**Use smart pointers** for automatic memory management:

```cpp
// ✅ Good: unique_ptr for automatic cleanup
std::unique_ptr<Iterator> it(db->NewIterator());
// No need to delete - automatic cleanup

// ❌ Bad: Raw pointer
Iterator* it = db->NewIterator();
delete it; // Easy to forget or miss in exception paths
```

### 2. Atomic Operations for Lock-Free State

Use `std::atomic` for simple state variables that need thread-safe access:

```cpp
// ✅ Good: Atomic for simple state
std::atomic<bool> finished_{false};

bool tryFinish() {
    bool expected = false;
    return finished_.compare_exchange_strong(expected, true);
}

// ❌ Bad: Bool with mutex (overkill for simple state)
std::mutex mutex_;
bool finished_ = false;

bool tryFinish() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (finished_) return false;
    finished_ = true;
    return true;
}
```

### 3. Minimize Lock Scope

Keep critical sections as small as possible:

```cpp
// ✅ Good: Copy data under lock, process outside
std::vector<Item> getItems() {
    std::vector<Item> result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result = items_; // Quick copy under lock
    }
    // Expensive processing outside lock
    std::sort(result.begin(), result.end());
    return result;
}

// ❌ Bad: Hold lock during expensive operation
std::vector<Item> getItems() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Item> result = items_;
    std::sort(result.begin(), result.end()); // Blocks other threads!
    return result;
}
```

### 4. Document Thread-Safety Guarantees

Always document thread-safety in header files:

```cpp
/**
 * @brief Process data concurrently
 * 
 * Thread-safety: This method is thread-safe and can be called
 * concurrently from multiple threads. Internal synchronization
 * is handled automatically.
 */
void process();

/**
 * @brief Configure settings
 * 
 * Thread-safety: NOT thread-safe. Must be called before starting
 * worker threads or protected by external synchronization.
 */
void configure();
```

---

## Component-Specific Guidelines

### RocksDB Operations

**Lifecycle Management:**

```cpp
// ✅ Good: Reference counting for long-lived operations
class OperationGuard {
    const Wrapper* wrapper_;
public:
    OperationGuard(const Wrapper* w) : wrapper_(w) {
        wrapper_->incrementActiveOps();
    }
    ~OperationGuard() {
        wrapper_->decrementActiveOps();
    }
};

void scan() {
    OperationGuard guard(this);
    // Safe to use database - guard prevents close()
}
```

**Column Family Operations:**

```cpp
// ✅ Good: Mutex protection for check-create-insert
ColumnFamilyHandle* getOrCreate(const std::string& name) {
    std::lock_guard<std::mutex> lock(cf_mutex_);
    
    // Check
    for (auto* handle : handles_) {
        if (handle->GetName() == name) return handle;
    }
    
    // Create and insert - all under same lock
    auto* handle = db_->CreateColumnFamily(opts, name);
    handles_.push_back(handle);
    return handle;
}
```

### Transaction Management

**Atomic State Transitions:**

```cpp
// ✅ Good: Atomic compare-exchange for state
bool commit() {
    bool expected = false;
    if (!finished_.compare_exchange_strong(expected, true)) {
        return false; // Already finished
    }
    // Proceed with commit
}
```

**TOCTOU Prevention:**

```cpp
// ✅ Good: Check for existing entry
void moveToCompleted(TransactionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto active_it = active_.find(id);
    if (active_it == active_.end()) return;
    
    // Defensive check for duplicate
    if (completed_.count(id) > 0) {
        LOG_WARN("Transaction {} already in completed map", id);
        active_.erase(active_it);
        return;
    }
    
    completed_[id] = std::move(active_it->second);
    active_.erase(active_it);
}
```

### Cache Operations

**Vector Index Consistency:**

```cpp
// ✅ Good: Clean up both map and index
void evict(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it == cache_.end()) return;
    
    // Remove from vector index first
    if (vector_index_) {
        vector_index_->remove(key);
    }
    
    // Then remove from cache
    cache_.erase(it);
}
```

**Statistics Consistency:**

```cpp
/**
 * @brief Get cache statistics
 * 
 * Note: Statistics are eventually consistent. Counter values
 * may be slightly out of sync due to concurrent updates.
 * This is acceptable for monitoring purposes.
 */
CacheStats getStats() const;
```

### Connection Pools

**RAII for Borrowed Connections:**

```cpp
// ✅ Good: RAII wrapper for borrowed connections
class ConnectionGuard {
    ConnectionPool* pool_;
    Connection* conn_;
    
public:
    ConnectionGuard(ConnectionPool* pool) 
        : pool_(pool), conn_(pool->borrow()) {}
    
    ~ConnectionGuard() {
        if (conn_) pool_->return(conn_);
    }
    
    Connection* get() { return conn_; }
    
    // Prevent copying
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
};

// Usage
void useConnection() {
    ConnectionGuard conn_guard(&pool);
    conn_guard.get()->execute("SELECT ...");
    // Connection automatically returned to pool
}
```

**Pool Size Management:**

```cpp
// ✅ Good: Atomic counters for pool size
class ConnectionPool {
    std::atomic<size_t> active_connections_{0};
    std::atomic<size_t> available_connections_{0};
    
public:
    Connection* borrow() {
        available_connections_.fetch_sub(1);
        active_connections_.fetch_add(1);
        // ... get connection ...
    }
    
    void return(Connection* conn) {
        active_connections_.fetch_sub(1);
        available_connections_.fetch_add(1);
        // ... return connection ...
    }
};
```

---

## Anti-Patterns to Avoid

### 1. Check-Then-Act Without Lock

```cpp
// ❌ Bad: Race between check and act
if (map_.count(key) == 0) {
    // Another thread could insert here!
    map_[key] = value;
}

// ✅ Good: Atomic insert
map_.insert({key, value}); // Returns {iterator, bool}
```

### 2. Double-Checked Locking (Usually Wrong)

```cpp
// ❌ Bad: Classic double-checked locking antipattern
if (!initialized_) { // Check without lock
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) { // Check again with lock
        initialize();
        initialized_ = true;
    }
}

// ✅ Good: Use std::call_once or atomic
std::once_flag init_flag_;
std::call_once(init_flag_, []() {
    initialize();
});
```

### 3. Holding Locks Across External Calls

```cpp
// ❌ Bad: Lock held during external call
void process() {
    std::lock_guard<std::mutex> lock(mutex_);
    externalLibrary.call(); // Could deadlock or be slow!
}

// ✅ Good: Release lock before external call
void process() {
    Data data;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data = internal_data_;
    }
    externalLibrary.call(data);
}
```

### 4. Raw Pointers for Shared Resources

```cpp
// ❌ Bad: Raw pointer with manual management
Iterator* iter = db->NewIterator();
// Use iterator... could forget to delete
delete iter;

// ✅ Good: Smart pointer
std::unique_ptr<Iterator> iter(db->NewIterator());
// Automatic cleanup, exception-safe
```

---

## Testing for Thread Safety

### 1. Enable Thread Sanitizer (TSan)

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make
./tests
```

TSan will detect:
- Data races
- Lock order inversions
- Use-after-free

### 2. Stress Testing

Create tests that:
- Run operations from multiple threads simultaneously
- Use high thread counts (16+)
- Run for extended periods
- Test edge cases (close during operations, etc.)

Example:
```cpp
void stressTest() {
    const int NUM_THREADS = 16;
    const int OPS_PER_THREAD = 10000;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                // Mix of operations
                db.put(key, value);
                db.get(key);
                db.scan(prefix, callback);
            }
        });
    }
    
    for (auto& t : threads) t.join();
}
```

### 3. Code Review Checklist

When reviewing code for thread-safety:

- [ ] Are shared mutable variables protected?
- [ ] Are RAII patterns used for locks and resources?
- [ ] Is lock scope minimized?
- [ ] Are atomic operations used appropriately?
- [ ] Is thread-safety documented?
- [ ] Are there any check-then-act races?
- [ ] Do iterators outlive their data sources?
- [ ] Are resources cleaned up in all code paths?

---

## Common Pitfalls

### 1. Iterator Invalidation

```cpp
// ❌ Bad: Modifying container during iteration
for (auto& item : items_) {
    if (should_remove(item)) {
        items_.erase(item); // Iterator invalidated!
    }
}

// ✅ Good: Use erase-remove idiom or collect keys
for (auto it = items_.begin(); it != items_.end(); ) {
    if (should_remove(*it)) {
        it = items_.erase(it); // Returns next valid iterator
    } else {
        ++it;
    }
}
```

### 2. Dangling References

```cpp
// ❌ Bad: Returning reference to temporary
const Data& getData() {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_; // Lock released, data_ could be modified!
}

// ✅ Good: Return by value or shared_ptr
Data getData() {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_; // Copy is safe
}
```

### 3. Deadlocks

```cpp
// ❌ Bad: Inconsistent lock ordering
void transfer(Account& from, Account& to) {
    std::lock_guard<std::mutex> lock1(from.mutex);
    std::lock_guard<std::mutex> lock2(to.mutex); // Deadlock risk!
}

// ✅ Good: Consistent lock ordering or std::scoped_lock
void transfer(Account& from, Account& to) {
    std::scoped_lock lock(from.mutex, to.mutex); // C++17
    // Or manually order by address
}
```

---

## Summary

**Key Takeaways:**

1. ✅ Use RAII everywhere (locks, resources, connections)
2. ✅ Use atomic operations for simple state
3. ✅ Minimize critical sections
4. ✅ Document thread-safety guarantees
5. ✅ Test with Thread Sanitizer
6. ✅ Use smart pointers, not raw pointers
7. ✅ Avoid check-then-act patterns
8. ✅ Prevent iterator invalidation
9. ✅ Be careful with lock ordering
10. ✅ Review code with thread-safety checklist

**Remember:** Thread-safety is not optional in a concurrent system. Build it in from the start, document it clearly, and test it thoroughly.

---

## References

- [RACE_CONDITION_ANALYSIS.md](./RACE_CONDITION_ANALYSIS.md) - Detailed analysis of race conditions
- [RACE_CONDITION_TESTING_GUIDE.md](../../reviews/RACE_CONDITION_TESTING_GUIDE.md) - Testing procedures
- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition) - Recommended reading
- [Thread Sanitizer Documentation](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

---

**Status:** Best Practices Documented ✅  
**Applies To:** All ThemisDB Components  
**Review:** Include in code review process
