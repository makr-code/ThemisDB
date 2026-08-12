/**
 * @file policy_validation.h
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
#include "governance/pci_dss_rules.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// PolicyValidator performs automated validation of policy rules
class PolicyValidator {
public:
    struct ConflictResult {
        std::string conflict_id;
        std::string conflict_type;                 // contradictory, overlapping, circular
        std::vector<std::string> conflicting_rule_ids;
        std::string description;
        std::string severity;                      // low, medium, high, critical
        std::string recommendation;
        
        nlohmann::json toJson() const;
    };
    
    struct EffectivenessMetrics {
        std::string rule_id;
        int hit_count = 0;                         // Number of times rule was evaluated
        int64_t last_used = 0;                     // Last time rule was used (timestamp)
        int64_t created_at = 0;                    // When rule was created
        int days_since_last_use = 0;               // Days since last use
        bool is_unused = false;                    // True if never used
        double effectiveness_score = 0.0;          // 0-100 score
        
        nlohmann::json toJson() const;
    };
    
    struct SecurityCheckResult {
        std::string check_id;
        std::string check_type;                    // overly_permissive, missing_encryption, missing_audit, etc.
        std::string rule_id;
        std::string severity;                      // low, medium, high, critical
        std::string description;
        std::string recommendation;
        bool passed = true;
        
        nlohmann::json toJson() const;
    };
    
    struct ValidationReport {
        int total_rules_checked = 0;
        int conflicts_found = 0;
        int security_issues_found = 0;
        int effectiveness_issues_found = 0;
        std::vector<ConflictResult> conflicts;
        std::vector<SecurityCheckResult> security_checks;
        std::vector<std::string> recommendations;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
    };
    
    /// Detect conflicts between rules
    std::vector<ConflictResult> detectConflicts(const PolicyManager& policy_mgr) const;
    
    /// Check for contradictory rules (same resource/action but different effects)
    std::vector<ConflictResult> detectContradictoryRules(const PolicyManager& policy_mgr) const;
    
    /// Check for overlapping permissions
    std::vector<ConflictResult> detectOverlappingPermissions(const PolicyManager& policy_mgr) const;
    
    /// Check for circular dependencies
    std::vector<ConflictResult> detectCircularDependencies(const PolicyManager& policy_mgr) const;

    /// Detect conflicts between CCPA and HIPAA rules.
    /// HIPAA mandates disclosure/audit requirements that can conflict with
    /// CCPA right-to-delete and opt-out-of-sale obligations.
    /// Integrates the CcpaRuleSet::detectHipaaConflicts() evaluator.
    /// @return List of ConflictResult entries, one per detected cross-framework conflict.
    std::vector<ConflictResult> detectCcpaHipaaConflicts(const PolicyManager& policy_mgr) const;

    /// Detect conflicts between PCI-DSS and GDPR requirements for a policy rule set.
    ///
    /// PCI-DSS Req 10.7 mandates 12-month audit-log retention while GDPR Art. 5(1)(e)
    /// (storage limitation) pushes for minimal retention.  Additionally, PCI-DSS Req 4
    /// forbids unencrypted export while GDPR Art. 32 requires appropriate technical
    /// measures — rules with allow_export=true and require_encryption=false violate both
    /// simultaneously.  Integrates the PciDssRuleSet::detectGdprConflicts() evaluator.
    ///
    /// @return List of ConflictResult entries, one per detected cross-framework conflict.
    std::vector<ConflictResult> detectPciDssGdprConflicts(const PolicyManager& policy_mgr) const;
    
    /// Calculate effectiveness metrics for all rules
    std::unordered_map<std::string, EffectivenessMetrics> calculateEffectiveness(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {}
    ) const;
    
    /// Identify unused rules
    std::vector<std::string> identifyUnusedRules(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {},
        int min_days_unused = 30
    ) const;
    
    /// Perform security best practices checks
    std::vector<SecurityCheckResult> performSecurityChecks(const PolicyManager& policy_mgr) const;
    
    /// Check for overly permissive rules
    std::vector<SecurityCheckResult> checkOverlyPermissive(const PolicyManager& policy_mgr) const;
    
    /// Validate encryption requirements
    std::vector<SecurityCheckResult> checkEncryptionRequirements(const PolicyManager& policy_mgr) const;
    
    /// Verify audit logging is enabled
    std::vector<SecurityCheckResult> checkAuditLogging(const PolicyManager& policy_mgr) const;
    
    /// Check retention period compliance
    std::vector<SecurityCheckResult> checkRetentionCompliance(
        const PolicyManager& policy_mgr,
        int min_retention_days = 90
    ) const;
    
    /// Generate comprehensive validation report
    ValidationReport generateValidationReport(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {}
    ) const;
    
    /// Validate a single rule against best practices
    std::vector<SecurityCheckResult> validateSingleRule(const PolicyRule& rule) const;
};

/// PolicyMetricsCollector tracks policy usage and performance
class PolicyMetricsCollector {
public:
    struct RuleMetrics {
        std::string rule_id;
        int evaluation_count = 0;                  // Times evaluated
        int match_count = 0;                       // Times matched
        int64_t total_evaluation_time_us = 0;      // Total evaluation time in microseconds
        int64_t avg_evaluation_time_us = 0;        // Average evaluation time
        int64_t last_evaluation_time = 0;          // Last evaluation timestamp
        double match_rate = 0.0;                   // Percentage of evaluations that matched
        
        nlohmann::json toJson() const;
    };
    
    struct PerformanceImpact {
        std::string rule_id;
        int64_t avg_evaluation_time_us = 0;
        std::string performance_category;          // fast, normal, slow, critical
        std::string impact_description;
        std::string optimization_suggestion;
        
        nlohmann::json toJson() const;
    };
    
    /// Record a rule evaluation
    void recordEvaluation(const std::string& rule_id, bool matched, int64_t evaluation_time_us);
    
    /// Get metrics for a specific rule
    std::optional<RuleMetrics> getRuleMetrics(const std::string& rule_id) const;
    
    /// Get metrics for all rules
    std::unordered_map<std::string, RuleMetrics> getAllMetrics() const;
    
    /// Analyze performance impact
    std::vector<PerformanceImpact> analyzePerformanceImpact() const;
    
    /// Get rules with slow performance
    std::vector<std::string> getSlowRules(int64_t threshold_us = 1000) const;
    
    /// Export metrics as JSON
    nlohmann::json exportMetrics() const;
    
    /// Import metrics from JSON
    bool importMetrics(const nlohmann::json& j);
    
    /// Reset all metrics
    void resetMetrics();
    
    /// Reset metrics for a specific rule
    void resetRuleMetrics(const std::string& rule_id);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RuleMetrics> metrics_;
};

/// PolicyOptimizer provides optimization recommendations
class PolicyOptimizer {
public:
    struct OptimizationRecommendation {
        std::string recommendation_id;
        std::string rule_id;
        std::string optimization_type;             // merge, split, simplify, reorder, remove
        std::string description;
        std::string rationale;
        std::string expected_benefit;
        int priority = 0;                          // 1-10
        
        nlohmann::json toJson() const;
    };
    
    struct OptimizationReport {
        int total_recommendations = 0;
        int high_priority_recommendations = 0;
        std::vector<OptimizationRecommendation> recommendations;
        std::string summary;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
    };
    
    /// Generate optimization recommendations
    std::vector<OptimizationRecommendation> generateRecommendations(
        const PolicyManager& policy_mgr,
        const PolicyValidator::ValidationReport& validation_report,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
    
    /// Recommend rules to merge (similar rules)
    std::vector<OptimizationRecommendation> recommendMerges(const PolicyManager& policy_mgr) const;
    
    /// Recommend rules to simplify (overly complex)
    std::vector<OptimizationRecommendation> recommendSimplifications(const PolicyManager& policy_mgr) const;
    
    /// Recommend rule reordering for performance
    std::vector<OptimizationRecommendation> recommendReordering(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
    
    /// Recommend rules to remove (unused/redundant)
    std::vector<OptimizationRecommendation> recommendRemovals(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts
    ) const;
    
    /// Generate comprehensive optimization report
    OptimizationReport generateOptimizationReport(
        const PolicyManager& policy_mgr,
        const PolicyValidator::ValidationReport& validation_report,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
};

} // namespace governance
} // namespace themis
