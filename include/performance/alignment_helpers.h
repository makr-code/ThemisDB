/**
 * @file alignment_helpers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Alignment Verification Helpers
// Provides compile-time and runtime alignment verification for SIMD and cache-critical structures
//
// Critical for:
// - ARM platforms (strict alignment requirements)
// - SIMD operations (16, 32-byte alignment)
// - Cache line optimization (64-byte alignment)

#pragma once

#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace themis {
namespace performance {

/**
 * @brief Compile-time alignment verification
 * 
 * Verifies that a type has the required alignment at compile time.
 * Fails to compile if alignment requirement not met.
 * 
 * @tparam T Type to verify
 * @tparam RequiredAlignment Expected alignment in bytes
 * @return true if T has exact alignment, false otherwise
 * 
 * @code
 * struct alignas(64) MyStruct { uint64_t x; };
 * static_assert(check_alignment<MyStruct, 64>());  // OK
 * static_assert(check_alignment<MyStruct, 32>());  // COMPILE ERROR
 * @endcode
 */
template<typename T, size_t RequiredAlignment>
constexpr bool check_alignment() noexcept {
    return alignof(T) == RequiredAlignment;
}

/**
 * @brief Verify alignment is at least the required value
 * 
 * @tparam T Type to verify
 * @tparam MinAlignment Minimum alignment in bytes
 * @return true if T has at least MinAlignment, false otherwise
 */
template<typename T, size_t MinAlignment>
constexpr bool check_min_alignment() noexcept {
    return alignof(T) >= MinAlignment;
}

/**
 * @brief Verify pointer is aligned to required boundary
 * 
 * @tparam Alignment Required alignment in bytes (must be power of 2)
 * @param ptr Pointer to check
 * @return true if pointer is aligned, false otherwise
 */
template<size_t Alignment>
constexpr bool is_aligned(const void* ptr) noexcept {
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of 2");
    return (reinterpret_cast<uintptr_t>(ptr) & (Alignment - 1)) == 0;
}

/**
 * @brief Align a pointer up to the next alignment boundary
 * 
 * @tparam Alignment Required alignment in bytes (must be power of 2)
 * @param ptr Pointer to align
 * @return Aligned pointer (>= original pointer)
 */
template<size_t Alignment>
inline void* align_up(void* ptr) noexcept {
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of 2");
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<void*>((addr + Alignment - 1) & ~(Alignment - 1));
}

/**
 * @brief Align a pointer up to the next alignment boundary (const version)
 * 
 * @tparam Alignment Required alignment in bytes (must be power of 2)
 * @param ptr Pointer to align
 * @return Aligned pointer (>= original pointer)
 */
template<size_t Alignment>
inline const void* align_up(const void* ptr) noexcept {
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of 2");
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<const void*>((addr + Alignment - 1) & ~(Alignment - 1));
}

/**
 * @brief Align a pointer down to the nearest alignment boundary
 * 
 * @tparam Alignment Required alignment in bytes (must be power of 2)
 * @param ptr Pointer to align
 * @return Aligned pointer (<= original pointer)
 */
template<size_t Alignment>
inline void* align_down(void* ptr) noexcept {
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of 2");
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<void*>(addr & ~(Alignment - 1));
}

/**
 * @brief Align a pointer down to the nearest alignment boundary (const version)
 * 
 * @tparam Alignment Required alignment in bytes (must be power of 2)
 * @param ptr Pointer to align
 * @return Aligned pointer (<= original pointer)
 */
template<size_t Alignment>
inline const void* align_down(const void* ptr) noexcept {
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of 2");
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<const void*>(addr & ~(Alignment - 1));
}

/**
 * @brief Calculate padding needed to align to boundary
 * 
 * @tparam Alignment Required alignment in bytes
 * @param current_offset Current offset to align
 * @return Number of bytes of padding needed
 */
template<size_t Alignment>
constexpr size_t padding_for_alignment(size_t current_offset) noexcept {
    return (Alignment - (current_offset % Alignment)) % Alignment;
}

} // namespace performance
} // namespace themis

// ============================================================================
// Compile-time assertion macros
// ============================================================================

// Helper macro for stringification
#define _THEMIS_STRINGIFY_IMPL(x) #x
#define _THEMIS_STRINGIFY(x) _THEMIS_STRINGIFY_IMPL(x)

/// Assert type has exact alignment
#define THEMIS_STATIC_ASSERT_ALIGNED(Type, Alignment) \
    static_assert(alignof(Type) == (Alignment), \
                  #Type " must be aligned to " #Alignment " bytes, but has alignof=" \
                  _THEMIS_STRINGIFY(alignof(Type)))

/// Assert type has at least minimum alignment
#define THEMIS_STATIC_ASSERT_MIN_ALIGNED(Type, MinAlignment) \
    static_assert(alignof(Type) >= (MinAlignment), \
                  #Type " must be aligned to at least " #MinAlignment " bytes, but has alignof=" \
                  _THEMIS_STRINGIFY(alignof(Type)))

/// Assert structure size is exact (catches unexpected padding)
#define THEMIS_STATIC_ASSERT_SIZE(Type, Size) \
    static_assert(sizeof(Type) == (Size), \
                  #Type " must be exactly " #Size " bytes, but has sizeof=" \
                  _THEMIS_STRINGIFY(sizeof(Type)))
