/**
 * @file policy_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include "governance/ccpa_rules.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Conflict detection result
struct PolicyConflict {
    std::string conflict_type;                         // "contradictory", "overlapping", "circular"
    std::string severity;                              // "critical", "high", "medium", "low"
    std::vector<std::string> affected_rules;
    std::string description;
    std::vector<std::string> resolution_suggestions;
    
    nlohmann::json toJson() const;
};

/// Effectiveness metrics for a rule
struct RuleEffectiveness {
    std::string rule_id;
    int hit_count = 0;                                 // How many times rule was applied
    bool is_unused = false;                            // Rule never applied
    double performance_impact_ms = 0.0;                // Evaluation time
    std::string effectiveness_rating;                  // "high", "medium", "low", "unused"
    
    nlohmann::json toJson() const;
};

/// Security best practice violation
struct SecurityViolation {
    std::string violation_type;                        // "overly_permissive", "weak_encryption", etc.
    std::string severity;                              // "critical", "high", "medium", "low"
    std::vector<std::string> affected_rules;
    std::string description;
    std::vector<std::string> recommendations;
    
    nlohmann::json toJson() const;
};

/// Validation report
struct ValidationReport {
    std::string report_id;
    std::int64_t generated_at;
    
    std::vector<PolicyConflict> conflicts;
    std::vector<SecurityViolation> violations;
    std::vector<RuleEffectiveness> effectiveness_metrics;
    
    bool has_critical_issues = false;
    int total_issues = 0;
    double validation_score = 0.0;  // 0-100
    
    nlohmann::json toJson() const;
};

/// Policy validator for conflict detection and optimization
class PolicyValidator {
public:
    PolicyValidator(std::shared_ptr<PolicyManager> policy_manager);
    
    /// Detect contradictory rules
    /// @return List of detected conflicts
    std::vector<PolicyConflict> detectConflicts() const;
    
    /// Detect overlapping permissions
    /// @return List of overlapping rule pairs
    std::vector<PolicyConflict> detectOverlappingPermissions() const;
    
    /// Detect circular dependencies
    /// @return List of circular dependency chains
    std::vector<PolicyConflict> detectCircularDependencies() const;
    
    /// Calculate effectiveness metrics for all rules
    /// @return Effectiveness metrics for each rule
    std::vector<RuleEffectiveness> calculateEffectiveness() const;
    
    /// Detect unused rules
    /// @return List of rules that are never applied
    std::vector<std::string> detectUnusedRules() const;
    
    /// Check security best practices
    /// @return List of security violations
    std::vector<SecurityViolation> checkSecurityBestPractices() const;
    
    /// Detect conflicts between CCPA/CPRA requirements and policy rules.
    ///
    /// Identifies policy rules whose retention or export settings may conflict
    /// with CCPA data subject rights (e.g., a HIPAA-mandated long retention
    /// rule that would prevent honoring a CCPA right-to-delete request).
    /// Intended to be called at policy load time.
    ///
    /// @return List of security violations describing each detected conflict.
    std::vector<SecurityViolation> detectCcpaConflicts() const;
    
    /// Validate current ruleset
    /// @return Comprehensive validation report
    ValidationReport validateRuleset() const;
    
    /// Validate a single rule
    /// @param rule Rule to validate
    /// @return Validation issues for this rule
    std::vector<std::string> validateSingleRule(const PolicyRule& rule) const;
    
    /// Record rule hit (for effectiveness tracking)
    /// @param rule_id Rule that was applied
    /// @param evaluation_time_ms Time taken to evaluate
    void recordRuleHit(const std::string& rule_id, double evaluation_time_ms = 0.0);
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    
    // Hit tracking for effectiveness
    mutable std::unordered_map<std::string, int> rule_hits_;
    mutable std::unordered_map<std::string, double> rule_eval_times_;
    
    /// Helper: Check if two rules contradict each other
    bool areContradictory(const PolicyRule& rule1, const PolicyRule& rule2) const;
    
    /// Helper: Check if rule follows security best practices
    bool followsSecurityBestPractices(const PolicyRule& rule) const;
    
    /// Helper: Calculate validation score
    double calculateValidationScore(const ValidationReport& report) const;
};

} // namespace governance
} // namespace themis
