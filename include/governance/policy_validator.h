/**
 * @file policy_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct PolicyConflict {
    std::string conflict_type;                         // "contradictory", "overlapping", "circular"
    std::string severity;                              // "critical", "high", "medium", "low"
    std::vector<std::string> affected_rules;
    std::string description;
    std::vector<std::string> resolution_suggestions;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct RuleEffectiveness {
    std::string rule_id;
    int hit_count = 0;                                 // How many times rule was applied
    bool is_unused = false;                            // Rule never applied
    double performance_impact_ms = 0.0;                // Evaluation time
    std::string effectiveness_rating;                  // "high", "medium", "low", "unused"
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct SecurityViolation {
    std::string violation_type;                        // "overly_permissive", "weak_encryption", etc.
    std::string severity;                              // "critical", "high", "medium", "low"
    std::vector<std::string> affected_rules;
    std::string description;
    std::vector<std::string> recommendations;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct ValidationReport {
    std::string report_id;
    std::int64_t generated_at;
    
    std::vector<PolicyConflict> conflicts;
    std::vector<SecurityViolation> violations;
    std::vector<RuleEffectiveness> effectiveness_metrics;
    
    bool has_critical_issues = false;
    int total_issues = 0;
    double validation_score = 0.0;  // 0-100
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class PolicyValidator {
public:
    PolicyValidator(std::shared_ptr<PolicyManager> policy_manager);
    
    /**
     * @brief Detect Conflicts.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectConflicts() const;
    
    /**
     * @brief Detect Overlapping Permissions.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectOverlappingPermissions() const;
    
    /**
     * @brief Detect Circular Dependencies.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectCircularDependencies() const;
    
    /**
     * @brief Calculate Effectiveness.
     * @return Return value.
     */
    std::vector<RuleEffectiveness> calculateEffectiveness() const;
    
    /**
     * @brief Detect Unused Rules.
     * @return Return value.
     */
    std::vector<std::string> detectUnusedRules() const;
    
    /**
     * @brief Check Security Best Practices.
     * @return Return value.
     */
    std::vector<SecurityViolation> checkSecurityBestPractices() const;
    
    /**
     * @brief Detect Ccpa Conflicts.
     * @return Return value.
     */
    std::vector<SecurityViolation> detectCcpaConflicts() const;
    
    /**
     * @brief Validate Ruleset.
     * @return Return value.
     */
    ValidationReport validateRuleset() const;
    
    /**
     * @brief Validate Single Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<std::string> validateSingleRule(const PolicyRule& rule) const;
    
    void recordRuleHit(const std::string& rule_id, double evaluation_time_ms = 0.0);
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    
    // Hit tracking for effectiveness
    mutable std::unordered_map<std::string, int> rule_hits_;
    mutable std::unordered_map<std::string, double> rule_eval_times_;
    
    /**
     * @brief Are Contradictory.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool areContradictory(const PolicyRule& rule1, const PolicyRule& rule2) const;
    
    /**
     * @brief Follows Security Best Practices.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool followsSecurityBestPractices(const PolicyRule& rule) const;
    
    /**
     * @brief Calculate Validation Score.
     * @param[in] report Input parameter.
     * @return Return value.
     */
    double calculateValidationScore(const ValidationReport& report) const;
};

} // namespace governance
} // namespace themis
