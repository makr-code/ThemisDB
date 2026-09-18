/**
 * @file field_diagnostics_collector.cpp
 * @brief Field Diagnostics Collector Implementation
 * @version 0.0.1
 *
 * Implementation of thread-safe diagnostic event collection for ThemisDB.
 */

#include "observability/field_diagnostics_collector.h"
#include "observability/metrics_collector.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace themis {
namespace observability {

/**
 * @brief Get Instance.
 * @return Return value.
 * @details Implements getInstance without additional internal calls.
 */
FieldDiagnosticsCollector& FieldDiagnosticsCollector::getInstance() {
    static FieldDiagnosticsCollector instance;
    return instance;
}

FieldDiagnosticsCollector::FieldDiagnosticsCollector() {
    // Default configuration
    config_.max_buffer_size = 1000;
    config_.enable_pii_masking = true;
    config_.enable_metrics_emission = true;
    config_.enable_batching = true;
    config_.batch_size = 100;
    config_.batch_flush_interval_ms = 5000;
    config_.enabled = true;
}

FieldDiagnosticsCollector::~FieldDiagnosticsCollector() {
    // Flush any pending events on shutdown
    flush();
}

/**
 * @brief Configure.
 * @param[in] config Input parameter.
 * @details Calls: lock().
 */
void FieldDiagnosticsCollector::configure(const FieldDiagnosticsConfig& config) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    config_ = config;
}

/**
 * @brief Emit With PIIMasking.
 * @param[in] event Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), sanitizePII(), emitDiagnosticEvent().
 */
bool FieldDiagnosticsCollector::emitWithPIIMasking(const DiagnosticEvent& event) {
    bool enabled, pii_masking;
    {
        std::shared_lock<std::shared_mutex> lock(buffer_mu_);
        enabled = config_.enabled;
        pii_masking = config_.enable_pii_masking;
    }

    if (!enabled) {
        return false;
    }

    DiagnosticEvent sanitized_event = event;
    
    if (pii_masking) {
        sanitizePII(sanitized_event);
        pii_sanitizations_++;
    }

    return emitDiagnosticEvent(sanitized_event);
}

/**
 * @brief Emit Diagnostic Event.
 * @param[in] event Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), addEventToBuffer(), updateMetricsForEvent(), invokeCallbacks().
 */
bool FieldDiagnosticsCollector::emitDiagnosticEvent(const DiagnosticEvent& event) {
    bool enabled, metrics_enabled;
    {
        std::shared_lock<std::shared_mutex> lock(buffer_mu_);
        enabled = config_.enabled;
        metrics_enabled = config_.enable_metrics_emission;
    }

    if (!enabled) {
        return false;
    }

    if (!addEventToBuffer(event)) {
        events_dropped_++;
        return false;
    }

    total_events_emitted_++;

    // Update metrics
    if (metrics_enabled) {
        updateMetricsForEvent(event);
    }

    // Invoke callbacks
    invokeCallbacks(event);

    return true;
}

/**
 * @brief Add Event To Buffer.
 * @param[in] event Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), size(), pop_front(), push_back().
 */
bool FieldDiagnosticsCollector::addEventToBuffer(const DiagnosticEvent& event) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);

    if (config_.max_buffer_size == 0) {
        return false;
    }

    // Buffer full: evict oldest event to make room; count the eviction as dropped
    if (event_buffer_.size() >= config_.max_buffer_size) {
        event_buffer_.pop_front();
        events_dropped_++;
    }

    event_buffer_.push_back(event);
    return true;
}

std::vector<DiagnosticEvent> FieldDiagnosticsCollector::getEventsSince(
    const std::chrono::system_clock::time_point& since_timestamp) const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    std::vector<DiagnosticEvent> result = {};

    for (const auto& evt : event_buffer_) {
        if (evt.timestamp >= since_timestamp) {
            result.push_back(evt);
        }
    }
    return result;
}

std::vector<DiagnosticEvent> FieldDiagnosticsCollector::getAllEvents() const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return std::vector<DiagnosticEvent>(event_buffer_.begin(), event_buffer_.end());
}

std::map<DiagnosticFailureCategory, size_t> 
FieldDiagnosticsCollector::getEventCountsByCategory() const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    std::map<DiagnosticFailureCategory, size_t> counts = {};

    for (const auto& evt : event_buffer_) {
        counts[evt.failure_category]++;
    }
    return counts;
}

/**
 * @brief Clear Buffer.
 * @details Calls: lock(), clear().
 */
void FieldDiagnosticsCollector::clearBuffer() {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    event_buffer_.clear();
}

size_t FieldDiagnosticsCollector::getBufferSize() const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return event_buffer_.size();
}

/**
 * @brief Set Enabled.
 * @param[in] enabled Input parameter.
 * @details Calls: lock().
 */
void FieldDiagnosticsCollector::setEnabled(bool enabled) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    config_.enabled = enabled;
}

bool FieldDiagnosticsCollector::isEnabled() const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return config_.enabled;
}

void FieldDiagnosticsCollector::registerEmitCallback(
    std::function<void(const DiagnosticEvent&)> callback) {
    /**
     * @brief Lock.
     * @param[in] callback_mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(callback_mu_);
    emit_callbacks_.push_back(callback);
}

void FieldDiagnosticsCollector::invokeCallbacks(const DiagnosticEvent& event) const {
    /**
     * @brief Lock.
     * @param[in] callback_mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(callback_mu_);
    for (const auto& cb : emit_callbacks_) {
        try {
            cb(event);
        } catch (...) {
            // Suppress callback exceptions to prevent cascade failures
        }
    }
}

nlohmann::json FieldDiagnosticsCollector::exportAsJSON() const {
    /**
     * @brief Lock.
     * @param[in] buffer_mu_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& evt : event_buffer_) {
        arr.push_back(evt.toJson());
    }
    return arr;
}

/**
 * @brief Flush.
 * @details Implements flush without additional internal calls.
 */
void FieldDiagnosticsCollector::flush() {
    // For now, just ensure all buffered events are accessible
    // In a real implementation, this would trigger async export
}

nlohmann::json FieldDiagnosticsCollector::getStats() const {
    nlohmann::json stats;
    stats["total_events_emitted"] = total_events_emitted_.load();
    stats["events_dropped"] = events_dropped_.load();
    stats["pii_sanitizations"] = pii_sanitizations_.load();

    // Snapshot config and buffer size under lock to avoid data races
    size_t buf_size, max_buf;
    bool enabled, pii_masking, metrics_emit;
    {
        /**
         * @brief Lock.
         * @param[in] buffer_mu_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lock(buffer_mu_);
        buf_size    = event_buffer_.size();
        max_buf     = config_.max_buffer_size;
        enabled     = config_.enabled;
        pii_masking = config_.enable_pii_masking;
        metrics_emit = config_.enable_metrics_emission;
    }

    stats["current_buffer_size"] = buf_size;
    stats["max_buffer_size"] = max_buf;
    stats["enabled"] = enabled;
    stats["pii_masking_enabled"] = pii_masking;
    stats["metrics_emission_enabled"] = metrics_emit;
    
    // Add category breakdown
    auto counts = getEventCountsByCategory();
    nlohmann::json category_counts;
    for (const auto& [cat, count] : counts) {
        category_counts[failureCategoryToString(cat)] = count;
    }
    stats["events_by_category"] = category_counts;
    
    return stats;
}

/**
 * @brief Update Metrics For Event.
 * @param[in] event Input parameter.
 * @details Calls: MetricsCollector::getInstance(), addCounter(), failureCategoryToString(), severityToString(), setGauge().
 */
void FieldDiagnosticsCollector::updateMetricsForEvent(const DiagnosticEvent& event) {
    auto& metrics = MetricsCollector::getInstance();

    metrics.addCounter(
        "field_diagnostic_events_total",
        1,
        {
            {"category", failureCategoryToString(event.failure_category)},
            {"module", event.module_name},
            {"severity", severityToString(event.severity_level)}
        });

    // Emit gauge for affected user count if present
    if (event.affected_user_count >= 0) {
        metrics.setGauge(
            "field_diagnostic_affected_users",
            static_cast<double>(event.affected_user_count),
            {
                {"category", failureCategoryToString(event.failure_category)},
                {"module", event.module_name}
            });
    }
}

}  // namespace observability
}  // namespace themis

