/**
 * @file alignment_examples.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Alignment Best Practices Examples
// Demonstrates proper alignment verification for production code
//
// This file provides examples that developers can reference when writing
// performance-critical code with alignment requirements.

#pragma once

#include "performance/alignment_helpers.h"
#include "utils/unaligned_access.h"
#include <atomic>
#include <cstdint>
#include <array>

namespace themis {
namespace examples {

// ============================================================================
// Example 1: Cache-Line Aligned Counter (Prevents False Sharing)
// ============================================================================

/**
 * @brief Cache-line aligned atomic counter
 * 
 * Prevents false sharing in multi-threaded environments where multiple
 * threads update separate counters. Each counter occupies its own cache line.
 * 
 * False sharing occurs when:
 * - Two threads update variables in the same cache line
 * - CPU must invalidate/synchronize the entire cache line
 * - Performance degrades due to cache coherency traffic
 * 
 * Solution: Align each counter to cache line boundary (64 bytes)
 */
struct alignas(64) CacheLineAlignedCounter {
    std::atomic<uint64_t> counter;
    char padding[56];  // 64 - 8 = 56 bytes padding
};

// Compile-time verification
THEMIS_STATIC_ASSERT_ALIGNED(CacheLineAlignedCounter, 64);
THEMIS_STATIC_ASSERT_SIZE(CacheLineAlignedCounter, 64);

// ============================================================================
// Example 2: SIMD Vector Types (16-byte alignment for SSE/NEON)
// ============================================================================

/**
 * @brief 4-component float vector for SIMD operations
 * 
 * Required for:
 * - SSE/SSE2/SSE4 instructions (x86)
 * - NEON instructions (ARM)
 * - Aligned load/store instructions (_mm_load_ps, vld1q_f32)
 * 
 * Unaligned access:
 * - Can cause crashes on some ARM processors
 * - Forces slower unaligned load instructions
 * - Degrades performance by 2-10x
 */
struct alignas(16) Vec4f {
    float x, y, z, w;
    
    Vec4f() : x(0), y(0), z(0), w(0) {}
    Vec4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

THEMIS_STATIC_ASSERT_ALIGNED(Vec4f, 16);
THEMIS_STATIC_ASSERT_SIZE(Vec4f, 16);

/**
 * @brief 2-component double vector for SIMD operations
 * 
 * Used in scientific computing and high-precision calculations
 */
struct alignas(16) Vec2d {
    double x, y;
    
    Vec2d() : x(0), y(0) {}
    Vec2d(double x_, double y_) : x(x_), y(y_) {}
};

THEMIS_STATIC_ASSERT_ALIGNED(Vec2d, 16);
THEMIS_STATIC_ASSERT_SIZE(Vec2d, 16);

// ============================================================================
// Example 3: AVX-256 Vector Types (32-byte alignment)
// ============================================================================

/**
 * @brief 8-component float vector for AVX2 operations
 * 
 * Required for AVX/AVX2 instructions on x86_64:
 * - _mm256_load_ps requires 32-byte alignment
 * - Unaligned access uses slower _mm256_loadu_ps
 * - Performance difference can be 20-30%
 */
struct alignas(32) Vec8f {
    float data[8];
    
    Vec8f() : data{} {}  // Zero-initialize using aggregate initialization
};

THEMIS_STATIC_ASSERT_ALIGNED(Vec8f, 32);
THEMIS_STATIC_ASSERT_SIZE(Vec8f, 32);

// ============================================================================
// Example 4: AVX-512 Vector Types (64-byte alignment)
// ============================================================================

/**
 * @brief 16-component float vector for AVX-512 operations
 * 
 * Required for AVX-512 instructions on modern x86_64 CPUs:
 * - _mm512_load_ps requires 64-byte alignment
 * - Critical for HPC and AI workloads
 */
struct alignas(64) Vec16f {
    float data[16];
    
    Vec16f() : data{} {}  // Zero-initialize using aggregate initialization
};

THEMIS_STATIC_ASSERT_ALIGNED(Vec16f, 64);
THEMIS_STATIC_ASSERT_SIZE(Vec16f, 64);

// ============================================================================
// Example 5: Batch Processing with Proper Alignment
// ============================================================================

/**
 * @brief Batch metadata structure with cache-line alignment
 * 
 * Used in distributed processing where batch metadata is frequently
 * accessed by multiple threads. Cache-line alignment prevents false sharing.
 */
struct alignas(64) BatchMetadata {
    uint64_t batch_id;           // 8 bytes
    uint32_t num_items;          // 4 bytes
    uint32_t processed_items;    // 4 bytes
    std::atomic<uint64_t> timestamp;  // 8 bytes
    char reserved[40];           // Padding to 64 bytes
};

THEMIS_STATIC_ASSERT_ALIGNED(BatchMetadata, 64);
THEMIS_STATIC_ASSERT_SIZE(BatchMetadata, 64);

// ============================================================================
// Example 6: Safe Unaligned Access (Network Packets, File I/O)
// ============================================================================

/**
 * @brief Parse network packet header safely
 * 
 * Network packets often arrive with arbitrary alignment.
 * Direct pointer casting can cause SIGBUS on ARM.
 * 
 * @param buffer Raw packet buffer (potentially unaligned)
 * @return Safely parsed header
 */
struct PacketHeader {
    uint32_t magic;
    uint32_t length;
    uint64_t timestamp;
};

inline PacketHeader parse_packet_header(const uint8_t* buffer) {
    using namespace themis::utils;
    
    PacketHeader header;
    // Safe: Works on all platforms including ARM
    header.magic = read_unaligned<uint32_t>(buffer);
    header.length = read_unaligned<uint32_t>(buffer + 4);
    header.timestamp = read_unaligned<uint64_t>(buffer + 8);
    
    return header;
}

// ============================================================================
// Example 7: Runtime Alignment Checking
// ============================================================================

/**
 * @brief Verify pointer alignment before SIMD operations
 * 
 * @param data Pointer to float array
 * @param size Number of elements
 * @return true if safe for aligned SIMD operations
 */
inline bool is_simd_safe(const float* data, size_t size) {
    using namespace themis::performance;
    
    // Check if pointer is 16-byte aligned for SSE/NEON
    if (!is_aligned<16>(data)) {
        return false;
    }
    
    // Check if size allows efficient SIMD processing
    // (at least 4 elements for Vec4f)
    if (size < 4) {
        return false;
    }
    
    return true;
}

/**
 * @brief Align pointer for SIMD operations
 * 
 * @param buffer Input buffer
 * @param size Buffer size
 * @return Aligned pointer within buffer (or nullptr if not enough space)
 */
inline float* align_for_simd(float* buffer, size_t size) {
    using namespace themis::performance;
    
    float* aligned = static_cast<float*>(align_up<16>(buffer));
    
    // Check if we have enough space after alignment
    size_t offset = reinterpret_cast<uint8_t*>(aligned) - 
                    reinterpret_cast<uint8_t*>(buffer);
    
    if (offset >= size * sizeof(float)) {
        return nullptr;  // Not enough space
    }
    
    return aligned;
}

// ============================================================================
// Example 8: Checked Cast for Type Safety
// ============================================================================

/**
 * @brief Safely cast void* to Vec4f* with alignment verification
 * 
 * @param ptr Generic pointer
 * @return Vec4f* if properly aligned, nullptr otherwise
 */
inline Vec4f* safe_cast_to_vec4f(void* ptr) {
    return themis::utils::checked_aligned_cast<Vec4f>(ptr);
}

/**
 * @brief Process vector data with alignment checking
 * 
 * @param data Generic pointer to vector data
 * @param count Number of vectors
 * @return true if processing succeeded
 */
inline bool process_vectors(void* data, size_t count) {
    Vec4f* vectors = safe_cast_to_vec4f(data);
    
    if (!vectors) {
        // Data not properly aligned - use fallback processing
        // or reallocate with proper alignment
        return false;
    }
    
    // Safe to use SIMD instructions now
    for (size_t i = 0; i < count; ++i) {
        // Process vectors[i] with SIMD...
    }
    
    return true;
}

// ============================================================================
// Example 9: Compile-Time Size Verification
// ============================================================================

/**
 * @brief Fixed-size structure with explicit padding
 * 
 * Useful for:
 * - Binary file formats
 * - Network protocols
 * - Memory-mapped I/O
 * 
 * Static assertions catch layout changes during development
 */
struct FileHeader {
    uint32_t magic;        // 4 bytes
    uint32_t version;      // 4 bytes
    uint64_t file_size;    // 8 bytes
    uint64_t timestamp;    // 8 bytes
    char reserved[40];     // 40 bytes padding
    // Total: 64 bytes
};

THEMIS_STATIC_ASSERT_SIZE(FileHeader, 64);

} // namespace examples
} // namespace themis
