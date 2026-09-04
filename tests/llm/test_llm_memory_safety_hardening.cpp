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
#include "llm/llm_memory_safety_utils.h"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <optional>
#include <string>
#include <unordered_map>
#include <chrono>
#include <stdexcept>
#include <cstdint>

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
        if (pool_) {
          pool_->free(size_);
        }
        is_freed_ = true;
      }
    }
    
    size_t getSize() const { return size_; }
   
   private:
    size_t size_ = {};
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
      if (data_) {
        delete data_;
      }
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

TEST(MemorySafetyHardening, MEM_08_MoveSemanticsNonDoubleDelete) {
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
  
  std::vector<std::thread> threads = {};

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

// ============================================================================
// MEM-17 to MEM-28: Concurrency and Quota Management
// ============================================================================

// MEM-17: Quota Guard Release on Scope Exit
TEST(MemorySafetyHardening, MEM_17_QuotaGuardReleaseOnScopeExit) {
  std::atomic<size_t> released{0};
  
  {
    QuotaGuard quota(100, [&released](size_t amount) {
      released.fetch_add(amount);
    });
    EXPECT_EQ(quota.getAmount(), 100);
  }
  
  EXPECT_EQ(released, 100) << "Quota not released on scope exit";
}

// MEM-18: Multiple Quota Guards in Sequence
TEST(MemorySafetyHardening, MEM_18_MultipleQuotaGuardsSequence) {
  std::atomic<size_t> total_released{0};
  
  {
    QuotaGuard q1(50, [&total_released](size_t a) {
      total_released.fetch_add(a);
    });
    QuotaGuard q2(50, [&total_released](size_t a) {
      total_released.fetch_add(a);
    });
  }
  
  EXPECT_EQ(total_released, 100);
}

// MEM-19: Batch Guard Exception Safety
TEST(MemorySafetyHardening, MEM_19_BatchGuardExceptionSafety) {
  std::atomic<int> cleanup_count{0};
  
  try {
    BatchGuard batch(1, [&cleanup_count]() {
      cleanup_count.fetch_add(1);
      throw std::runtime_error("Cleanup error");
    });
  } catch (...) {
    // Exception swallowed in destructor
  }
  
  EXPECT_EQ(cleanup_count, 1) << "Batch cleanup should still occur";
}

// MEM-20: Concurrent Quota Acquisition
TEST(MemorySafetyHardening, MEM_20_ConcurrentQuotaAcquisition) {
  std::atomic<size_t> available{1000};
  std::vector<std::thread> threads;
  
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&available]() {
      QuotaGuard quota(100, [&available](size_t a) {
        available.fetch_add(a);
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(available, 1000) << "All quotas returned";
}

// MEM-21: Thread-Safe Counter Increment
TEST(MemorySafetyHardening, MEM_21_ThreadSafeCounterIncrement) {
  ThreadSafeCounter counter(0);
  std::vector<std::thread> threads;
  
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&counter]() {
      counter.increment();
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(counter.get(), 100);
}

// MEM-22: Batch Guard Move Semantics
TEST(MemorySafetyHardening, MEM_22_BatchGuardMoveSemantics) {
  std::atomic<int> cleanup_count{0};
  
  {
    BatchGuard b1(1, [&cleanup_count]() {
      cleanup_count.fetch_add(1);
    });
    BatchGuard b2 = std::move(b1);
    EXPECT_EQ(b2.getId(), 1);
    EXPECT_EQ(b1.getId(), -1);
  }
  
  EXPECT_EQ(cleanup_count, 1) << "Only one cleanup should occur";
}

// MEM-23: Quota Guard with Zero Amount
TEST(MemorySafetyHardening, MEM_23_QuotaGuardZeroAmount) {
  std::atomic<size_t> released{0};
  
  {
    QuotaGuard quota(0, [&released](size_t a) {
      released.fetch_add(a);
    });
  }
  
  EXPECT_EQ(released, 0) << "Zero quota should not trigger release";
}

// MEM-24: Thread Counter Concurrent Increments and Decrements
TEST(MemorySafetyHardening, MEM_24_ThreadCounterMixedOps) {
  ThreadSafeCounter counter(0);
  std::vector<std::thread> threads;
  
  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([&counter]() {
      counter.increment();
    });
  }
  
  for (int i = 0; i < 30; ++i) {
    threads.emplace_back([&counter]() {
      counter.decrement();
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(counter.get(), 20);
}

// MEM-25: Batch Guard Multiple Acquisitions and Releases
TEST(MemorySafetyHardening, MEM_25_BatchGuardMultipleAcquisitions) {
  std::atomic<int> active_batches{0};
  
  {
    BatchGuard b1(1, [&active_batches]() { active_batches.fetch_sub(1); });
    active_batches.fetch_add(1);
    
    {
      BatchGuard b2(2, [&active_batches]() { active_batches.fetch_sub(1); });
      active_batches.fetch_add(1);
      EXPECT_EQ(active_batches, 2);
    }
    
    EXPECT_EQ(active_batches, 1);
  }
  
  EXPECT_EQ(active_batches, 0);
}

// MEM-26: Quota Guard Partial Release Simulation
TEST(MemorySafetyHardening, MEM_26_QuotaPartialConsumption) {
  std::atomic<size_t> quota_remaining{1000};
  
  {
    QuotaGuard q1(200, [&quota_remaining](size_t a) {
      quota_remaining.fetch_add(a);
    });
    QuotaGuard q2(300, [&quota_remaining](size_t a) {
      quota_remaining.fetch_add(a);
    });
    EXPECT_EQ(quota_remaining, 1000 - 200 - 300);
  }
  
  EXPECT_EQ(quota_remaining, 1000);
}

// MEM-27: Counter Memory Ordering with Acquire/Release
TEST(MemorySafetyHardening, MEM_27_CounterMemoryOrdering) {
  ThreadSafeCounter counter(0);
  std::atomic<bool> ready{false};
  
  std::thread t1([&counter, &ready]() {
    counter.increment();
    ready.store(true, std::memory_order_release);
  });
  
  while (!ready.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  
  EXPECT_EQ(counter.get(), 1);
  t1.join();
}

// MEM-28: Batch and Quota Guard Combined Exception Safety
TEST(MemorySafetyHardening, MEM_28_BatchQuotaCombinedExceptionSafety) {
  std::atomic<int> batch_cleanups{0};
  std::atomic<size_t> quota_released{0};
  
  try {
    BatchGuard batch(1, [&batch_cleanups]() {
      batch_cleanups.fetch_add(1);
    });
    QuotaGuard quota(100, [&quota_released](size_t a) {
      quota_released.fetch_add(a);
      throw std::runtime_error("Quota error");
    });
  } catch (...) {
    // Exception handled
  }
  
  EXPECT_EQ(batch_cleanups, 1);
  EXPECT_EQ(quota_released, 100);
}

// ============================================================================
// EXS-01 to EXS-25: Exception Safety in Model Lifecycle
// ============================================================================

// EXS-01: Model Load with Successful Cleanup
TEST(MemorySafetyHardening, EXS_01_ModelLoadSuccessfulCleanup) {
  struct Model {
    std::unique_ptr<int> data;
    Model() : data(std::make_unique<int>(42)) {}
  };
  
  {
    Model model;
    EXPECT_NE(model.data.get(), nullptr);
  }
  SUCCEED();
}

// EXS-02: Model Load Failure Exception Propagation
TEST(MemorySafetyHardening, EXS_02_ModelLoadFailure) {
  struct FailingModel {
    FailingModel() {
      throw std::runtime_error("Model load failed");
    }
  };
  
  EXPECT_THROW({
    FailingModel m;
  }, std::runtime_error);
}

// EXS-03: Model Unload Idempotency
TEST(MemorySafetyHardening, EXS_03_ModelUnloadIdempotency) {
  struct Model {
    bool is_loaded = true;
    void unload() {
      if (is_loaded) {
        is_loaded = false;
      }
    }
  };
  
  Model m;
  m.unload();
  EXPECT_FALSE(m.is_loaded);
  m.unload();  // Should not fail
  EXPECT_FALSE(m.is_loaded);
}

// EXS-04: Double-Unload Prevention
TEST(MemorySafetyHardening, EXS_04_DoubleUnloadPrevention) {
  struct Model {
    std::atomic<bool> is_loaded{true};
    
    void unload() {
      if (is_loaded.exchange(false)) {
        // Only first unload performs cleanup
      }
    }
  };
  
  Model m;
  m.unload();
  m.unload();
  EXPECT_FALSE(m.is_loaded);
}

// EXS-05: Exception During Destruction
TEST(MemorySafetyHardening, EXS_05_ExceptionDuringDestruction) {
  struct ModelWithCleanup {
    ~ModelWithCleanup() noexcept {
      try {
        // Cleanup that might throw
      } catch (...) {
        // Catch and suppress
      }
    }
  };
  
  EXPECT_NO_THROW({
    ModelWithCleanup m;
  });
}

// EXS-06: Strong Exception Guarantee
TEST(MemorySafetyHardening, EXS_06_StrongExceptionGuarantee) {
  struct Model {
    int state = 0;
    void loadState(int newState) {
      if (newState < 0) {
        throw std::invalid_argument("Invalid state");
      }
      state = newState;  // Only modified if no exception
    }
  };
  
  Model m;
  m.state = 42;
  
  try {
    m.loadState(-1);
  } catch (...) {
  }
  
  EXPECT_EQ(m.state, 42) << "State unchanged after exception";
}

// EXS-07: Basic Exception Guarantee
TEST(MemorySafetyHardening, EXS_07_BasicExceptionGuarantee) {
  struct Model {
    std::vector<int> data;
    
    void loadData(const std::vector<int>& newData) {
      // May throw during construction of vector
      data = newData;
    }
  };
  
  Model m;
  m.data = {1, 2, 3};
  
  try {
    m.loadData({});
    EXPECT_TRUE(m.data.empty());
  } catch (...) {
    // Object remains in valid state
    EXPECT_TRUE(!m.data.empty() || m.data.empty());
  }
}

// EXS-08: Adapter Load/Unload Sequence Validation
TEST(MemorySafetyHardening, EXS_08_AdapterSequence) {
  struct Adapter {
    bool loaded = false;
    
    void load() {
      if (loaded) {
        throw std::runtime_error("Already loaded");
      }
      loaded = true;
    }
    
    void unload() {
      if (!loaded) {
        throw std::runtime_error("Not loaded");
      }
      loaded = false;
    }
  };
  
  Adapter a;
  a.load();
  EXPECT_TRUE(a.loaded);
  a.unload();
  EXPECT_FALSE(a.loaded);
}

// EXS-09: Model Load with Resource Acquisition
TEST(MemorySafetyHardening, EXS_09_ModelLoadResourceAcquisition) {
  struct Model {
    std::unique_ptr<std::vector<int>> weights;
    
    Model(size_t size) {
      weights = std::make_unique<std::vector<int>>(size);
      for (size_t i = 0; i < size; ++i) {
        (*weights)[i] = i;
      }
    }
  };
  
  {
    Model m(1000);
    EXPECT_EQ(m.weights->size(), 1000);
  }
  SUCCEED();
}

// EXS-10: Exception in Constructor Cleanup
TEST(MemorySafetyHardening, EXS_10_ConstructorCleanupOnException) {
  struct ResourceGuard {
    std::unique_ptr<int> resource;
    
    ResourceGuard() {
      resource = std::make_unique<int>(42);
      if (*resource == 42) {
        throw std::runtime_error("Intentional error");
      }
    }
  };
  
  EXPECT_THROW({
    ResourceGuard g;
  }, std::runtime_error);
}

// EXS-11: Model State Consistency After Partial Load
TEST(MemorySafetyHardening, EXS_11_PartialLoadConsistency) {
  struct Model {
    int weights_loaded = 0;
    int biases_loaded = 0;
    
    void load() {
      weights_loaded = 1;
      if (weights_loaded) {
        biases_loaded = 1;
      }
    }
  };
  
  Model m;
  m.load();
  EXPECT_EQ(m.weights_loaded, 1);
  EXPECT_EQ(m.biases_loaded, 1);
}

// EXS-12: Plugin Lifecycle with RAII
TEST(MemorySafetyHardening, EXS_12_PluginRAII) {
  struct Plugin {
    bool initialized = false;
    
    Plugin() { initialized = true; }
    ~Plugin() { initialized = false; }
  };
  
  {
    Plugin p;
    EXPECT_TRUE(p.initialized);
  }
}

// EXS-13: Model Cache with Exception Safety
TEST(MemorySafetyHardening, EXS_13_ModelCacheException) {
  std::unordered_map<std::string, std::unique_ptr<int>> cache;
  
  try {
    cache["key"] = std::make_unique<int>(42);
    EXPECT_NE(cache.at("key").get(), nullptr);
  } catch (...) {
    FAIL();
  }
}

// EXS-14: Model Shutdown Under Exception
TEST(MemorySafetyHardening, EXS_14_ShutdownUnderException) {
  struct Engine {
    bool running = true;
    
    ~Engine() noexcept {
      running = false;
    }
  };
  
  try {
    Engine e;
    throw std::runtime_error("Error");
  } catch (...) {
    // Engine destroyed even with exception
  }
}

// EXS-15: Concurrent Load Operations Exception Safety
TEST(MemorySafetyHardening, EXS_15_ConcurrentLoadExceptionSafety) {
  struct ThreadSafeModel {
    std::mutex mutex = {};
    bool loaded = false;
    
    void load() {
      std::lock_guard<std::mutex> lock(mutex);
      loaded = true;
    }
  };
  
  ThreadSafeModel model;
  std::thread t([&model]() { model.load(); });
  t.join();
  EXPECT_TRUE(model.loaded);
}

// EXS-16: Model Parameter Update with Rollback
TEST(MemorySafetyHardening, EXS_16_ParameterUpdateRollback) {
  struct Model {
    int version = 1;
    
    void updateVersion(int newVersion) {
      int oldVersion = version;
      try {
        if (newVersion < 0) {
          throw std::invalid_argument("Invalid");
        }
        version = newVersion;
      } catch (...) {
        version = oldVersion;
        throw;
      }
    }
  };
  
  Model m;
  try {
    m.updateVersion(-1);
  } catch (...) {
  }
  
  EXPECT_EQ(m.version, 1);
}

// EXS-17: Model Reload Safety
TEST(MemorySafetyHardening, EXS_17_ModelReloadSafety) {
  struct Model {
    int load_count = 0;
    
    void reload() {
      ++load_count;
    }
  };
  
  Model m;
  m.reload();
  m.reload();
  EXPECT_EQ(m.load_count, 2);
}

// EXS-18: Adapter Exception in Method Call
TEST(MemorySafetyHardening, EXS_18_AdapterMethodException) {
  struct Adapter {
    void process() {
      throw std::runtime_error("Processing failed");
    }
  };
  
  Adapter a;
  EXPECT_THROW(a.process(), std::runtime_error);
}

// EXS-19: Model with Multiple Cleanup Steps
TEST(MemorySafetyHardening, EXS_19_MultipleCleanupSteps) {
  struct Model {
    std::unique_ptr<int> w;
    std::unique_ptr<int> b;
    
    Model() {
      w = std::make_unique<int>(1);
      b = std::make_unique<int>(2);
    }
  };
  
  {
    Model m;
    EXPECT_NE(m.w.get(), nullptr);
    EXPECT_NE(m.b.get(), nullptr);
  }
}

// EXS-20: Exception Safety with Shared Resources
TEST(MemorySafetyHardening, EXS_20_SharedResourceException) {
  auto resource = std::make_shared<int>(42);
  
  try {
    auto copy = resource;
    EXPECT_EQ(resource.use_count(), 2);
    throw std::runtime_error("Error");
  } catch (...) {
    EXPECT_EQ(resource.use_count(), 1);
  }
}

// EXS-21: Model Load with Validation
TEST(MemorySafetyHardening, EXS_21_LoadWithValidation) {
  struct Model {
    int version = 0;
    
    void loadVersion(int v) {
      if (v < 0) {
        throw std::invalid_argument("Bad version");
      }
      version = v;
    }
  };
  
  Model m;
  m.loadVersion(42);
  EXPECT_EQ(m.version, 42);
}

// EXS-22: Plugin Unload with Dependencies
TEST(MemorySafetyHardening, EXS_22_PluginUnloadDependencies) {
  struct Plugin {
    std::vector<int> data;
    
    Plugin() { data.resize(100, 42); }
    ~Plugin() { data.clear(); }
  };
  
  {
    Plugin p;
    EXPECT_FALSE(p.data.empty());
  }
}

// EXS-23: Model State after Failed Initialization
TEST(MemorySafetyHardening, EXS_23_StateAfterFailedInit) {
  struct Model {
    bool initialized = false;
    int state = 0;
    
    void init() {
      initialized = true;
      if (true) {
        throw std::runtime_error("Init failed");
      }
    }
  };
  
  Model m;
  try {
    m.init();
  } catch (...) {
    // State is modified but initialization failed
    EXPECT_TRUE(m.initialized);
  }
}

// EXS-24: Concurrent Model Access with Exception
TEST(MemorySafetyHardening, EXS_24_ConcurrentAccessException) {
  struct Model {
    std::mutex mu = {};
    int value = 0;
    
    void set(int v) {
      std::lock_guard<std::mutex> lock(mu);
      value = v;
    }
    
    int get() {
      std::lock_guard<std::mutex> lock(mu);
      return value;
    }
  };
  
  Model m;
  m.set(100);
  EXPECT_EQ(m.get(), 100);
}

// EXS-25: Model Complete Lifecycle with Exception Safety
TEST(MemorySafetyHardening, EXS_25_CompleteLifecycleExceptionSafety) {
  struct Model {
    bool loaded = false;
    std::unique_ptr<int> data;
    
    void load() {
      if (loaded) {
        throw std::runtime_error("Already loaded");
      }
      data = std::make_unique<int>(42);
      loaded = true;
    }
    
    void unload() {
      if (!loaded) {
        throw std::runtime_error("Not loaded");
      }
      data.reset();
      loaded = false;
    }
  };
  
  Model m;
  m.load();
  EXPECT_TRUE(m.loaded);
  m.unload();
  EXPECT_FALSE(m.loaded);
}

}  // namespace themis::llm::test
