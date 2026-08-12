/**
 * @file unaligned_access.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Safe Unaligned Memory Access
// Provides portable, safe access to unaligned memory
//
// Critical for ARM platforms where unaligned access can cause SIGBUS
// Uses memcpy internally which compilers optimize to efficient code

#pragma once

#include "performance/alignment_helpers.h"
#include <cstring>
#include <type_traits>

namespace themis {
namespace utils {

/**
 * @brief Safe unaligned memory read
 * 
 * Reads from potentially unaligned address safely using memcpy.
 * This is the only portable way to handle unaligned access on ARM.
 * Compilers optimize memcpy to efficient code (often single instruction).
 * 
 * @tparam T Type to read (must be trivially copyable)
 * @param ptr Potentially unaligned pointer
 * @return Value read from memory
 * 
 * @code
 * // Dangerous on ARM:
 * uint32_t value = *reinterpret_cast<const uint32_t*>(unaligned_ptr);
 * 
 * // Safe and portable:
 * uint32_t value = read_unaligned<uint32_t>(unaligned_ptr);
 * @endcode
 */
template<typename T>
inline T read_unaligned(const void* ptr) noexcept {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable");
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    return value;
}

/**
 * @brief Safe unaligned memory write
 * 
 * Writes to potentially unaligned address safely using memcpy.
 * 
 * @tparam T Type to write (must be trivially copyable)
 * @param ptr Potentially unaligned pointer
 * @param value Value to write
 * 
 * @code
 * // Dangerous on ARM:
 * *reinterpret_cast<uint32_t*>(unaligned_ptr) = value;
 * 
 * // Safe and portable:
 * write_unaligned<uint32_t>(unaligned_ptr, value);
 * @endcode
 */
template<typename T>
inline void write_unaligned(void* ptr, const T& value) noexcept {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable");
    std::memcpy(ptr, &value, sizeof(T));
}

/**
 * @brief Cast with alignment verification
 * 
 * Casts pointer only if properly aligned, otherwise returns nullptr.
 * Safe for use on ARM and other strict platforms.
 * 
 * @tparam T Target type
 * @param ptr Pointer to cast
 * @return Cast pointer if aligned, nullptr if not
 * 
 * @code
 * auto* typed_ptr = checked_aligned_cast<MyStruct>(void_ptr);
 * if (typed_ptr) {
 *     // Safe to dereference - properly aligned
 *     typed_ptr->member = value;
 * } else {
 *     // Not aligned - use unaligned access or handle error
 * }
 * @endcode
 */
template<typename T>
inline T* checked_aligned_cast(void* ptr) noexcept {
    if (performance::is_aligned<alignof(T)>(ptr)) {
        return static_cast<T*>(ptr);
    }
    return nullptr;  // Not properly aligned
}

/**
 * @brief Cast with alignment verification (const version)
 * 
 * @tparam T Target type
 * @param ptr Pointer to cast
 * @return Cast pointer if aligned, nullptr if not
 */
template<typename T>
inline const T* checked_aligned_cast(const void* ptr) noexcept {
    if (performance::is_aligned<alignof(T)>(ptr)) {
        return static_cast<const T*>(ptr);
    }
    return nullptr;  // Not properly aligned
}

} // namespace utils
} // namespace themis
