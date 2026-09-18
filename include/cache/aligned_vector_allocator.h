/**
 * @file aligned_vector_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    
    /**
     * @brief Aligned Vector Allocator.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    constexpr AlignedVectorAllocator() noexcept = default;
    /**
     * @brief Aligned Vector Allocator.
     * @param[in] param Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    constexpr AlignedVectorAllocator(const AlignedVectorAllocator&) noexcept = default;
    
    template<typename U>
    constexpr AlignedVectorAllocator(const AlignedVectorAllocator<U, Alignment>&) noexcept {}
    
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            /**
             * @brief Bad alloc.
             * @return Return value.
             */
            throw std::bad_alloc();
        }
        
        std::size_t bytes = n * sizeof(T);
        void* ptr = memory::allocate_aligned(bytes, Alignment);
        
        if (!ptr) {
            /**
             * @brief Bad alloc.
             * @return Return value.
             */
            throw std::bad_alloc();
        }
        
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* ptr, [[maybe_unused]] std::size_t n) noexcept {
        // Unused parameter
        memory::deallocate_aligned(ptr, Alignment);
    }
    
    std::size_t max_size() const noexcept {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
    
    template<typename U>
    bool operator==(const AlignedVectorAllocator<U, Alignment>&) const noexcept {
        return true;
    }
    
    template<typename U>
    bool operator!=(const AlignedVectorAllocator<U, Alignment>&) const noexcept {
        return false;
    }
    
    template<typename U>
    struct rebind {
        using other = AlignedVectorAllocator<U, Alignment>;
    };
};

// ============================================================================
// Convenience type aliases
// ============================================================================

template<typename T>
using AlignedVector = std::vector<T, AlignedVectorAllocator<T, 32>>;

template<typename T>
using CacheLineVector = std::vector<T, AlignedVectorAllocator<T, 64>>;

template<typename T>
using SimdVector = std::vector<T, AlignedVectorAllocator<T, 16>>;

} // namespace cache
} // namespace themis
