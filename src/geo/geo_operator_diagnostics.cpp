// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_operator_diagnostics.cpp
 * @brief Implementation of GeoOperatorDiagnostics.
 *
 * @see include/geo/geo_operator_diagnostics.h
 * @see src/geo/ROADMAP.md — Phase 2/3 Q4 2026
 */

#include "geo/geo_operator_diagnostics.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Recording
// ============================================================================

void GeoOperatorDiagnostics::recordIncident(
        std::string_view    incident_id,
        GeoIncidentSeverity severity,
        std::string_view    description,
        std::string_view    remediation,
        std::optional<GeoErrorCode> error_code) noexcept {
    try {
        GeoIncident inc{
            std::string(incident_id),
            severity,
            std::string(description),
            std::string(remediation),
            nowNs(),
            error_code
        };
        std::lock_guard<std::mutex> lock(mutex_);
        if (incidents_.size() >= kMaxIncidents) {
            incidents_.erase(incidents_.begin());
        }
        incidents_.push_back(std::move(inc));
        ++total_count_;
    } catch (...) {
        // Incident recording must never throw.
    }
}

void GeoOperatorDiagnostics::recordFromCallback(
        std::string_view incident_id,
        std::string_view description) noexcept {
    auto sev = severityFromId(incident_id);
    auto rem = remediationForId(incident_id);
    recordIncident(incident_id, sev, description, rem, std::nullopt);
}

// ============================================================================
// § 2  Querying
// ============================================================================

std::vector<GeoIncident> GeoOperatorDiagnostics::recentIncidents(
        std::size_t max_count) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (incidents_.empty()) return {};
    // Newest at back; return newest first.
    std::vector<GeoIncident> result(incidents_.rbegin(), incidents_.rend());
    if (max_count > 0 && result.size() > max_count) {
        result.resize(max_count);
    }
    return result;
}

std::vector<GeoIncident> GeoOperatorDiagnostics::incidentsBySeverity(
        GeoIncidentSeverity min_severity) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<GeoIncident> result;
    for (auto it = incidents_.rbegin(); it != incidents_.rend(); ++it) {
        if (static_cast<uint8_t>(it->severity) >= static_cast<uint8_t>(min_severity)) {
            result.push_back(*it);
        }
    }
    return result;
}

std::size_t GeoOperatorDiagnostics::countBySeverity(
        GeoIncidentSeverity severity) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<std::size_t>(
        std::count_if(incidents_.begin(), incidents_.end(),
            [severity](const GeoIncident& inc) {
                return inc.severity == severity;
            }));
}

bool GeoOperatorDiagnostics::hasCriticalIncidents() const noexcept {
    return countBySeverity(GeoIncidentSeverity::CRITICAL) > 0;
}

uint64_t GeoOperatorDiagnostics::totalIncidentCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_count_;
}

// ============================================================================
// § 3  Export
// ============================================================================

std::string GeoOperatorDiagnostics::formatSummary(
        std::size_t max_count) const noexcept {
    auto recent = recentIncidents(max_count);
    std::ostringstream oss;
    oss << "=== Geo Module Diagnostic Summary ===\n";
    oss << "Total incidents recorded: " << total_count_ << "\n";
    if (recent.empty()) {
        oss << "(no incidents)\n";
        return oss.str();
    }
    for (const auto& inc : recent) {
        oss << "[" << severityName(inc.severity) << "] "
            << inc.incident_id << ": " << inc.description;
        if (!inc.remediation.empty()) {
            oss << " | Remediation: " << inc.remediation;
        }
        oss << "\n";
    }
    return oss.str();
}

void GeoOperatorDiagnostics::clearIncidents() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    incidents_.clear();
}

// ============================================================================
// § 4  Internal Helpers
// ============================================================================

int64_t GeoOperatorDiagnostics::nowNs() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

GeoIncidentSeverity GeoOperatorDiagnostics::severityFromId(
        std::string_view id) noexcept {
    // Convention: suffix determines severity.
    auto has_suffix = [&](std::string_view suffix) {
        return id.size() >= suffix.size() &&
               id.substr(id.size() - suffix.size()) == suffix;
    };
    if (has_suffix("PERSISTENT") || has_suffix("CRITICAL")) {
        return GeoIncidentSeverity::CRITICAL;
    }
    if (has_suffix("DRIFT"))     return GeoIncidentSeverity::WARNING;
    if (has_suffix("FALLBACK"))  return GeoIncidentSeverity::WARNING;
    if (has_suffix("INVALID"))   return GeoIncidentSeverity::ERROR;
    if (has_suffix("ERROR"))     return GeoIncidentSeverity::ERROR;
    return GeoIncidentSeverity::INFO;
}

std::string GeoOperatorDiagnostics::remediationForId(
        std::string_view id) noexcept {
    if (id.find("DRIFT-PERSISTENT") != std::string_view::npos) {
        return "Recalibrate or reinitialize GPU backend, then call resetDriftCounter()";
    }
    if (id.find("DRIFT") != std::string_view::npos) {
        return "Monitor GPU/CPU parity; if persistent, reset and recalibrate GPU backend";
    }
    if (id.find("FALLBACK") != std::string_view::npos) {
        return "Check GPU health via DeviceDetector; verify GPU driver and memory";
    }
    if (id.find("INVALID") != std::string_view::npos) {
        return "Validate input geometry against RFC 7946 before submission";
    }
    return "Consult geo module runbook at docs/operations/geo/RUNBOOK.md";
}

} // namespace geo
} // namespace themis
