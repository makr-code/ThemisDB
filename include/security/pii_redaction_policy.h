/**
 * @file pii_redaction_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/pii_detector.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace themis {
namespace security {

/**
 * @brief PII Redaction Policy for Logs, Traces, and Metrics
 *
 * Central enforcement point that integrates PII detection with the observability
 * pipeline (Logger, Tracer::Span::setAttribute, MetricsCollector labels).
 *
 * Design goals:
 * - Zero-configuration default: picks up config/pii_patterns.yaml automatically.
 * - Enforce consistent redaction rules across all three telemetry channels.
 * - Thread-safe singleton so it can be called from anywhere without explicit
 *   dependency injection.
 *
 * Channels and behaviour:
 *
 * | Channel  | Method             | Default mode                                |
 * |----------|--------------------|---------------------------------------------|
 * | Logs     | redactForLog()     | Replace every detected PII span with masked |
 * |          |                    | form (e.g. "a***\@example.com").             |
 * | Traces   | redactAttributes() | Redact values in span attribute maps; keys  |
 * |          |                    | matching PII field hints are also redacted. |
 * | Metrics  | redactLabels()     | Same as Trace for Prometheus label maps.    |
 *
 * Usage:
 * @code
 * // Logs
 * std::string safe_msg = PIIRedactionPolicy::get().redactForLog(raw_message);
 * THEMIS_INFO("{}", safe_msg);
 *
 * // Traces
 * auto safe_attrs = PIIRedactionPolicy::get().redactAttributes(span_attrs);
 * span.setAttribute("user.info", safe_attrs["user.info"]);
 *
 * // Metrics
 * auto safe_labels = PIIRedactionPolicy::get().redactLabels(metric_labels);
 * MetricsCollector::getInstance().recordQuery(safe_labels["type"], latency, count);
 * @endcode
 *
 * Configuration:
 * The policy inherits all redaction modes from config/pii_patterns.yaml.
 * Set the THEMIS_PII_STRICT environment variable to "1" to force strict
 * (full-replace) redaction regardless of per-pattern configuration.
 */
class PIIRedactionPolicy {
public:
    /**
     * @brief Access the process-wide singleton instance.
     *
     * Lazily initialised on first call. The detector is created with the
     * default config path ("config/pii_patterns.yaml"); if the file is
     * absent the regex engine embedded defaults are used.
     */
    static PIIRedactionPolicy& get();

    // Non-copyable, non-movable singleton
    PIIRedactionPolicy(const PIIRedactionPolicy&) = delete;
    PIIRedactionPolicy& operator=(const PIIRedactionPolicy&) = delete;

    // -------------------------------------------------------------------------
    // Core redaction methods
    // -------------------------------------------------------------------------

    /**
     * @brief Redact PII from a plain-text log message.
     *
     * Scans @p message for all recognised PII types and replaces each match
     * with the masked form defined by the detector's redaction mode.
     *
     * @param message  Raw log line or message string.
     * @return A copy of @p message with all detected PII replaced.
     */
    std::string redactForLog(const std::string& message) const;

    /**
     * @brief Redact PII from OpenTelemetry / tracing span attribute map.
     *
     * Both the keys and values of the map are inspected:
     * - If a *key* matches a PII field-name hint (e.g. "email", "ssn"), its
     *   value is replaced with the appropriate mask.
     * - If a *value* contains inline PII the matching substrings are masked.
     *
     * @param attributes  Mutable copy of span attributes to sanitise.
     * @return Sanitised attribute map (same keys, redacted values where needed).
     */
    std::map<std::string, std::string> redactAttributes(
        const std::map<std::string, std::string>& attributes) const;

    /**
     * @brief Redact a single attribute key+value pair efficiently.
     *
     * Equivalent to `redactAttributes({{key, value}}).at(key)` but avoids
     * constructing a temporary map, making it suitable for per-call-site use
     * in hot paths such as `Tracer::Span::setAttribute`.
     *
     * @param key    Attribute key (checked against PII field-name hints).
     * @param value  Attribute value (scanned for inline PII).
     * @return Redacted value string.
     */
    std::string redactAttributeValue(const std::string& key,
                                     const std::string& value) const;

    /**
     * @brief Redact PII from Prometheus / metrics label map.
     *
     * Identical semantics to redactAttributes(); provided as a separate
     * entry-point so call-sites remain readable and policy enforcement can
     * be specialised in future (e.g. different masking token per channel).
     *
     * @param labels  Mutable copy of label map to sanitise.
     * @return Sanitised label map.
     */
    std::map<std::string, std::string> redactLabels(
        const std::map<std::string, std::string>& labels) const;

    // -------------------------------------------------------------------------
    // Policy management
    // -------------------------------------------------------------------------

    /**
     * @brief Reload the underlying PIIDetector from disk.
     *
     * Call after rotating pii_patterns.yaml at runtime.
     *
     * @param config_path  Optional path override; uses the constructor path
     *                     when empty.
     * @return true if reload succeeded.
     */
    bool reload(const std::string& config_path = "");

    /**
     * @brief Return true when strict mode is active.
     *
     * Strict mode forces full-replace ("****") for every PII type regardless
     * of the per-pattern "partial" configuration.
     * Enabled by setting THEMIS_PII_STRICT=1 before process start.
     */
    bool isStrictMode() const;

    /**
     * @brief Override strict mode programmatically (e.g., for tests).
     */
    void setStrictMode(bool strict);

private:
    PIIRedactionPolicy();

    /**
     * @brief Replace PII occurrences in @p text according to policy.
     *
     * Internal helper shared by redactForLog(), redactAttributes(), and
     * redactLabels().
     */
    std::string applyRedaction(const std::string& text) const;

    mutable std::mutex mutex_;
    std::shared_ptr<themis::utils::PIIDetector> detector_;
    bool strict_mode_;
};

} // namespace security
} // namespace themis
