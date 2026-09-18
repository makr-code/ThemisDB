/**
 * @file metric_labels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_metrics.h"
#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

class MetricLabels {
public:
    MetricLabels() = default;

    /**
     * @brief Add.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     * @details Calls: std::string().
     */
    MetricLabels& add(std::string_view key, std::string_view value) {
        labels_[std::string(key)] = std::string(value);
        return *this;
    }

    IMetrics::Labels toLabels() const { return labels_; }

    operator IMetrics::Labels() const { return labels_; }  // NOLINT(google-explicit-constructor)

    bool empty() const noexcept { return labels_.empty(); }

    std::size_t size() const noexcept { return labels_.size(); }

private:
    IMetrics::Labels labels_;
};

namespace labels {

// HTTP / RPC dimensions
inline constexpr std::string_view kMethod   = "method";
inline constexpr std::string_view kStatus   = "status";
inline constexpr std::string_view kEndpoint = "endpoint";

// Database / storage dimensions
inline constexpr std::string_view kOperation = "operation";
inline constexpr std::string_view kTable      = "table";
inline constexpr std::string_view kDatabase   = "database";

// Service identity dimensions
inline constexpr std::string_view kService  = "service";
inline constexpr std::string_view kEnv      = "env";
inline constexpr std::string_view kInstance = "instance";

// Error / result dimensions
inline constexpr std::string_view kError  = "error";
inline constexpr std::string_view kResult = "result";

// Cache-specific dimensions
inline constexpr std::string_view kCacheName = "cache_name";
inline constexpr std::string_view kCacheResult = "cache_result";

} // namespace labels

} // namespace concerns
} // namespace core
} // namespace themis
