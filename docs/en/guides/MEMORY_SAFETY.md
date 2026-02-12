# Memory Safety Guide for ThemisDB

## Overview

This guide explains memory safety best practices in ThemisDB, focusing on modern C++ RAII (Resource Acquisition Is Initialization) principles and smart pointer usage.

## Table of Contents

1. [Why Memory Safety Matters](#why-memory-safety-matters)
2. [Smart Pointers Overview](#smart-pointers-overview)
3. [RAII Principles](#raii-principles)
4. [Usage Examples](#usage-examples)
5. [Custom Deleters for C APIs](#custom-deleters-for-c-apis)
6. [Testing with Sanitizers](#testing-with-sanitizers)
7. [Best Practices](#best-practices)

## Why Memory Safety Matters

Memory safety issues are among the most common sources of bugs and security vulnerabilities in C++ applications:

- **Memory leaks**: Allocated memory that is never freed, leading to gradual resource exhaustion
- **Use-after-free**: Accessing memory after it has been deallocated, causing undefined behavior
- **Double-free**: Freeing the same memory twice, corrupting the heap
- **Unclear ownership**: Ambiguity about which component is responsible for cleanup
- **Exception unsafety**: Memory leaks when exceptions occur before manual cleanup

ThemisDB uses modern C++ smart pointers and RAII to eliminate these issues.

## Smart Pointers Overview

### std::unique_ptr<T>

Use for **exclusive ownership** - when one owner is responsible for the resource.

```cpp
// Factory function returning unique ownership
std::unique_ptr<Model> create_model(const Config& cfg) {
    return std::make_unique<Model>(cfg);  // ✅ Clear ownership
}

void use_model() {
    auto model = create_model(config);
    // model is automatically deleted when it goes out of scope
}
```

**Advantages:**
- Zero overhead (same as raw pointer)
- Cannot be accidentally copied
- Automatic cleanup
- Move semantics for transfer of ownership

### std::shared_ptr<T>

Use for **shared ownership** - when multiple owners may need the resource.

```cpp
class Cache {
    std::unordered_map<std::string, std::shared_ptr<Model>> models_;
    
public:
    std::shared_ptr<Model> get(const std::string& key) {
        auto it = models_.find(key);
        return it != models_.end() ? it->second : nullptr;
    }
};
```

**Advantages:**
- Reference counting ensures safe cleanup
- Can be safely copied and shared
- Resource lives until last owner is destroyed

### std::weak_ptr<T>

Use for **non-owning references** to avoid circular dependencies.

```cpp
class Node {
    std::weak_ptr<Node> parent_;  // ✅ Non-owning, breaks cycle
    std::vector<std::shared_ptr<Node>> children_;  // ✅ Children are owned
    
public:
    std::shared_ptr<Node> get_parent() const {
        return parent_.lock();  // Convert to shared_ptr if still alive
    }
};
```

## RAII Principles

**Resource Acquisition Is Initialization** means resources are acquired in constructors and released in destructors.

### Example: Manual Management (Bad)

```cpp
class Buffer {
    char* data_;
    size_t size_;
    
public:
    Buffer(size_t size) : size_(size) {
        data_ = new char[size];  // ⚠️ What if constructor throws after this?
    }
    
    ~Buffer() {
        delete[] data_;  // ⚠️ Must remember delete[]
    }
};
```

### Example: RAII with std::vector (Good)

```cpp
class Buffer {
    std::vector<char> data_;  // ✅ Automatic memory management
    
public:
    explicit Buffer(size_t size) : data_(size) {}
    
    char* data() noexcept { return data_.data(); }
    size_t size() const noexcept { return data_.size(); }
    // ✅ No manual cleanup needed
};
```

### Example: RAII with std::unique_ptr (Good)

```cpp
class Buffer {
    std::unique_ptr<char[]> data_;
    size_t size_;
    
public:
    explicit Buffer(size_t size) 
        : data_(std::make_unique<char[]>(size)), size_(size) {}
    
    char* data() noexcept { return data_.get(); }
    size_t size() const noexcept { return size_; }
    // ✅ Automatic cleanup via smart pointer
};
```

## Usage Examples

### ThemisDB Memory Utilities

ThemisDB provides RAII wrappers for CUDA memory in `include/utils/memory_utils.h`:

#### CudaBuffer - RAII for CUDA Device Memory

```cpp
#include "utils/memory_utils.h"

using namespace themis::utils::memory;

void process_on_gpu() {
    // Allocate GPU memory - automatically freed on scope exit
    auto gpu_buffer = make_cuda_buffer(1024 * sizeof(float));
    
    if (!gpu_buffer) {
        throw std::runtime_error("Allocation failed");
    }
    
    void* ptr = gpu_buffer.get();
    // Use ptr for CUDA operations
    
    // ✅ cudaFree automatically called when gpu_buffer goes out of scope
}
```

#### PinnedMemory - RAII for Pinned Host Memory

```cpp
#include "utils/memory_utils.h"

using namespace themis::utils::memory;

void transfer_data() {
    // Allocate pinned memory for fast CPU-GPU transfers
    PinnedMemory<float> host_buffer(1024);
    
    float* ptr = host_buffer.get();
    size_t size = host_buffer.size();
    
    // Use ptr for operations
    
    // ✅ cudaFreeHost automatically called when host_buffer goes out of scope
}
```

#### CudaUniquePtr - Typed Smart Pointer

```cpp
#include "utils/memory_utils.h"

using namespace themis::utils::memory;

void process_typed_data() {
    // Typed allocation with automatic cleanup
    auto gpu_floats = make_cuda_unique<float>(1024);
    
    float* ptr = gpu_floats.get();
    // Use ptr for CUDA operations
    
    // ✅ cudaFree automatically called
}
```

### GPU Memory Manager

The `GPUMemoryManager` uses internal RAII wrappers for all allocations:

```cpp
// Internal implementation (simplified)
namespace detail {
    class MemoryHolder {
        void* ptr_;
        size_t bytes_;
        // ... type and cleanup info
        
    public:
        ~MemoryHolder() {
            // Automatic cleanup based on type (GPU/CPU/Pinned)
            // Includes secure clearing before free
        }
    };
}

struct MemoryAllocation {
    void* gpu_ptr = nullptr;
    void* cpu_ptr = nullptr;
    std::shared_ptr<detail::MemoryHolder> holder;  // ✅ RAII cleanup
};
```

## Custom Deleters for C APIs

Many C libraries (CUDA, RocksDB, etc.) require custom cleanup functions. Use custom deleters:

### CUDA Example

```cpp
struct CudaDeleter {
    void operator()(void* ptr) const noexcept {
        if (ptr) {
            cudaFree(ptr);
        }
    }
};

using CudaPtr = std::unique_ptr<void, CudaDeleter>;

CudaPtr allocate_cuda(size_t bytes) {
    void* ptr = nullptr;
    cudaError_t err = cudaMalloc(&ptr, bytes);
    if (err != cudaSuccess) {
        throw std::runtime_error("CUDA allocation failed");
    }
    return CudaPtr(ptr);  // ✅ Automatic cudaFree
}
```

### RocksDB Iterator Example

```cpp
struct RocksIteratorDeleter {
    void operator()(rocksdb::Iterator* it) const {
        delete it;
    }
};

using IteratorPtr = std::unique_ptr<rocksdb::Iterator, RocksIteratorDeleter>;

IteratorPtr create_iterator(rocksdb::DB* db) {
    return IteratorPtr(db->NewIterator(rocksdb::ReadOptions()));
}
```

## Testing with Sanitizers

ThemisDB supports multiple sanitizers for detecting memory safety issues:

### AddressSanitizer (ASAN)

Detects:
- Use-after-free
- Heap buffer overflows
- Stack buffer overflows
- Global buffer overflows
- Use-after-return

```bash
# Build with AddressSanitizer
cmake -B build -DTHEMIS_ENABLE_ASAN=ON
cmake --build build

# Run tests
./build/tests/all_tests
```

### LeakSanitizer (LSAN)

Detects memory leaks:

```bash
# Build with LeakSanitizer
cmake -B build -DTHEMIS_ENABLE_LSAN=ON
cmake --build build

# Run tests with leak detection
LSAN_OPTIONS=detect_leaks=1 ./build/tests/all_tests
```

### UndefinedBehaviorSanitizer (UBSAN)

Detects undefined behavior:

```bash
# Build with UBSan
cmake -B build -DTHEMIS_ENABLE_UBSAN=ON
cmake --build build

# Run tests
./build/tests/all_tests
```

### Combining Sanitizers

```bash
# ASAN and LSAN can be combined
cmake -B build -DTHEMIS_ENABLE_ASAN=ON -DTHEMIS_ENABLE_LSAN=ON
cmake --build build

ASAN_OPTIONS=detect_leaks=1 ./build/tests/all_tests
```

## Best Practices

### ✅ DO: Use Smart Pointers for Dynamic Allocation

```cpp
// Prefer std::make_unique/std::make_shared
auto model = std::make_unique<Model>(config);
auto cache = std::make_shared<Cache>();
```

### ✅ DO: Use std::vector for Dynamic Arrays

```cpp
// Prefer std::vector over new[]
std::vector<float> data(1024);
```

### ✅ DO: Document Ownership

```cpp
// Make ownership explicit in interfaces
std::unique_ptr<Model> create_model(...);  // ✅ Caller owns
std::shared_ptr<Model> get_cached_model(...);  // ✅ Shared ownership
Model* get_current_model();  // ✅ Non-owning (document lifetime)
```

### ✅ DO: Use RAII for All Resources

```cpp
class FileHandle {
    int fd_;
public:
    FileHandle(const char* path) {
        fd_ = open(path, O_RDONLY);
        if (fd_ < 0) throw std::system_error(...);
    }
    ~FileHandle() {
        if (fd_ >= 0) close(fd_);
    }
};
```

### ❌ DON'T: Mix Smart and Raw Pointer Ownership

```cpp
// Bad: Unclear ownership
Model* create_model() {
    return new Model();  // ⚠️ Who deletes this?
}

// Good: Clear ownership
std::unique_ptr<Model> create_model() {
    return std::make_unique<Model>();  // ✅ Caller owns
}
```

### ❌ DON'T: Store Raw Pointers to Owned Resources

```cpp
// Bad: Dangling pointer risk
class Manager {
    Model* model_;  // ⚠️ Who owns this?
public:
    void set_model(Model* m) { model_ = m; }
};

// Good: Clear ownership
class Manager {
    std::unique_ptr<Model> model_;  // ✅ Manager owns
public:
    void set_model(std::unique_ptr<Model> m) { 
        model_ = std::move(m); 
    }
};
```

### ❌ DON'T: Create Circular Shared Pointers

```cpp
// Bad: Memory leak due to cycle
class Node {
    std::shared_ptr<Node> parent_;  // ⚠️ Circular reference
    std::vector<std::shared_ptr<Node>> children_;
};

// Good: Break cycle with weak_ptr
class Node {
    std::weak_ptr<Node> parent_;  // ✅ Non-owning
    std::vector<std::shared_ptr<Node>> children_;
};
```

## Summary

- **Always use smart pointers** for dynamic allocation
- **Prefer std::vector** over raw arrays
- **Use RAII** for all resource management
- **Document ownership** in APIs
- **Test with sanitizers** regularly
- **Avoid raw pointers** unless necessary for non-owning references

Following these guidelines eliminates entire classes of memory safety bugs and makes ThemisDB more reliable and maintainable.
