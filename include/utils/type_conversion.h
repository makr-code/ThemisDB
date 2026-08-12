/**
 * @file type_conversion.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <optional>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {

/**
 * @brief Type conversion exception with detailed error information
 */
class ConversionException : public std::runtime_error {
public:
    explicit ConversionException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Safe type conversion utilities with overflow/underflow detection
 * 
 * Provides safe conversions between integer types with proper error handling.
 * All functions check for overflow/underflow and log warnings.
 */
namespace conversion {

    // ===== Checked conversion functions =====
    
    /**
     * @brief Safe conversion from size_t to int32_t with overflow check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value exceeds int32_t range
     * 
     * @par Example
     *   size_t size = 1000;
     *   int32_t count = safe_size_to_int32(size);
     */
    inline int32_t safe_size_to_int32(size_t value) {
        if (value > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            std::string msg = "Overflow: size_t value " + std::to_string(value) + 
                            " exceeds int32_t max (" + 
                            std::to_string(std::numeric_limits<int32_t>::max()) + ")";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<int32_t>(value);
    }

    /**
     * @brief Safe conversion from size_t to int with overflow check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value exceeds int range
     */
    inline int safe_size_to_int(size_t value) {
        if (value > static_cast<size_t>(std::numeric_limits<int>::max())) {
            std::string msg = "Overflow: size_t value " + std::to_string(value) + 
                            " exceeds int max (" + 
                            std::to_string(std::numeric_limits<int>::max()) + ")";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<int>(value);
    }

    /**
     * @brief Safe conversion from int64_t to int32_t with overflow check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value exceeds int32_t range
     */
    inline int32_t safe_int64_to_int32(int64_t value) {
        if (value > std::numeric_limits<int32_t>::max() || 
            value < std::numeric_limits<int32_t>::min()) {
            std::string msg = "Overflow: int64_t value " + std::to_string(value) + 
                            " out of int32_t range [" + 
                            std::to_string(std::numeric_limits<int32_t>::min()) + ", " +
                            std::to_string(std::numeric_limits<int32_t>::max()) + "]";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<int32_t>(value);
    }

    /**
     * @brief Safe conversion from uint64_t to int with overflow check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value exceeds int range
     */
    inline int safe_uint64_to_int(uint64_t value) {
        if (value > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            std::string msg = "Overflow: uint64_t value " + std::to_string(value) + 
                            " exceeds int max (" + 
                            std::to_string(std::numeric_limits<int>::max()) + ")";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<int>(value);
    }

    /**
     * @brief Safe conversion from double to float with precision loss warning
     * @param value Source value
     * @param allow_loss Whether to allow loss of precision
     * @return Converted value
     * @throws ConversionException if value exceeds float range
     */
    inline float safe_double_to_float(double value, bool allow_loss = false) {
        if (value > std::numeric_limits<float>::max() || 
            value < -std::numeric_limits<float>::max()) {
            std::string msg = "Overflow: double value " + std::to_string(value) + 
                            " exceeds float range [" + 
                            std::to_string(-std::numeric_limits<float>::max()) + ", " +
                            std::to_string(std::numeric_limits<float>::max()) + "]";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        
        if (!allow_loss && (value != 0.0)) {
            float result = static_cast<float>(value);
            if (static_cast<double>(result) != value) {
                spdlog::debug("Type conversion: precision loss converting {} to float", value);
            }
        }
        
        return static_cast<float>(value);
    }

    /**
     * @brief Safe conversion from signed to unsigned with sign check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value is negative
     */
    inline uint64_t safe_signed_to_unsigned(int64_t value) {
        if (value < 0) {
            std::string msg = "Sign error: cannot convert negative int64_t value " + 
                            std::to_string(value) + " to uint64_t";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<uint64_t>(value);
    }

    // ===== Optional return versions (no exceptions) =====
    
    /**
     * @brief Safe size_t to int conversion returning optional
     * @param value Source value
     * @return Converted value or std::nullopt on overflow
     */
    inline std::optional<int32_t> try_size_to_int32(size_t value) noexcept {
        if (value > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            spdlog::warn("Type conversion: size_t {} exceeds int32_t max", value);
            return std::nullopt;
        }
        return static_cast<int32_t>(value);
    }

    /**
     * @brief Safe size_t to int conversion returning optional
     * @param value Source value
     * @return Converted value or std::nullopt on overflow
     */
    inline std::optional<int> try_size_to_int(size_t value) noexcept {
        if (value > static_cast<size_t>(std::numeric_limits<int>::max())) {
            spdlog::warn("Type conversion: size_t {} exceeds int max", value);
            return std::nullopt;
        }
        return static_cast<int>(value);
    }

    /**
     * @brief Safe int64_t to int32_t conversion returning optional
     * @param value Source value
     * @return Converted value or std::nullopt on overflow
     */
    inline std::optional<int32_t> try_int64_to_int32(int64_t value) noexcept {
        if (value > std::numeric_limits<int32_t>::max() || 
            value < std::numeric_limits<int32_t>::min()) {
            spdlog::warn("Type conversion: int64_t {} out of int32_t range", value);
            return std::nullopt;
        }
        return static_cast<int32_t>(value);
    }

    /**
     * @brief Safe double to float conversion returning optional
     * @param value Source value
     * @return Converted value or std::nullopt on overflow
     */
    inline std::optional<float> try_double_to_float(double value) noexcept {
        if (value > std::numeric_limits<float>::max() || 
            value < -std::numeric_limits<float>::max()) {
            spdlog::warn("Type conversion: double {} exceeds float range", value);
            return std::nullopt;
        }
        return static_cast<float>(value);
    }

    // ===== Clamping versions (saturate on bounds) =====
    
    /**
     * @brief Clamp size_t to int32_t range
     * @param value Source value
     * @return Clamped value (max int32_t if exceeds)
     */
    inline int32_t clamp_size_to_int32(size_t value) noexcept {
        if (value > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            spdlog::debug("Type conversion: clamping size_t {} to int32_t max", value);
            return std::numeric_limits<int32_t>::max();
        }
        return static_cast<int32_t>(value);
    }

    /**
     * @brief Clamp double to float range
     * @param value Source value
     * @return Clamped value (saturated to float bounds if exceeds)
     */
    inline float clamp_double_to_float(double value) noexcept {
        if (value > std::numeric_limits<float>::max()) {
            spdlog::debug("Type conversion: clamping double {} to float max", value);
            return std::numeric_limits<float>::max();
        }
        if (value < -std::numeric_limits<float>::max()) {
            spdlog::debug("Type conversion: clamping double {} to float min", value);
            return -std::numeric_limits<float>::max();
        }
        return static_cast<float>(value);
    }

    /**
     * @brief Clamp int64_t to int32_t range
     * @param value Source value
     * @return Clamped value (saturated to int32_t bounds if exceeds)
     */
    inline int32_t clamp_int64_to_int32(int64_t value) noexcept {
        if (value > std::numeric_limits<int32_t>::max()) {
            spdlog::debug("Type conversion: clamping int64_t {} to int32_t max", value);
            return std::numeric_limits<int32_t>::max();
        }
        if (value < std::numeric_limits<int32_t>::min()) {
            spdlog::debug("Type conversion: clamping int64_t {} to int32_t min", value);
            return std::numeric_limits<int32_t>::min();
        }
        return static_cast<int32_t>(value);
    }

    // ===== Additional utilities for signed/unsigned safety =====

    /**
     * @brief Safe conversion from int to size_t with negative check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value is negative
     * 
     * @par Example
     *   int count = get_count();
     *   size_t size = safe_int_to_size(count);
     */
    inline size_t safe_int_to_size(int value) {
        if (value < 0) {
            std::string msg = "Cannot convert negative int value " + 
                            std::to_string(value) + " to size_t";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<size_t>(value);
    }

    /**
     * @brief Safe conversion from int64_t to size_t with negative check
     * @param value Source value
     * @return Converted value
     * @throws ConversionException if value is negative
     */
    inline size_t safe_int64_to_size(int64_t value) {
        if (value < 0) {
            std::string msg = "Cannot convert negative int64_t value " + 
                            std::to_string(value) + " to size_t";
            spdlog::warn("Type conversion: {}", msg);
            throw ConversionException(msg);
        }
        return static_cast<size_t>(value);
    }

    /**
     * @brief Safe index validation helper
     * @param index Index to validate (can be negative)
     * @param size Container size
     * @return true if index is valid (non-negative and within bounds)
     * 
     * @par Example
     *   std::vector<int> vec = {1, 2, 3};
     *   int index = -1;
     *   if (is_valid_index(index, vec.size())) {
     *       // Safe to access vec[index]
     *   }
     */
    inline bool is_valid_index(int index, size_t size) noexcept {
        return index >= 0 && static_cast<size_t>(index) < size;
    }

    /**
     * @brief Safe subtraction of size_t values returning signed result
     * @param a First value
     * @param b Second value
     * @return Signed difference (a - b)
     * @throws ConversionException if difference exceeds ptrdiff_t range
     * 
     * @par Example
     *   size_t pos1 = 100;
     *   size_t pos2 = 50;
     *   ptrdiff_t diff = safe_diff(pos1, pos2);  // 50
     *   ptrdiff_t neg_diff = safe_diff(pos2, pos1);  // -50
     */
    inline std::ptrdiff_t safe_diff(size_t a, size_t b) {
        // Check if positive difference would overflow ptrdiff_t
        if (a > b) {
            size_t diff = a - b;
            if (diff > static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
                std::string msg = "Difference " + std::to_string(diff) + 
                                " too large for ptrdiff_t";
                spdlog::warn("Type conversion: {}", msg);
                throw ConversionException(msg);
            }
            return static_cast<std::ptrdiff_t>(diff);
        } else {
            size_t diff = b - a;
            if (diff > static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
                std::string msg = "Difference " + std::to_string(diff) + 
                                " too large for ptrdiff_t";
                spdlog::warn("Type conversion: {}", msg);
                throw ConversionException(msg);
            }
            return -static_cast<std::ptrdiff_t>(diff);
        }
    }

} // namespace conversion

} // namespace utils
} // namespace themis

