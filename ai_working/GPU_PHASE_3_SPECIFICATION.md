# GPU Phase 3: Memory Management Hardening Specification

**Phase**: 3 of 5  
**Status**: ⏳ Ready to Start (after Phase 2 completion)  
**Date**: 2026-08-01  
**Target**: Fix 50+ HIGH findings in memory management paths  

---

## Overview

Phase 3 hardens the GPU memory management subsystem to achieve exception-safe allocation/deallocation and quota tracking. The goal is to eliminate resource leaks on exception paths and ensure deterministic cleanup behavior.

**Files to Modify**:
1. `src/gpu/gpu_memory_manager_edition.cpp` (43 findings)
2. `src/gpu/memory_pool.cpp` (9 findings)
3. Tests: `tests/gpu/test_gpu_memory_management.cpp`

**Total Findings**: 52 HIGH/CRITICAL across both files
- gpu_memory_manager_edition.cpp: 42 HIGH (resource management)
- memory_pool.cpp: 8 HIGH + 1 CRITICAL

---

## Critical Issue Analysis

### gpu_memory_manager_edition.cpp (43 findings)

**Dominant Categories**:
- `resource_leaked_in_exception` (20+): Allocation without guaranteed cleanup
- `db_connection_leak` (20+): Resource tracking error
- `uninitialized_access` (3): Fields accessed before initialization

**Root Causes**:
1. Manual allocation/deallocation without RAII
2. Missing try-catch cleanup in error paths
3. Quota map updated before verification (non-atomic)
4. No rollback on allocation failure

### memory_pool.cpp (9 findings)

**Dominant Categories**:
- `resource_leaked_in_exception` (8): Pool expansion without cleanup guard
- `uninitialized_access` (1): Pool state queried before init

**Root Causes**:
1. Pool reallocation without exception guard
2. Allocation list not cleared on failure

---

## Phase 3 Deliverables

### 1. GPU Memory Manager Edition Hardening

**File**: `src/gpu/gpu_memory_manager_edition.cpp`

#### High-Priority Methods (exception-unsafe → exception-safe)

**Method 1: `allocateTenantQuota(tenant_id, bytes)`**

Current issue (lines 13, 50-80):
```cpp
// BEFORE: Resource leak if assignment throws
bool allocateTenantQuota(const std::string& tenant_id, size_t bytes) {
  if (!policy_->canAllocate(tenant_id, bytes)) return false;
  
  void* ptr = nullptr;
  if (cudaMalloc(&ptr, bytes) != cudaSuccess) return false;
  
  // BUG: If this line throws, ptr leaks
  quota_map_[tenant_id] = {ptr, bytes};
  
  policy_->onAllocate(tenant_id, bytes);
  return true;
}
```

Fixed version (exception-safe):
```cpp
// AFTER: Exception-safe with RAII
bool allocateTenantQuota(const std::string& tenant_id, size_t bytes) {
  if (!policy_->canAllocate(tenant_id, bytes)) return false;
  
  try {
    // Allocate with RAII — fails or succeeds atomically
    auto gpu_mem = make_unique_gpu<uint8_t>(bytes);
    
    // Create temporary entry (doesn't commit yet)
    struct TenantAlloc {
      unique_gpu_ptr<uint8_t> mem;
      size_t size;
    } alloc{std::move(gpu_mem), bytes};
    
    // Update quota map atomically (move, not copy)
    quota_map_[tenant_id] = std::move(alloc);
    
    // Notify policy after successful allocation
    policy_->onAllocate(tenant_id, bytes);
    
    return true;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to allocate quota for {}: {}", tenant_id, e.what());
    // Auto-cleanup: gpu_mem destroyed on exception
    return false;
  }
}
```

**Method 2: `deallocateTenantQuota(tenant_id)`**

Ensure cleanup is exception-safe:
```cpp
bool deallocateTenantQuota(const std::string& tenant_id) {
  auto it = quota_map_.find(tenant_id);
  if (it == quota_map_.end()) return false;
  
  try {
    size_t bytes = it->second.size;
    
    // Notify policy BEFORE deallocation
    policy_->onDeallocate(tenant_id, bytes);
    
    // Erase from map (destructor calls unique_gpu_ptr::~unique_gpu_ptr)
    quota_map_.erase(it);
    // If erase throws, mem still valid (exception safety)
    
    return true;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to deallocate quota for {}: {}", tenant_id, e.what());
    return false;
  }
}
```

**Method 3: `expandAllocationPool(new_size)`**

Current issue (lines 235, 279):
```cpp
// BEFORE: Pool reallocation without exception guard
bool expandAllocationPool(size_t new_size) {
  void* new_ptr = nullptr;
  if (cudaMalloc(&new_ptr, new_size) != cudaSuccess) return false;
  
  // BUG: If copy throws, old pool freed but not replaced
  std::vector<uint8_t> new_pool(reinterpret_cast<uint8_t*>(new_ptr), 
                                reinterpret_cast<uint8_t*>(new_ptr) + new_size);
  
  // Old pool implicitly freed
  pool_ = std::move(new_pool);
  pool_size_ = new_size;
  return true;
}
```

Fixed version:
```cpp
// AFTER: Atomic pool expansion with rollback
bool expandAllocationPool(size_t new_size) {
  try {
    // Allocate new pool with RAII
    auto new_pool = make_unique_gpu<uint8_t>(new_size);
    
    // Update atomically (old pool auto-freed when moved)
    gpu_pool_ = std::move(new_pool);
    pool_size_ = new_size;
    
    SPDLOG_DEBUG("Pool expanded to {} bytes", new_size);
    return true;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Pool expansion failed: {}", e.what());
    // pool_ unchanged — no partial state
    return false;
  }
}
```

#### Data Structure Changes

Replace raw pointers in quota map:
```cpp
// BEFORE
struct TenantQuota {
  void* device_ptr;
  size_t bytes;
};
std::map<std::string, TenantQuota> quota_map_;

// AFTER
struct TenantQuota {
  unique_gpu_ptr<uint8_t> device_ptr;  // RAII
  size_t bytes;
};
std::map<std::string, TenantQuota> quota_map_;
```

### 2. Memory Pool Hardening

**File**: `src/gpu/memory_pool.cpp`

#### Critical Methods

**Method: `GPUMemoryPool::allocate(tenant_id, bytes)`**

Ensure allocation tracking is atomic:
```cpp
// AFTER: Exception-safe allocation tracking
std::optional<GPUAllocation> GPUMemoryPool::allocate(
    const std::string& tenant_id, size_t bytes) {
  if (!isInitialized()) return std::nullopt;
  
  try {
    // Check quota first (non-allocating)
    if (!canAllocateForTenant(tenant_id, bytes)) {
      return std::nullopt;
    }
    
    // Allocate with RAII
    auto gpu_mem = make_unique_gpu<uint8_t>(bytes);
    
    // Create allocation entry
    GPUAllocation alloc{
      .tenant_id = tenant_id,
      .ptr = gpu_mem.get(),
      .size = bytes,
      .timestamp = std::chrono::steady_clock::now(),
    };
    
    // Track allocation (move, not copy)
    active_allocations_.push_back({std::move(gpu_mem), alloc});
    tenant_usage_[tenant_id] += bytes;
    
    return alloc;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to allocate {} bytes for {}: {}", 
                 bytes, tenant_id, e.what());
    return std::nullopt;  // Not std::nullopt; failure is clear
  }
}
```

**Method: `GPUMemoryPool::deallocate(alloc_id)`**

Ensure deallocation is idempotent:
```cpp
bool GPUMemoryPool::deallocate(const AllocationId& alloc_id) {
  if (!isInitialized()) return false;
  
  try {
    auto it = std::find_if(active_allocations_.begin(),
                          active_allocations_.end(),
                          [&](const auto& pair) { 
                            return pair.second.id == alloc_id; 
                          });
    
    if (it == active_allocations_.end()) {
      SPDLOG_WARN("Allocation {} not found (already freed?)", alloc_id);
      return false;  // Idempotent: no error if already freed
    }
    
    // Update tenant usage BEFORE erasing
    tenant_usage_[it->second.tenant_id] -= it->second.size;
    
    // Erase (destructor frees GPU memory via unique_gpu_ptr)
    active_allocations_.erase(it);
    
    return true;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to deallocate {}: {}", alloc_id, e.what());
    return false;
  }
}
```

### 3. Test Suite

**File**: `tests/gpu/test_gpu_memory_management.cpp`

#### New Test Cases

**Test 1: Exception Safety on Allocation**
```cpp
TEST(GPUMemoryManagerHardening, AllocationFailureDoesNotLeakMemory) {
  auto manager = createTestMemoryManager();
  
  // Simulate allocation failure (inject error via mock)
  // Verify that failed allocation doesn't corrupt quota map
  EXPECT_THROW(
    manager->allocateTenantQuota("tenant1", 1e100),  // Impossible size
    std::bad_alloc
  );
  
  // Quota map should be unchanged
  EXPECT_FALSE(manager->hasQuota("tenant1"));
}
```

**Test 2: Quota Map Atomic Update**
```cpp
TEST(GPUMemoryManagerHardening, QuotaMapAtomicUpdate) {
  auto manager = createTestMemoryManager();
  
  // Allocate first quota
  EXPECT_TRUE(manager->allocateTenantQuota("tenant1", 1024));
  EXPECT_EQ(manager->getTenantUsage("tenant1"), 1024);
  
  // Deallocate
  EXPECT_TRUE(manager->deallocateTenantQuota("tenant1"));
  EXPECT_EQ(manager->getTenantUsage("tenant1"), 0);
}
```

**Test 3: Pool Expansion Exception Safety**
```cpp
TEST(GPUMemoryPoolHardening, PoolExpansionRollback) {
  auto pool = createTestMemoryPool();
  size_t original_size = pool->getPoolSize();
  
  // Simulate expansion failure
  // (Inject error via mock GPU allocator)
  EXPECT_FALSE(pool->expandAllocationPool(1e100));
  
  // Pool size should be unchanged
  EXPECT_EQ(pool->getPoolSize(), original_size);
}
```

**Test 4: Deallocation Idempotency**
```cpp
TEST(GPUMemoryPoolHardening, DeallocateIdempotent) {
  auto pool = createTestMemoryPool();
  auto alloc = pool->allocate("tenant1", 512);
  
  // First deallocation should succeed
  EXPECT_TRUE(pool->deallocate(alloc.id));
  
  // Second deallocation should not error (idempotent)
  EXPECT_FALSE(pool->deallocate(alloc.id));  // Returns false but doesn't throw
}
```

**Test 5: Address Sanitizer - No Leaks**
```cpp
TEST(GPUMemoryManagerASAN, NoMemoryLeaksOnExceptions) {
  auto manager = createTestMemoryManager();
  
  for (int i = 0; i < 100; ++i) {
    // Allocate
    EXPECT_TRUE(manager->allocateTenantQuota("tenant" + std::to_string(i), 1024));
  }
  
  for (int i = 0; i < 100; ++i) {
    // Deallocate
    EXPECT_TRUE(manager->deallocateTenantQuota("tenant" + std::to_string(i)));
  }
  
  // ASAN should report zero leaks
}
```

---

## Acceptance Criteria

### Code Quality

- [ ] All raw GPU pointers replaced with `unique_gpu_ptr<T>` in quota_map_ and pool_
- [ ] All allocation paths wrapped with `make_unique_gpu<T>()` and CHECKED_CUDA()
- [ ] All deallocation paths exception-safe (try-catch or RAII guard)
- [ ] No manual cudaFree() calls remain
- [ ] Quota map update is atomic (alloc succeeds or fails cleanly)

### Testing

- [ ] test_gpu_memory_management passes all tests (5+ new exception injection tests)
- [ ] test_gpu_memory_hierarchy passes (tenant isolation verified)
- [ ] Zero Address Sanitizer warnings (ASAN_OPTIONS=detect_leaks=1)
- [ ] Zero Memory Sanitizer warnings
- [ ] Benchmark: memory allocation throughput within ±5% baseline

### Documentation

- [ ] Update src/gpu/ROADMAP.md: Phase 3 complete marker
- [ ] Add @file header to modified gpu_memory_manager_edition.cpp
- [ ] Update include/themis/gpu/README.md: Phase 3 completion
- [ ] Code comments explain exception-safe patterns

### Gap Reduction

- [ ] Reduce HIGH/CRITICAL findings from 43 → ≤21 (50% reduction)
- [ ] Audit script confirms ≤21 findings remain
- [ ] Roadmap updated: Phase 3 completion

---

## Timeline

**Start**: After Phase 2 completion (estimated 2026-08-02)  
**Duration**: 3-4 days  
**End**: 2026-08-05

---

## References

- Phase C Plan: ai_working/gpu_phase_c_readiness_plan.md
- Implementation Guide: ai_working/GPU_PHASE_2_3_4_IMPLEMENTATION_GUIDE.md
- Phase 1 Deliverables: include/themis/gpu/{gpu_error,gpu_memory,gpu_timeout}.h
- Current Gaps: src/gpu/MODULE_GAPS.md (lines 43 findings in gpu_memory_manager_edition.cpp)
- Roadmap: src/gpu/ROADMAP.md
