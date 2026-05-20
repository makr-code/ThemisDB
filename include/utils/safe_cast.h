/*
 * ThemisDB | File: safe_cast.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace themis {
namespace utils {

/**
 * @brief Safely convert between types of equal size
 * 
 * This function performs a type-safe bitwise conversion that does not
 * violate strict aliasing rules. It uses memcpy instead of reinterpret_cast.
 * 
 * @tparam Target Target type
 * @tparam Source Source type
 * @param source Value to convert
 * @return Converted value with same bit pattern
 * 
 * @note Both types must have the same size
 * @note This is a bitwise conversion - no semantic meaning
 * 
 * @code
 * float f = 3.14f;
 * uint32_t bits = safe_cast<uint32_t>(f);  // Bitwise copy, no rounding
 * @endcode
 */
template<typename Target, typename Source>
inline Target safe_cast(const Source& source) noexcept {
    static_assert(sizeof(Target) == sizeof(Source),
                  "safe_cast requires equal-sized types");
    static_assert(std::is_trivially_copyable_v<Target>,
                  "Target must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<Source>,
                  "Source must be trivially copyable");

    using DecayedSource = std::remove_cv_t<Source>;
    DecayedSource temp = source; // copy to drop cv qualifiers safely

    Target result;
    std::memcpy(&result, &temp, sizeof(Target));
    return result;
}

/**
 * @brief Type-safe bit patterns for floating point numbers
 * 
 * Provides safe conversion between floating point and integer bit representations
 * without strict aliasing violations.
 */
class FloatBits {
public:
    /// Interpret float bits as uint32
    static uint32_t to_u32(float f) noexcept {
        return safe_cast<uint32_t>(f);
    }
    
    /// Interpret uint32 bits as float
    static float from_u32(uint32_t u) noexcept {
        return safe_cast<float>(u);
    }
    
    /// Interpret double bits as uint64
    static uint64_t to_u64(double d) noexcept {
        return safe_cast<uint64_t>(d);
    }
    
    /// Interpret uint64 bits as double
    static double from_u64(uint64_t u) noexcept {
        return safe_cast<double>(u);
    }
};

} // namespace utils
} // namespace themis
