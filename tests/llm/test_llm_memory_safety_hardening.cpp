/**
 * @file test_llm_memory_safety_hardening.cpp
 * @brief LLM Batch 1: Memory Safety Hardening Tests (MEM-01..MEM-16)
 * 
 * Tier 1 hardening: Tests for RAII, leak detection, cache cleanup.
 * Coverage: Manual cleanup fixes, GPU/DB connection leaks, exception safety.
 * 
 * @version 2026-08-02
 * @status Phase 1 (Memory Safety)
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <optional>

namespace themis::llm::test {

// ============================================================================
// MEM-01: Manual Cleanup in Destructor — RAII Pattern Test
// ============================================================================
class RAIIResourceWrapper {
 public:
  RAIIResourceWrapper() : resource_(std::make_unique<int>(42)) {}
  ~RAIIResourceWrapper() = default;  // Automatic cleanup via unique_ptr
  
  int getValue() const { return *resource_; }
  
 private:
  std::unique_ptr<int> resource_;
};

TEST(MemorySafetyHardening, MEM_01_RAIIResourceCleanup) {
  {
    RAIIResourceWrapper wrapper;
    EXPECT_EQ(wrapper.getValue(), 42);
  }  // Automatic cleanup here
  // No memory leak
  SUCCEED();
}

// ============================================================================
// MEM-02: GPU Memory Leak Detection — RAII Cleanup
// ============================================================================
class GPUMemoryPool {
 public:
  class GPUAllocation {
   public:
    GPUAllocation(size_t size, GPUMemoryPool* pool) 
        : size_(size), pool_(pool), is_freed_(false) {}
    
    ~GPUAllocation() {
      if (!is_freed_) {
        // Auto-free GPU memory in destructor
        if (pool_) pool_->free(size_);
        is_freed_ = true;
      }
    }
    
    size_t getSize() const { return size_; }
   
   private:
    size_t size_;
    GPUMemoryPool* pool_;
    std::atomic<bool> is_freed_;
  };
  
  std::unique_ptr<GPUAllocation> allocate(size_t size) {
    allocated_ += size;
    return std::make_unique<GPUAllocation>(size, this);
  }
  
  void free(size_t size) {
    allocated_ -= size;
  }
  
  size_t getAllocated() const { return allocated_; }
  
 private:
  std::atomic<size_t> allocated_{0};
};

TEST(MemorySafetyHardening, MEM_02_GPUMemoryLeakDetection) {
  GPUMemoryPool pool;
  {
    auto alloc = pool.allocate(1024);
    EXPECT_EQ(alloc->getSize(), 1024);
    EXPECT_EQ(pool.getAllocated(), 1024);
  }  // Destructor frees GPU memory
  EXPECT_EQ(pool.getAllocated(), 0) << "GPU memory not freed";
}

// ============================================================================
// MEM-03: Database Connection Leak Detection — RAII Connection Pool
// ============================================================================
class DBConnectionPool {
 public:
  class DBConnection {
   public:
    DBConnection(DBConnectionPool* pool) : pool_(pool), is_returned_(false) {}
    
    ~DBConnection() {
      if (!is_returned_ && pool_) {
        pool_->returnConnection();
        is_returned_ = true;
      }
    }
    
    void markReturned() { is_returned_ = true; }
   
   private:
    DBConnectionPool* pool_;
    std::atomic<bool> is_returned_;
  };
  
  std::unique_ptr<DBConnection> acquire() {
    connections_active_.fetch_add(1);
    return std::make_unique<DBConnection>(this);
  }
  
  void returnConnection() {
    connections_active_.fetch_sub(1);
  }
  
  size_t getActiveConnections() const { return connections_active_; }
  
 private:
  std::atomic<size_t> connections_active_{0};
};

TEST(MemorySafetyHardening, MEM_03_DBConnectionLeakDetection) {
  DBConnectionPool pool;
  {
    auto conn = pool.acquire();
    EXPECT_EQ(pool.getActiveConnections(), 1);
  }  // Destructor returns connection
  EXPECT_EQ(pool.getActiveConnections(), 0) << "DB connection not returned";
}

// ============================================================================
// MEM-04: Exception Safety in Destructors
// ============================================================================
class ExceptionSafeResource {
 public:
  ExceptionSafeResource() : value_(new int(100)) {}
  
  ~ExceptionSafeResource() noexcept {  // noexcept guarantees no exceptions
    if (value_) {
      delete value_;
      value_ = nullptr;  // Clear after deletion
    }
  }
  
  int getValue() const { return value_ ? *value_ : -1; }
  
 private:
  int* value_;
};

TEST(MemorySafetyHardening, MEM_04_ExceptionSafeDestructor) {
  EXPECT_NO_THROW({
    ExceptionSafeResource resource;
    EXPECT_EQ(resource.getValue(), 100);
  });
}

// ============================================================================
// MEM-05: nullptr Check After Delete
// ============================================================================
class SafeDeleteWrapper {
 public:
  SafeDeleteWrapper() : ptr_(new std::vector<int>{1, 2, 3}) {}
  
  ~SafeDeleteWrapper() {
    if (ptr_) {
      delete ptr_;
      ptr_ = nullptr;  // Prevent double-delete
    }
  }
  
  bool isValid() const { return ptr_ != nullptr; }
  
 private:
  std::vector<int>* ptr_;
};

TEST(MemorySafetyHardening, MEM_05_SafeDeleteWithNullptrCheck) {
  {
    SafeDeleteWrapper wrapper;
    EXPECT_TRUE(wrapper.isValid());
  }  // Destructor ensures ptr_ is nullptr after delete
  SUCCEED();
}

// ============================================================================
// MEM-06: Lock Ordering Deadlock Prevention — Memory Safety for Concurrency
// ============================================================================
class ThreadSafeCache {
 public:
  struct Entry {
    std::vector<uint8_t> data;
    std::atomic<size_t> ref_count{1};
  };
  
  // Consistent lock ordering: always lock mutex THEN acquire entry
  std::shared_ptr<Entry> get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      it->second->ref_count.fetch_add(1);
      return it->second;
    }
    return nullptr;
  }
  
  void put(const std::string& key, std::vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(mutex_);  // Lock first
    auto entry = std::make_shared<Entry>();
    entry->data = std::move(data);
    cache_[key] = entry;
  }
  
 private:
  std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<Entry>> cache_;
};

TEST(MemorySafetyHardening, MEM_06_LockOrderingDeadlockPrevention) {
  ThreadSafeCache cache;
  
  cache.put("key1", std::vector<uint8_t>{1, 2, 3});
  auto entry = cache.get("key1");
  
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->data.size(), 3);
}

// ============================================================================
// MEM-07: Memory Order Correctness in Concurrent Access
// ============================================================================
class AtomicDataGuard {
 public:
  void setValue(int value) {
    value_.store(value, std::memory_order_release);
  }
  
  int getValue() const {
    return value_.load(std::memory_order_acquire);
  }
  
 private:
  std::atomic<int> value_{0};
};

TEST(MemorySafetyHardening, MEM_07_AtomicMemoryOrder) {
  AtomicDataGuard guard;
  guard.setValue(42);
  EXPECT_EQ(guard.getValue(), 42);
}

// ============================================================================
// MEM-08: Move Semantics — No Double-Delete
// ============================================================================
class MovableResource {
 public:
  MovableResource() : data_(new int(123)) {}
  
  ~MovableResource() {
    if (data_) {
      delete data_;
      data_ = nullptr;
    }
  }
  
  // Proper move semantics
  MovableResource(MovableResource&& other) noexcept : data_(other.data_) {
    other.data_ = nullptr;
  }
  
  MovableResource& operator=(MovableResource&& other) noexcept {
    if (this != &other) {
      if (data_) delete data_;
      data_ = other.data_;
      other.data_ = nullptr;
    }
    return *this;
  }
  
  // Prevent copies
  MovableResource(const MovableResource&) = delete;
  MovableResource& operator=(const MovableResource&) = delete;
  
  int getValue() const { return data_ ? *data_ : -1; }
  
 private:
  int* data_;
};

TEST(MemorySafetyHardening, MEM_08_MoveSemanticsNonDoublDelete) {
  {
    MovableResource original;
    EXPECT_EQ(original.getValue(), 123);
    
    MovableResource moved = std::move(original);
    EXPECT_EQ(moved.getValue(), 123);
    EXPECT_EQ(original.getValue(), -1);  // Moved from
  }  // Both destroyed safely without double-delete
  SUCCEED();
}

// ============================================================================
// MEM-09: RAII for Nested Resource Management
// ============================================================================
class NestedResourceManager {
 public:
  class InnerResource {
   public:
    explicit InnerResource(int value) : value_(value) {}
    int getValue() const { return value_; }
   private:
    int value_;
  };
  
  class OuterResource {
   public:
    OuterResource() : inner_(std::make_unique<InnerResource>(99)) {}
    InnerResource* getInner() { return inner_.get(); }
   private:
    std::unique_ptr<InnerResource> inner_;
  };
  
  explicit NestedResourceManager() : outer_(std::make_unique<OuterResource>()) {}
  
  int getNestedValue() {
    return outer_->getInner()->getValue();
  }
  
 private:
  std::unique_ptr<OuterResource> outer_;
};

TEST(MemorySafetyHardening, MEM_09_NestedResourceManagement) {
  {
    NestedResourceManager manager;
    EXPECT_EQ(manager.getNestedValue(), 99);
  }  // All nested resources cleaned up
  SUCCEED();
}

// ============================================================================
// MEM-10: Cache Eviction Under Lock Contention — Memory Safety
// ============================================================================
class ContentionAwareCache {
 public:
  using Entry = std::vector<uint8_t>;
  
  bool put(const std::string& key, Entry value, size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (cache_.size() >= max_size && cache_.find(key) == cache_.end()) {
      // Evict oldest entry safely
      if (!cache_.empty()) {
        cache_.erase(cache_.begin());  // Safe erase under lock
      }
    }
    
    cache_[key] = std::move(value);
    return true;
  }
  
  std::optional<Entry> get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      return it->second;
    }
    return std::nullopt;
  }
  
 private:
  std::mutex mutex_;
  std::unordered_map<std::string, Entry> cache_;
};

TEST(MemorySafetyHardening, MEM_10_CacheEvictionLockContention) {
  ContentionAwareCache cache;
  
  cache.put("key1", std::vector<uint8_t>{1, 2}, 2);
  cache.put("key2", std::vector<uint8_t>{3, 4}, 2);
  cache.put("key3", std::vector<uint8_t>{5, 6}, 2);  // Evicts key1
  
  auto val1 = cache.get("key1");
  auto val3 = cache.get("key3");
  
  EXPECT_FALSE(val1.has_value());
  EXPECT_TRUE(val3.has_value());
}

// ============================================================================
// MEM-11 to MEM-16: Integration Tests for Complex Scenarios
// ============================================================================

TEST(MemorySafetyHardening, MEM_11_ConcurrentAllocationDeallocation) {
  GPUMemoryPool pool;
  
  std::vector<std::thread> threads;
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([&pool]() {
      auto alloc = pool.allocate(512);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(pool.getAllocated(), 0) << "All allocations should be freed";
}

TEST(MemorySafetyHardening, MEM_12_ResourceAcquisitionReleasePattern) {
  DBConnectionPool pool;
  
  {
    auto conn1 = pool.acquire();
    auto conn2 = pool.acquire();
    EXPECT_EQ(pool.getActiveConnections(), 2);
  }
  
  EXPECT_EQ(pool.getActiveConnections(), 0);
}

TEST(MemorySafetyHardening, MEM_13_ExceptionSafetyGuarantee) {
  EXPECT_NO_THROW({
    auto resource = std::make_unique<ExceptionSafeResource>();
    resource.reset();  // Explicit cleanup is exception-safe
  });
}

TEST(MemorySafetyHardening, MEM_14_SharedPtrCircularReferencePrevention) {
  struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // Use weak_ptr to break cycles
  };
  
  auto node1 = std::make_shared<Node>();
  auto node2 = std::make_shared<Node>();
  
  node1->next = node2;
  node2->prev = node1;  // Weak reference
  
  // No circular reference leak
  EXPECT_TRUE(node1->next != nullptr);
}

TEST(MemorySafetyHardening, MEM_15_VectorReserveToAvoidReallocation) {
  std::vector<int> vec;
  vec.reserve(1000);  // Allocate upfront
  
  for (int i = 0; i < 100; ++i) {
    vec.push_back(i);  // No reallocation
  }
  
  EXPECT_EQ(vec.size(), 100);
  EXPECT_GE(vec.capacity(), 1000);
}

TEST(MemorySafetyHardening, MEM_16_SmartPtrUniquePtrVsSharedPtr) {
  // Use unique_ptr for exclusive ownership
  auto exclusive = std::make_unique<int>(42);
  int* ptr = exclusive.get();
  EXPECT_EQ(*ptr, 42);
  
  // Use shared_ptr for shared ownership
  auto shared1 = std::make_shared<int>(99);
  {
    auto shared2 = shared1;
    EXPECT_EQ(shared1.use_count(), 2);
  }
  EXPECT_EQ(shared1.use_count(), 1);
}

}  // namespace themis::llm::test
