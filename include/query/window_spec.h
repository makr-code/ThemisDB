/**
 * @file window_spec.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace query {

/**
 * @brief Specification for a continuous-query window.
 *
 * Three sub-types are supported:
 *   - TIME_SLIDING  – [range_ms] width, slides every [slide_ms]
 *   - COUNT_SLIDING – [rows] tuples, slides every [slide_rows]; optional partition key
 *   - TUMBLING      – non-overlapping windows of [range_ms] each
 */
struct WindowSpec {
    enum class Type {
        TIME_SLIDING,   ///< Sliding time window
        COUNT_SLIDING,  ///< Sliding count window
        TUMBLING        ///< Tumbling (non-overlapping) time window
    };

    Type type{Type::TIME_SLIDING};

    // TIME_SLIDING / TUMBLING fields
    int64_t range_ms{60'000};  ///< Window width in milliseconds
    int64_t slide_ms{1'000};   ///< Slide interval in milliseconds (TIME_SLIDING only)

    // COUNT_SLIDING fields
    int64_t rows{1'000};        ///< Window width in tuples
    int64_t slide_rows{100};    ///< Slide step in tuples
    std::string partition_by;  ///< Optional partition-key field name

    // ---- helpers ----

    /** @return slide interval as std::chrono::milliseconds (TIME_SLIDING / TUMBLING). */
    [[nodiscard]] std::chrono::milliseconds slideInterval() const noexcept {
        if (type == Type::TUMBLING) {
            return std::chrono::milliseconds{range_ms};
        }
        return std::chrono::milliseconds{slide_ms};
    }

    /** @return window width as std::chrono::milliseconds (time-based windows). */
    [[nodiscard]] std::chrono::milliseconds windowWidth() const noexcept {
        return std::chrono::milliseconds{range_ms};
    }

    /** @return true if this is a time-based window. */
    [[nodiscard]] constexpr bool isTimeBased() const noexcept {
        return type == Type::TIME_SLIDING || type == Type::TUMBLING;
    }
};

}  // namespace query
}  // namespace themis
