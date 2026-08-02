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
        d.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
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

DiagnosticAggregator& getGlobalDiagnosticAggregator() {
    static DiagnosticAggregator aggregator;
    return aggregator;
}

// ========== ConflictDiagnosticHelper Implementation ==========

ConflictDiagnosticHelper::ConflictDiagnosticHelper(
    ResolutionStrategy strategy,
    DiagnosticAggregator* aggregator
) : strategy_(strategy), aggregator_(aggregator) {
    if (aggregator_ == nullptr) {
        aggregator_ = &getGlobalDiagnosticAggregator();
        owns_aggregator_ = false;  // Don't delete global singleton
    }
}

ConflictDiagnosticHelper::ConflictDetectionResult 
ConflictDiagnosticHelper::detectConflict(
    const std::vector<std::string>& policy_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ConflictDetectionResult result;
    result.recommended_strategy = strategy_;
    result.diagnostic_code = static_cast<int32_t>(GovDiagnosticCode::kConflictDetected);
    
    // Detect conflicts between policy pairs
    // For Phase 3B, this is a stub implementation that checks policy_ids count
    if (policy_ids.size() > 1) {
        // Simple conflict detection: if multiple policies exist, potential conflict
        result.has_conflicts = true;
        
        for (size_t i = 0; i < policy_ids.size(); ++i) {
            for (size_t j = i + 1; j < policy_ids.size(); ++j) {
                result.conflicting_pairs.emplace_back(policy_ids[i], policy_ids[j]);
                result.descriptions.push_back(
                    "Potential conflict between policies: " + 
                    policy_ids[i] + " and " + policy_ids[j]
                );
            }
        }
    }
    
    return result;
}

void ConflictDiagnosticHelper::recordConflict(
    const ConflictDetectionResult& result,
    const std::unordered_map<std::string, std::string>& additional_context
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!result.has_conflicts || !aggregator_) {
        return;
    }
    
    // Record conflict history
    conflict_history_.push_back(result);
    
    // Create and emit diagnostic
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kConflictDetected;
    diag.component = "conflict_detector";
    diag.description = "Policy conflicts detected";
    diag.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Add conflict pairs to remediation steps
    for (const auto& pair : result.conflicting_pairs) {
        diag.remediation_steps.push_back(
            "Review conflict between policies: " + pair.first + " and " + pair.second
        );
    }
    
    // Add additional context
    diag.context = additional_context;
    diag.context["conflict_count"] = std::to_string(result.conflicting_pairs.size());
    diag.context["resolution_strategy"] = 
        (result.recommended_strategy == ResolutionStrategy::EXPLICIT_DENY) ? "EXPLICIT_DENY" : 
        (result.recommended_strategy == ResolutionStrategy::EXPLICIT_ALLOW) ? "EXPLICIT_ALLOW" :
        (result.recommended_strategy == ResolutionStrategy::FIRST_MATCH) ? "FIRST_MATCH" :
        (result.recommended_strategy == ResolutionStrategy::MOST_RESTRICTIVE) ? "MOST_RESTRICTIVE" :
        "WHITELIST";
    
    aggregator_->recordDiagnostic(diag);
}

std::vector<GovernanceDiagnostic> ConflictDiagnosticHelper::getConflictDiagnostics() const {
    if (!aggregator_) {
        return {};
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    return aggregator_->getDiagnosticsForCode(GovDiagnosticCode::kConflictDetected);
}

void ConflictDiagnosticHelper::clearConflictHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    conflict_history_.clear();
}

ConflictDiagnosticHelper::ResolutionStrategy 
ConflictDiagnosticHelper::getCurrentStrategy() const {
    return strategy_;
}

void ConflictDiagnosticHelper::setResolutionStrategy(ResolutionStrategy strategy) {
    strategy_ = strategy;
}

} // namespace themis::governance

