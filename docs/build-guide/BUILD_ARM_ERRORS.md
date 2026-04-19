# ARM Build Errors & Solutions

This document catalogs common errors when cross-compiling ThemisDB for ARM platforms (ARM64/AArch64 and ARMv7).

## Table of Contents

1. [Cross-Compilation Setup](#cross-compilation-setup)
2. [ABI Issues](#abi-issues)
3. [Endianness Problems](#endianness-problems)
4. [SIMD/Intrinsics](#simdintrinsics)
5. [Memory Alignment](#memory-alignment)
6. [Performance Considerations](#performance-considerations)

## Cross-Compilation Setup

### Toolchain Configuration

**For ARM64 (AArch64)**:
```cmake
# arm64-toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**For ARMv7**:
```cmake
# armv7-toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard")

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**Building**:
```bash
# Install cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# Or for ARMv7
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm64-toolchain.cmake
cmake --build build
```

## ABI Issues

### Error: Incompatible Function Pointer Types

**Symptom**:
```
error: cast between incompatible function types
```

**Cause**: Different calling conventions or ABI between x86 and ARM

**Solution**:
```cpp
// Ensure consistent calling convention
#if defined(__ARM_EABI__)
    #define THEMIS_ARM_CALL __attribute__((pcs("aapcs")))
#else
    #define THEMIS_ARM_CALL
#endif

// Use in function declarations
typedef void (THEMIS_ARM_CALL *CallbackFunc)(void*);
```

### Error: va_list Issues

**Symptom**:
```
error: cannot convert 'va_list' to 'va_list*'
```

**Cause**: Different va_list implementation on ARM

**Solution**:
```cpp
// Portable variadic function handling
#include <cstdarg>

void myPrintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // Use va_copy for passing to other functions
    va_list args_copy;
    va_copy(args_copy, args);
    vprintf(fmt, args_copy);
    va_end(args_copy);
    
    va_end(args);
}
```

## Endianness Problems

### Error: Data Corruption in Serialization

**Symptom**: Data written on x86 (little-endian) is corrupted when read on ARM (which may be big-endian)

**Solution**:

```cpp
#include <cstdint>
#include <bit>

// Portable byte order functions
inline uint32_t toNetworkOrder(uint32_t value) {
    if constexpr (std::endian::native == std::endian::little) {
        return __builtin_bswap32(value);
    }
    return value;
}

inline uint32_t fromNetworkOrder(uint32_t value) {
    if constexpr (std::endian::native == std::endian::little) {
        return __builtin_bswap32(value);
    }
    return value;
}

// For C++17 and earlier (without std::endian)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define THEMIS_BIG_ENDIAN 1
#else
    #define THEMIS_BIG_ENDIAN 0
#endif

inline uint32_t toNetworkOrderCompat(uint32_t value) {
    #if THEMIS_BIG_ENDIAN
        return value;
    #else
        return __builtin_bswap32(value);
    #endif
}
```

**Serialization Example**:
```cpp
class Serializer {
public:
    void writeUint32(uint32_t value) {
        uint32_t network_value = toNetworkOrder(value);
        buffer_.append(reinterpret_cast<char*>(&network_value), sizeof(network_value));
    }
    
    uint32_t readUint32() {
        uint32_t network_value;
        std::memcpy(&network_value, &buffer_[pos_], sizeof(network_value));
        pos_ += sizeof(network_value);
        return fromNetworkOrder(network_value);
    }
};
```

## SIMD/Intrinsics

### Error: Undefined SSE/AVX Intrinsics

**Symptom**:
```
error: '_mm_add_ps' was not declared in this scope
```

**Cause**: x86 SIMD instructions not available on ARM

**Solution**: Provide ARM NEON equivalents

```cpp
#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>
    #define THEMIS_HAS_SSE 1
    #define THEMIS_HAS_NEON 0
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    #define THEMIS_HAS_SSE 0
    #define THEMIS_HAS_NEON 1
#else
    #define THEMIS_HAS_SSE 0
    #define THEMIS_HAS_NEON 0
#endif

// Vector addition example
inline void vectorAdd(float* result, const float* a, const float* b, size_t count) {
    #if THEMIS_HAS_SSE
        // SSE implementation
        for (size_t i = 0; i < count; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vb = _mm_loadu_ps(&b[i]);
            __m128 vr = _mm_add_ps(va, vb);
            _mm_storeu_ps(&result[i], vr);
        }
    #elif THEMIS_HAS_NEON
        // NEON implementation
        for (size_t i = 0; i < count; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            float32x4_t vr = vaddq_f32(va, vb);
            vst1q_f32(&result[i], vr);
        }
    #else
        // Scalar fallback
        for (size_t i = 0; i < count; ++i) {
            result[i] = a[i] + b[i];
        }
    #endif
}
```

### SSE to NEON Mapping

| SSE Intrinsic | NEON Equivalent | Notes |
|---------------|-----------------|-------|
| `_mm_add_ps` | `vaddq_f32` | 4x float32 addition |
| `_mm_mul_ps` | `vmulq_f32` | 4x float32 multiplication |
| `_mm_load_ps` | `vld1q_f32` | Load 4x float32 |
| `_mm_store_ps` | `vst1q_f32` | Store 4x float32 |
| `_mm_set1_ps` | `vdupq_n_f32` | Broadcast scalar |
| `_mm_shuffle_ps` | `vcombine_f32` + `vget_low/high_f32` | More complex on NEON |

### Header for Portable SIMD

```cpp
// portable_simd.h
#pragma once

#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>
    using vec4f = __m128;
    
    inline vec4f vec_load(const float* p) { return _mm_loadu_ps(p); }
    inline void vec_store(float* p, vec4f v) { _mm_storeu_ps(p, v); }
    inline vec4f vec_add(vec4f a, vec4f b) { return _mm_add_ps(a, b); }
    
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    using vec4f = float32x4_t;
    
    inline vec4f vec_load(const float* p) { return vld1q_f32(p); }
    inline void vec_store(float* p, vec4f v) { vst1q_f32(p, v); }
    inline vec4f vec_add(vec4f a, vec4f b) { return vaddq_f32(a, b); }
    
#else
    #error "No SIMD support available"
#endif
```

## Memory Alignment

### Error: Bus Error / SIGBUS

**Symptom**:
```
Bus error (core dumped)
```

**Cause**: Unaligned memory access on ARM

**Solution**:

```cpp
// Check alignment
static_assert(alignof(MyStruct) >= 8, "Insufficient alignment");

// Force alignment
struct alignas(16) MyStruct {
    float data[4];
};

// Use memcpy for potentially unaligned data
uint32_t readUnaligned(const void* ptr) {
    uint32_t value;
    std::memcpy(&value, ptr, sizeof(value));
    return value;
}

void writeUnaligned(void* ptr, uint32_t value) {
    std::memcpy(ptr, &value, sizeof(value));
}
```

### Compiler Alignment Attributes

```cpp
// GCC/Clang attribute
struct __attribute__((aligned(16))) AlignedStruct {
    // ...
};

// C++11 alignas
struct alignas(16) ModernAlignedStruct {
    // ...
};

// Check alignment at runtime
void* aligned_alloc_wrapper(size_t size, size_t alignment) {
    #if defined(__ANDROID__) || defined(__ARM_ARCH_7A__)
        // posix_memalign for older ARM platforms
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
    #else
        return std::aligned_alloc(alignment, size);
    #endif
}
```

## Performance Considerations

### Cache Line Size

```cpp
// x86 typically has 64-byte cache lines
// ARM can have 32, 64, or 128-byte cache lines

constexpr size_t getCacheLineSize() {
    #if defined(__ARM_ARCH)
        return 64;  // Common on ARMv8
    #else
        return 64;  // x86-64
    #endif
}

// Align hot data structures to cache line
struct alignas(getCacheLineSize()) HotData {
    std::atomic<uint64_t> counter;
    // ... other frequently accessed data
};
```

### Atomic Operations

```cpp
#include <atomic>

// ARM has different memory ordering semantics
// Be explicit about memory order
std::atomic<int> counter{0};

// Release-acquire for synchronization
void producer() {
    data = 42;
    counter.store(1, std::memory_order_release);
}

void consumer() {
    while (counter.load(std::memory_order_acquire) == 0) {
        // Wait
    }
    use(data);  // Safe to read
}
```

## Testing on ARM

### Using QEMU for Testing

```bash
# Install QEMU
sudo apt-get install qemu-user-static

# Run ARM binary on x86
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./my_arm_binary

# With debugging
qemu-aarch64-static -g 1234 -L /usr/aarch64-linux-gnu ./my_arm_binary
# In another terminal:
gdb-multiarch ./my_arm_binary
(gdb) target remote :1234
```

### Docker for Cross-Compilation

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    cmake \
    qemu-user-static

WORKDIR /build
COPY . .

RUN cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm64-toolchain.cmake
RUN cmake --build build
```

## CMake Configuration

```cmake
# Detect ARM architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(THEMIS_ARCH_ARM64 ON)
    add_compile_definitions(THEMIS_ARCH_ARM64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(THEMIS_ARCH_ARMV7 ON)
    add_compile_definitions(THEMIS_ARCH_ARMV7=1)
endif()

# Enable NEON if available
if(THEMIS_ARCH_ARM64 OR THEMIS_ARCH_ARMV7)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-mfpu=neon" HAS_NEON_FLAG)
    
    if(HAS_NEON_FLAG)
        add_compile_options(-mfpu=neon)
    endif()
    
    # ARM-specific optimizations
    add_compile_options(
        -march=native  # Or specific like -march=armv8-a
        -mtune=native
    )
endif()
```

## References

- [ARM Architecture Reference Manual](https://developer.arm.com/documentation/)
- [ARM NEON Intrinsics](https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/intrinsics)
- [Cross-Compilation Guide](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)

## Getting Help

If you encounter an ARM-specific error:

1. Run the diagnostic scanner:
   ```bash
   python tools/compiler_diagnostics/diagnostic_scanner.py build.log --platform arm
   ```

2. Check the [Platform Compatibility Matrix](../PLATFORM_COMPATIBILITY_MATRIX.md)

3. See [COMPILER_TROUBLESHOOTING.md](COMPILER_TROUBLESHOOTING.md)
