/**
 * @file field_diagnostics_collector.h
 * @brief Field Diagnostics Collector - Thread-Safe Event Collection
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Gap Summary: total=0; Stub=0, Unimpl=0, Mock=0, Sim=0
 *
 * Central collection point for structured diagnostic events from all ThemisDB modules.
 * Handles thread-safe buffering, PII masking, and integration with Prometheus metrics.
 *
 * Design:
 * - Singleton pattern for process-wide collection
 * - Lock-free circular buffer with std::shared_mutex for efficiency
 * - Automatic PII masking before emission
 * - Optional async batch export to observability backend
 * - <1% CPU overhead target
 *
 * Usage:
 * ```cpp
 * auto& collector = FieldDiagnosticsCollector::getInstance();
 * 
 * DiagnosticEvent evt{
 *     .timestamp = std::chrono::system_clock::now(),
 *     .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
 *     .module_name = "rag",
 *     .error_message = "Model inference failed",
 *     .severity_level = DiagnosticSeverity::ERROR,
 *     .deployment_environment = "production",
 *     .version = "1.5.0"
 * };
 * 
 * collector.emitWithPIIMasking(evt);
 * ```
 */

#pragma once

#include "utils/field_diagnostics_schema.h"
#include <deque>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>

namespace themis {
namespace observability {

/**
 * @brief Configuration for FieldDiagnosticsCollector
 */
struct FieldDiagnosticsConfig {
    /// Maximum number of events to buffer in memory
    size_t max_buffer_size{1000};

    /// Enable automatic PII masking (default: true)
    bool enable_pii_masking{true};

    /// Enable Prometheus metrics emission (default: true)
    bool enable_metrics_emission{true};

    /// Batch events for export (default: true)
    bool enable_batching{true};

    /// Maximum events per batch before export
    size_t batch_size{100};

    /// Interval to flush batched events (milliseconds)
    int64_t batch_flush_interval_ms{5000};

    /// Enable collection (can be toggled at runtime)
    bool enabled{true};
};

/**
 * @brief Central diagnostic event collector for field deployments.
 *
 * Thread-safe singleton that collects diagnostic events from all modules
 * and exposes them for analysis and observability integration.
 *
 * Key features:
 * - Thread-safe concurrent event emission from multiple components
 * - Automatic PII masking before storage/emission
 * - Buffering with configurable max size
 * - Integration with Prometheus metrics
 * - Lightweight (<1% overhead)
 */
class FieldDiagnosticsCollector {
public:
    /**
     * @brief Get process-wide singleton instance.
     *
     * @return Reference to global collector instance
     */
    static FieldDiagnosticsCollector& getInstance();

    // Deletion of copy/move to enforce singleton
    FieldDiagnosticsCollector(const FieldDiagnosticsCollector&) = delete;
    FieldDiagnosticsCollector& operator=(const FieldDiagnosticsCollector&) = delete;
    FieldDiagnosticsCollector(FieldDiagnosticsCollector&&) = delete;
    FieldDiagnosticsCollector& operator=(FieldDiagnosticsCollector&&) = delete;

    /**
     * @brief Configure the collector.
     *
     * Should be called early in initialization (before first event emission).
     * Thread-safe but best called before production load begins.
     *
     * @param config Configuration parameters
     */
    void configure(const FieldDiagnosticsConfig& config);

    /**
     * @brief Emit a diagnostic event with automatic PII masking.
     *
     * This is the primary interface for modules to report diagnostics.
     * The event is automatically sanitized before storage.
     *
     * @param event Diagnostic event to emit
     * @return true if event was accepted and buffered, false if disabled or buffer full
     */
    bool emitWithPIIMasking(const DiagnosticEvent& event);

    /**
     * @brief Emit a diagnostic event directly (assumes pre-sanitized).
     *
     * Use this when you've already applied PII masking or confirmed
     * the event contains no sensitive data.
     *
     * @param event Pre-sanitized diagnostic event
     * @return true if event was accepted, false if disabled or buffer full
     */
    bool emitDiagnosticEvent(const DiagnosticEvent& event);

    /**
     * @brief Retrieve diagnostic events since a given timestamp.
     *
     * Useful for ad-hoc analysis or exporting to external systems.
     * Returns a snapshot; buffer may be modified concurrently.
     *
     * @param since_timestamp Only include events after this time
     * @return Vector of matching events (copy, not reference)
     */
    std::vector<DiagnosticEvent> getEventsSince(
        const std::chrono::system_clock::time_point& since_timestamp) const;

    /**
     * @brief Get all currently buffered events.
     *
     * @return Vector of all events in buffer
     */
    std::vector<DiagnosticEvent> getAllEvents() const;

    /**
     * @brief Get count of events by failure category.
     *
     * Useful for summary statistics.
     *
     * @return Map of category -> count
     */
    std::map<DiagnosticFailureCategory, size_t> getEventCountsByCategory() const;

    /**
     * @brief Clear all buffered events.
     *
     * Useful for testing or resetting state. Thread-safe.
     */
    void clearBuffer();

    /**
     * @brief Get current buffer size.
     *
     * @return Number of events currently buffered
     */
    size_t getBufferSize() const;

    /**
     * @brief Enable or disable event collection.
     *
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if collection is currently enabled.
     *
     * @return true if enabled
     */
    bool isEnabled() const;

    /**
     * @brief Register a callback to be invoked on event emission.
     *
     * Useful for integration with external observability systems.
     * Callback is invoked synchronously during emit; keep it fast.
     *
     * @param callback Function to call with each emitted event
     */
    void registerEmitCallback(std::function<void(const DiagnosticEvent&)> callback);

    /**
     * @brief Export all events as JSON array.
     *
     * @return JSON array of all buffered events
     */
    nlohmann::json exportAsJSON() const;

    /**
     * @brief Force flush of any pending batches.
     *
     * Useful before shutdown or when synchronous export is needed.
     */
    void flush();

    /**
     * @brief Get collector statistics.
     *
     * @return JSON object with stats (total emitted, buffered, dropped, etc.)
     */
    nlohmann::json getStats() const;

private:
    // Private constructor for singleton
    FieldDiagnosticsCollector();
    ~FieldDiagnosticsCollector();

    // Configuration
    FieldDiagnosticsConfig config_;

    // Event buffer with thread-safe access
    mutable std::shared_mutex buffer_mu_;
    std::deque<DiagnosticEvent> event_buffer_;

    // Metrics counters
    std::atomic<uint64_t> total_events_emitted_{0};
    std::atomic<uint64_t> events_dropped_{0};
    std::atomic<uint64_t> pii_sanitizations_{0};

    // Optional callbacks
    mutable std::mutex callback_mu_;
    std::vector<std::function<void(const DiagnosticEvent&)>> emit_callbacks_;

    // Helper: add event to buffer
    bool addEventToBuffer(const DiagnosticEvent& event);

    // Helper: invoke emission callbacks
    void invokeCallbacks(const DiagnosticEvent& event) const;

    // Helper: update metrics based on event
    void updateMetricsForEvent(const DiagnosticEvent& event);
};

}  // namespace observability
}  // namespace themis
