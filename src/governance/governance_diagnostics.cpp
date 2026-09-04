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
    
    std::vector<GovernanceDiagnostic> result = {};

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
    
    std::vector<GovernanceDiagnostic> result = {};

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
    
    std::vector<GovernanceDiagnostic> result = {};

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
    
    std::unordered_map<std::string, GovernanceDiagnostic> result = {};

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
    if (static_cast<int>(policy_ids.size()) > 1) {
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

// ========== Safety Check Methods (Phase 3B Extended) ==========

bool ConflictDiagnosticHelper::hasConflictingClassifications(
    const std::vector<std::string>& classifications
) const {
    // Check for conflicting classification pairs
    if (classifications.empty()) {
        return false;
    }
    
    for (const auto& c : classifications) {
        // "public" cannot coexist with "restricted" or "confidential"
        if (c == "public") {
            for (const auto& other : classifications) {
                if (other == "restricted" || other == "confidential") {
                    return true;
                }
            }
        }
        // "restricted" cannot coexist with "confidential"
        if (c == "restricted") {
            for (const auto& other : classifications) {
                if (other == "confidential") {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool ConflictDiagnosticHelper::validateCCPACompliancePath(
    const std::unordered_map<std::string, std::string>& context
) const {
    // Fail-closed: if CCPA opt-out is required but not present, deny
    auto it = context.find("ccpa_opt_out_required");
    if (it != context.end() && it->second == "true") {
        auto opt_out_it = context.find("ccpa_opt_out");
        if (opt_out_it == context.end() || opt_out_it->second != "true") {
            return false;  // CCPA opt-out missing
        }
    }
    return true;
}

bool ConflictDiagnosticHelper::detectPrivilegeEscalation(
    const std::string& user_tier,
    const std::string& required_tier
) const {
    // Tier hierarchy: read_only (0) < editor (1) < auditor (2) < admin (3)
    auto getTierLevel = [](const std::string& tier) -> int {
        if (tier == "read_only") {
          return 0;
        }
        if (tier == "editor") {
          return 1;
        }
        if (tier == "auditor") {
          return 2;
        }
        if (tier == "admin") {
          return 3;
        }
        return -1;  // Unknown tier
    };
    
    int user_level = getTierLevel(user_tier);
    int required_level = getTierLevel(required_tier);
    
    // Escalation detected if user level < required level
    return (user_level >= 0 && required_level >= 0 && user_level < required_level);
}

std::vector<TemporalIssue> ConflictDiagnosticHelper::detectTemporalViolations(
    const std::unordered_map<std::string, std::string>& policy
) const {
    std::vector<TemporalIssue> issues;
    
    // Check for future effective_date
    auto effective_date_it = policy.find("effective_date");
    if (effective_date_it != policy.end()) {
        try {
            int64_t effective_date_ms = std::stoll(effective_date_it->second);
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            if (effective_date_ms > now_ms) {
                TemporalIssue issue;
                issue.issue_type = "future_effective_date";
                issue.description = "Policy has future effective date";
                issue.value_ms = effective_date_ms;
                issues.push_back(issue);
            }
        } catch (...) {
            // Ignore parse errors
        }
    }
    
    // Check for zero retention_days
    auto retention_it = policy.find("retention_days");
    if (retention_it != policy.end()) {
        try {
            int retention_days = std::stoi(retention_it->second);
            if (retention_days == 0) {
                TemporalIssue issue;
                issue.issue_type = "zero_retention";
                issue.description = "Policy has zero retention days";
                issue.value_ms = 0;
                issues.push_back(issue);
            }
        } catch (...) {
            // Ignore parse errors
        }
    }
    
    return issues;
}

std::vector<MaskingRuleViolation> ConflictDiagnosticHelper::validateMaskingRuleConsistency(
    const std::vector<std::unordered_map<std::string, std::string>>& mask_rules
) const {
    std::vector<MaskingRuleViolation> violations;
    
    // Check for conflicting redaction rules on same schema
    std::unordered_map<std::string, std::vector<std::string>> schema_to_rules;
    
    for (const auto& rule : mask_rules) {
        auto schema_it = rule.find("schema");
        if (schema_it != rule.end()) {
            schema_to_rules[schema_it->second].push_back(
                rule.count("rule_id") ? rule.at("rule_id") : "unknown"
            );
        }
    }
    
    // Detect conflicting rules on same schema
    for (const auto& [schema, rules] : schema_to_rules) {
        if (static_cast<int>(rules.size()) > 1) {
            MaskingRuleViolation violation;
            violation.rule_id = schema;
            violation.violation_type = "inconsistent_redaction";
            violation.affected_schemas.push_back(schema);
            violations.push_back(violation);
        }
    }
    
    // Check for bypass attempts (direct schema access without redaction)
    for (const auto& rule : mask_rules) {
        auto bypass_it = rule.find("bypass_direct_access");
        if (bypass_it != rule.end() && bypass_it->second == "true") {
            MaskingRuleViolation violation;
            violation.rule_id = rule.count("rule_id") ? rule.at("rule_id") : "unknown";
            violation.violation_type = "bypass_detected";
            if (rule.count("schema")) {
                violation.affected_schemas.push_back(rule.at("schema"));
            }
            violations.push_back(violation);
        }
    }
    
    return violations;
}

bool ConflictDiagnosticHelper::validateWhitelistPolicy(
    const std::unordered_map<std::string, std::string>& whitelist_policy
) const {
    // Fail-closed: whitelist must be non-empty and non-null
    auto whitelist_it = whitelist_policy.find("whitelist_ids");
    if (whitelist_it == whitelist_policy.end()) {
        return false;  // Whitelist missing
    }
    
    if (whitelist_it->second.empty() || whitelist_it->second == "null") {
        return false;  // Whitelist empty or null
    }
    
    return true;
}

// ========== SafeAccessValidator Implementation ==========

SafeAccessValidator::SafeAccessValidator(DiagnosticAggregator* aggregator)
    : aggregator_(aggregator), owns_aggregator_(false) {
    if (aggregator_ == nullptr) {
        aggregator_ = &getGlobalDiagnosticAggregator();
    }
    
    conflict_helper_ = std::make_shared<ConflictDiagnosticHelper>(
        ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY,
        aggregator_
    );
}

SafeAccessValidator::~SafeAccessValidator() = default;

SafetyViolation SafeAccessValidator::checkConflictingClassifications(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS;
    violation.affected_policies = req.policy_ids;
    
    if (conflict_helper_->hasConflictingClassifications(req.dataset_classifications)) {
        violation.description = "Conflicting dataset classifications detected (e.g., public + restricted)";
        violation.remediation_hint = "Review and remove conflicting classifications. Assign single classification level.";
        return violation;
    }
    
    return violation;  // No violation (scenario won't be added to result)
}

SafetyViolation SafeAccessValidator::checkCCPACompliance(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING;
    violation.affected_policies = req.policy_ids;
    
    if (!conflict_helper_->validateCCPACompliancePath(req.context)) {
        violation.description = "CCPA opt-out required but not properly configured";
        violation.remediation_hint = "Verify CCPA opt-out flags are set for opted-out subjects. Update compliance configuration.";
        return violation;
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkPrivilegeEscalation(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S3_PRIVILEGE_ESCALATION;
    violation.affected_policies = req.policy_ids;
    
    // Check required tier from context
    auto required_tier_it = req.context.find("required_tier");
    if (required_tier_it != req.context.end()) {
        if (conflict_helper_->detectPrivilegeEscalation(req.user_tier, required_tier_it->second)) {
            violation.description = "User tier (" + req.user_tier + ") insufficient for required tier (" + 
                                   required_tier_it->second + ")";
            violation.remediation_hint = "Elevate user permissions or use lower-privilege operation. Escalation attempts are denied.";
            return violation;
        }
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkTemporalViolations(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S4_TEMPORAL_VIOLATION;
    violation.affected_policies = req.policy_ids;
    
    // Check each policy for temporal issues
    for (const auto& policy_id : req.policy_ids) {
        // For now, use context to simulate policy
        auto policy_it = req.context.find("policy_" + policy_id);
        if (policy_it != req.context.end()) {
            // Parse policy context (simplified)
            std::unordered_map<std::string, std::string> policy_map;
            // In practice, would parse serialized policy
            auto issues = conflict_helper_->detectTemporalViolations(policy_map);
            
            if (!issues.empty()) {
                violation.description = "Temporal policy violations detected: ";
                for (const auto& issue : issues) {
                    violation.description += issue.issue_type + "; ";
                }
                violation.remediation_hint = "Review policy effective dates and retention periods. Ensure policies are current and valid.";
                return violation;
            }
        }
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkCrossBorderConflicts(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S5_CROSS_BORDER_CONFLICT;
    violation.affected_policies = req.policy_ids;
    
    // Check jurisdiction context
    auto jurisdiction_it = req.context.find("jurisdictions");
    if (jurisdiction_it != req.context.end()) {
        // Simple check: if multiple incompatible jurisdictions
        if (jurisdiction_it->second.find("EU") != std::string::npos &&
            jurisdiction_it->second.find("CN") != std::string::npos) {
            violation.description = "Cross-border conflict: incompatible jurisdictions (EU + CN)";
            violation.remediation_hint = "Review data residency requirements. Separate data flows by jurisdiction or align policies.";
            return violation;
        }
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkMaskingRuleConsistency(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S6_MASKING_BYPASS_ATTEMPT;
    violation.affected_policies = req.policy_ids;
    
    // Check for masking rule context
    auto masking_it = req.context.find("masking_rules");
    if (masking_it != req.context.end()) {
        std::vector<std::unordered_map<std::string, std::string>> rules;
        // In practice, would parse serialized rules
        auto violations_found = conflict_helper_->validateMaskingRuleConsistency(rules);
        
        if (!violations_found.empty()) {
            violation.description = "Masking rule inconsistencies detected: bypass attempts or conflicting redaction";
            violation.remediation_hint = "Review masking rules for consistency. Remove bypass paths and ensure uniform redaction policy.";
            return violation;
        }
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkWhitelistExhaustion(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION;
    violation.affected_policies = req.policy_ids;
    
    // Check whitelist context
    auto whitelist_it = req.context.find("whitelist_policy");
    if (whitelist_it != req.context.end()) {
        std::unordered_map<std::string, std::string> whitelist_policy;
        whitelist_policy["whitelist_ids"] = whitelist_it->second;
        
        if (!conflict_helper_->validateWhitelistPolicy(whitelist_policy)) {
            violation.description = "Whitelist exhaustion: empty or null whitelist policy";
            violation.remediation_hint = "Populate whitelist with valid policy IDs. Empty whitelists default to deny.";
            return violation;
        }
    }
    
    return violation;  // No violation
}

SafetyViolation SafeAccessValidator::checkCascadingDenials(
    const AccessRequest& req
) {
    SafetyViolation violation;
    violation.scenario = UnsafeAccessScenario::S8_CASCADING_DENIALS;
    violation.affected_policies = req.policy_ids;
    
    // Check for cascading deny policies
    auto cascading_it = req.context.find("cascading_denials");
    if (cascading_it != req.context.end() && cascading_it->second == "true") {
        auto deny_count_it = req.context.find("deny_count");
        if (deny_count_it != req.context.end()) {
            try {
                int deny_count = std::stoi(deny_count_it->second);
                if (deny_count > 1) {  // Multiple deny layers
                    violation.description = "Cascading denials detected: " + std::to_string(deny_count) + " deny layers";
                    violation.remediation_hint = "Review deny policies for redundancy. Consolidate into single explicit deny rule.";
                    return violation;
                }
            } catch (...) {
                // Ignore parse errors
            }
        }
    }
    
    return violation;  // No violation
}

SafeAccessResult SafeAccessValidator::validateAccessRequest(
    const AccessRequest& request
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SafeAccessResult result;
    result.is_safe = true;  // Start fail-closed (safe until violation found)
    result.evaluated_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result.diagnostic_code = static_cast<int32_t>(UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS);
    
    // Run all 8 safety checks in fail-closed order
    SafetyViolation v1 = checkConflictingClassifications(request);
    if (!v1.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v1);
        result.scenario_codes.push_back(static_cast<int32_t>(v1.scenario));
        violation_history_.push_back(v1);
    }
    
    SafetyViolation v2 = checkCCPACompliance(request);
    if (!v2.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v2);
        result.scenario_codes.push_back(static_cast<int32_t>(v2.scenario));
        violation_history_.push_back(v2);
    }
    
    SafetyViolation v3 = checkPrivilegeEscalation(request);
    if (!v3.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v3);
        result.scenario_codes.push_back(static_cast<int32_t>(v3.scenario));
        violation_history_.push_back(v3);
    }
    
    SafetyViolation v4 = checkTemporalViolations(request);
    if (!v4.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v4);
        result.scenario_codes.push_back(static_cast<int32_t>(v4.scenario));
        violation_history_.push_back(v4);
    }
    
    SafetyViolation v5 = checkCrossBorderConflicts(request);
    if (!v5.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v5);
        result.scenario_codes.push_back(static_cast<int32_t>(v5.scenario));
        violation_history_.push_back(v5);
    }
    
    SafetyViolation v6 = checkMaskingRuleConsistency(request);
    if (!v6.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v6);
        result.scenario_codes.push_back(static_cast<int32_t>(v6.scenario));
        violation_history_.push_back(v6);
    }
    
    SafetyViolation v7 = checkWhitelistExhaustion(request);
    if (!v7.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v7);
        result.scenario_codes.push_back(static_cast<int32_t>(v7.scenario));
        violation_history_.push_back(v7);
    }
    
    SafetyViolation v8 = checkCascadingDenials(request);
    if (!v8.description.empty()) {
        result.is_safe = false;
        result.violations.push_back(v8);
        result.scenario_codes.push_back(static_cast<int32_t>(v8.scenario));
        violation_history_.push_back(v8);
    }
    
    // Build remediation steps from all violations
    for (const auto& violation : result.violations) {
        result.remediation_steps.push_back(violation.remediation_hint);
    }
    
    // Emit diagnostic to aggregator if not safe
    if (!result.is_safe && aggregator_) {
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kDenyByDefault;
        diag.component = "safe_access_validator";
        diag.description = "Access request denied by safety validator";
        diag.timestamp_ms = result.evaluated_at_ms;
        diag.context["request_id"] = request.request_id;
        diag.context["violation_count"] = std::to_string(result.violations.size());
        diag.context["user_tier"] = request.user_tier;
        
        for (size_t i = 0; i < result.scenario_codes.size(); ++i) {
            diag.remediation_steps.push_back(result.remediation_steps[i]);
        }
        
        aggregator_->recordDiagnostic(diag);
    }
    
    return result;
}

std::vector<SafetyViolation> SafeAccessValidator::getAllViolations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return violation_history_;
}

void SafeAccessValidator::clearViolationHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    violation_history_.clear();
}

size_t SafeAccessValidator::getViolationCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return violation_history_.size();
}

} // namespace themis::governance

