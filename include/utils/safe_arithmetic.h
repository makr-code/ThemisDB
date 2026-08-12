/**
 * @file safe_arithmetic.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace themis {
namespace utils {

/**
 * @brief Safe arithmetic utilities for signed/unsigned operations
 * 
 * This header provides utilities to safely handle mixed signed/unsigned arithmetic
 * and prevent common bugs like:
 * - Buffer overflows from negative indices
 * - Integer overflow/underflow
 * - Incorrect comparisons between signed and unsigned values
 * 
 * These utilities help eliminate C4018 (MSVC) and -Wsign-compare/-Wsign-conversion
 * compiler warnings while maintaining correctness.
 */

/**
 * @brief Safely add a signed offset to an unsigned base
 * 
 * Handles both positive and negative offsets correctly, with overflow/underflow detection.
 * 
 * @param base Unsigned base value
 * @param offset Signed offset to add (can be negative)
 * @return Result if operation is valid, std::nullopt on overflow/underflow
 * 
 * @code
 * size_t index = 10;
 * int offset = -5;
 * auto result = safe_add(index, offset);  // Returns 5
 * 
 * size_t small = 3;
 * int large_neg = -10;
 * auto underflow = safe_add(small, large_neg);  // Returns nullopt
 * @endcode
 */
inline std::optional<size_t> safe_add(size_t base, int offset) noexcept {
    if (offset < 0) {
        // Handle negative offset - convert to subtraction
        size_t abs_offset = static_cast<size_t>(-static_cast<long long>(offset));
        if (base < abs_offset) {
            return std::nullopt;  // Would underflow
        }
        return base - abs_offset;
    }
    // Positive offset - check for overflow
    size_t u_offset = static_cast<size_t>(offset);
    if (base > SIZE_MAX - u_offset) {
        return std::nullopt;  // Would overflow
    }
    return base + u_offset;
}

/**
 * @brief Safely subtract two unsigned values
 * 
 * @param a Minuend
 * @param b Subtrahend
 * @return Result if a >= b, std::nullopt if would underflow
 */
inline std::optional<size_t> safe_sub(size_t a, size_t b) noexcept {
    if (a < b) {
        return std::nullopt;  // Would underflow
    }
    return a - b;
}

/**
 * @brief Safely convert signed integer to size_t
 * 
 * @param value Signed integer value
 * @return size_t value if non-negative, std::nullopt if negative
 * 
 * @code
 * int count = 10;
 * if (auto size = safe_int_to_size(count)) {
 *     std::vector<int> vec(*size);  // Safe
 * }
 * @endcode
 */
inline std::optional<size_t> safe_int_to_size(int value) noexcept {
    if (value < 0) {
        return std::nullopt;
    }
    return static_cast<size_t>(value);
}

/**
 * @brief Safely convert int64_t to size_t
 * 
 * @param value Signed 64-bit integer
 * @return size_t value if non-negative and within range, std::nullopt otherwise
 */
inline std::optional<size_t> safe_int64_to_size(int64_t value) noexcept {
    if (value < 0) {
        return std::nullopt;
    }
    if constexpr (sizeof(size_t) < sizeof(int64_t)) {
        if (static_cast<uint64_t>(value) > SIZE_MAX) {
            return std::nullopt;
        }
    }
    return static_cast<size_t>(value);
}

/**
 * @brief Check if signed index is within valid range for container
 * 
 * Safely checks that index is non-negative and less than size.
 * 
 * @param index Signed index to check
 * @param size Container size
 * @return true if index is valid (0 <= index < size)
 * 
 * @code
 * std::vector<int> vec = {1, 2, 3};
 * int idx = get_index();
 * if (in_range(idx, vec.size())) {
 *     process(vec[idx]);  // Safe
 * }
 * @endcode
 */
inline bool in_range(int index, size_t size) noexcept {
    return index >= 0 && static_cast<size_t>(index) < size;
}

/**
 * @brief Check if int64_t index is within valid range for container
 * 
 * @param index Signed 64-bit index to check
 * @param size Container size
 * @return true if index is valid (0 <= index < size)
 */
inline bool in_range(int64_t index, size_t size) noexcept {
    return index >= 0 && static_cast<size_t>(index) < size;
}

/**
 * @brief Safely convert size_t to int with overflow check
 * 
 * Many legacy APIs require int parameters but work with containers.
 * This provides a safe conversion with overflow detection.
 * 
 * @param value Unsigned value to convert
 * @return int value if within INT_MAX, std::nullopt on overflow
 * 
 * @code
 * std::vector<int> data = get_data();
 * if (auto count = safe_size_to_int(data.size())) {
 *     legacy_api(*count);  // Safe
 * } else {
 *     // Handle overflow - container too large for int
 * }
 * @endcode
 */
inline std::optional<int> safe_size_to_int(size_t value) noexcept {
    if (value > static_cast<size_t>(INT_MAX)) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

/**
 * @brief Safely compare signed and unsigned values
 * 
 * Performs comparison without implicit conversions that could cause bugs.
 * 
 * @param signed_val Signed value
 * @param unsigned_val Unsigned value
 * @return true if signed_val < unsigned_val, considering sign
 * 
 * @code
 * int count = -1;
 * std::vector<int> vec = {1, 2, 3};
 * if (safe_less_than(count, vec.size())) {
 *     // This is TRUE - correctly handles negative
 * }
 * @endcode
 */
inline bool safe_less_than(int signed_val, size_t unsigned_val) noexcept {
    if (signed_val < 0) {
        return true;  // Negative is always less than unsigned
    }
    return static_cast<size_t>(signed_val) < unsigned_val;
}

/**
 * @brief Safely compare int64_t and size_t
 * 
 * @param signed_val Signed 64-bit value
 * @param unsigned_val Unsigned value
 * @return true if signed_val < unsigned_val
 */
inline bool safe_less_than(int64_t signed_val, size_t unsigned_val) noexcept {
    if (signed_val < 0) {
        return true;
    }
    return static_cast<uint64_t>(signed_val) < static_cast<uint64_t>(unsigned_val);
}

/**
 * @brief Safely compare for greater-than
 * 
 * @param signed_val Signed value
 * @param unsigned_val Unsigned value
 * @return true if signed_val > unsigned_val
 */
inline bool safe_greater_than(int signed_val, size_t unsigned_val) noexcept {
    if (signed_val < 0) {
        return false;  // Negative is never greater than unsigned
    }
    return static_cast<size_t>(signed_val) > unsigned_val;
}

/**
 * @brief Helper to safely iterate with signed index over container
 * 
 * Validates that iteration is safe before starting.
 * 
 * @param container Container to iterate over
 * @param start_index Starting index (can be negative)
 * @param callback Function to call for each valid index
 * @return true if iteration completed, false if start_index invalid
 * 
 * @code
 * std::vector<int> data = {1, 2, 3, 4, 5};
 * safe_iterate(data, 2, [&](size_t i) {
 *     process(data[i]);
 * });
 * @endcode
 */
template<typename Container, typename Func>
inline bool safe_iterate(const Container& container, int start_index, Func&& callback) {
    if (!in_range(start_index, container.size())) {
        return false;
    }
    size_t start = static_cast<size_t>(start_index);
    for (size_t i = start; i < container.size(); ++i) {
        callback(i);
    }
    return true;
}

} // namespace utils
} // namespace themis

