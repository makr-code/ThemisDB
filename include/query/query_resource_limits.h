/**
 * @file query_resource_limits.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace themis {
namespace query {

/**
 * @brief Per-query resource limits for row count, memory, and execution time.
 *
 * Zero means "unlimited" for every field.
 */
struct QueryResourceLimits {
    /// Maximum number of result rows returned.  0 = unlimited.
    size_t max_rows = 0;

    /// Maximum estimated memory for the result set (bytes).  0 = unlimited.
    size_t max_memory_bytes = 0;

    /// Maximum query wall-clock execution time (milliseconds).  0 = unlimited.
    uint32_t timeout_ms = 0;
};

/**
 * @brief RAII guard that enforces per-query resource limits during execution.
 *
 * Usage:
 * @code
 *   QueryResourceGuard guard(limits);
 *   // … produce rows one-by-one …
 *   auto err = guard.checkRow(estimated_row_bytes);
 *   if (err) return Err<…>(err->code(), err->message());
 * @endcode
 *
 * The guard is not thread-safe; use one instance per query execution.
 */
class QueryResourceGuard {
public:
    explicit QueryResourceGuard(const QueryResourceLimits& limits)
        : limits_(limits)
        , row_count_(0)
        , memory_bytes_(0)
        , start_(std::chrono::steady_clock::now())
    {}

    /**
     * @brief Check whether the current query has exceeded its timeout.
     * @return true if timeout_ms > 0 and the elapsed time exceeds timeout_ms.
     */
    [[nodiscard]] bool isTimedOut() const noexcept {
        if (limits_.timeout_ms == 0) return false;
        auto elapsed = std::chrono::steady_clock::now() - start_;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
               >= static_cast<long long>(limits_.timeout_ms);
    }

    /**
     * @brief Record an additional row with an estimated byte size and check limits.
     *
     * @param row_bytes  Estimated size of this row in bytes (used for memory check).
     * @return LimitViolation enum value indicating which limit was breached,
     *         or None if all limits are satisfied.
     */
    enum class Violation { None, RowLimit, MemoryLimit, Timeout };

    [[nodiscard]] Violation checkRow(size_t row_bytes = 0) noexcept {
        ++row_count_;
        memory_bytes_ += row_bytes;

        if (isTimedOut()) return Violation::Timeout;
        if (limits_.max_rows > 0 && row_count_ > limits_.max_rows)
            return Violation::RowLimit;
        if (limits_.max_memory_bytes > 0 && memory_bytes_ > limits_.max_memory_bytes)
            return Violation::MemoryLimit;
        return Violation::None;
    }

    /// Total rows counted so far.
    size_t rowCount()    const noexcept { return row_count_; }
    /// Total memory accumulated so far (bytes).
    size_t memoryBytes() const noexcept { return memory_bytes_; }
    /// Elapsed time since guard creation (milliseconds).
    uint64_t elapsedMs() const noexcept {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    }

    const QueryResourceLimits& limits() const noexcept { return limits_; }

private:
    QueryResourceLimits limits_;
    size_t row_count_{0};
    size_t memory_bytes_{0};
    std::chrono::steady_clock::time_point start_;
};

} // namespace query
} // namespace themis
