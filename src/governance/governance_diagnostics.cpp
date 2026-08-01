/**
 * @file governance_diagnostics.cpp
 * @brief Implementation of diagnostic aggregation for governance module.
 * @version 0.1.0
 */

#include "governance/governance_diagnostics.h"

#include <algorithm>
#include <chrono>

namespace themis::governance {

// ========== GovernanceDiagnostic Implementation ==========

nlohmann::json GovernanceDiagnostic::toJson() const {
    nlohmann::json j;
    j["code"] = static_cast<int32_t>(code);
    j["component"] = component;
    j["description"] = description;
    j["remediation_steps"] = remediation_steps;
    j["timestamp_ms"] = timestamp_ms;
    j["context"] = context;
    return j;
}

// ========== DiagnosticAggregator Implementation ==========

void DiagnosticAggregator::recordDiagnostic(const GovernanceDiagnostic& diag) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GovernanceDiagnostic d = diag;
    if (d.timestamp_ms == 0) {
        d.timestamp_ms = std::chrono::system_clock::now()
            .time_since_epoch()
            .count() / 1'000'000;  // Convert nanoseconds to milliseconds
    }
    diagnostics_.push_back(d);
}

std::vector<GovernanceDiagnostic> DiagnosticAggregator::getDiagnosticsForComponent(
    const std::string& component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GovernanceDiagnostic> result;
    for (const auto& d : diagnostics_) {
        if (d.component == component) {
            result.push_back(d);
        }
    }
    return result;
}

std::vector<GovernanceDiagnostic> DiagnosticAggregator::getDiagnosticsForCode(
    GovDiagnosticCode code) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GovernanceDiagnostic> result;
    for (const auto& d : diagnostics_) {
        if (d.code == code) {
            result.push_back(d);
        }
    }
    return result;
}

std::vector<GovernanceDiagnostic> DiagnosticAggregator::getDiagnosticsInTimeRange(
    int64_t start_ms, int64_t end_ms) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GovernanceDiagnostic> result;
    for (const auto& d : diagnostics_) {
        bool in_range = true;
        if (start_ms > 0 && d.timestamp_ms < start_ms) {
            in_range = false;
        }
        if (end_ms > 0 && d.timestamp_ms > end_ms) {
            in_range = false;
        }
        if (in_range) {
            result.push_back(d);
        }
    }
    
    // Sort by timestamp
    std::sort(result.begin(), result.end(), 
              [](const GovernanceDiagnostic& a, const GovernanceDiagnostic& b) {
                  return a.timestamp_ms < b.timestamp_ms;
              });
    
    return result;
}

std::unordered_map<std::string, GovernanceDiagnostic> 
DiagnosticAggregator::getLatestPerComponent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, GovernanceDiagnostic> result;
    for (const auto& d : diagnostics_) {
        auto it = result.find(d.component);
        if (it == result.end() || it->second.timestamp_ms < d.timestamp_ms) {
            result[d.component] = d;
        }
    }
    return result;
}

nlohmann::json DiagnosticAggregator::exportAsJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& d : diagnostics_) {
        arr.push_back(d.toJson());
    }
    return arr;
}

void DiagnosticAggregator::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics_.clear();
}

size_t DiagnosticAggregator::getTotalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return diagnostics_.size();
}

} // namespace themis::governance
