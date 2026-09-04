/**
 * @file allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Memory Allocator
// Provides unified interface for memory allocation with optional optimizations
//
// When THEMIS_ENABLE_JEMALLOC is defined, uses jemalloc allocator (best fragmentation resistance)
// When THEMIS_ENABLE_MIMALLOC is defined, uses mimalloc allocator for improved performance
// Otherwise, uses standard system allocator
//
// Priority: jemalloc > mimalloc > system
// Note: THEMIS_ENABLE_JEMALLOC and THEMIS_ENABLE_MIMALLOC should not both be defined.

#pragma once

#include <cstddef>
#include <new>

#ifdef THEMIS_ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

#ifdef THEMIS_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

namespace themis {
namespace memory {

/**
 * @brief Allocate memory using the configured allocator
 * 
 * When jemalloc is enabled (THEMIS_ENABLE_JEMALLOC), uses je_malloc
 * When mimalloc is enabled (THEMIS_ENABLE_MIMALLOC), uses mi_malloc
 * Otherwise uses standard operator new
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory
 */
inline void* allocate(size_t size) {
    #ifdef THEMIS_ENABLE_JEMALLOC
    return je_malloc(size);
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    return mi_malloc(size);
    #else
    return ::operator new(size);
    #endif
}

/**
 * @brief Deallocate memory allocated with allocate()
 * 
 * @param ptr Pointer to memory to deallocate
 */
inline void deallocate(void* ptr) {
    if (!ptr) {
      return;
    }
    
    #ifdef THEMIS_ENABLE_JEMALLOC
    je_free(ptr);
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    mi_free(ptr);
    #else
    ::operator delete(ptr);
    #endif
}

/**
 * @brief Allocate aligned memory
 * 
 * @param size Number of bytes to allocate
 * @param alignment Alignment requirement in bytes (must be power of 2)
 * @return Pointer to aligned memory
 */
inline void* allocate_aligned(size_t size, size_t alignment) {
    #ifdef THEMIS_ENABLE_JEMALLOC
    return je_aligned_alloc(alignment, size);
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    return mi_malloc_aligned(size, alignment);
    #else
    return ::operator new(size, std::align_val_t(alignment));
    #endif
}

/**
 * @brief Deallocate aligned memory
 * 
 * @param ptr Pointer to aligned memory
 * @param alignment Alignment that was used for allocation
 */
inline void deallocate_aligned(void* ptr, [[maybe_unused]] size_t alignment) {
    if (!ptr) {
      return;
    }
    
    #ifdef THEMIS_ENABLE_JEMALLOC
    je_free(ptr);
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    mi_free(ptr);
    #else
    ::operator delete(ptr, std::align_val_t(alignment));
    #endif
}

/**
 * @brief Get allocator name for logging/diagnostics
 */
inline const char* allocator_name() {
    #ifdef THEMIS_ENABLE_JEMALLOC
    return "jemalloc";
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    return "mimalloc";
    #else
    return "system";
    #endif
}

/**
 * @brief Check if jemalloc optimization is active
 */
inline bool is_jemalloc_enabled() {
    #ifdef THEMIS_ENABLE_JEMALLOC
    return true;
    #else
    return false;
    #endif
}

/**
 * @brief Check if mimalloc optimization is active
 */
inline bool is_mimalloc_enabled() {
    #ifdef THEMIS_ENABLE_MIMALLOC
    return true;
    #else
    return false;
    #endif
}

} // namespace memory
} // namespace themis
