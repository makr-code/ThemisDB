/**
 * @file query_resource_limits.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace themis {
namespace query {

struct QueryResourceLimits {
    size_t max_rows = 0;

    size_t max_memory_bytes = 0;

    uint32_t timeout_ms = 0;
};

class QueryResourceGuard {
public:
    /**
     * @brief Query Resource Guard.
     * @param[in] limits Input parameter.
     * @return Return value.
     */
    explicit QueryResourceGuard(const QueryResourceLimits& limits)
        : limits_(limits)
        , row_count_(0)
        , memory_bytes_(0)
        , start_(std::chrono::steady_clock::now())
    {}

    [[nodiscard]] bool isTimedOut() const noexcept {
        if (limits_.timeout_ms == 0) {
          return false;
        }
        auto elapsed = std::chrono::steady_clock::now() - start_;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
               >= static_cast<long long>(limits_.timeout_ms);
    }

    enum class Violation { None, RowLimit, MemoryLimit, Timeout };

    [[nodiscard]] Violation checkRow(size_t row_bytes = 0) noexcept {
        ++row_count_;
        memory_bytes_ += row_bytes;

        if (isTimedOut()) {
          return Violation::Timeout;
        }
        if (limits_.max_rows > 0 && row_count_ > limits_.max_rows)
            return Violation::RowLimit;
        if (limits_.max_memory_bytes > 0 && memory_bytes_ > limits_.max_memory_bytes)
            return Violation::MemoryLimit;
        return Violation::None;
    }

    size_t rowCount()    const noexcept { return row_count_; }
    size_t memoryBytes() const noexcept { return memory_bytes_; }
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
