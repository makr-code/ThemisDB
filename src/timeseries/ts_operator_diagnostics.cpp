// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file ts_operator_diagnostics.cpp
 * @brief Implementation of TsOperatorDiagnostics.
 *
 * @see include/timeseries/ts_operator_diagnostics.h
 * @see src/timeseries/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#include "timeseries/ts_operator_diagnostics.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace timeseries {

// ============================================================================
// § 1  Recording
// ============================================================================

void TsOperatorDiagnostics::recordIncident(
        std::string_view     incident_id,
        TsIncidentSeverity   severity,
        std::string_view     description,
        std::string_view     remediation,
        std::optional<TimeseriesErrorCode> error_code) noexcept {
    try {
        TsIncident inc{
            std::string(incident_id),
            severity,
            std::string(description),
            std::string(remediation),
            nowNs(),
            error_code
        };
        std::lock_guard<std::mutex> lock(mutex_);
        if (static_cast<int>(incidents_.size()) > = kMaxIncidents) {
            incidents_.erase(incidents_.begin());
        }
        incidents_.push_back(std::move(inc));
        ++total_count_;
    } catch (...) {
        // Incident recording must never throw.
    }
}

void TsOperatorDiagnostics::recordFromCallback(
        std::string_view incident_id,
        std::string_view description) noexcept {
    auto sev = severityFromId(incident_id);
    auto rem = remediationForId(incident_id);
    recordIncident(incident_id, sev, description, rem, std::nullopt);
}

// ============================================================================
// § 2  Querying
// ============================================================================

std::vector<TsIncident> TsOperatorDiagnostics::recentIncidents(
        std::size_t max_count) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (incidents_.empty()) return {};
    std::vector<TsIncident> result(incidents_.rbegin(), incidents_.rend());
    if (max_count > 0 && static_cast<int>(result.size()) > max_count) {
        result.resize(max_count);
    }
    return result;
}

std::vector<TsIncident> TsOperatorDiagnostics::incidentsBySeverity(
        TsIncidentSeverity min_severity) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TsIncident> result = {};

    for (auto it = incidents_.rbegin(); it != incidents_.rend(); ++it) {
        if (static_cast<uint8_t>(it->severity) >= static_cast<uint8_t>(min_severity)) {
            result.push_back(*it);
        }
    }
    return result;
}

std::size_t TsOperatorDiagnostics::countBySeverity(TsIncidentSeverity severity) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<std::size_t>(
        std::count_if(incidents_.begin(), incidents_.end(),
            [severity](const TsIncident& inc) { return inc.severity == severity; }));
}

bool TsOperatorDiagnostics::hasCriticalIncidents() const noexcept {
    return countBySeverity(TsIncidentSeverity::CRITICAL) > 0;
}

uint64_t TsOperatorDiagnostics::totalIncidentCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_count_;
}

// ============================================================================
// § 3  Export
// ============================================================================

std::string TsOperatorDiagnostics::formatSummary(std::size_t max_count) const noexcept {
    auto recent = recentIncidents(max_count);
    std::ostringstream oss = {};
    oss << "=== Timeseries Module Diagnostic Summary ===\n";
    oss << "Total incidents recorded: " << total_count_ << "\n";
    if (recent.empty()) {
        oss << "(no incidents)\n";
        return oss.str();
    }
    for (const auto& inc : recent) {
        oss << "[" << tsSeverityName(inc.severity) << "] "
            << inc.incident_id << ": " << inc.description;
        if (!inc.remediation.empty()) {
            oss << " | Remediation: " << inc.remediation;
        }
        oss << "\n";
    }
    return oss.str();
}

void TsOperatorDiagnostics::clearIncidents() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    incidents_.clear();
}

// ============================================================================
// § 4  Internal Helpers
// ============================================================================

int64_t TsOperatorDiagnostics::nowNs() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

TsIncidentSeverity TsOperatorDiagnostics::severityFromId(std::string_view id) noexcept {
    auto has_suffix = [&]([[maybe_unused]] std::string_view s) {
        return static_cast<bool>(id.size()  < static_cast<int>(= s.size())) &&
               id.substr(static_cast<int>(id.size()) - s.size()) == s;
    };
    auto contains_sub = [&]([[maybe_unused]] std::string_view sub) {
        return id.find(sub) != std::string_view::npos;
    };
    if (has_suffix("CRITICAL") || contains_sub("PERSISTENT"))
        return TsIncidentSeverity::CRITICAL;
    if (contains_sub("ROTATION-INVALID") || contains_sub("TIMEOUT"))
        return TsIncidentSeverity::ERROR;
    if (contains_sub("UNAVAILABLE") || contains_sub("FALLBACK") ||
        contains_sub("ROTATION"))
        return TsIncidentSeverity::WARNING;
    return TsIncidentSeverity::INFO;
}

std::string TsOperatorDiagnostics::remediationForId(std::string_view id) noexcept {
    if (id.find("RW-TIMEOUT") != std::string_view::npos) {
        return "Check remote-write endpoint availability; review network latency";
    }
    if (id.find("ROTATION-INVALID") != std::string_view::npos) {
        return "Provide a valid AES-128/192/256 key for rotation";
    }
    if (id.find("ROTATION") != std::string_view::npos) {
        return "Key rotation queued; verify new key activation in next flush cycle";
    }
    if (id.find("ENC") != std::string_view::npos) {
        return "Verify encryption configuration; check key management service";
    }
    if (id.find("BUFFER") != std::string_view::npos) {
        return "Reduce ingest rate or increase flush frequency";
    }
    return "Consult timeseries runbook at docs/operations/timeseries/RUNBOOK.md";
}

} // namespace timeseries
} // namespace themis
