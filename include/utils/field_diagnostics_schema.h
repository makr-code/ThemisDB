/**
 * @file field_diagnostics_schema.h
 * @brief Field Diagnostics Schema for ThemisDB Production Observability
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Gap Summary: total=0; Stub=0, Unimpl=0, Mock=0, Sim=0
 *
 * Defines structured diagnostic event types for field deployment feedback loops.
 * Used by FieldDiagnosticsCollector to emit diagnostic events to Prometheus.
 *
 * Thread-Safety: Schema types are value types; thread-safety is delegated to
 * the collector that holds them.
 *
 * PII Handling: Fields marked with [PII] should be sanitized before logging.
 */

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Portable UTC time helpers
// ---------------------------------------------------------------------------
namespace themis::observability::detail {

/**
 * @brief Thread-safe UTC breakdown of a `time_t` value.
 *
 * `std::gmtime()` returns a pointer to static storage that is not protected by
 * any lock, making it unsafe to call concurrently.  This wrapper uses the
 * POSIX `gmtime_r()` on non-Windows platforms, and the Windows-specific
 * `gmtime_s()` on MSVC/MinGW.
 *
 * @param tt Input POSIX timestamp.
 * @param result Output `std::tm` structure filled in UTC.
 * @return `true` on success; `false` if the platform call fails.
 */
inline bool gmtime_utc(std::time_t tt, std::tm& result) noexcept {
#ifdef _WIN32
    return ::gmtime_s(&result, &tt) == 0;
#else
    return ::gmtime_r(&tt, &result) != nullptr;
#endif
}

/**
 * @brief Inverse of `gmtime_utc()`: convert a UTC `std::tm` to `time_t`.
 *
 * `std::mktime()` interprets its argument as *local* time, which produces
 * incorrect results for UTC timestamps when the host timezone differs from UTC.
 * This wrapper uses POSIX `timegm()` on non-Windows platforms and the
 * Windows-specific `_mkgmtime()` on MSVC/MinGW.
 *
 * @param tm UTC-based `std::tm` structure.
 * @return Corresponding POSIX `time_t`, or `(time_t)-1` on failure.
 */
inline std::time_t mktime_utc(std::tm& tm) noexcept {
#ifdef _WIN32
    return ::_mkgmtime(&tm);
#else
    return ::timegm(&tm);
#endif
}

}  // namespace themis::observability::detail

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Diagnostic failure category enum.
 *
 * Categories are used to classify diagnostic events and route them to
 * appropriate analysis and alerting pipelines.
 */
enum class DiagnosticFailureCategory {
    /// NLI (Natural Language Inference) prediction failures or latency
    NLI_INFERENCE = 0,
    /// mTLS connection acquisition or SSL errors
    MTLS_CONNECTION = 1,
    /// Query execution timeout or slow query detection
    QUERY_TIMEOUT = 2,
    /// Shard routing or rebalancing failures
    SHARD_ROUTING = 3,
    /// Cache miss rate degradation or eviction issues
    CACHE_DEGRADATION = 4,
    /// Storage layer I/O errors or latency
    STORAGE_ERROR = 5,
    /// Internal RPC communication failures
    RPC_ERROR = 6,
    /// Memory pressure or resource exhaustion
    RESOURCE_PRESSURE = 7,
    /// Configuration or deployment issues
    CONFIG_ERROR = 8,
    /// Sentinel: unknown category
    UNKNOWN = 9
};

/**
 * @brief Severity level for diagnostic events.
 *
 * Used to filter and prioritize diagnostic events in observability backend.
 */
enum class DiagnosticSeverity {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief String representation of failure category.
 */
inline std::string failureCategoryToString(DiagnosticFailureCategory cat) {
    switch (cat) {
        case DiagnosticFailureCategory::NLI_INFERENCE:
            return "NLI_INFERENCE";
        case DiagnosticFailureCategory::MTLS_CONNECTION:
            return "MTLS_CONNECTION";
        case DiagnosticFailureCategory::QUERY_TIMEOUT:
            return "QUERY_TIMEOUT";
        case DiagnosticFailureCategory::SHARD_ROUTING:
            return "SHARD_ROUTING";
        case DiagnosticFailureCategory::CACHE_DEGRADATION:
            return "CACHE_DEGRADATION";
        case DiagnosticFailureCategory::STORAGE_ERROR:
            return "STORAGE_ERROR";
        case DiagnosticFailureCategory::RPC_ERROR:
            return "RPC_ERROR";
        case DiagnosticFailureCategory::RESOURCE_PRESSURE:
            return "RESOURCE_PRESSURE";
        case DiagnosticFailureCategory::CONFIG_ERROR:
            return "CONFIG_ERROR";
        case DiagnosticFailureCategory::UNKNOWN:
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief String representation of severity level.
 */
inline std::string severityToString(DiagnosticSeverity sev) {
    switch (sev) {
        case DiagnosticSeverity::DEBUG:
            return "DEBUG";
        case DiagnosticSeverity::INFO:
            return "INFO";
        case DiagnosticSeverity::WARN:
            return "WARN";
        case DiagnosticSeverity::ERROR:
            return "ERROR";
        case DiagnosticSeverity::CRITICAL:
            return "CRITICAL";
        default:
            return "INFO";
    }
}

/**
 * @brief Structured diagnostic event.
 *
 * Captures operational telemetry from field deployments for analysis and debugging.
 * All string fields should be sanitized before emission to prevent PII leaks.
 *
 * Example usage:
 * ```cpp
 * DiagnosticEvent evt{
 *     .timestamp = std::chrono::system_clock::now(),
 *     .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
 *     .module_name = "rag",
 *     .error_message = "ONNX model inference failed",
 *     .severity_level = DiagnosticSeverity::ERROR,
 *     .deployment_environment = "production",
 *     .version = "1.5.0",
 *     .stacktrace_hash = "abc123def456"
 * };
 * ```
 */
struct DiagnosticEvent {
    /// Timestamp when event occurred
    std::chrono::system_clock::time_point timestamp;

    /// Classification of failure/issue
    DiagnosticFailureCategory failure_category{DiagnosticFailureCategory::UNKNOWN};

    /// Module/component name (e.g., "rag", "sharding", "cache")
    std::string module_name;

    /// [PII] Sanitized error message (query strings, user IDs must be redacted)
    std::string error_message;

    /// Severity/importance level
    DiagnosticSeverity severity_level{DiagnosticSeverity::INFO};

    /// Deployment environment (e.g., "production", "staging", "development")
    std::string deployment_environment;

    /// Semantic version of themis binary (e.g., "1.5.0")
    std::string version;

    /// [PII] Abbreviated stack trace hash for correlation (SHA256 first 16 chars)
    std::string stacktrace_hash;

    /// Optional: number of users affected by this event
    int32_t affected_user_count{-1};

    /// Optional: request ID for tracing (UUID or trace ID)
    std::string request_id;

    /// Optional: additional context as key-value pairs (should be pre-sanitized)
    std::map<std::string, std::string> context_data;

    /**
     * @brief Serialize event to JSON for observability backend.
     *
     * Assumes all string fields have been pre-sanitized for PII.
     * The timestamp is serialized as an ISO 8601 UTC string with millisecond
     * precision (e.g. `"2026-07-19T10:30:00.123Z"`).
     *
     * Thread-safety: this method is safe to call concurrently; it does not
     * rely on `std::gmtime()` (which uses static storage).
     *
     * @return JSON object with event data
     */
    json toJson() const {
        json obj;
        
        // Timestamp as ISO 8601 UTC string.
        // Use thread-safe gmtime_utc() instead of std::gmtime().
        auto tt = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
        
        std::tm tm_utc = {};
        std::ostringstream oss;
        if (themis::observability::detail::gmtime_utc(tt, tm_utc)) {
            oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");
        } else {
            oss << "1970-01-01T00:00:00";
        }
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        
        obj["timestamp"] = oss.str();
        obj["failure_category"] = failureCategoryToString(failure_category);
        obj["module_name"] = module_name;
        obj["error_message"] = error_message;
        obj["severity_level"] = severityToString(severity_level);
        obj["deployment_environment"] = deployment_environment;
        obj["version"] = version;
        obj["stacktrace_hash"] = stacktrace_hash;
        
        if (affected_user_count >= 0) {
            obj["affected_user_count"] = affected_user_count;
        }
        
        if (!request_id.empty()) {
            obj["request_id"] = request_id;
        }
        
        if (!context_data.empty()) {
            json ctx;
            for (const auto& [k, v] : context_data) {
                ctx[k] = v;
            }
            obj["context_data"] = ctx;
        }
        
        return obj;
    }

    /**
     * @brief Return brief string representation for logging.
     *
     * @return Single-line summary of event
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "[" << failureCategoryToString(failure_category) << "] "
            << module_name << ": " << error_message
            << " (" << severityToString(severity_level) << ")";
        return oss.str();
    }
};

/**
 * @brief PII Field Mask List
 *
 * Fields that typically contain PII and should be automatically redacted
 * during diagnostic event sanitization.
 */
static constexpr const char* PII_MASK_FIELDS[] = {
    "query",
    "user_id",
    "user_email",
    "user_name",
    "user_",
    "email",
    "customer_id",
    "api_key",
    "token",
    "password",
    "session_id",
    "request_body",
    "response_body",
    "personal_data",
    "pii"
};

/**
 * @brief Check if a field name is in the PII mask list.
 *
 * @param field_name Name of field to check
 * @return true if field is PII-sensitive
 */
inline bool isPIIField(const std::string& field_name) {
    for (const auto* pii_field : PII_MASK_FIELDS) {
        if (field_name.find(pii_field) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Sanitize a diagnostic event by masking PII fields.
 *
 * @param event Event to sanitize (modified in-place)
 * @param mask_char Character to use for masking (default '*')
 * @return Reference to sanitized event
 */
inline DiagnosticEvent& sanitizePII(DiagnosticEvent& event, char mask_char = '*') {
    // Sanitize context data
    for (auto& [key, value] : event.context_data) {
        if (isPIIField(key) && !value.empty()) {
            // Replace middle part of value with asterisks
            if (value.length() > 4) {
                value = value.substr(0, 2) + std::string(value.length() - 4, mask_char)
                    + value.substr(value.length() - 2);
            } else {
                value = std::string(value.length(), mask_char);
            }
        }
    }
    return event;
}

}  // namespace observability
}  // namespace themis

// Enable JSON serialization with nlohmann
namespace nlohmann {
template <>
struct adl_serializer<themis::observability::DiagnosticEvent> {
    static void to_json(json& j, const themis::observability::DiagnosticEvent& evt) {
        j = evt.toJson();
    }
    static void from_json(const json& j, themis::observability::DiagnosticEvent& evt) {
        using namespace themis::observability;

        // Parse timestamp from ISO 8601 UTC string; fall back to epoch on failure.
        // Use mktime_utc() (wraps timegm / _mkgmtime) so the tm is interpreted
        // as UTC rather than the host local timezone.
        if (j.contains("timestamp")) {
            std::string ts_str;
            j.at("timestamp").get_to(ts_str);
            std::tm tm = {};
            std::istringstream ss(ts_str);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (!ss.fail()) {
                std::time_t tt = themis::observability::detail::mktime_utc(tm);
                if (tt != static_cast<std::time_t>(-1)) {
                    evt.timestamp = std::chrono::system_clock::from_time_t(tt);
                }
            }
        }

        // Parse failure_category from its string representation
        if (j.contains("failure_category")) {
            std::string cat;
            j.at("failure_category").get_to(cat);
            if (cat == "NLI_INFERENCE") {
              evt.failure_category = DiagnosticFailureCategory::NLI_INFERENCE;
            }
            else if (cat == "MTLS_CONNECTION") evt.failure_category = DiagnosticFailureCategory::MTLS_CONNECTION;
            else if (cat == "QUERY_TIMEOUT")   evt.failure_category = DiagnosticFailureCategory::QUERY_TIMEOUT;
            else if (cat == "SHARD_ROUTING")   evt.failure_category = DiagnosticFailureCategory::SHARD_ROUTING;
            else if (cat == "CACHE_DEGRADATION") evt.failure_category = DiagnosticFailureCategory::CACHE_DEGRADATION;
            else if (cat == "STORAGE_ERROR")   evt.failure_category = DiagnosticFailureCategory::STORAGE_ERROR;
            else if (cat == "RPC_ERROR")       evt.failure_category = DiagnosticFailureCategory::RPC_ERROR;
            else if (cat == "RESOURCE_PRESSURE") evt.failure_category = DiagnosticFailureCategory::RESOURCE_PRESSURE;
            else if (cat == "CONFIG_ERROR")    evt.failure_category = DiagnosticFailureCategory::CONFIG_ERROR;
            else                               evt.failure_category = DiagnosticFailureCategory::UNKNOWN;
        }

        if (j.contains("module_name")) {
          j.at("module_name").get_to(evt.module_name);
        }
        if (j.contains("error_message")) {
          j.at("error_message").get_to(evt.error_message);
        }

        // Parse severity_level from its string representation
        if (j.contains("severity_level")) {
            std::string sev;
            j.at("severity_level").get_to(sev);
            if (sev == "DEBUG") {
              evt.severity_level = DiagnosticSeverity::DEBUG;
            }
            else if (sev == "INFO")     evt.severity_level = DiagnosticSeverity::INFO;
            else if (sev == "WARN")     evt.severity_level = DiagnosticSeverity::WARN;
            else if (sev == "ERROR")    evt.severity_level = DiagnosticSeverity::ERROR;
            else if (sev == "CRITICAL") evt.severity_level = DiagnosticSeverity::CRITICAL;
            else                        evt.severity_level = DiagnosticSeverity::INFO;
        }

        if (j.contains("deployment_environment")) {
          j.at("deployment_environment").get_to(evt.deployment_environment);
        }
        if (j.contains("version")) {
          j.at("version").get_to(evt.version);
        }
        if (j.contains("stacktrace_hash")) {
          j.at("stacktrace_hash").get_to(evt.stacktrace_hash);
        }
        if (j.contains("affected_user_count")) {
          j.at("affected_user_count").get_to(evt.affected_user_count);
        }
        if (j.contains("request_id")) {
          j.at("request_id").get_to(evt.request_id);
        }

        if (j.contains("context_data") && j.at("context_data").is_object()) {
            for (auto& [k, v] : j.at("context_data").items()) {
                if (v.is_string()) {
                    evt.context_data[k] = v.get<std::string>();
                }
            }
        }
    }
};
}  // namespace nlohmann
