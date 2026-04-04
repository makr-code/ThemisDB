# Alignment Verification Utilities

## Overview

ThemisDB now includes comprehensive alignment verification utilities to ensure correct and efficient memory access patterns across all platforms, with special emphasis on ARM architecture where unaligned access can cause crashes.

## Quick Start

```cpp
#include "performance/alignment_helpers.h"
#include "utils/unaligned_access.h"

// 1. Verify structure alignment at compile time
struct alignas(64) CacheLineData {
    uint64_t value;
    char padding[56];
};
THEMIS_STATIC_ASSERT_ALIGNED(CacheLineData, 64);
THEMIS_STATIC_ASSERT_SIZE(CacheLineData, 64);

// 2. Check pointer alignment at runtime
alignas(64) char buffer[128];
if (is_aligned<64>(buffer)) {
    // Safe for cache-optimized access
}

// 3. Safe unaligned memory access
uint32_t value = read_unaligned<uint32_t>(unaligned_ptr);
write_unaligned<uint32_t>(unaligned_ptr, value);
```

## Features

### Compile-Time Verification

**Static Assertions:**
- `THEMIS_STATIC_ASSERT_ALIGNED(Type, Alignment)` - Verify exact alignment
- `THEMIS_STATIC_ASSERT_MIN_ALIGNED(Type, MinAlignment)` - Verify minimum alignment
- `THEMIS_STATIC_ASSERT_SIZE(Type, Size)` - Verify structure size (catches padding issues)

**Template Functions:**
- `check_alignment<T, Alignment>()` - Returns true if T has exact alignment
- `check_min_alignment<T, MinAlignment>()` - Returns true if T meets minimum

### Runtime Verification

**Pointer Alignment:**
- `is_aligned<Alignment>(ptr)` - Check if pointer is aligned
- `align_up<Alignment>(ptr)` - Align pointer up to next boundary
- `align_down<Alignment>(ptr)` - Align pointer down to nearest boundary
- `padding_for_alignment<Alignment>(offset)` - Calculate padding needed

### Safe Unaligned Access

**For Network/File I/O:**
- `read_unaligned<T>(ptr)` - Safe read from unaligned address
- `write_unaligned<T>(ptr, value)` - Safe write to unaligned address
- `checked_aligned_cast<T>(ptr)` - Cast with alignment verification

## Platform-Specific Behavior

### ARM/AARCH64
- Unaligned access can cause **SIGBUS** crashes
- Compiler flag `-Werror=cast-align` catches alignment issues at compile time
- `THEMIS_STRICT_ALIGNMENT` definition enables platform-specific checks

### x86/x86_64
- Tolerates unaligned access but with performance penalty
- 2-10x slower for unaligned SIMD operations
- Still benefits from alignment for cache efficiency

### Build Configuration

**ARM/Android Automatic Flags:**
```cmake
-Werror=cast-align          # Catch alignment casts
-DTHEMIS_STRICT_ALIGNMENT=1 # Enable strict checks
```

**Optional Sanitizer:**
```bash
cmake -DTHEMIS_ENABLE_UBSAN=ON  # Detect alignment violations at runtime
```

## Common Use Cases

### 1. Cache-Line Optimization

Prevent false sharing between threads:
```cpp
struct alignas(64) PerThreadCounter {
    std::atomic<uint64_t> counter;
    char padding[56];
};
THEMIS_STATIC_ASSERT_ALIGNED(PerThreadCounter, 64);
```

### 2. SIMD Vector Operations

Ensure proper alignment for vector instructions:
```cpp
struct alignas(16) Vec4f {
    float x, y, z, w;
};
THEMIS_STATIC_ASSERT_ALIGNED(Vec4f, 16);
THEMIS_STATIC_ASSERT_SIZE(Vec4f, 16);
```

### 3. Network Protocol Parsing

Safe parsing of unaligned packet headers:
```cpp
const uint8_t* packet = receive_packet();
uint32_t length = read_unaligned<uint32_t>(packet + 4);
uint64_t timestamp = read_unaligned<uint64_t>(packet + 8);
```

### 4. File Format Handling

Verify binary file structure layout:
```cpp
struct FileHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t size;
    char reserved[48];
};
THEMIS_STATIC_ASSERT_SIZE(FileHeader, 64);  // Ensure no unexpected padding
```

## Examples

See `include/performance/alignment_examples.h` for comprehensive examples including:
- Cache-line aligned counters
- SIMD vector types (SSE, AVX, AVX-512, NEON)
- Safe unaligned access patterns
- Runtime alignment verification
- Checked casting with alignment

## Testing

Run the alignment test suite:
```bash
# Build tests
cmake -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build

# Run alignment tests
./build/themis_test --gtest_filter="AlignmentHelpersTest.*:UnalignedAccessTest.*"
```

## Performance Impact

- **Compile-time checks**: Zero runtime overhead
- **Runtime checks**: Minimal overhead (usually optimized away)
- **Unaligned access helpers**: Compiler optimizes memcpy to efficient code
- **Overall**: No performance penalty, only safety improvements

## Best Practices

1. **Always verify alignment** for SIMD structures with static assertions
2. **Use cache-line alignment** (64 bytes) for heavily-contested atomic variables
3. **Use unaligned access helpers** for network/file I/O instead of direct casts
4. **Test on ARM** if your code uses alignment-sensitive operations
5. **Enable UBSAN** during development to catch alignment issues early

## References

- Header: `include/performance/alignment_helpers.h`
- Safe Access: `include/utils/unaligned_access.h`
- Examples: `include/performance/alignment_examples.h`
- Tests: `tests/test_alignment_helpers.cpp`
- Build Config: `cmake/CompilerOptions.cmake`

## Troubleshooting

**Compilation Error: "must be aligned to X bytes"**
- Add `alignas(X)` to your structure definition
- Verify with `THEMIS_STATIC_ASSERT_ALIGNED(YourStruct, X)`

**Runtime SIGBUS on ARM:**
- Use `read_unaligned<T>()` instead of pointer dereferencing
- Check pointer alignment with `is_aligned<T>()` before casting
- Use `checked_aligned_cast<T>()` for safe casting

**False Sharing Performance Issues:**
- Use 64-byte alignment for per-thread data structures
- Add explicit padding to reach cache-line size
- Verify with `THEMIS_STATIC_ASSERT_SIZE(YourStruct, 64)`

## Support

For questions or issues, refer to:
- Architecture documentation: `docs/architecture/`
- Performance guide: `docs/performance/`
- Build guide: `docs/guides/guides_build_strategy.md`
