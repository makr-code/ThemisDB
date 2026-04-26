# Null-Pointer Safety Guide

## Overview

This guide provides best practices for preventing null-pointer dereferences in ThemisDB. Null-pointer dereferences are a critical source of crashes and undefined behavior in C++ applications.

## Table of Contents

1. [Common Risk Areas](#common-risk-areas)
2. [Safety Utilities](#safety-utilities)
3. [Best Practices](#best-practices)
4. [Code Review Checklist](#code-review-checklist)
5. [Testing Guidelines](#testing-guidelines)

## Common Risk Areas

### 1. Raw Pointer Usage

**Problem:**
```cpp
SomeType* ptr = get_pointer();
ptr->method();  // ⚠️ CRASH if ptr == nullptr
```

**Solution:**
```cpp
#include "utils/pointer_utils.h"
using namespace themis::utils::pointer;

// Option 1: Exception-based validation
SomeType* ptr = get_pointer();
auto* safe_ptr = require_non_null(ptr, "get_pointer returned null");
safe_ptr->method();  // Safe - guaranteed non-null

// Option 2: Optional-based handling
if (auto opt = as_optional(ptr)) {
    (*opt)->method();
} else {
    // Handle null case
}
```

### 2. Dynamic Cast Without Validation

**Problem:**
```cpp
Base* base = get_base();
Derived* derived = dynamic_cast<Derived*>(base);
derived->derived_method();  // ⚠️ CRASH if cast fails
```

**Solution:**
```cpp
#include "utils/pointer_utils.h"
using namespace themis::utils::pointer;

Base* base = get_base();
if (auto derived = safe_dynamic_cast<Derived>(base)) {
    (*derived)->derived_method();  // Safe
} else {
    spdlog::warn("Dynamic cast failed");
}
```

### 3. RocksDB Iterator Creation

**Problem:**
```cpp
std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));
it->Seek(key);  // ⚠️ NewIterator can return nullptr
```

**Solution:**
```cpp
std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));
if (!it) {
    spdlog::error("Failed to create RocksDB iterator");
    return error_value;
}
it->Seek(key);  // Safe
```

### 4. Weak Pointer Locking

**Problem:**
```cpp
std::weak_ptr<Resource> weak = get_weak();
auto shared = weak.lock();
shared->use();  // ⚠️ CRASH if resource expired
```

**Solution:**
```cpp
#include "utils/pointer_utils.h"
using namespace themis::utils::pointer;

std::weak_ptr<Resource> weak = get_weak();
if (auto shared = safe_lock(weak, "Resource expired")) {
    (*shared)->use();  // Safe
} else {
    // Handle expired resource
}
```

### 5. Container Access

**Problem:**
```cpp
std::map<std::string, Value> cache;
Value val = cache[key];  // ⚠️ Creates default value if key doesn't exist
```

**Solution:**
```cpp
#include "utils/pointer_utils.h"
using namespace themis::utils::pointer;

// Option 1: Use safe_at utility
if (auto val = safe_at(cache, key)) {
    // Use val.value()
}

// Option 2: Use find()
if (auto it = cache.find(key); it != cache.end()) {
    Value val = it->second;
}

// Option 3: Use at() with exception handling
try {
    Value val = cache.at(key);
} catch (const std::out_of_range&) {
    // Key not found
}
```

### 6. C API Calls

**Problem:**
```cpp
FILE* f = fopen("file.txt", "r");
fread(buffer, size, 1, f);  // ⚠️ fopen can return nullptr
```

**Solution:**
```cpp
#include "utils/pointer_utils.h"
using namespace themis::utils::pointer;

// Option 1: RAII wrapper
auto file = wrap_c_ptr(fopen("file.txt", "r"), [](FILE* f) {
    if (f) fclose(f);
});
fread(buffer, size, 1, file.get());  // Safe - exception thrown if null

// Option 2: Explicit check
FILE* f = fopen("file.txt", "r");
if (!f) {
    spdlog::error("Failed to open file");
    return error_value;
}
fread(buffer, size, 1, f);
fclose(f);
```

## Safety Utilities

ThemisDB provides a comprehensive set of null-safety utilities in `include/utils/pointer_utils.h`:

### require_non_null()

Validates that a pointer is non-null, throws exception if null.

```cpp
template<typename T>
T* require_non_null(T* ptr, const char* message = "Null pointer");
```

**Example:**
```cpp
int* ptr = get_pointer();
auto* validated = require_non_null(ptr, "get_pointer failed");
// validated is guaranteed non-null here
```

### as_optional()

Converts a raw pointer to `std::optional`.

```cpp
template<typename T>
std::optional<T*> as_optional(T* ptr) noexcept;
```

**Example:**
```cpp
if (auto opt = as_optional(ptr)) {
    // Use *opt
}
```

### safe_invoke()

Safely invokes a function on a pointer if non-null.

```cpp
template<typename T, typename Func>
auto safe_invoke(T* ptr, Func&& func) -> std::optional<decltype(func(*ptr))>;
```

**Example:**
```cpp
auto result = safe_invoke(ptr, [](MyType& obj) {
    return obj.compute();
});
if (result) {
    // Use result.value()
}
```

### wrap_c_ptr()

RAII wrapper for C API pointers with custom deleter.

```cpp
template<typename T, typename Deleter>
std::unique_ptr<T, Deleter> wrap_c_ptr(T* ptr, Deleter deleter);
```

**Example:**
```cpp
auto file = wrap_c_ptr(fopen("file.txt", "r"), [](FILE* f) {
    if (f) fclose(f);
});
// File automatically closed on scope exit
```

### safe_dynamic_cast()

Validates dynamic_cast result, returns optional.

```cpp
template<typename Derived, typename Base>
std::optional<Derived*> safe_dynamic_cast(Base* base) noexcept;
```

**Example:**
```cpp
if (auto derived = safe_dynamic_cast<Derived>(base)) {
    (*derived)->derived_method();
}
```

### safe_lock()

Validates shared_ptr from weak_ptr::lock().

```cpp
template<typename T>
std::optional<std::shared_ptr<T>> safe_lock(
    const std::weak_ptr<T>& weak,
    const char* message = "weak_ptr lock failed"
) noexcept;
```

**Example:**
```cpp
if (auto locked = safe_lock(weak, "Resource expired")) {
    (*locked)->use();
}
```

### safe_at()

Safe container access with bounds checking.

```cpp
// For maps
template<typename Container, typename Key>
auto safe_at(const Container& container, const Key& key) 
    -> std::optional<typename Container::mapped_type>;

// For vectors
template<typename T>
std::optional<std::reference_wrapper<T>> safe_at(
    std::vector<T>& vec, 
    size_t index
) noexcept;
```

**Example:**
```cpp
// Map access
if (auto val = safe_at(map, "key")) {
    // Use val.value()
}

// Vector access
if (auto elem = safe_at(vec, 5)) {
    // Use elem.value().get()
}
```

## Best Practices

### 1. Prefer Smart Pointers

Use `std::unique_ptr` and `std::shared_ptr` instead of raw pointers when possible.

```cpp
// Good
std::unique_ptr<Resource> resource = create_resource();

// Avoid
Resource* resource = new Resource();
```

### 2. Use std::optional for Nullable Returns

```cpp
// Good
std::optional<Config> load_config();

// Avoid
Config* load_config();  // Caller must check for null
```

### 3. Document Null Expectations

```cpp
/// @brief Gets a node by ID
/// @param id Node identifier
/// @returns Pointer to node, or nullptr if not found
/// @note Caller must check for nullptr before dereferencing
Node* get_node(size_t id);
```

### 4. Check C API Returns

Always check return values from C APIs (RocksDB, CUDA, file operations, etc.).

```cpp
void* ptr;
cudaError_t err = cudaMalloc(&ptr, size);
if (err != cudaSuccess) {
    spdlog::error("CUDA allocation failed: {}", cudaGetErrorString(err));
    return error_value;
}
if (!ptr) {
    spdlog::error("CUDA allocation returned null");
    return error_value;
}
```

### 5. Use RAII for Resource Management

Wrap C API resources in RAII wrappers to ensure cleanup.

```cpp
class GPUMemory {
    void* ptr_ = nullptr;
    
public:
    explicit GPUMemory(size_t bytes) {
        cudaError_t err = cudaMalloc(&ptr_, bytes);
        if (err != cudaSuccess || !ptr_) {
            throw std::runtime_error("GPU allocation failed");
        }
    }
    
    ~GPUMemory() {
        if (ptr_) cudaFree(ptr_);
    }
    
    // Non-copyable, movable
    GPUMemory(const GPUMemory&) = delete;
    GPUMemory(GPUMemory&& other) noexcept 
        : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    void* get() const { return ptr_; }
};
```

## Code Review Checklist

Use this checklist when reviewing code for null-pointer safety:

- [ ] Are all raw pointer dereferences preceded by null checks?
- [ ] Do functions returning pointers document null-return conditions?
- [ ] Are `dynamic_cast` results checked before use?
- [ ] Are RocksDB iterators checked after `NewIterator()`?
- [ ] Are C API return values checked (cudaMalloc, fopen, etc.)?
- [ ] Are `weak_ptr::lock()` results checked before use?
- [ ] Are container accesses bounds-checked or use safe alternatives?
- [ ] Are smart pointers preferred over raw pointers?
- [ ] Is RAII used for resource management?
- [ ] Are exceptions used appropriately for unrecoverable errors?

## Testing Guidelines

### Unit Tests

Write unit tests that specifically validate null-handling:

```cpp
TEST(MyClass, HandlesNullPointer) {
    MyClass obj;
    EXPECT_NO_THROW(obj.process(nullptr));
    // Or
    EXPECT_THROW(obj.process(nullptr), std::runtime_error);
}

TEST(MyClass, HandlesInvalidIterator) {
    // Test with mock that returns null iterator
}
```

### Integration Tests with Sanitizers

Run tests with AddressSanitizer (ASan) to detect null-pointer dereferences:

```bash
# Configure with ASan
cmake -B build -DENABLE_ASAN=ON

# Build and run tests
cmake --build build
./build/tests/all_tests
```

### Static Analysis

Use static analysis tools to detect potential null-pointer issues:

```bash
# Run clang-tidy
clang-tidy src/**/*.cpp -- -I include/

# Run cppcheck
cppcheck --enable=all src/
```

## Examples from ThemisDB

### RocksDB Iterator Safety

```cpp
// From src/llm/feedback_store.cpp
std::unique_ptr<rocksdb::Iterator> it;
if (cf_) {
    it.reset(db_->NewIterator(read_opts, cf_));
} else {
    it.reset(db_->NewIterator(read_opts));
}

if (!it) {
    THEMIS_ERROR("Failed to create RocksDB iterator");
    return empty_results;
}

it->Seek(start_key);  // Safe now
```

### Weak Pointer Safety

```cpp
// From src/server/mqtt_session.cpp
for (auto& sessionWeak : sessions) {
    if (auto session = sessionWeak.lock()) {
        session->sendPublish(topic, payload, qos, retain);
    }
}
```

### Dynamic Cast Safety

```cpp
// From src/query/let_evaluator.cpp
if (auto lit = dynamic_cast<query::LiteralExpr*>(expr.get())) {
    return evaluateLiteral(lit);  // Safe within if block
}
```

## Additional Resources

- [CppCoreGuidelines: Bounds and Safety](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-bounds)
- [Google C++ Style Guide: Ownership and Smart Pointers](https://google.github.io/styleguide/cppguide.html#Ownership_and_Smart_Pointers)
- [CERT C++ Secure Coding: EXP34-C](https://wiki.sei.cmu.edu/confluence/display/c/EXP34-C.+Do+not+dereference+null+pointers)

## Support

For questions or issues related to null-pointer safety:

1. Check the [pointer_utils.h](../../../include/utils/pointer_utils.h) header for utility documentation
2. Review test examples in [test_pointer_utils.cpp](../../../tests/test_pointer_utils.cpp)
3. Contact the development team via GitHub issues

---

**Last Updated:** 2026-04-06  
**Version:** 1.0.0
