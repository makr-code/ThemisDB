/**
 * @file aligned_vector_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Aligned Vector Allocator
// Cache-line aligned allocator for high-dimensional embedding vectors
//
// CACHE OPTIMIZATION:
// - 32-byte alignment for AVX2/AVX-512 SIMD operations
// - Reduces unaligned load penalties in distance calculations
// - Optimized for 1536D vectors (OpenAI ada-002 embeddings)
//
// Usage:
//   std::vector<float, AlignedVectorAllocator<float, 32>> embedding;
//   // or use the convenience alias:
//   AlignedVector<float> embedding;

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>
#include <type_traits>
#include "performance/allocator.h"

namespace themis {
namespace cache {

/**
 * @brief STL-compatible allocator with custom alignment
 * 
 * Allocates memory aligned to the specified boundary, which is critical
 * for SIMD operations and cache efficiency.
 * 
 * @tparam T Type of elements to allocate
 * @tparam Alignment Alignment requirement in bytes (must be power of 2)
 *                   Typical values: 16 (SSE), 32 (AVX2), 64 (cache line)
 */
template<typename T, std::size_t Alignment = 32>
class AlignedVectorAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;
    
    static_assert((Alignment & (Alignment - 1)) == 0, 
                  "Alignment must be a power of 2");
    static_assert(Alignment >= alignof(T), 
                  "Alignment must be at least alignof(T)");
    
    constexpr AlignedVectorAllocator() noexcept = default;
    constexpr AlignedVectorAllocator(const AlignedVectorAllocator&) noexcept = default;
    
    template<typename U>
    constexpr AlignedVectorAllocator(const AlignedVectorAllocator<U, Alignment>&) noexcept {}
    
    /**
     * @brief Allocate aligned memory for n elements
     * 
     * @param n Number of elements to allocate
     * @return Pointer to aligned memory block
     * @throws std::bad_alloc if allocation fails
     */
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }
        
        std::size_t bytes = n * sizeof(T);
        void* ptr = memory::allocate_aligned(bytes, Alignment);
        
        if (!ptr) {
            throw std::bad_alloc();
        }
        
        return static_cast<T*>(ptr);
    }
    
    /**
     * @brief Deallocate aligned memory
     * 
     * @param ptr Pointer to memory to deallocate
     * @param n Number of elements (unused but required by STL)
     */
    void deallocate(T* ptr, [[maybe_unused]] std::size_t n) noexcept {
        // Unused parameter
        memory::deallocate_aligned(ptr, Alignment);
    }
    
    /**
     * @brief Get maximum number of elements that can be allocated
     */
    std::size_t max_size() const noexcept {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
    
    /**
     * @brief Equality comparison (allocators are stateless)
     */
    template<typename U>
    bool operator==(const AlignedVectorAllocator<U, Alignment>&) const noexcept {
        return true;
    }
    
    /**
     * @brief Inequality comparison
     */
    template<typename U>
    bool operator!=(const AlignedVectorAllocator<U, Alignment>&) const noexcept {
        return false;
    }
    
    /**
     * @brief Rebind allocator to different type (required by STL)
     */
    template<typename U>
    struct rebind {
        using other = AlignedVectorAllocator<U, Alignment>;
    };
};

// ============================================================================
// Convenience type aliases
// ============================================================================

/**
 * @brief 32-byte aligned vector for AVX2/AVX-512 SIMD operations
 * 
 * Optimal for most modern x86-64 CPUs with AVX2 support.
 * Use for embedding vectors to enable efficient SIMD distance calculations.
 */
template<typename T>
using AlignedVector = std::vector<T, AlignedVectorAllocator<T, 32>>;

/**
 * @brief 64-byte aligned vector for cache-line optimization
 * 
 * Use when false-sharing prevention is critical or when working with
 * data structures that benefit from cache-line alignment.
 */
template<typename T>
using CacheLineVector = std::vector<T, AlignedVectorAllocator<T, 64>>;

/**
 * @brief 16-byte aligned vector for SSE/NEON operations
 * 
 * Use for older CPUs without AVX2 support or ARM NEON operations.
 */
template<typename T>
using SimdVector = std::vector<T, AlignedVectorAllocator<T, 16>>;

} // namespace cache
} // namespace themis
