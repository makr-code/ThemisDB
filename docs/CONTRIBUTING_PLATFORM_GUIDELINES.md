# Contributing: Platform Guidelines

Guidelines for writing portable, cross-platform C++ code in ThemisDB.

## Table of Contents

1. [Export Macros](#export-macros)
2. [Platform Detection](#platform-detection)
3. [Compiler Intrinsics](#compiler-intrinsics)
4. [Template Best Practices](#template-best-practices)
5. [Symbol Visibility](#symbol-visibility)
6. [Memory Alignment](#memory-alignment)
7. [Endianness](#endianness)
8. [Testing Requirements](#testing-requirements)

## Export Macros

### When to Use Export Macros

Use export macros for:
- ✅ Public classes in headers under `include/`
- ✅ Public functions that cross DLL boundaries
- ✅ Virtual destructors in public classes
- ✅ Template specializations that are instantiated in one place

Do NOT use for:
- ❌ Private/protected members
- ❌ Internal classes (in `src/` or `detail/` namespace)
- ❌ Header-only template classes
- ❌ Static functions

### Available Export Macros

```cpp
#include <themis/export.h>

// Module-specific macros
THEMIS_BASE_API        // Core types and utilities
THEMIS_STORAGE_API     // Storage engine
THEMIS_QUERY_API       // Query engine
THEMIS_SECURITY_API    // Security features
THEMIS_NETWORK_API     // Network layer
THEMIS_TRANSACTION_API // Transaction management
THEMIS_SHARDING_API    // Distributed features
THEMIS_LLM_API         // LLM integration
THEMIS_CONTENT_API     // Content processing
THEMIS_TIMESERIES_API  // Time-series
THEMIS_GEO_API         // Geospatial
THEMIS_GRAPH_API       // Graph analytics
```

### Usage Examples

#### Public Class

```cpp
// include/themis/base/types.h
#include <themis/export.h>

class THEMIS_BASE_API DatabaseConfig {
public:
    DatabaseConfig();
    ~DatabaseConfig();
    
    void setOption(const std::string& key, const std::string& value);
    std::string getOption(const std::string& key) const;

private:
    class Impl;  // OK to be private without export
    std::unique_ptr<Impl> impl_;
};
```

#### Public Functions

```cpp
// include/themis/storage/api.h
#include <themis/export.h>

// Exported function
THEMIS_STORAGE_API bool initializeStorage(const std::string& path);

// Exported function with C linkage
extern "C" {
    THEMIS_STORAGE_API void* createStorageEngine();
}
```

#### Template Classes (Header-Only)

```cpp
// include/themis/base/result.h
// No export macro needed for header-only templates

template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), has_error_(false) {}
    Result(std::string error) : error_(std::move(error)), has_error_(true) {}
    
    bool hasError() const { return has_error_; }
    const T& value() const { return value_; }
    const std::string& error() const { return error_; }

private:
    T value_;
    std::string error_;
    bool has_error_;
};
```

#### Mixed Public/Private

```cpp
// include/themis/query/executor.h
#include <themis/export.h>

class THEMIS_QUERY_API QueryExecutor {
public:
    QueryExecutor();  // Exported
    ~QueryExecutor(); // Exported
    
    void execute(const Query& query);  // Exported

private:
    // Private members don't need export
    struct ExecutionPlan {
        // ...
    };
    
    void optimizePlan(ExecutionPlan& plan);  // Private, no export needed
    
    class CostModel;  // Private class, no export needed
    std::unique_ptr<CostModel> cost_model_;
};
```

## Platform Detection

### Recommended Approach

```cpp
// include/themis/base/platform.h
#pragma once

// Operating System
#if defined(_WIN32) || defined(_WIN64)
    #define THEMIS_OS_WINDOWS 1
    #define THEMIS_OS_POSIX 0
#elif defined(__linux__)
    #define THEMIS_OS_LINUX 1
    #define THEMIS_OS_POSIX 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define THEMIS_OS_MACOS 1
    #define THEMIS_OS_POSIX 1
#else
    #error "Unsupported operating system"
#endif

// Architecture
#if defined(__x86_64__) || defined(_M_X64)
    #define THEMIS_ARCH_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define THEMIS_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define THEMIS_ARCH_ARM32 1
#else
    #define THEMIS_ARCH_UNKNOWN 1
#endif

// Compiler
#if defined(_MSC_VER)
    #define THEMIS_COMPILER_MSVC 1
#elif defined(__GNUC__)
    #define THEMIS_COMPILER_GCC 1
#elif defined(__clang__)
    #define THEMIS_COMPILER_CLANG 1
#endif
```

### Usage in Code

```cpp
#include <themis/base/platform.h>

void platformSpecificFunction() {
    #if THEMIS_OS_WINDOWS
        // Windows implementation
        #include <windows.h>
        // ...
    #elif THEMIS_OS_POSIX
        // POSIX implementation
        #include <unistd.h>
        // ...
    #endif
}
```

## Compiler Intrinsics

### Portable Wrappers

Always provide fallbacks for compiler intrinsics:

```cpp
// include/themis/base/intrinsics.h
#pragma once

#include <cstdint>

namespace themis {
namespace intrinsics {

// Population count (number of set bits)
inline int popcount(uint32_t x) {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcount(x);
    #elif defined(_MSC_VER)
        return static_cast<int>(__popcnt(x));
    #else
        // Software fallback
        int count = 0;
        while (x) {
            count += x & 1;
            x >>= 1;
        }
        return count;
    #endif
}

// Count leading zeros
inline int clz(uint32_t x) {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_clz(x);
    #elif defined(_MSC_VER)
        unsigned long index;
        _BitScanReverse(&index, x);
        return 31 - static_cast<int>(index);
    #else
        if (x == 0) return 32;
        int count = 0;
        if (!(x & 0xFFFF0000u)) { count += 16; x <<= 16; }
        if (!(x & 0xFF000000u)) { count += 8;  x <<= 8;  }
        if (!(x & 0xF0000000u)) { count += 4;  x <<= 4;  }
        if (!(x & 0xC0000000u)) { count += 2;  x <<= 2;  }
        if (!(x & 0x80000000u)) { count += 1; }
        return count;
    #endif
}

// Byte swap
inline uint32_t bswap32(uint32_t x) {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(x);
    #elif defined(_MSC_VER)
        return _byteswap_ulong(x);
    #else
        return ((x & 0xFF000000u) >> 24) |
               ((x & 0x00FF0000u) >>  8) |
               ((x & 0x0000FF00u) <<  8) |
               ((x & 0x000000FFu) << 24);
    #endif
}

} // namespace intrinsics
} // namespace themis
```

### SIMD Wrappers

```cpp
// include/themis/base/simd.h
#pragma once

#include <themis/base/platform.h>

#if THEMIS_ARCH_X86_64
    #include <immintrin.h>
    #define THEMIS_SIMD_AVAILABLE 1
#elif THEMIS_ARCH_ARM64 || THEMIS_ARCH_ARM32
    #include <arm_neon.h>
    #define THEMIS_SIMD_AVAILABLE 1
#else
    #define THEMIS_SIMD_AVAILABLE 0
#endif

namespace themis {
namespace simd {

// Vector add (4x float)
inline void add4f(float* result, const float* a, const float* b) {
    #if THEMIS_ARCH_X86_64 && defined(__SSE__)
        __m128 va = _mm_loadu_ps(a);
        __m128 vb = _mm_loadu_ps(b);
        __m128 vr = _mm_add_ps(va, vb);
        _mm_storeu_ps(result, vr);
    #elif (THEMIS_ARCH_ARM64 || THEMIS_ARCH_ARM32) && defined(__ARM_NEON)
        float32x4_t va = vld1q_f32(a);
        float32x4_t vb = vld1q_f32(b);
        float32x4_t vr = vaddq_f32(va, vb);
        vst1q_f32(result, vr);
    #else
        // Scalar fallback
        for (int i = 0; i < 4; ++i) {
            result[i] = a[i] + b[i];
        }
    #endif
}

} // namespace simd
} // namespace themis
```

## Template Best Practices

### Header-Only Templates

```cpp
// include/themis/base/container.h
template<typename T>
class Container {
public:
    void add(const T& item) {
        items_.push_back(item);
    }
    
    size_t size() const {
        return items_.size();
    }

private:
    std::vector<T> items_;
};
```

### Templates with Separate Implementation

```cpp
// include/themis/base/processor.h
template<typename T>
class Processor {
public:
    void process(const T& data);
    
private:
    void doProcessing(const T& data);
};

// Include implementation at end of header
#include <themis/base/processor.tpp>
```

```cpp
// include/themis/base/processor.tpp
template<typename T>
void Processor<T>::process(const T& data) {
    doProcessing(data);
}

template<typename T>
void Processor<T>::doProcessing(const T& data) {
    // Implementation
}
```

### Explicit Template Instantiation

```cpp
// include/themis/storage/serializer.h
template<typename T>
class Serializer {
public:
    THEMIS_STORAGE_API void serialize(const T& obj, std::ostream& out);
    THEMIS_STORAGE_API T deserialize(std::istream& in);
};

// Declare explicit instantiations
extern template class Serializer<int>;
extern template class Serializer<std::string>;
extern template class Serializer<MyClass>;
```

```cpp
// src/storage/serializer.cpp
#include <themis/storage/serializer.h>

// Define template methods
template<typename T>
void Serializer<T>::serialize(const T& obj, std::ostream& out) {
    // Implementation
}

template<typename T>
T Serializer<T>::deserialize(std::istream& in) {
    // Implementation
}

// Explicit instantiations
template class Serializer<int>;
template class Serializer<std::string>;
template class Serializer<MyClass>;
```

## Symbol Visibility

### Linux/macOS

```cmake
# In CMakeLists.txt
if(UNIX)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()
```

### Selective Visibility

```cpp
// Public API
class __attribute__((visibility("default"))) PublicClass {
    // ...
};

// Internal class
class __attribute__((visibility("hidden"))) InternalClass {
    // ...
};

// Or use export macros (preferred)
class THEMIS_BASE_API PublicClass {
    // ...
};
```

## Memory Alignment

### Aligned Structures

```cpp
// Ensure proper alignment for SIMD
struct alignas(16) Vec4 {
    float x, y, z, w;
};

// Alignment for cache lines
struct alignas(64) CacheLinePadded {
    std::atomic<uint64_t> counter;
    char padding[56];  // Total 64 bytes
};
```

### Unaligned Access

```cpp
// WRONG on ARM - may cause SIGBUS
uint32_t value = *reinterpret_cast<const uint32_t*>(unaligned_ptr);

// CORRECT - portable
uint32_t value;
std::memcpy(&value, unaligned_ptr, sizeof(value));
```

### Aligned Allocation

```cpp
void* allocateAligned(size_t size, size_t alignment) {
    #if defined(_WIN32)
        return _aligned_malloc(size, alignment);
    #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
    #endif
}

void freeAligned(void* ptr) {
    #if defined(_WIN32)
        _aligned_free(ptr);
    #else
        free(ptr);
    #endif
}
```

## Endianness

### Byte Order Conversion

```cpp
#include <cstdint>

// C++20 way (preferred)
#if __cplusplus >= 202002L
    #include <bit>
    
    inline uint32_t toNetworkOrder(uint32_t value) {
        if constexpr (std::endian::native == std::endian::little) {
            return __builtin_bswap32(value);
        }
        return value;
    }
#else
    // C++17 and earlier
    inline uint32_t toNetworkOrder(uint32_t value) {
        #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            return value;
        #else
            return __builtin_bswap32(value);  // Use intrinsics wrapper
        #endif
    }
#endif
```

### Serialization

```cpp
class Serializer {
public:
    void writeUint32(uint32_t value) {
        uint32_t network = toNetworkOrder(value);
        buffer_.append(reinterpret_cast<const char*>(&network), sizeof(network));
    }
    
    uint32_t readUint32() {
        uint32_t network;
        std::memcpy(&network, &buffer_[pos_], sizeof(network));
        pos_ += sizeof(network);
        return fromNetworkOrder(network);
    }

private:
    std::string buffer_;
    size_t pos_ = 0;
};
```

## Testing Requirements

### Minimum Testing Matrix

Before merging code that modifies public APIs or platform-specific code:

| Platform | Compiler | Required | CI Workflow |
|----------|----------|----------|-------------|
| Windows | MSVC 2019+ | ✅ Yes | `ci-windows-full.yml` |
| Linux | GCC 11+ | ✅ Yes | `ci-linux-full.yml` |
| Linux | Clang 14+ | ⚠️ Recommended | `ci-linux-full.yml` |
| macOS | Apple Clang 13+ | ⚠️ Recommended | Manual testing |
| ARM64 | GCC 11+ | ⚠️ If ARM-specific | `ci-arm-cross.yml` |

### Pre-Commit Checklist

- [ ] Code compiles on at least 2 platforms
- [ ] All public APIs have export macros
- [ ] Platform-specific code is properly guarded
- [ ] Intrinsics have fallback implementations
- [ ] Templates are either header-only or explicitly instantiated
- [ ] No unaligned memory access
- [ ] Endianness handled in serialization
- [ ] Tests pass on target platforms
- [ ] Documentation updated
- [ ] Platform compatibility matrix updated

### Running Tests

```bash
# Build all platforms (if available)
cmake -B build-linux -G Ninja
cmake --build build-linux

cmake -B build-windows -G "Visual Studio 17 2022"
cmake --build build-windows --config Release

# Run tests
cd build-linux && ctest --output-on-failure
cd build-windows && ctest -C Release --output-on-failure

# Check with sanitizers
cmake -B build-asan -DENABLE_ASAN=ON
cmake --build build-asan
cd build-asan && ctest
```

## Code Review Guidelines

Reviewers should check for:

1. **Export Macros**: Are public APIs properly exported?
2. **Platform Guards**: Is platform-specific code guarded?
3. **Intrinsics**: Do intrinsics have fallbacks?
4. **Templates**: Are templates properly handled?
5. **Alignment**: Is memory access properly aligned?
6. **Endianness**: Is byte order handled in I/O?
7. **Testing**: Has code been tested on multiple platforms?

## Tools

Use these tools before committing:

```bash
# Source code audit
python tools/compiler_diagnostics/source_audit.py --root . --output audit.md

# Check specific file
python tools/compiler_diagnostics/source_audit.py --root . --include src/myfile.cpp
```

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Compiler Troubleshooting Guide](build-guide/COMPILER_TROUBLESHOOTING.md)
- [Platform Compatibility Matrix](PLATFORM_COMPATIBILITY_MATRIX.md)
- [Windows Build Errors](build-guide/BUILD_WINDOWS_ERRORS.md)
- [Linux Build Errors](build-guide/BUILD_LINUX_ERRORS.md)
- [ARM Build Errors](build-guide/BUILD_ARM_ERRORS.md)
