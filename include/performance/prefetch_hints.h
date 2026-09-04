/**
 * @file prefetch_hints.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Prefetch Hints for Random Access Performance Optimization
// Provides cross-platform CPU prefetch hints for improved data locality
//
// Critical for:
// - Random access patterns (multiGet, point lookups)
// - Iterator scanning (prefix scan, range scan)
// - Batch operations (pre-loading next batch items)
//
// Performance Impact:
// - Can reduce random access latency by 20-40%
// - Most effective for large datasets (> L3 cache size)
// - Minimal overhead if data already in cache

#pragma once

#include <cstddef>
#include <cstdint>

// Platform-specific includes for prefetch instructions
// Note: clang-cl defines _MSC_VER but should use __builtin_prefetch
#if defined(_MSC_VER) && !defined(__clang__)
    #include <xmmintrin.h>  // _mm_prefetch on MSVC
#endif

namespace themis {
namespace performance {

/**
 * @brief Prefetch cache locality hints
 * 
 * Controls which cache level to prefetch data into:
 * - T0: Temporal data with high locality (L1 cache) - most aggressive
 * - T1: Temporal data with moderate locality (L2 cache)
 * - T2: Temporal data with low locality (L3 cache)
 * - NTA: Non-temporal data (bypass cache) - for streaming workloads
 */
enum class PrefetchHint {
    /// High temporal locality - prefetch to L1 cache
    /// Use for: hot data, imminent access within ~100 cycles
    T0 = 3,
    
    /// Moderate temporal locality - prefetch to L2 cache
    /// Use for: data accessed soon, within ~1000 cycles
    T1 = 2,
    
    /// Low temporal locality - prefetch to L3 cache
    /// Use for: data that may be reused, accessed within ~10000 cycles
    T2 = 1,
    
    /// Non-temporal - bypass cache hierarchy
    /// Use for: streaming data accessed once, no reuse expected
    NTA = 0
};

/**
 * @brief Prefetch memory location into CPU cache
 * 
 * Issues a prefetch instruction to bring data from main memory into cache
 * before it's actually accessed. This can hide memory latency for predictable
 * access patterns.
 * 
 * @param ptr Pointer to memory location to prefetch
 * @param hint Cache locality hint (default: T0 for high locality)
 * 
 * @note This is a hint to the CPU and may be ignored
 * @note No-op on platforms without prefetch support
 * @note Safe to call with null pointer (becomes no-op)
 * 
 * @code
 * // Prefetch next key before processing current key
 * for (size_t i = 0; i < keys.size(); ++i) {
 *     if (i + 1 < keys.size()) {
 *         prefetch(&keys[i + 1], PrefetchHint::T0);
 *     }
 *     process(keys[i]);
 * }
 * @endcode
 */
inline void prefetch(const void* ptr, PrefetchHint hint = PrefetchHint::T0) noexcept {
    if (!ptr) return;  // Safe no-op for null pointers
    
    #if defined(_MSC_VER) && !defined(__clang__)
        // MSVC: Use _mm_prefetch with locality hints
        // _MM_HINT_T0 = fetch to L1, _MM_HINT_T1 = L2, _MM_HINT_T2 = L3, _MM_HINT_NTA = non-temporal
        switch (hint) {
            case PrefetchHint::T0:
                _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0);
                break;
            case PrefetchHint::T1:
                _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T1);
                break;
            case PrefetchHint::T2:
                _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T2);
                break;
            case PrefetchHint::NTA:
                _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_NTA);
                break;
        }
    #elif defined(__GNUC__) || defined(__clang__)
        // GCC/Clang: Use __builtin_prefetch(addr, rw, locality)
        // rw: 0 = read, 1 = write
        // locality: 0 (NTA) to 3 (T0)
        switch (hint) {
            case PrefetchHint::T0:
                __builtin_prefetch(ptr, 0, 3);
                break;
            case PrefetchHint::T1:
                __builtin_prefetch(ptr, 0, 2);
                break;
            case PrefetchHint::T2:
                __builtin_prefetch(ptr, 0, 1);
                break;
            case PrefetchHint::NTA:
                __builtin_prefetch(ptr, 0, 0);
                break;
        }
    #else
        // No prefetch support on this platform
    #endif
}

/**
 * @brief Prefetch for write access
 * 
 * Prefetches memory location with intent to write. Some CPUs can optimize
 * cache line ownership transfer when write intent is known.
 * 
 * @param ptr Pointer to memory location to prefetch
 * @param hint Cache locality hint (default: T0 for high locality)
 */
inline void prefetch_write(void* ptr, PrefetchHint hint = PrefetchHint::T0) noexcept {
    if (!ptr) {
      return;
    }
    
    #if defined(_MSC_VER) && !defined(__clang__)
        // MSVC doesn't distinguish read/write in _mm_prefetch
        prefetch(ptr, hint);
    #elif defined(__GNUC__) || defined(__clang__)
        // GCC/Clang: rw = 1 for write
        switch (hint) {
            case PrefetchHint::T0:
                __builtin_prefetch(ptr, 1, 3);
                break;
            case PrefetchHint::T1:
                __builtin_prefetch(ptr, 1, 2);
                break;
            case PrefetchHint::T2:
                __builtin_prefetch(ptr, 1, 1);
                break;
            case PrefetchHint::NTA:
                __builtin_prefetch(ptr, 1, 0);
                break;
        }
    #else
    #endif
}

/**
 * @brief Prefetch multiple cache lines starting from pointer
 * 
 * Useful for prefetching large data structures that span multiple cache lines
 * (typically 64 bytes per line on modern CPUs).
 * 
 * @param ptr Starting address
 * @param size Size in bytes to prefetch
 * @param hint Cache locality hint
 * 
 * @code
 * // Prefetch entire key-value pair structure
 * struct KVPair { char key[128]; char value[256]; };
 * prefetch_range(&kv, sizeof(KVPair));
 * @endcode
 */
inline void prefetch_range(const void* ptr, size_t size, PrefetchHint hint = PrefetchHint::T0) noexcept {
    if (!ptr || size == 0) {
      return;
    }
    
    // Typical cache line size is 64 bytes
    constexpr size_t CACHE_LINE_SIZE = 64;
    
    const uint8_t* addr = static_cast<const uint8_t*>(ptr);
    const uint8_t* end = addr + size;
    
    // Prefetch each cache line in the range
    for (; addr < end; addr += CACHE_LINE_SIZE) {
        prefetch(addr, hint);
    }
}

/**
 * @brief Configuration for adaptive prefetch distance
 * 
 * Allows runtime tuning of prefetch distance based on memory latency
 * and access patterns.
 */
struct PrefetchConfig {
    /// Number of items to prefetch ahead in sequential access
    /// Default: 2 (good for most workloads)
    /// Range: 1-8 (higher = more aggressive, more memory bandwidth)
    size_t prefetch_distance = 2;
    
    /// Cache locality hint for prefetched data
    PrefetchHint hint = PrefetchHint::T0;
    
    /// Enable/disable prefetching at runtime
    bool enabled = true;
    
    /// Minimum batch size to enable prefetching
    /// Prefetching overhead isn't worth it for tiny batches
    size_t min_batch_size = 4;
};

/**
 * @brief Helper for batch prefetching with configurable distance
 * 
 * @tparam T Type of items being prefetched
 * @param items Array of items
 * @param current_index Current item being processed
 * @param total_count Total number of items
 * @param config Prefetch configuration
 * 
 * @code
 * PrefetchConfig config{.prefetch_distance = 3};
 * for (size_t i = 0; i < keys.size(); ++i) {
 *     batch_prefetch(keys.data(), i, keys.size(), config);
 *     process(keys[i]);
 * }
 * @endcode
 */
template<typename T>
inline void batch_prefetch(const T* items, size_t current_index, 
                          size_t total_count, const PrefetchConfig& config) noexcept {
    if (!config.enabled || !items || total_count < config.min_batch_size) {
        return;
    }
    
    // Prefetch items at distance ahead
    for (size_t d = 1; d <= config.prefetch_distance; ++d) {
        size_t prefetch_index = current_index + d;
        if (prefetch_index < total_count) {
            prefetch(&items[prefetch_index], config.hint);
        }
    }
}

} // namespace performance
} // namespace themis
