/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            allocator.h                                        ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     131                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • d66369dc5  2026-01-16  feat: Add ThemisDB Static Initialization Crash Analyzer ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Memory Allocator
// Provides unified interface for memory allocation with optional optimizations
//
// When THEMIS_ENABLE_MIMALLOC is defined, uses mimalloc allocator for improved performance
// Otherwise, uses standard system allocator

#pragma once

#include <cstddef>
#include <new>

#ifdef THEMIS_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

namespace themis {
namespace memory {

/**
 * @brief Allocate memory using the configured allocator
 * 
 * When mimalloc is enabled (THEMIS_ENABLE_MIMALLOC), uses mi_malloc
 * Otherwise uses standard operator new
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory
 */
inline void* allocate(size_t size) {
    #ifdef THEMIS_ENABLE_MIMALLOC
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
    if (!ptr) return;
    
    #ifdef THEMIS_ENABLE_MIMALLOC
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
    #ifdef THEMIS_ENABLE_MIMALLOC
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
inline void deallocate_aligned(void* ptr, size_t alignment) {
    if (!ptr) return;
    
    #ifdef THEMIS_ENABLE_MIMALLOC
    mi_free(ptr);
    #else
    ::operator delete(ptr, std::align_val_t(alignment));
    #endif
}

/**
 * @brief Get allocator name for logging/diagnostics
 */
inline const char* allocator_name() {
    #ifdef THEMIS_ENABLE_MIMALLOC
    return "mimalloc";
    #else
    return "system";
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
