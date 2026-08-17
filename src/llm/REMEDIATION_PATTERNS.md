# LLM Module — Gap Remediation Patterns

**Purpose**: Standardized approaches for closing common gap categories in the LLM module.  
**Version**: 1.0  
**Date**: 2026-08-17

---

## Pattern 1: Braces Imbalance Fix

### Problem
Missing opening/closing braces at file start/end causing compilation issues or scope mismatches.

### Symptom
- File reports error at line 1: "missing closing brace"
- Nested scopes not properly closed
- clang-format fails or reports imbalance

### Remediation Template

```cpp
// BEFORE (broken)
// file: example.cpp

namespace themis::llm {

class MyClass {
  // ... methods and members

} // Missing outer brace for namespace!

// AFTER (fixed)
// file: example.cpp

namespace themis::llm {

class MyClass {
  // ... methods and members
};

} // namespace themis::llm
```

### Steps
1. View entire file structure (use bash `head -30` and `tail -30`)
2. Verify namespace opening at start matches closing at end
3. Count opening { vs closing } for all nested scopes
4. Add missing braces or remove excess ones
5. Run: `clang-format -i filename.cpp`
6. Verify: `clang -fsyntax-only filename.cpp`

---

## Pattern 2: Thread-Safety — Data Race

### Problem
Multiple threads access shared mutable state without synchronization, causing race conditions.

### Symptom
- ThreadSanitizer (TSan) detects "data race"
- Non-deterministic crashes or data corruption under load
- Tests pass individually but fail under stress

### Remediation Template: Mutex-Protected State

```cpp
// BEFORE (unsafe)
class InferenceEngine {
  std::vector<Model*> loaded_models_;  // ❌ Unprotected shared state
  
  void addModel(Model* m) {
    loaded_models_.push_back(m);  // Data race!
  }
  
  Model* getModel(int idx) {
    return loaded_models_[idx];   // Data race!
  }
};

// AFTER (safe with mutex)
class InferenceEngine {
 private:
  mutable std::mutex models_mutex_;  // Guard for loaded_models_
  std::vector<std::unique_ptr<Model>> loaded_models_;  // ✓ Guarded

 public:
  void addModel(std::unique_ptr<Model> m) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    loaded_models_.push_back(std::move(m));
  }
  
  Model* getModel(int idx) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    if (idx < 0 || idx >= static_cast<int>(loaded_models_.size())) {
      return nullptr;
    }
    return loaded_models_[idx].get();
  }
};
```

### Remediation Template: Atomic for Primitives

```cpp
// BEFORE (unsafe)
class TokenQuotaManager {
  int tokens_used_ = 0;  // ❌ Unsynchronized counter
  
  void decrementQuota(int n) {
    tokens_used_ += n;  // Data race!
  }
};

// AFTER (safe with std::atomic)
class TokenQuotaManager {
  std::atomic<int> tokens_used_{0};  // ✓ Thread-safe counter
  
  void decrementQuota(int n) {
    tokens_used_.fetch_add(n, std::memory_order_relaxed);
  }
};
```

### Remediation Steps
1. Identify shared mutable state (fields accessed by multiple threads)
2. Choose guard mechanism:
   - Simple read/write → std::mutex + std::lock_guard
   - Simple counter/flag → std::atomic<T>
   - Multiple related fields → Single std::mutex
3. Add documentation: `@thread_safety` comment block
4. Test with: `clang -fsanitize=thread -g your_test.cpp`

### Lock Ordering Rule
**Must establish a global lock order to prevent deadlocks:**
```
// Document at module level:
// Lock Order (always acquire in this order):
// 1. models_mutex_
// 2. cache_mutex_
// 3. io_mutex_
```

---

## Pattern 3: RAII — Resource Leak Fix

### Problem
Manually allocated resources (new/delete, malloc/free, CUDA alloc) not freed on exception.

### Symptom
- AddressSanitizer detects "heap-use-after-free" or "definitely lost"
- Memory leaks grow under stress/repeated operations
- Resource destruction order is fragile/manual

### Remediation Template: unique_ptr

```cpp
// BEFORE (unsafe — manual cleanup, not exception-safe)
class GgufLoader {
  void loadModel(const std::string& path) {
    uint8_t* model_data = (uint8_t*)malloc(1024 * 1024);
    
    // If any of these throw, model_data is leaked!
    parseHeader(model_data);
    validateIntegrity(model_data);
    transferToGPU(model_data);
    
    free(model_data);  // Only reached if no exception
  }
};

// AFTER (safe — RAII with unique_ptr)
class GgufLoader {
  void loadModel(const std::string& path) {
    // Allocate with unique_ptr; cleanup automatic on exception
    auto model_data = std::make_unique<uint8_t[]>(1024 * 1024);
    
    parseHeader(model_data.get());
    validateIntegrity(model_data.get());
    transferToGPU(model_data.get());
    
    // Automatic cleanup when model_data goes out of scope
  }
};
```

### Remediation Template: custom RAII wrapper for GPU memory

```cpp
// Custom RAII wrapper for CUDA memory
class CudaMemoryGuard {
  uint8_t* ptr_ = nullptr;
  size_t size_ = 0;
  
 public:
  CudaMemoryGuard() = default;
  
  explicit CudaMemoryGuard(size_t size) : size_(size) {
    CUDA_CHECK(cudaMalloc(&ptr_, size));
  }
  
  ~CudaMemoryGuard() {
    if (ptr_) {
      cudaFree(ptr_);  // Safe: never throws
    }
  }
  
  // Move-only semantics
  CudaMemoryGuard(CudaMemoryGuard&& other) noexcept
      : ptr_(other.ptr_), size_(other.size_) {
    other.ptr_ = nullptr;
  }
  
  CudaMemoryGuard& operator=(CudaMemoryGuard&& other) noexcept {
    std::swap(ptr_, other.ptr_);
    size_ = other.size_;
    return *this;
  }
  
  uint8_t* get() { return ptr_; }
  const uint8_t* get() const { return ptr_; }
};

// Usage:
void loadModelToGPU() {
  CudaMemoryGuard gpu_mem(1024 * 1024);  // Allocate
  
  // If any operation throws, GPU memory is automatically freed
  parseHeader(gpu_mem.get());
  validateIntegrity(gpu_mem.get());
  
  // Cleanup guaranteed here
}
```

### Remediation Steps
1. Find all manual allocations: `new`, `malloc`, `CUDA_Malloc`, etc.
2. Replace with:
   - `std::make_unique<T>()` for single objects
   - `std::make_unique<T[]>()` for arrays
   - `std::make_shared<T>()` for shared ownership
   - Custom RAII wrapper for GPU/DB resources
3. Remove corresponding `delete` / `free` / `cudaFree` calls (RAII will handle it)
4. Test with: `clang -fsanitize=address -g your_test.cpp`

### Exception-Safety Levels
- **Strong**: Operation succeeds completely or has no effect → Use atomic operations
- **Basic**: System in valid state after exception → Use RAII for cleanup + locking
- **NoThrow**: Never throws, used for destructors → Must not call throwing functions

---

## Pattern 4: Exception Safety in Destructors

### Problem
Destructors calling throwing functions, violating the no-throw contract.

### Symptom
- `~ClassName() noexcept(false)` — should be noexcept(true)
- Destructor logs exceptions instead of handling them
- Cleanup code doesn't run if exception is thrown

### Remediation Template

```cpp
// BEFORE (unsafe — destructor can throw!)
class ModelCache {
  ~ModelCache() {
    for (auto& [key, model] : cache_) {
      model->flush();  // ❌ Can throw! Violates no-throw contract
      delete model;
    }
  }
};

// AFTER (safe — destructor never throws)
class ModelCache {
  ~ModelCache() noexcept {
    for (auto& [key, model] : cache_) {
      try {
        model->flush();  // Attempt cleanup
      } catch (const std::exception& e) {
        // Log error but don't throw
        LOG(ERROR) << "Model flush failed during destruction: " << e.what();
      }
      // model is std::unique_ptr, will cleanup even if flush() threw
    }
  }
};
```

### Steps
1. Audit all destructors for throwing function calls
2. Wrap throwing calls in try-catch
3. Log errors but don't re-throw
4. Mark destructor `noexcept` (should be default)
5. Test with: `clang -Wexceptions your_test.cpp`

---

## Pattern 5: Null Pointer Dereference Prevention

### Problem
Dereferencing pointers without null checks, causing crashes on invalid input.

### Symptom
- Crash with "segmentation fault" or "null pointer dereference"
- AddressSanitizer reports "SEGV on unknown address 0x000000000000"
- Occurs only with specific input patterns

### Remediation Template

```cpp
// BEFORE (unsafe — assumes pointer is valid)
Model* InferenceEngine::getLoadedModel(const std::string& name) {
  auto it = models_.find(name);
  return it->second;  // ❌ Crash if name not found!
}

// AFTER (safe — check before use)
Model* InferenceEngine::getLoadedModel(const std::string& name) {
  auto it = models_.find(name);
  if (it == models_.end()) {
    LOG(WARNING) << "Model not found: " << name;
    return nullptr;
  }
  return it->second;
}

// Or with std::optional (modern C++)
std::optional<std::reference_wrapper<Model>>
InferenceEngine::getLoadedModel(const std::string& name) {
  auto it = models_.find(name);
  if (it == models_.end()) {
    return std::nullopt;
  }
  return std::ref(it->second);
}
```

### Guidelines
- Check return value of find(), map access, vector access
- Use std::optional<T> for "value or nothing" cases
- Use std::reference_wrapper<T> for references in containers
- Add explicit nullptr checks before dereferencing
- Don't over-check: trust preconditions of internal functions

---

## Pattern 6: Documentation — Doxygen Header

### Problem
Missing or incomplete documentation of class/function purpose, parameters, thread-safety.

### Remediation Template

```cpp
/// @file llm_model_loader.cpp
/// @brief Model loading and lifecycle management for inference engines.
/// 
/// Provides GgufModelLoader with single-model caching, GPU memory
/// management, and exception-safe unload. Thread-safe via internal mutex.
/// Used by InferenceEngine and PluginManager.
/// 
/// @version 1.0
/// @maturity PRODUCTION
/// @author ThemisDB LLM Team

#include "llm/model_loader.h"

namespace themis::llm {

/// @brief Asynchronously load a GGUF model from disk to GPU memory.
/// 
/// @param model_path Path to .gguf file (must be canonical/validated)
/// @param vram_budget_mb Maximum VRAM to allocate for model weights
/// @param callback Invoked with (success, error_msg) on completion
/// 
/// @return LoadHandle that can be used to query loading progress
/// 
/// @throws std::invalid_argument if model_path is empty or not canonical
/// @throws std::runtime_error if file not found or read fails
/// 
/// @thread_safety Thread-safe: queues load operation; returns immediately
/// 
/// @exception_safety Strong: on exception, no state change; LoadHandle not created
/// 
/// @note Callback is invoked on internal thread pool; caller must be
///       reentrant-safe if accessing shared state.
///
/// @see unloadModel(), getLoadedModel()
GgufLoadHandle loadModel(
    const std::string& model_path,
    size_t vram_budget_mb,
    LoadCallback callback);

}  // namespace themis::llm
```

### Remediation Steps
1. Add @file header to every .cpp source file
2. Document all public functions with:
   - @brief (one-line summary)
   - @param (for each parameter)
   - @return (what is returned)
   - @throws (what exceptions can be thrown)
   - @thread_safety (is it thread-safe? how?)
   - @exception_safety (which guarantee level?)
3. Use markdown formatting (code blocks, lists, emphasis)
4. Link to related functions via @see

---

## Pattern 7: Documentation — Thread-Safety Contract

### Problem
Thread-safety behavior is unclear or inconsistent.

### Remediation Template

```cpp
class InferenceEngine {
 private:
  // ====== Thread-Safety Model ======
  // FIELD SYNCHRONIZATION:
  // - inference_queue_: guarded by queue_mutex_
  // - loaded_models_: guarded by models_mutex_ (read-write mutex for parallel reads)
  // - performance_metrics_: atomic accesses only
  // 
  // LOCK ORDER (to prevent deadlock):
  // 1. queue_mutex_ (outer)
  // 2. models_mutex_ (inner)
  // Never acquire in reverse order.
  // ===================================
  
  std::mutex queue_mutex_;
  std::queue<InferenceRequest> inference_queue_;
  
  mutable std::shared_mutex models_mutex_;  // Allows concurrent reads
  std::map<std::string, std::unique_ptr<Model>> loaded_models_;
  
  std::atomic<uint64_t> total_inferences_{0};

 public:
  /// @thread_safety Thread-safe. Multiple threads can submit inferences
  ///               concurrently. Model loading is serialized via queue.
  void submitInference(const InferenceRequest& req) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    inference_queue_.push(req);
  }
  
  /// @thread_safety Thread-safe. Multiple threads can query metrics
  ///               concurrently (atomic access).
  uint64_t getTotalInferences() const {
    return total_inferences_.load(std::memory_order_acquire);
  }
};
```

---

## Validation Checklist

After applying any remediation pattern:

- [ ] Code compiles without errors: `clang++ -std=c++17 -Wall -Wextra file.cpp`
- [ ] No new compiler warnings introduced
- [ ] All existing tests still pass: `ctest --output-on-failure`
- [ ] AddressSanitizer clean: No new leaks or use-after-free
- [ ] ThreadSanitizer clean: No new data races (if applicable)
- [ ] Doxygen builds: `doxygen Doxyfile` produces no errors
- [ ] Code review: Self-review change before submitting

---

## References

- C++ Core Guidelines: https://github.com/isocpp/CppCoreGuidelines
- RAII Principle: https://en.cppreference.com/w/cpp/language/raii
- Exception Safety: https://en.cppreference.com/w/cpp/error/exception
- Thread Safety: https://en.cppreference.com/w/cpp/thread
- AddressSanitizer: https://github.com/google/sanitizers/wiki/AddressSanitizer
- ThreadSanitizer: https://github.com/google/sanitizers/wiki/ThreadSanitzerCppManual

---

**Version History**:
- v1.0 (2026-08-17): Initial patterns document
