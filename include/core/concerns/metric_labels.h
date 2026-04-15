/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metric_labels.h                                    ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:34:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     189                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_metrics.h"
#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Type-safe, fluent builder for metric label sets.
 *
 * `MetricLabels` replaces raw `std::map<std::string, std::string>` at call
 * sites so that label names can be validated at compile time via the
 * predefined constant helpers in the `labels` namespace.
 *
 * ## Usage
 *
 * ### Inline builder (ad-hoc labels)
 * @code
 *   metrics.incrementCounter("http_requests_total", 1,
 *       MetricLabels()
 *           .add(labels::kMethod, "GET")
 *           .add(labels::kStatus, "200")
 *           .add(labels::kEndpoint, "/api/v1/query"));
 * @endcode
 *
 * ### Structured labels (domain-specific, prevents typos)
 * @code
 *   struct HttpRequestLabels {
 *       std::string method;
 *       std::string status;
 *       std::string endpoint;
 *
 *       IMetrics::Labels toMetricLabels() const {
 *           return MetricLabels()
 *               .add(labels::kMethod,   method)
 *               .add(labels::kStatus,   status)
 *               .add(labels::kEndpoint, endpoint)
 *               .toLabels();
 *       }
 *   };
 * @endcode
 *
 * ### Conversion to IMetrics::Labels
 *
 * `MetricLabels` is implicitly convertible to `IMetrics::Labels` so it can
 * be passed directly to any `IMetrics` method that accepts a `Labels`
 * argument:
 * @code
 *   metrics.recordLatency("db.query", 42.0,
 *       MetricLabels().add(labels::kOperation, "SELECT"));
 * @endcode
 */
class MetricLabels {
public:
    /// Construct an empty label set.
    MetricLabels() = default;

    /**
     * @brief Add a label key/value pair and return *this for chaining.
     *
     * If @p key already exists it is overwritten.
     *
     * @param key   Label name (use a constant from the `labels` namespace to
     *              prevent typos, e.g. `labels::kMethod`).
     * @param value Label value.
     * @return Reference to *this for method chaining.
     */
    MetricLabels& add(std::string_view key, std::string_view value) {
        labels_[std::string(key)] = std::string(value);
        return *this;
    }

    /**
     * @brief Return the underlying `IMetrics::Labels` map.
     *
     * Useful when an explicit conversion is needed; the implicit
     * `operator IMetrics::Labels()` handles most cases automatically.
     *
     * @return Copy of the accumulated label map.
     */
    IMetrics::Labels toLabels() const { return labels_; }

    /**
     * @brief Implicit conversion to `IMetrics::Labels`.
     *
     * Allows passing a `MetricLabels` object wherever `IMetrics::Labels` is
     * expected without an explicit conversion call.
     */
    operator IMetrics::Labels() const { return labels_; }  // NOLINT(google-explicit-constructor)

    /**
     * @brief Return true if no labels have been added.
     * @return true iff the label set is empty.
     */
    bool empty() const noexcept { return labels_.empty(); }

    /**
     * @brief Return the number of labels in the set.
     * @return Number of key/value pairs.
     */
    std::size_t size() const noexcept { return labels_.size(); }

private:
    IMetrics::Labels labels_;
};

/**
 * @namespace themis::core::concerns::labels
 * @brief Predefined label-name constants for the most common metrics dimensions.
 *
 * Using these constants instead of raw strings prevents typos and makes
 * label names refactorable:
 * @code
 *   // Bad  – easy to typo:
 *   MetricLabels().add("metod", "GET")
 *
 *   // Good – compile-time checked:
 *   MetricLabels().add(labels::kMethod, "GET")
 * @endcode
 */
namespace labels {

// HTTP / RPC dimensions
/// HTTP method (e.g. "GET", "POST").
inline constexpr std::string_view kMethod   = "method";
/// HTTP status code (e.g. "200", "404", "500").
inline constexpr std::string_view kStatus   = "status";
/// API endpoint path (e.g. "/api/v1/query").
inline constexpr std::string_view kEndpoint = "endpoint";

// Database / storage dimensions
/// Storage operation name (e.g. "SELECT", "INSERT", "flush").
inline constexpr std::string_view kOperation = "operation";
/// Table or collection name.
inline constexpr std::string_view kTable      = "table";
/// Database name.
inline constexpr std::string_view kDatabase   = "database";

// Service identity dimensions
/// Logical service name (e.g. "themisdb", "query-worker").
inline constexpr std::string_view kService  = "service";
/// Deployment environment (e.g. "production", "staging", "ci").
inline constexpr std::string_view kEnv      = "env";
/// Service instance or node identifier.
inline constexpr std::string_view kInstance = "instance";

// Error / result dimensions
/// Error class or code (e.g. "timeout", "permission_denied").
inline constexpr std::string_view kError  = "error";
/// Boolean result of an operation ("true" / "false", "hit" / "miss").
inline constexpr std::string_view kResult = "result";

// Cache-specific dimensions
/// Cache name or namespace.
inline constexpr std::string_view kCacheName = "cache_name";
/// Cache outcome ("hit" or "miss").
inline constexpr std::string_view kCacheResult = "cache_result";

} // namespace labels

} // namespace concerns
} // namespace core
} // namespace themis
