# GPU Phase 1 Infrastructure — Quick Reference Guide

**Date**: 2026-08-01  
**Purpose**: Quick reference for using Phase 1 foundational error handling  
**Audience**: GPU module developers, query accelerator hardening (Phase 2)  

---

## Quick Links

- **Error Handling**: `include/themis/gpu/gpu_error.h`
- **Memory Management**: `include/themis/gpu/gpu_memory.h`
- **Timeout Enforcement**: `include/themis/gpu/gpu_timeout.h`
- **Implementation**: `src/gpu/gpu_error.cpp`
- **Tests**: `tests/gpu/test_gpu_error_handling.cpp`
- **Full Report**: `ai_working/GPU_PHASE_C_PHASE1_IMPLEMENTATION_REPORT.md`

---

## 1. Error Handling with CHECKED_CUDA / CHECKED_HIP

### Basic Pattern
```cpp
#include "themis/gpu/gpu_error.h"
using namespace themis::gpu;

// Allocate GPU memory — throws on OOM
float* d_data = nullptr;
CHECKED_CUDA(cudaMalloc(&d_data, num_bytes));

// Transfer data
CHECKED_CUDA(cudaMemcpy(d_data, h_data, num_bytes, cudaMemcpyHostToDevice));

// Always use CHECKED_CUDA for all GPU calls
CHECKED_CUDA(cudaFree(d_data));
```

### Error Classification
```cpp
auto handler = GPUErrorHandler::Create();

// Classify error (returns GPUErrorClass)
auto error_class = handler->classifyError(cudaErrorMemoryAllocation);
// → GPUErrorClass::kQuotaExceeded

// Get recovery policy for this class
auto policy = handler->defaultPolicy(error_class);
// → ErrorRecoveryPolicy::kFallbackCPU

// Get error name for logging
auto name = handler->cudaErrorName(cudaErrorMemoryAllocation);
// → "cudaErrorMemoryAllocation"
```

### Error Taxonomy

| Error Class | Meaning | Recovery |
|---|---|---|
| `kQuotaExceeded` | VRAM budget exceeded | CPU fallback |
| `kKernelTimeout` | 5s SLA violated | CPU fallback + log |
| `kBackendUnavailable` | Device offline | Mark unavailable |
| `kMemoryCommunication` | H2D/D2H failure | Retry once |
| `kNumerical` | Precision loss | Emit warning |
| `kUnsupportedOperation` | Kernel not available | CPU fallback |

### Custom Error Handling
```cpp
// Log error without recovery
auto handler = GPUErrorHandler::Create();
handler->logError(cuda_err, "my_kernel");

// Custom retry logic
TRY_CUDA(cudaMalloc(&ptr, size), {
  if (++retry_count < 3) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // retry
  } else {
    throw std::runtime_error("OOM after 3 retries");
  }
});
```

---

## 2. RAII GPU Memory with unique_gpu_ptr / shared_gpu_ptr

### Unique Ownership (Default)
```cpp
#include "themis/gpu/gpu_memory.h"
using namespace themis::gpu;

// Allocate GPU memory (automatic cleanup)
{
  auto d_buffer = make_unique_gpu<float>(1000);  // 4KB on GPU
  
  // Use as raw pointer
  kernel<<<grid, block>>>(d_buffer.get(), ...);
  
  // Access individual elements
  auto first = d_buffer[0];  // dereference
  
  // Manual ownership transfer
  auto d_copy = std::move(d_buffer);
  // d_buffer is now nullptr; d_copy owns allocation
  
}  // Automatic cleanup: CHECKED_CUDA(cudaFree(ptr))
```

### Shared Ownership
```cpp
// When multiple modules need same GPU memory
auto d_tensor = make_shared_gpu<float>(1000);

// Share across modules (increments refcount)
void process_batch(shared_gpu_ptr<float> d_batch) {
  // d_batch owns reference; will be freed when all holders destroyed
}

process_batch(d_tensor);  // Module A holds ref
process_batch(d_tensor);  // Module B holds ref
// Freed only when all 3 destroyed (original + 2 copies)
```

### Memory Management Patterns

**Pattern 1: Factory function (recommended)**
```cpp
auto d_weights = make_unique_gpu<float>(model_size);
// CHECKED_CUDA handles OOM; throws on failure
```

**Pattern 2: Manual management (advanced)**
```cpp
unique_gpu_ptr<float> d_buffer;
try {
  d_buffer = make_unique_gpu<float>(size);
} catch (const std::runtime_error& e) {
  // OOM: fallback to CPU
  return compute_on_cpu(h_data);
}
```

**Pattern 3: Move semantics**
```cpp
unique_gpu_ptr<float> allocate_workspace() {
  return make_unique_gpu<float>(workspace_size);  // moves out
}

auto workspace = allocate_workspace();  // moves in
```

---

## 3. Kernel SLA Enforcement with KernelSLAGuard

### Basic Usage
```cpp
#include "themis/gpu/gpu_timeout.h"
using namespace themis::gpu;

// Create SLA guard (default: 5 seconds)
KernelSLAGuard guard;

// Launch kernel
my_kernel<<<grid, block, 0, stream>>>(args);

// Check if timeout exceeded
if (guard.checkTimeoutDeadline()) {
  // Handle timeout: fallback to CPU
  SPDLOG_WARN("Kernel SLA exceeded; degrading to CPU");
  return compute_on_cpu(h_data);
}
```

### Testing with Custom Timeout
```cpp
// Test with 100ms timeout
KernelSLAGuard guard(std::chrono::milliseconds(100));

// Quick kernel
fast_kernel<<<1, 1>>>();

// Should NOT timeout
ASSERT_FALSE(guard.checkTimeoutDeadline());

// Sleep past deadline
std::this_thread::sleep_for(std::chrono::milliseconds(150));

// NOW it should timeout
ASSERT_TRUE(guard.checkTimeoutDeadline());
```

### Timeout Diagnostics
```cpp
KernelSLAGuard guard(5s);

// ... kernel execution ...

auto elapsed = guard.getElapsedTime();
auto remaining = guard.getRemainingTime();

SPDLOG_INFO("Kernel elapsed: {}ms; remaining: {}ms",
  std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
  std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count());
```

### SLA Durations

| Duration | Use Case |
|---|---|
| 5s | Production (default) |
| 10s | Long-running queries (custom) |
| 100ms | Unit tests |
| 1s | Integration tests |

---

## 4. Integration Examples

### Example 1: Query Accelerator Pattern
```cpp
// In src/gpu/query_accelerator.cpp

auto handler = GPUErrorHandler::Create();

// Allocate working memory
auto d_input = make_unique_gpu<float>(input_size);
auto d_output = make_unique_gpu<float>(output_size);

// Transfer input
CHECKED_CUDA(cudaMemcpy(d_input.get(), h_input, input_size, cudaMemcpyHostToDevice));

// Enforce SLA
KernelSLAGuard guard;

// Launch kernel
accelerated_scan<<<grid, block, 0, stream>>>(
  d_input.get(), d_output.get(), n);

// Check result
if (guard.checkTimeoutDeadline()) {
  SPDLOG_WARN("Query SLA exceeded; using CPU result");
  return run_on_cpu(h_input);
}

// Transfer result
CHECKED_CUDA(cudaMemcpy(h_output, d_output.get(), output_size, cudaMemcpyDeviceToHost));

// Automatic cleanup on scope exit
return result;
```

### Example 2: Memory Manager Pattern
```cpp
// In src/gpu/gpu_memory_manager_edition.cpp

class GPUMemoryManager {
  std::unordered_map<std::string, unique_gpu_ptr<uint8_t>> pools_;
  
  bool allocate_pool(const std::string& tenant_id, size_t size) {
    try {
      auto pool = make_unique_gpu<uint8_t>(size);
      pools_[tenant_id] = std::move(pool);
      return true;
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Failed to allocate pool for {}: {}", tenant_id, e.what());
      return false;
    }
  }
  
  // Automatic cleanup on tenant eviction
  void evict_tenant(const std::string& tenant_id) {
    pools_.erase(tenant_id);  // ~unique_gpu_ptr calls cudaFree
  }
};
```

### Example 3: Error Handling Pattern
```cpp
// In error-prone code path

try {
  // Allocate large tensor
  auto d_tensor = make_unique_gpu<float>(huge_size);
  
  // Process with SLA
  KernelSLAGuard guard(std::chrono::seconds(10));
  process_kernel<<<grid, block>>>(d_tensor.get(), ...);
  
  if (guard.checkTimeoutDeadline()) {
    throw std::runtime_error("Kernel SLA exceeded");
  }
  
} catch (const std::bad_alloc& e) {
  // OOM recovery
  SPDLOG_ERROR("GPU OOM: falling back to CPU");
  return run_on_cpu(input);
} catch (const std::runtime_error& e) {
  // Timeout recovery
  SPDLOG_WARN("Kernel timeout: {}", e.what());
  return run_on_cpu(input);
}
```

---

## 5. Best Practices

### DO ✅
```cpp
// 1. Always use CHECKED_CUDA for every CUDA call
CHECKED_CUDA(cudaMalloc(...));

// 2. Use make_unique_gpu for allocation
auto d_ptr = make_unique_gpu<float>(size);

// 3. Check timeout before result
if (guard.checkTimeoutDeadline()) { /* handle */ }

// 4. Let unique_gpu_ptr handle cleanup
{
  auto d_data = make_unique_gpu<float>(size);
  // ... use ...
}  // auto-freed

// 5. Document error handling strategy
// Handle OOM by degrading to CPU
auto d_buffer = make_unique_gpu<float>(size);
```

### DON'T ❌
```cpp
// 1. Don't forget to check CUDA errors
float* d_ptr;
cudaMalloc(&d_ptr, size);  // WRONG: no error check

// 2. Don't use raw malloc for GPU memory
float* d_ptr = new float[size];  // WRONG: CPU alloc, not GPU

// 3. Don't mix unique_gpu_ptr and raw cudaFree
{
  auto d_ptr = make_unique_gpu<float>(size);
  cudaFree(d_ptr.get());  // WRONG: will double-free
}

// 4. Don't copy unique_gpu_ptr
auto copy = d_ptr;  // COMPILE ERROR: copy deleted

// 5. Don't ignore timeout
KernelSLAGuard guard;
kernel<<<...>>>();
// Forgot to check: guard.checkTimeoutDeadline()
```

---

## 6. Compilation Requirements

### Include Headers
```cpp
#include "themis/gpu/gpu_error.h"      // Error handling
#include "themis/gpu/gpu_memory.h"     // RAII wrappers
#include "themis/gpu/gpu_timeout.h"    // SLA enforcement
```

### Compiler Flags
- **C++ Standard**: C++17 or higher (std::shared_ptr, std::unique_ptr)
- **MSVC**: /W4 (suppress warnings in tests)
- **GCC/Clang**: -Wall -Wextra

### Link Libraries
- `themis_core` (provides gpu_error.cpp implementation)
- `spdlog::spdlog` (logging)
- `Threads::Threads` (pthread_mutex for thread safety)

### Conditional CUDA/HIP
```cpp
// gpu_error.h automatically detects:
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
  #include <cuda_runtime.h>
#endif

#if defined(THEMIS_HIP_ENABLED) || defined(__HIP__)
  #include <hip/hip_runtime.h>
#endif
```

---

## 7. Migration from Raw Pointers

### Before (Phase 0)
```cpp
float* d_weights = nullptr;
cudaError_t err = cudaMalloc(&d_weights, size);
if (err != cudaSuccess) {
  // error handling...
}
// ... use d_weights ...
err = cudaFree(d_weights);
if (err != cudaSuccess) {
  // error handling...
}
```

### After (Phase 1)
```cpp
auto d_weights = make_unique_gpu<float>(size);
// CHECKED_CUDA handles errors; automatic cleanup
// ... use d_weights.get() ...
// (no manual cleanup needed)
```

### Migration Checklist
- [ ] Replace `float* d_ptr` with `auto d_ptr = make_unique_gpu<float>(...)`
- [ ] Replace `cudaMalloc` with `make_unique_gpu`
- [ ] Replace `cudaFree` with scope-based cleanup
- [ ] Wrap all CUDA calls with `CHECKED_CUDA(...)`
- [ ] Add `KernelSLAGuard` for long-running kernels
- [ ] Test with 100ms timeout in unit tests
- [ ] Verify ASAN clean (no memory errors)

---

## 8. Troubleshooting

### Compilation Error: "unique_gpu_ptr<float> = copy not allowed"
```cpp
// WRONG: Trying to copy (not allowed)
auto copy = d_ptr;

// RIGHT: Move instead
auto copy = std::move(d_ptr);
```

### Runtime Error: "cudaFree failed: invalid argument"
```cpp
// Symptom: Double-free or use-after-free

// WRONG: Manual cudaFree + unique_gpu_ptr cleanup
auto d_ptr = make_unique_gpu<float>(size);
cudaFree(d_ptr.get());  // Don't do this!

// RIGHT: Let unique_gpu_ptr handle it
auto d_ptr = make_unique_gpu<float>(size);
// d_ptr auto-frees on scope exit
```

### Test Timeout Always Fails
```cpp
// WRONG: Forgot to check timeout
KernelSLAGuard guard(100ms);
kernel<<<...>>>();
// Missing: if (guard.checkTimeoutDeadline()) { ... }

// RIGHT: Check after kernel
if (guard.checkTimeoutDeadline()) {
  SPDLOG_WARN("Timeout!");
}
```

### "RocksDB not found" during build
```bash
# Use allow-missing-rocksdb preset for development
cmake --preset community-release-allow-missing-rocksdb
```

---

## 9. Phase 2 Integration Checklist

When refactoring existing GPU code (query_accelerator.cpp, etc.):

### Code Review
- [ ] All `float* d_` replaced with `make_unique_gpu<float>`
- [ ] All `cudaMalloc` calls wrapped with `CHECKED_CUDA`
- [ ] All `cudaMemcpy` calls wrapped with `CHECKED_CUDA`
- [ ] All `cudaFree` calls removed (auto cleanup)
- [ ] Manual error checks removed (CHECKED_CUDA handles)
- [ ] `KernelSLAGuard` added for long-running kernels
- [ ] Timeout verified in unit tests

### Testing
- [ ] Compile with /W4 (MSVC) / -Wall -Wextra (GCC)
- [ ] All unit tests pass
- [ ] Run with ASAN: no memory errors
- [ ] Benchmark: ±5% throughput vs baseline
- [ ] Parity tests: GPU result == CPU result (within tolerance)

---

## 10. Further Reading

- **Full Report**: `ai_working/GPU_PHASE_C_PHASE1_IMPLEMENTATION_REPORT.md`
- **Phase C Roadmap**: `ai_working/gpu_phase_c_readiness_plan.md`
- **Module README**: `include/themis/gpu/README.md`
- **Source Files**:
  - `include/themis/gpu/gpu_error.h` (463 lines)
  - `include/themis/gpu/gpu_memory.h` (559 lines)
  - `include/themis/gpu/gpu_timeout.h` (308 lines)
  - `src/gpu/gpu_error.cpp` (300 lines)
  - `tests/gpu/test_gpu_error_handling.cpp` (429 lines)

---

**Last Updated**: 2026-08-01  
**Owner**: ThemisDB GPU Team  
**Phase**: GPU Phase C - Phase 1 (Foundational Error Handling)
