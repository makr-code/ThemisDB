# ThemisDB - Cross-Compilation Context

## Platform Support Overview

ThemisDB supports multiple platforms and architectures:

| Platform | Architecture | Status | Notes |
|----------|-------------|--------|-------|
| **Windows** | x86_64 | ✅ Full | Primary development platform |
| **Linux** | x86_64 | ✅ Full | Production platform |
| **Linux** | ARM64 | ✅ Full | Raspberry Pi 4+, ARM servers |
| **macOS** | x86_64, ARM64 | ⚠️ Community | Not officially tested |
| **QNAP NAS** | x86_64 | ✅ Full | Custom optimizations |

## Platform-Specific Rules

### Windows (MSVC)

**Compiler Flags:**
```cmake
if(MSVC)
    target_compile_options(themis_core PRIVATE
        /W4           # Warning level 4
        /WX           # Warnings as errors
        /O2           # Optimize for speed
        /bigobj       # Large object files
        /std:c++17    # C++17 standard
    )
endif()
```

**Preprocessor Definitions:**
```cpp
#ifdef _WIN32
    #define THEMIS_EXPORT __declspec(dllexport)
    #define THEMIS_IMPORT __declspec(dllimport)
#else
    #define THEMIS_EXPORT __attribute__((visibility("default")))
    #define THEMIS_IMPORT
#endif
```

**Path Handling:**
```cpp
// Use std::filesystem for cross-platform paths
#include <filesystem>
namespace fs = std::filesystem;

// ✅ Good: Cross-platform
fs::path db_path = fs::path(root) / "data" / "db";

// ❌ Bad: Platform-specific
std::string db_path = root + "\\data\\db";  // Windows only
```

### Linux (GCC/Clang)

**Compiler Flags:**
```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(themis_core PRIVATE
        -Wall           # All warnings
        -Wextra         # Extra warnings
        -Wpedantic      # Pedantic warnings
        -O3             # Optimize for speed
        -march=native   # Use native CPU features
    )
endif()
```

**Thread Library:**
```cmake
# Required on Linux
find_package(Threads REQUIRED)
target_link_libraries(themis_core PRIVATE Threads::Threads)
```

**Dynamic Linking:**
```cmake
# Linux may need explicit linking
target_link_libraries(themis_core PRIVATE
    ${CMAKE_DL_LIBS}  # Dynamic linking library
)
```

### ARM64 (Raspberry Pi, ARM Servers)

**Architecture Detection:**
```cmake
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(THEMIS_ARCH_ARM64 ON)
    message(STATUS "Building for ARM64")
endif()
```

**SIMD Optimizations:**
```cpp
#ifdef __ARM_NEON
    #include <arm_neon.h>
    // Use NEON intrinsics for vector operations
    float32x4_t vec = vld1q_f32(data);
#else
    // Fallback to scalar operations
#endif
```

**Memory Constraints:**
```cmake
# Reduce parallelism on ARM
if(THEMIS_ARCH_ARM64)
    set(THEMIS_BUILD_PARALLEL 2 CACHE STRING "Build parallelism")
else()
    set(THEMIS_BUILD_PARALLEL 8 CACHE STRING "Build parallelism")
endif()
```

**Raspberry Pi Specific:**
- Target: ARMv8-A (Cortex-A72 on RPi 4)
- RAM: 4GB/8GB models recommended
- Disable LLM features on 4GB models
- Use `-O2` instead of `-O3` (reduces memory usage)

### QNAP NAS

**GLIBC Compatibility:**
```cmake
# QNAP uses Ubuntu 20.04 base (GLIBC 2.31)
if(THEMIS_QNAP_BUILD)
    # Ensure compatibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
endif()
```

**CPU Features:**
```cmake
# QNAP: Use SSE4.2 baseline (no AVX/AVX2)
if(THEMIS_QNAP_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mno-avx")
endif()
```

**Static Linking:**
```cmake
# Static build for portability
if(THEMIS_QNAP_BUILD)
    set(BUILD_SHARED_LIBS OFF)
    set(THEMIS_STATIC_BUILD ON)
endif()
```

## vcpkg Cross-Compilation

### Triplet Selection

```bash
# Windows
vcpkg install --triplet x64-windows

# Linux x86_64
vcpkg install --triplet x64-linux

# Linux ARM64
vcpkg install --triplet arm64-linux

# Static builds
vcpkg install --triplet x64-linux-static
```

### Custom Triplets

For QNAP and special builds, use custom triplets:

```cmake
# vcpkg/triplets/x64-linux-qnap.cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)
```

## Docker Multi-Architecture

### BuildKit Multi-Platform

```bash
# Build for multiple platforms
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis-server:latest \
  -f docker/Dockerfile.themis-server \
  .
```

### Architecture-Specific Stages

```dockerfile
# Dockerfile.themis-server
ARG TARGETPLATFORM
ARG BUILDPLATFORM

FROM --platform=$BUILDPLATFORM ubuntu:22.04 AS builder

# Architecture detection
RUN if [ "$TARGETPLATFORM" = "linux/arm64" ]; then \
      export ARCH=arm64; \
    else \
      export ARCH=x64; \
    fi
```

## Endianness

ThemisDB assumes **little-endian** architectures (x86_64, ARM64).

For big-endian support (rare), add:

```cpp
#include <bit>

if constexpr (std::endian::native == std::endian::big) {
    // Handle big-endian byte swapping
    value = __builtin_bswap32(value);
}
```

## File Systems

### Case Sensitivity

```cpp
// ✅ Good: Assume case-sensitive
fs::path config = "config.json";  // All lowercase

// ⚠️ Warning: Case-insensitive on Windows
// "Config.json" == "config.json" on Windows
// "Config.json" != "config.json" on Linux
```

### Line Endings

Configure git to handle line endings:

```gitattributes
# .gitattributes
* text=auto
*.cpp text eol=lf
*.h text eol=lf
*.sh text eol=lf
*.bat text eol=crlf
```

## Testing Cross-Platform Code

### Platform-Specific Tests

```cpp
#ifdef _WIN32
TEST(WindowsSpecificTest, FilePermissions) {
    // Windows-specific behavior
}
#endif

#ifdef __linux__
TEST(LinuxSpecificTest, SignalHandling) {
    // Linux-specific behavior
}
#endif
```

### Portable Test Data

```cpp
// ✅ Good: Use std::filesystem
fs::path test_file = fs::temp_directory_path() / "test.db";

// ❌ Bad: Hardcoded paths
const char* test_file = "/tmp/test.db";  // Fails on Windows
```

## Common Pitfalls

### 1. Assuming x86_64

```cpp
// ❌ Bad: Assumes x86
#include <x86intrin.h>  // Not available on ARM

// ✅ Good: Check architecture
#ifdef __x86_64__
    #include <x86intrin.h>
#elif defined(__aarch64__)
    #include <arm_neon.h>
#endif
```

### 2. Hardcoded Sizes

```cpp
// ❌ Bad: Assumes 64-bit
static_assert(sizeof(void*) == 8);

// ✅ Good: Portable
static_assert(sizeof(void*) == sizeof(std::size_t));
```

### 3. Compiler Extensions

```cpp
// ⚠️ Warning: GCC/Clang specific
__attribute__((packed)) struct Data { ... };

// ✅ Better: Use standard or conditionals
#ifdef __GNUC__
    __attribute__((packed))
#elif defined(_MSC_VER)
    #pragma pack(push, 1)
#endif
struct Data { ... };
```

## Build Validation

### Multi-Platform CI

GitHub Actions workflow:

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, windows-latest]
    arch: [x64, arm64]
```

### Cross-Compilation Testing

```bash
# On x86_64, test ARM64 build with QEMU
docker run --rm --platform linux/arm64 \
  themis-server:latest \
  ./themis_server --version
```

## Documentation

Platform-specific build instructions:
- [BUILD_WINDOWS.md](../../docs/build-guide/BUILD_WINDOWS.md)
- [BUILD_LINUX.md](../../docs/build-guide/BUILD_LINUX.md)
- [BUILD_ARM.md](../../docs/build-guide/BUILD_ARM.md)
- [BUILD_RASPBERRY_PI.md](../../docs/build-guide/BUILD_RASPBERRY_PI.md)
- [BUILD_QNAP.md](../../docs/build-guide/BUILD_QNAP.md)

## Resources

- CMake Cross-Compilation: https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html
- vcpkg Triplets: https://learn.microsoft.com/en-us/vcpkg/users/triplets
- Docker BuildKit: https://docs.docker.com/build/buildkit/
