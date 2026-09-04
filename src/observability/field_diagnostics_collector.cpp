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
 * @brief Return the process-wide singleton instance.
 *
 * Uses a function-local static to guarantee both lazy initialisation and
 * thread-safe construction (C++11 magic statics).
 *
 * @return Reference to the global `FieldDiagnosticsCollector`.
 */
FieldDiagnosticsCollector& FieldDiagnosticsCollector::getInstance() {
    static FieldDiagnosticsCollector instance;
    return instance;
}

/**
 * @brief Default constructor; initialises the collector with production-safe defaults.
 *
 * Sets `max_buffer_size = 1000`, enables PII masking, metrics emission, and
 * batching.  The collector is enabled immediately after construction.  Call
 * `configure()` to override these defaults before processing load begins.
 */
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

/**
 * @brief Destructor; flushes any pending batched events before teardown.
 *
 * Calls `flush()` to export buffered events.  Because the singleton lives
 * until process exit, this destructor runs during static destruction; all
 * other static objects it depends on (e.g. `MetricsCollector`) must still be
 * alive at that point.
 */
FieldDiagnosticsCollector::~FieldDiagnosticsCollector() {
    // Flush any pending events on shutdown
    flush();
}

/**
 * @brief Replace the entire collector configuration atomically.
 *
 * Acquires an exclusive write lock so that in-flight `emitWithPIIMasking()`
 * or `emitDiagnosticEvent()` calls that are concurrently reading config fields
 * under a shared lock will not observe a partially updated state.
 *
 * @param config New configuration to apply.
 */
void FieldDiagnosticsCollector::configure(const FieldDiagnosticsConfig& config) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    config_ = config;
}

/**
 * @brief Emit a diagnostic event after applying automatic PII masking.
 *
 * Snapshots the `enabled` and `enable_pii_masking` flags under a shared lock,
 * then (when masking is active) passes a copy of @p event through
 * `sanitizePII()` and increments the `pii_sanitizations_` counter before
 * forwarding to `emitDiagnosticEvent()`.
 *
 * @param event Diagnostic event that may contain PII-sensitive fields.
 *
 * @return `true` if the event was buffered; `false` if collection is disabled
 *         or the buffer rejected the event (e.g. `max_buffer_size == 0`).
 */
bool FieldDiagnosticsCollector::emitWithPIIMasking([[maybe_unused]] const DiagnosticEvent& event) {
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
        sanitizePII([[maybe_unused]] sanitized_event);
        pii_sanitizations_++;
    }

    return emitDiagnosticEvent([[maybe_unused]] sanitized_event);
}

/**
 * @brief Emit a pre-sanitized diagnostic event directly.
 *
 * Snapshots `enabled` and `enable_metrics_emission` under a shared lock, then
 * — if collection is active — buffers the event, increments
 * `total_events_emitted_`, optionally updates Prometheus metrics, and invokes
 * any registered emission callbacks.
 *
 * @param event Diagnostic event assumed to contain no unmasked PII.
 *
 * @return `true` if the event was buffered; `false` if disabled or the buffer
 *         was full and the event was dropped (increments `events_dropped_`).
 */
bool FieldDiagnosticsCollector::emitDiagnosticEvent([[maybe_unused]] const DiagnosticEvent& event) {
    bool enabled, metrics_enabled;
    {
        std::shared_lock<std::shared_mutex> lock(buffer_mu_);
        enabled = config_.enabled;
        metrics_enabled = config_.enable_metrics_emission;
    }

    if (!enabled) {
        return false;
    }

    if ([[maybe_unused]] !addEventToBuffer(event)) {
        events_dropped_++;
        return false;
    }

    total_events_emitted_++;

    // Update metrics
    if (metrics_enabled) {
        updateMetricsForEvent([[maybe_unused]] event);
    }

    // Invoke callbacks
    invokeCallbacks([[maybe_unused]] event);

    return true;
}

/**
 * @brief Insert an event into the ring buffer under an exclusive lock.
 *
 * When the buffer is already at capacity, the oldest event is evicted from
 * the front of the deque and `events_dropped_` is incremented before the new
 * event is appended.  If `max_buffer_size == 0`, the buffer is effectively
 * disabled and all events are dropped immediately.
 *
 * @param event Event to insert.
 * @return `true` if the event was inserted; `false` if the buffer is disabled
 *         (`max_buffer_size == 0`).
 */
bool FieldDiagnosticsCollector::addEventToBuffer([[maybe_unused]] const DiagnosticEvent& event) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);

    if (config_.max_buffer_size == 0) {
        return false;
    }

    // Buffer full: evict oldest event to make room; count the eviction as dropped
    if ([[maybe_unused]] event_buffer_.size() >= config_.max_buffer_size) {
        event_buffer_.pop_front();
        events_dropped_++;
    }

    event_buffer_.push_back([[maybe_unused]] event);
    return true;
}

/**
 * @brief Return all buffered events whose timestamps are ≥ @p since_timestamp.
 *
 * Acquires a shared lock for the duration of the scan so that the returned
 * snapshot is consistent with respect to concurrent writes.
 *
 * @param since_timestamp Lower-bound (inclusive) for event timestamps.
 * @return Vector of matching events (by value; the buffer is not modified).
 */
std::vector<DiagnosticEvent> FieldDiagnosticsCollector::getEventsSince(
    const std::chrono::system_clock::time_point& since_timestamp) const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    std::vector<DiagnosticEvent> result = {};

    for ([[maybe_unused]] const auto& evt : event_buffer_) {
        if (evt.timestamp >= since_timestamp) {
            result.push_back(evt);
        }
    }
    return result;
}

/**
 * @brief Return a snapshot of all buffered events.
 *
 * Acquires a shared lock and copies the entire internal deque into a vector.
 * The caller receives an independent copy; subsequent buffer modifications do
 * not affect the returned vector.
 *
 * @return All currently buffered `DiagnosticEvent` instances.
 */
std::vector<DiagnosticEvent> FieldDiagnosticsCollector::getAllEvents() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return std::vector<DiagnosticEvent>([[maybe_unused]] event_buffer_.begin(), event_buffer_.end());
}

/**
 * @brief Count buffered events grouped by `DiagnosticFailureCategory`.
 *
 * Acquires a shared lock and iterates the buffer once to build the frequency
 * map.  Useful for dashboard summaries or health-check endpoints.
 *
 * @return Map of `DiagnosticFailureCategory` → count for every category that
 *         has at least one buffered event.
 */
std::map<DiagnosticFailureCategory, size_t> 
FieldDiagnosticsCollector::getEventCountsByCategory() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    std::map<DiagnosticFailureCategory, size_t> counts = {};

    for ([[maybe_unused]] const auto& evt : event_buffer_) {
        counts[evt.failure_category]++;
    }
    return counts;
}

/**
 * @brief Discard all buffered events.
 *
 * Acquires an exclusive lock and clears the internal deque.  Does not reset
 * the `total_events_emitted_` or `events_dropped_` counters; those reflect
 * lifetime totals.  Useful for test teardown or explicit state resets.
 */
void FieldDiagnosticsCollector::clearBuffer() {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    event_buffer_.clear();
}

/**
 * @brief Return the current number of events in the buffer.
 *
 * Acquires a shared lock to read the deque size atomically with respect to
 * concurrent writes.
 *
 * @return Number of events currently held in the internal buffer.
 */
size_t FieldDiagnosticsCollector::getBufferSize() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return static_cast<int>(event_buffer_.size());
}

/**
 * @brief Enable or disable event collection at runtime.
 *
 * Acquires an exclusive lock before modifying `config_.enabled` so that
 * concurrent readers (which snapshot the flag under a shared lock) always see
 * a consistent value.
 *
 * @param enabled `true` to enable collection; `false` to suppress all
 *                incoming events (they will be silently discarded).
 */
void FieldDiagnosticsCollector::setEnabled([[maybe_unused]] bool enabled) {
    std::unique_lock<std::shared_mutex> lock(buffer_mu_);
    config_.enabled = enabled;
}

/**
 * @brief Return whether event collection is currently enabled.
 *
 * Acquires a shared lock to read `config_.enabled` without racing against
 * concurrent `setEnabled()` or `configure()` calls.
 *
 * @return `true` if the collector is accepting events.
 */
bool FieldDiagnosticsCollector::isEnabled() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);
    return config_.enabled;
}

/**
 * @brief Register a callback to be invoked synchronously on every successful emit.
 *
 * Callbacks are called in registration order from within `emitDiagnosticEvent()`
 * **after** the event has been written to the buffer.  Each callback receives a
 * const reference to the (already-sanitized) event.
 *
 * Exceptions thrown by a callback are silently swallowed to prevent cascade
 * failures in the emitting thread.  Keep callbacks fast to avoid adding latency
 * to the emit hot path.
 *
 * @param callback Invocable `void(const DiagnosticEvent&)` to register.
 */
void FieldDiagnosticsCollector::registerEmitCallback(
    std::function<void([[maybe_unused]] const DiagnosticEvent&)> callback) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mu_);
    emit_callbacks_.push_back([[maybe_unused]] callback);
}

/**
 * @brief Invoke all registered emission callbacks for @p event.
 *
 * Acquires the callback mutex for the duration of the loop so that
 * concurrent `registerEmitCallback()` calls cannot corrupt the callback
 * vector mid-iteration.  Exceptions from individual callbacks are caught
 * and suppressed to preserve isolation.
 *
 * @param event The event that was just buffered (already PII-sanitized).
 */
void FieldDiagnosticsCollector::invokeCallbacks([[maybe_unused]] const DiagnosticEvent& event) const {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mu_);
    for ([[maybe_unused]] const auto& cb : emit_callbacks_) {
        try {
            cb([[maybe_unused]] event);
        } catch (...) {
            // Suppress callback exceptions to prevent cascade failures
        }
    }
}

/**
 * @brief Serialise all buffered events to a JSON array.
 *
 * Acquires a shared lock and converts each buffered `DiagnosticEvent` to its
 * JSON representation via `DiagnosticEvent::toJson()`.  The returned array
 * is a snapshot; subsequent events are not included.
 *
 * @return `nlohmann::json` array containing one object per buffered event.
 */
nlohmann::json FieldDiagnosticsCollector::exportAsJSON() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mu_);

    nlohmann::json arr = nlohmann::json::array();
    for ([[maybe_unused]] const auto& evt : event_buffer_) {
        arr.push_back(evt.toJson());
    }
    return arr;
}

/**
 * @brief Force-flush any pending batched events to the configured backend.
 *
 * Currently a no-op placeholder: all buffered events are already accessible
 * via `getAllEvents()` / `exportAsJSON()`.  A future implementation will
 * trigger a synchronous async-export cycle and wait for acknowledgement from
 * the backend before returning.
 *
 * Call this before process shutdown or before taking a checkpoint to ensure
 * no events are lost.
 */
void FieldDiagnosticsCollector::flush() {
    // For now, just ensure all buffered events are accessible
    // In a real implementation, this would trigger async export
}

/**
 * @brief Return a JSON object summarising collector health and counters.
 *
 * Atomically reads lifetime counters (`total_events_emitted_`,
 * `events_dropped_`, `pii_sanitizations_`) and then snapshots the current
 * buffer size, configuration flags, and per-category event counts under a
 * single shared lock to avoid data races.
 *
 * @return `nlohmann::json` object with the following fields:
 *   - `total_events_emitted`   — lifetime count of accepted events.
 *   - `events_dropped`         — lifetime count of dropped events.
 *   - `pii_sanitizations`      — lifetime count of PII sanitization passes.
 *   - `current_buffer_size`    — number of events currently buffered.
 *   - `max_buffer_size`        — configured buffer capacity.
 *   - `enabled`                — current collection state.
 *   - `pii_masking_enabled`    — whether PII masking is active.
 *   - `metrics_emission_enabled` — whether Prometheus metrics are emitted.
 *   - `events_by_category`     — object mapping category name → count.
 */
nlohmann::json FieldDiagnosticsCollector::getStats() const {
    nlohmann::json stats;
    stats["total_events_emitted"] = total_events_emitted_.load();
    stats["events_dropped"] = events_dropped_.load();
    stats["pii_sanitizations"] = pii_sanitizations_.load();

    // Snapshot config and buffer size under lock to avoid data races
    size_t buf_size, max_buf;
    bool enabled, pii_masking, metrics_emit;
    {
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
 * @brief Update Prometheus metrics counters/gauges for an emitted event.
 *
 * Increments the `field_diagnostic_events_total` counter labelled with the
 * event's category, module name, and severity.  When `affected_user_count` is
 * non-negative, also sets the `field_diagnostic_affected_users` gauge for the
 * corresponding category/module combination.
 *
 * @param event The buffered event whose metrics should be recorded.
 */
void FieldDiagnosticsCollector::updateMetricsForEvent([[maybe_unused]] const DiagnosticEvent& event) {
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
    if ([[maybe_unused]] event.affected_user_count >= 0) {
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
