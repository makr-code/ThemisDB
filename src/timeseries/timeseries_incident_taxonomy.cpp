/**
 * @file timeseries_incident_taxonomy.cpp
 * @brief Implementation of unified incident taxonomy and emission.
 *
 * Provides:
 *   - Global incident handler registration
 *   - Bounded-latency incident emission
 *   - Incident metadata construction helpers
 *
 * @see include/timeseries/timeseries_incident_taxonomy.h
 */

#include "timeseries/timeseries_incident_taxonomy.h"
#include "utils/logger.h"

#include <atomic>
#include <cstring>

namespace themis {
namespace timeseries {

// ============================================================================
// Global incident handler
// ============================================================================

/// Global incident handler pointer (relaxed atomicity, updated rarely).
static std::atomic<IncidentHandler> g_incident_handler{nullptr};

void setIncidentHandler([[maybe_unused]] IncidentHandler handler) noexcept {
    g_incident_handler.store(handler, std::memory_order_release);
}

IncidentHandler getIncidentHandler() noexcept {
    return g_incident_handler.load([[maybe_unused]] std::memory_order_acquire);
}

void emitIncident(const Incident& incident) noexcept {
    auto handler = g_incident_handler.load([[maybe_unused]] std::memory_order_acquire);
    if ([[maybe_unused]] handler) {
        try {
            handler([[maybe_unused]] incident);
        } catch (...) {
            // Swallow exceptions from handler to prevent cascade failures.
            // Log at DEBUG level only to avoid spam.
            THEMIS_DEBUG([[maybe_unused]] "Incident handler exception caught (swallowed)");
        }
    }

    // Always log incident at appropriate level for observability.
    switch (incident.severity) {
        case IncidentSeverity::CRITICAL:
            THEMIS_ERROR("Timeseries CRITICAL incident: class={} severity={} http_status={} "
                         "retries={} series={} hint={} tag={}",
                         static_cast<int>(incident.incident_class),
                         severityName(incident.severity),
                         incident.http_status,
                         incident.retry_count,
                         incident.context.series_id,
                         incident.context.recovery_hint,
                         incident.context.caller_tag);
            break;

        case IncidentSeverity::ERROR:
            THEMIS_ERROR("Timeseries ERROR incident: class={} severity={} http_status={} "
                         "retries={} series={} hint={} tag={}",
                         static_cast<int>(incident.incident_class),
                         severityName(incident.severity),
                         incident.http_status,
                         incident.retry_count,
                         incident.context.series_id,
                         incident.context.recovery_hint,
                         incident.context.caller_tag);
            break;

        case IncidentSeverity::WARN:
            THEMIS_WARN("Timeseries WARN incident: class={} severity={} http_status={} "
                        "retries={} series={} hint={} tag={}",
                        static_cast<int>(incident.incident_class),
                        severityName(incident.severity),
                        incident.http_status,
                        incident.retry_count,
                        incident.context.series_id,
                        incident.context.recovery_hint,
                        incident.context.caller_tag);
            break;

        case IncidentSeverity::INFO:
            THEMIS_INFO("Timeseries INFO incident: class={} severity={} http_status={} "
                        "retries={} series={} hint={} tag={}",
                        static_cast<int>(incident.incident_class),
                        severityName(incident.severity),
                        incident.http_status,
                        incident.retry_count,
                        incident.context.series_id,
                        incident.context.recovery_hint,
                        incident.context.caller_tag);
            break;
    }
}

} // namespace timeseries
} // namespace themis
