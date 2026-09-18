/**
 * @file policy_validation.h
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

class PolicyValidator {
public:
    struct ConflictResult {
        std::string conflict_id;
        std::string conflict_type;                 // contradictory, overlapping, circular
        std::vector<std::string> conflicting_rule_ids;
        std::string description;
        std::string severity;                      // low, medium, high, critical
        std::string recommendation;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Detect Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectConflicts(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Detect Contradictory Rules.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectContradictoryRules(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Detect Overlapping Permissions.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectOverlappingPermissions(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Detect Circular Dependencies.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectCircularDependencies(const PolicyManager& policy_mgr) const;

    /**
     * @brief Detect Ccpa Hipaa Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectCcpaHipaaConflicts(const PolicyManager& policy_mgr) const;

    /**
     * @brief Detect Pci Dss Gdpr Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ConflictResult> detectPciDssGdprConflicts(const PolicyManager& policy_mgr) const;
    
    std::unordered_map<std::string, EffectivenessMetrics> calculateEffectiveness(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {}
    ) const;
    
    std::vector<std::string> identifyUnusedRules(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {},
        int min_days_unused = 30
    ) const;
    
    /**
     * @brief Perform Security Checks.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<SecurityCheckResult> performSecurityChecks(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Check Overly Permissive.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<SecurityCheckResult> checkOverlyPermissive(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Check Encryption Requirements.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<SecurityCheckResult> checkEncryptionRequirements(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Check Audit Logging.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<SecurityCheckResult> checkAuditLogging(const PolicyManager& policy_mgr) const;
    
    std::vector<SecurityCheckResult> checkRetentionCompliance(
        const PolicyManager& policy_mgr,
        int min_retention_days = 90
    ) const;
    
    ValidationReport generateValidationReport(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts = {}
    ) const;
    
    /**
     * @brief Validate Single Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<SecurityCheckResult> validateSingleRule(const PolicyRule& rule) const;
};

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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct PerformanceImpact {
        std::string rule_id;
        int64_t avg_evaluation_time_us = 0;
        std::string performance_category;          // fast, normal, slow, critical
        std::string impact_description;
        std::string optimization_suggestion;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Record Evaluation.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] matched Input parameter.
     * @param[in] evaluation_time_us Input parameter.
     */
    void recordEvaluation(const std::string& rule_id, bool matched, int64_t evaluation_time_us);
    
    /**
     * @brief Get Rule Metrics.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<RuleMetrics> getRuleMetrics(const std::string& rule_id) const;
    
    std::unordered_map<std::string, RuleMetrics> getAllMetrics() const;
    
    /**
     * @brief Analyze Performance Impact.
     * @return Return value.
     */
    std::vector<PerformanceImpact> analyzePerformanceImpact() const;
    
    std::vector<std::string> getSlowRules(int64_t threshold_us = 1000) const;
    
    /**
     * @brief Export Metrics.
     * @return Return value.
     */
    nlohmann::json exportMetrics() const;
    
    /**
     * @brief Import Metrics.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importMetrics(const nlohmann::json& j);
    
    /**
     * @brief Reset Metrics.
     */
    void resetMetrics();
    
    /**
     * @brief Reset Rule Metrics.
     * @param[in] rule_id Identifier of the rule.
     */
    void resetRuleMetrics(const std::string& rule_id);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RuleMetrics> metrics_;
};

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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct OptimizationReport {
        int total_recommendations = 0;
        int high_priority_recommendations = 0;
        std::vector<OptimizationRecommendation> recommendations;
        std::string summary;
        int64_t generated_at = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    std::vector<OptimizationRecommendation> generateRecommendations(
        const PolicyManager& policy_mgr,
        const PolicyValidator::ValidationReport& validation_report,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
    
    /**
     * @brief Recommend Merges.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<OptimizationRecommendation> recommendMerges(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Recommend Simplifications.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<OptimizationRecommendation> recommendSimplifications(const PolicyManager& policy_mgr) const;
    
    std::vector<OptimizationRecommendation> recommendReordering(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
    
    std::vector<OptimizationRecommendation> recommendRemovals(
        const PolicyManager& policy_mgr,
        const std::unordered_map<std::string, int>& hit_counts
    ) const;
    
    OptimizationReport generateOptimizationReport(
        const PolicyManager& policy_mgr,
        const PolicyValidator::ValidationReport& validation_report,
        const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics>& metrics
    ) const;
};

} // namespace governance
} // namespace themis
