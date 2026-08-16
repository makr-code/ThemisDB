# Batch A-3 Remediation Plan: gpu_memory_leak

**Date:** 2026-08-15  
**Status:** 🟡 IN PROGRESS  
**Target Files:** 3 CRITICAL files (5 total with GPU memory operations)  

## Identified Gap Locations

### CRITICAL Hot Spots
1. `src/index/cuda_hnsw_graph_traversal.cpp:362` - CRITICAL
2. `src/index/cuda_hnsw_graph_traversal.cpp:370` - CRITICAL
3. `src/index/cuda_hnsw_graph_traversal.cpp:381` - CRITICAL
4. `src/index/gpu_memory_oversubscription.cpp:53` - CRITICAL

## Issue Analysis

### Current Implementation Pattern

**Location:** `src/index/cuda_hnsw_graph_traversal.cpp`

```cpp
// Line 318: Start fresh
impl_->freeDevice();

// Line 322: Allocate vectors
if (cudaMalloc(&impl_->d_vectors, vec_bytes) != cudaSuccess) {
    // Handle error
    impl_->cuda_available = false;
    return true;
}

// Line 328: Copy data
if (cudaMemcpy(...) != cudaSuccess) {
    impl_->freeDevice();  // Cleanup on failure
    impl_->cuda_available = false;
    return true;
}

// Line 341-342: Allocate graph structures
if (cudaMalloc(&impl_->d_offsets, off_bytes) != cudaSuccess ||
    cudaMalloc(&impl_->d_neighbours, nb_bytes) != cudaSuccess) {
    impl_->freeDevice();  // Cleanup on failure
    impl_->cuda_available = false;
    return true;
}

// Line 370: Allocate visited pool
cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
if (ve == cudaSuccess) {
    impl_->visited_pool_bytes = new_pool_sz;
} else {
    impl_->d_visited_pool = nullptr;
    impl_->visited_pool_bytes = 0;
    // Non-fatal failure
}
```

### Potential Leak Scenarios

1. **Compound Allocation Failure (Line 341-342)**
   - Issue: If first malloc succeeds and second fails, first allocation might not be freed
   - Status: Appears handled by freeDevice() call, but verify short-circuit behavior

2. **Visited Pool Allocation (Line 370)**
   - Issue: Allocated but may not be freed in all code paths
   - Current: Already has error handling; sets to nullptr on failure

3. **Stream Creation (Line 278)**
   - Issue: `cudaStreamCreate()` might leak if subsequent operations fail
   - Current: Freed in `freeDevice()` via `cudaStreamDestroy()`

4. **No RAII Wrapper**
   - Issue: Raw pointer allocations without RAII pattern
   - Risk: Exception or early return bypasses cleanup

## CUDA Memory Leak Prevention Patterns

### Current Pattern (Raw Pointers)
```cpp
// ❌ Risk: Manual cleanup required everywhere
float* d_data = nullptr;
cudaMalloc(&d_data, size);
if (error) {
    cudaFree(d_data);
    // Must remember to free everywhere
}
```

### Recommended: RAII Wrapper
```cpp
// ✅ Safe: Automatic cleanup on scope exit
class CudaBuffer {
    void* ptr_ = nullptr;
    size_t size_ = 0;
public:
    CudaBuffer(size_t size) : size_(size) {
        if (cudaMalloc(&ptr_, size) != cudaSuccess) {
            throw std::runtime_error("cudaMalloc failed");
        }
    }
    
    ~CudaBuffer() {
        if (ptr_) {
            cudaFree(ptr_);
            ptr_ = nullptr;
        }
    }
    
    void* get() { return ptr_; }
    
    // Non-copyable
    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;
    
    // Moveable
    CudaBuffer(CudaBuffer&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
};

// Usage
{
    CudaBuffer vec_buffer(vec_bytes);
    cudaMemcpy(vec_buffer.get(), vectors, vec_bytes, cudaMemcpyHostToDevice);
    // Automatically freed on scope exit
}
```

## Validation Procedures

### NVIDIA Tools
```bash
# Using nvidia-smi to monitor memory
nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits -l 1

# Using cuda-memcheck (deprecated, use compute-sanitizer)
cuda-memcheck ./test_program

# Using compute-sanitizer (newer, recommended)
compute-sanitizer --tool memcheck ./test_program
```

### Manual Verification Checklist
- [ ] All `cudaMalloc` calls have corresponding `cudaFree`
- [ ] Error paths call cleanup (freeDevice)
- [ ] No allocations in exception-prone code without cleanup
- [ ] Visitor pool allocation/deallocation paired
- [ ] Stream creation/destruction paired
- [ ] Device synchronization before deallocation

## GPU Memory Error Handling

### Pattern 1: Safe Allocation with Error Code
```cpp
void* d_ptr = nullptr;
cudaError_t err = cudaMalloc(&d_ptr, size);
if (err != cudaSuccess) {
    THEMIS_ERROR("cudaMalloc failed: {}", cudaGetErrorString(err));
    // d_ptr is nullptr, safe to proceed
    return;  // No leak - was never allocated
}
// Safe to use d_ptr
```

### Pattern 2: Paired Allocation and Deallocation
```cpp
// Allocation
if (cudaMalloc(&impl_->d_data, size) != cudaSuccess) {
    impl_->d_data = nullptr;
    return false;
}

// Deallocation (in freeDevice or destructor)
if (impl_->d_data) {
    cudaFree(impl_->d_data);
    impl_->d_data = nullptr;
}
```

## Files Status

### src/index/cuda_hnsw_graph_traversal.cpp
**Summary:** Contains primary GPU allocation logic with error handling

**Lines 362, 370, 381:**
- Line 362: Visited pool allocation comment
- Line 370: `cudaMalloc(&impl_->d_visited_pool, new_pool_sz)`
- Line 381: WARN message about fallback allocation

**Current Status:** Error handling present; needs validation for no leaks

**Verification Needed:**
- [ ] Ensure freeDevice() is called on all error paths
- [ ] Verify visited_pool cleanup in destructor
- [ ] Test allocation failure scenarios

### src/index/gpu_memory_oversubscription.cpp
**Summary:** Memory management for VRAM/host-RAM partitioning

**Line 53:** Needs review for allocation patterns

**Current Status:** Requires detailed analysis

## Remediation Tasks

### Task A-3.1: Audit cuda_hnsw_graph_traversal.cpp
- [ ] Trace all cudaMalloc calls
- [ ] Verify freeDevice() calls on all error paths
- [ ] Test buildIndex() failure scenarios
- [ ] Verify destructor calls freeDevice()

### Task A-3.2: Audit gpu_memory_oversubscription.cpp
- [ ] Identify all GPU allocations
- [ ] Verify deallocation patterns
- [ ] Check error handling completeness

### Task A-3.3: Add RAII Wrappers (Optional, Best Practice)
- [ ] Consider implementing CudaBuffer RAII wrapper
- [ ] Refactor allocations to use wrapper
- [ ] Eliminates manual cleanup burden

### Task A-3.4: Testing
- [ ] Run compute-sanitizer on CUDA-enabled builds
- [ ] Test allocation failure scenarios
- [ ] Monitor GPU memory usage during tests

## Compilation & Testing

```bash
# Build with CUDA support
cmake --preset <preset-with-cuda>
cmake --build <build_dir>

# Run with memory checking
compute-sanitizer --tool memcheck ctest -L index

# Monitor GPU memory
nvidia-smi --query-gpu=memory.used --format=csv -l 1
```

## Success Criteria
- ✅ 0 GPU memory leaks in sanitizer output
- ✅ All CUDA calls properly error-checked
- ✅ Allocation/deallocation pairs verified
- ✅ Focused tests PASS with clean GPU memory

---
