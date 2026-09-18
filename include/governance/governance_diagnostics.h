/**
 * @file governance_diagnostics.h
 * @brief Unified diagnostic model for governance module Phase 2-3 hardening.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Phase 2-3 foundation)
 * @note Provides structured error classification, diagnostic aggregation, and
 *       remediation guidance for policy engine, OPA adapter, and lifecycle management.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "themis/export.h"

namespace themis::governance {

enum class GovDiagnosticCode : int32_t {
    kConflictDetected         = 7300,  // Policy rules conflict
    kFallbackActivated        = 7301,  // Fallback default applied (e.g., OPA unavailable)
    kComplianceViolation      = 7302,  // Compliance check failed
    kAuditLogFailure          = 7303,  // Audit logging failed
    kOpaUnavailable           = 7304,  // OPA service unreachable
    kStateTransitionInvalid   = 7305,  // Policy state transition rejected
    kLineageBackpressure      = 7306,  // Data lineage processing backlog
    kPolicyNotFound           = 7307,  // Referenced policy rule not found
    kDenyByDefault            = 7308,  // Security fallback: deny-by-default applied
    // ─── Phase 2C: Lineage Backpressure Error Codes ──────────────────────
    kLineageAuditFailure      = 7360,  // Audit logger failure in lineage tracking
    kLineageSizeLimitExceeded = 7361,  // Lineage size limit reached
    kLineageMemoryPressure    = 7362,  // Memory pressure detected
    kLineageCircuitBreakerOpen= 7363,  // Circuit breaker engaged
    kLineageEventSequence     = 7364,  // Event sequence violation
};

struct THEMIS_SECURITY_API GovernanceDiagnostic {
    GovDiagnosticCode code = GovDiagnosticCode::kConflictDetected;
    
    std::string component;
    
    std::string description;
    
    std::vector<std::string> remediation_steps;
    
    int64_t timestamp_ms = 0;
    
    std::unordered_map<std::string, std::string> context;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class THEMIS_SECURITY_API DiagnosticAggregator {
public:
    DiagnosticAggregator() = default;
    ~DiagnosticAggregator() = default;
    
    DiagnosticAggregator(const DiagnosticAggregator&) = delete;
    DiagnosticAggregator& operator=(const DiagnosticAggregator&) = delete;
    DiagnosticAggregator(DiagnosticAggregator&&) = delete;
    DiagnosticAggregator& operator=(DiagnosticAggregator&&) = delete;
    
    /**
     * @brief Record Diagnostic.
     * @param[in] diag Input parameter.
     */
    void recordDiagnostic(const GovernanceDiagnostic& diag);
    
    /**
     * @brief Get Diagnostics For Component.
     * @param[in] component Input parameter.
     * @return Return value.
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsForComponent(
        const std::string& component) const;
    
    /**
     * @brief Get Diagnostics For Code.
     * @param[in] code Input parameter.
     * @return Return value.
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsForCode(
        GovDiagnosticCode code) const;
    
    /**
     * @brief Get Diagnostics In Time Range.
     * @param[in] start_ms Input parameter.
     * @param[in] end_ms Input parameter.
     * @return Return value.
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsInTimeRange(
        int64_t start_ms, int64_t end_ms) const;
    
    std::unordered_map<std::string, GovernanceDiagnostic> getLatestPerComponent() const;
    
    /**
     * @brief Export As Json.
     * @return Return value.
     */
    nlohmann::json exportAsJson() const;
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Get Total Count.
     * @return Return value.
     */
    size_t getTotalCount() const;

private:
    mutable std::mutex mutex_;
    std::vector<GovernanceDiagnostic> diagnostics_;
};

/**
 * @brief Get Global Diagnostic Aggregator.
 * @return Return value.
 */
THEMIS_SECURITY_API DiagnosticAggregator& getGlobalDiagnosticAggregator();

class THEMIS_SECURITY_API ConflictDiagnosticHelper {
public:
    enum class ResolutionStrategy {
        EXPLICIT_DENY    = 0,  // Conflict blocks both policies (strictest)
        EXPLICIT_ALLOW   = 1,  // Conflict allows both policies (permissive)
        FIRST_MATCH      = 2,  // First matching policy wins
        MOST_RESTRICTIVE = 3,  // Most restrictive policy wins
        WHITELIST        = 4,  // Explicit whitelist overrides conflict
    };
    
    struct ConflictDetectionResult {
        bool has_conflicts = false;
        
        std::vector<std::pair<std::string, std::string>> conflicting_pairs;
        
        std::vector<std::string> descriptions;
        
        ResolutionStrategy recommended_strategy = ResolutionStrategy::EXPLICIT_DENY;
        
        int32_t diagnostic_code = 7300;  // kConflictDetected
    };
    
    explicit ConflictDiagnosticHelper(
        ResolutionStrategy strategy = ResolutionStrategy::EXPLICIT_DENY,
        DiagnosticAggregator* aggregator = nullptr
    );
    
    /**
     * @brief Detect Conflict.
     * @param[in] policy_ids Input parameter.
     * @return Return value.
     */
    ConflictDetectionResult detectConflict(
        const std::vector<std::string>& policy_ids
    );
    
    void recordConflict(
        const ConflictDetectionResult& result,
        const std::unordered_map<std::string, std::string>& additional_context = {}
    );
    
    [[nodiscard]] std::vector<GovernanceDiagnostic> getConflictDiagnostics() const;
    
    /**
     * @brief Clear Conflict History.
     */
    void clearConflictHistory();
    
    [[nodiscard]] ResolutionStrategy getCurrentStrategy() const;
    
    /**
     * @brief Set Resolution Strategy.
     * @param[in] strategy Input parameter.
     */
    void setResolutionStrategy(ResolutionStrategy strategy);

    
    /**
     * @brief Has Conflicting Classifications.
     * @param[in] classifications Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasConflictingClassifications(
        const std::vector<std::string>& classifications
    ) const;
    
    bool validateCCPACompliancePath(
        const std::unordered_map<std::string, std::string>& context
    ) const;
    
    /**
     * @brief Detect Privilege Escalation.
     * @param[in] user_tier Input parameter.
     * @param[in] required_tier Input parameter.
     * @return True when the operation succeeds.
     */
    bool detectPrivilegeEscalation(
        const std::string& user_tier,
        const std::string& required_tier
    ) const;
    
    std::vector<struct TemporalIssue> detectTemporalViolations(
        const std::unordered_map<std::string, std::string>& policy
    ) const;
    
    std::vector<struct MaskingRuleViolation> validateMaskingRuleConsistency(
        const std::vector<std::unordered_map<std::string, std::string>>& mask_rules
    ) const;
    
    bool validateWhitelistPolicy(
        const std::unordered_map<std::string, std::string>& whitelist_policy
    ) const;

private:
    ResolutionStrategy strategy_;
    DiagnosticAggregator* aggregator_;
    bool owns_aggregator_ = false;
    mutable std::mutex mutex_;
    std::vector<ConflictDetectionResult> conflict_history_;
};

// Unsafe access scenario codes (7381-7388 range)
enum class UnsafeAccessScenario : int32_t {
    S1_CONFLICTING_CLASSIFICATIONS = 7381,  // Public + restricted
    S2_CCPA_OVERRIDE_MISSING        = 7382,  // Missing CCPA profile
    S3_PRIVILEGE_ESCALATION         = 7383,  // Lower→higher tier
    S4_TEMPORAL_VIOLATION           = 7384,  // Future policy or zero retention
    S5_CROSS_BORDER_CONFLICT        = 7385,  // Incompatible jurisdictions
    S6_MASKING_BYPASS_ATTEMPT       = 7386,  // Redaction circumvention
    S7_WHITELIST_EXHAUSTION         = 7387,  // Empty/null whitelist
    S8_CASCADING_DENIALS            = 7388,  // Multiple deny layers
};

// Temporal violation details
struct TemporalIssue {
    std::string issue_type;  // "future_effective_date", "zero_retention", etc.
    std::string description;
    int64_t value_ms = 0;
};

// Masking rule validation error
struct MaskingRuleViolation {
    std::string rule_id;
    std::string violation_type;  // "inconsistent_redaction", "bypass_detected", etc.
    std::vector<std::string> affected_schemas;
};

// Single safety violation
struct SafetyViolation {
    UnsafeAccessScenario scenario;
    std::string description;
    std::vector<std::string> affected_policies;
    std::string remediation_hint;
};

// Overall safety assessment
struct SafeAccessResult {
    bool is_safe = false;
    std::vector<SafetyViolation> violations;
    std::vector<int32_t> scenario_codes;
    int32_t diagnostic_code = 0;
    std::vector<std::string> remediation_steps;
    int64_t evaluated_at_ms = 0;
};

// AccessRequest context for validation
struct AccessRequest {
    std::string request_id;
    std::string user_id;
    std::string user_tier;  // "read_only", "editor", "admin", etc.
    std::vector<std::string> dataset_classifications;  // e.g. ["public", "restricted"]
    std::vector<std::string> policy_ids;  // Policies to evaluate
    std::string target_operation;  // "read", "write", "export", "train_model"
    int64_t requested_at_ms = 0;
    std::unordered_map<std::string, std::string> context;  // Additional context
};

class THEMIS_SECURITY_API SafeAccessValidator {
public:
    explicit SafeAccessValidator(DiagnosticAggregator* aggregator = nullptr);
    ~SafeAccessValidator();
    
    // Non-copyable, non-movable
    SafeAccessValidator(const SafeAccessValidator&) = delete;
    SafeAccessValidator& operator=(const SafeAccessValidator&) = delete;
    SafeAccessValidator(SafeAccessValidator&&) = delete;
    SafeAccessValidator& operator=(SafeAccessValidator&&) = delete;
    
    /**
     * @brief Validate Access Request.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    SafeAccessResult validateAccessRequest(const AccessRequest& request);
    
    /**
     * @brief Get All Violations.
     * @return Return value.
     */
    std::vector<SafetyViolation> getAllViolations() const;
    
    /**
     * @brief Clear Violation History.
     */
    void clearViolationHistory();
    
    /**
     * @brief Get Violation Count.
     * @return Return value.
     */
    size_t getViolationCount() const;

private:
    DiagnosticAggregator* aggregator_;
    bool owns_aggregator_ = false;
    mutable std::mutex mutex_;
    std::vector<SafetyViolation> violation_history_;
    std::shared_ptr<ConflictDiagnosticHelper> conflict_helper_;
    
    /**
     * @brief Check Conflicting Classifications.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkConflictingClassifications(const AccessRequest& req);
    /**
     * @brief Check CCPACompliance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkCCPACompliance(const AccessRequest& req);
    /**
     * @brief Check Privilege Escalation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkPrivilegeEscalation(const AccessRequest& req);
    /**
     * @brief Check Temporal Violations.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkTemporalViolations(const AccessRequest& req);
    /**
     * @brief Check Cross Border Conflicts.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkCrossBorderConflicts(const AccessRequest& req);
    /**
     * @brief Check Masking Rule Consistency.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkMaskingRuleConsistency(const AccessRequest& req);
    /**
     * @brief Check Whitelist Exhaustion.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkWhitelistExhaustion(const AccessRequest& req);
    /**
     * @brief Check Cascading Denials.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    SafetyViolation checkCascadingDenials(const AccessRequest& req);
};

} // namespace themis::governance

