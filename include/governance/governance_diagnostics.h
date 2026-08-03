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

namespace themis::governance {

/**
 * @brief Governance diagnostic codes (7300-7399 range reserved).
 *
 * Used to classify specific error conditions that may occur during
 * policy evaluation, lifecycle state management, compliance checks,
 * OPA integration, and lineage backpressure management.
 */
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

/**
 * @brief Diagnostic record for a single governance event.
 *
 * Captures comprehensive context about an error, fallback, or anomaly
 * detected during governance operations. Includes remediation guidance
 * for operational teams.
 */
struct GovernanceDiagnostic {
    /// Error classification code (see GovDiagnosticCode).
    GovDiagnosticCode code = GovDiagnosticCode::kConflictDetected;
    
    /// Component emitting the diagnostic (e.g., "policy_engine", "opa_adapter").
    std::string component;
    
    /// Human-readable description of the issue.
    std::string description;
    
    /// Suggested remediation steps for operators.
    std::vector<std::string> remediation_steps;
    
    /// Unix timestamp (milliseconds) when diagnostic was recorded.
    int64_t timestamp_ms = 0;
    
    /// Additional context (component-specific key-value pairs).
    std::unordered_map<std::string, std::string> context;
    
    /**
     * @brief Serialize diagnostic to JSON.
     * @return JSON object with all fields.
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Aggregator for governance diagnostics across all components.
 *
 * Thread-safe collector that records and retrieves diagnostics
 * by component, code, or time range. Supports export for monitoring
 * and alerting systems.
 */
class DiagnosticAggregator {
public:
    DiagnosticAggregator() = default;
    ~DiagnosticAggregator() = default;
    
    DiagnosticAggregator(const DiagnosticAggregator&) = delete;
    DiagnosticAggregator& operator=(const DiagnosticAggregator&) = delete;
    DiagnosticAggregator(DiagnosticAggregator&&) = delete;
    DiagnosticAggregator& operator=(DiagnosticAggregator&&) = delete;
    
    /**
     * @brief Record a diagnostic event.
     * 
     * Records timestamp automatically if not already set.
     * Thread-safe; multiple callers can record concurrently.
     * 
     * @param diag Diagnostic to record.
     */
    void recordDiagnostic(const GovernanceDiagnostic& diag);
    
    /**
     * @brief Retrieve diagnostics emitted by a specific component.
     * 
     * @param component Component name (e.g., "policy_engine").
     * @return Vector of diagnostics for that component (may be empty).
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsForComponent(
        const std::string& component) const;
    
    /**
     * @brief Retrieve diagnostics with a specific code.
     * 
     * @param code Diagnostic code to filter by.
     * @return Vector of diagnostics with that code.
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsForCode(
        GovDiagnosticCode code) const;
    
    /**
     * @brief Retrieve diagnostics within a time range.
     * 
     * @param start_ms Lower bound (inclusive), Unix milliseconds. 0 = no lower bound.
     * @param end_ms Upper bound (inclusive), Unix milliseconds. 0 = no upper bound.
     * @return Vector of matching diagnostics (ordered by timestamp).
     */
    std::vector<GovernanceDiagnostic> getDiagnosticsInTimeRange(
        int64_t start_ms, int64_t end_ms) const;
    
    /**
     * @brief Get the most recent diagnostic for each component.
     * 
     * @return Map from component name to its latest diagnostic (or empty if none recorded).
     */
    std::unordered_map<std::string, GovernanceDiagnostic> getLatestPerComponent() const;
    
    /**
     * @brief Export all diagnostics as JSON.
     * 
     * @return JSON array of diagnostic objects.
     */
    nlohmann::json exportAsJson() const;
    
    /**
     * @brief Clear all recorded diagnostics.
     */
    void clear();
    
    /**
     * @brief Get total count of recorded diagnostics.
     * 
     * @return Number of diagnostics in the aggregator.
     */
    size_t getTotalCount() const;

private:
    mutable std::mutex mutex_;
    std::vector<GovernanceDiagnostic> diagnostics_;
};

/**
 * @brief Get the process-global governance diagnostic aggregator.
 *
 * Enables cross-component diagnostic emission when a component does not
 * maintain its own local DiagnosticAggregator instance.
 *
 * @return Singleton DiagnosticAggregator instance.
 */
DiagnosticAggregator& getGlobalDiagnosticAggregator();

/**
 * @brief Conflict diagnostic helper for Phase 3B hardening.
 *
 * Detects, records, and reports policy conflicts with structured
 * diagnostics integration. Provides conflict resolution metadata
 * for policy engine and compliance reporter.
 */
class ConflictDiagnosticHelper {
public:
    /**
     * @brief Conflict resolution strategies.
     */
    enum class ResolutionStrategy {
        EXPLICIT_DENY    = 0,  // Conflict blocks both policies (strictest)
        EXPLICIT_ALLOW   = 1,  // Conflict allows both policies (permissive)
        FIRST_MATCH      = 2,  // First matching policy wins
        MOST_RESTRICTIVE = 3,  // Most restrictive policy wins
        WHITELIST        = 4,  // Explicit whitelist overrides conflict
    };
    
    /**
     * @brief Conflict detection result.
     */
    struct ConflictDetectionResult {
        /// true if conflicts detected
        bool has_conflicts = false;
        
        /// Conflicting rule IDs (pair format: [rule_a, rule_b])
        std::vector<std::pair<std::string, std::string>> conflicting_pairs;
        
        /// Conflict descriptions (human-readable)
        std::vector<std::string> descriptions;
        
        /// Recommended resolution strategy
        ResolutionStrategy recommended_strategy = ResolutionStrategy::EXPLICIT_DENY;
        
        /// Diagnostic code for aggregator
        int32_t diagnostic_code = 7300;  // kConflictDetected
    };
    
    /**
     * @brief Create conflict diagnostic helper.
     * 
     * @param strategy Default resolution strategy
     * @param aggregator Optional external aggregator (uses global if null)
     */
    explicit ConflictDiagnosticHelper(
        ResolutionStrategy strategy = ResolutionStrategy::EXPLICIT_DENY,
        DiagnosticAggregator* aggregator = nullptr
    );
    
    /**
     * @brief Detect conflicts between policy rules.
     * 
     * Identifies conflicting resource/action/effect combinations
     * and produces diagnostic output.
     * 
     * @param policies List of active policies to check
     * @return Detection result with conflict details
     */
    ConflictDetectionResult detectConflict(
        const std::vector<std::string>& policy_ids
    );
    
    /**
     * @brief Record conflict event with diagnostics.
     * 
     * Creates a GovernanceDiagnostic and emits to aggregator.
     * 
     * @param result Detection result to record
     * @param additional_context Optional key-value context
     */
    void recordConflict(
        const ConflictDetectionResult& result,
        const std::unordered_map<std::string, std::string>& additional_context = {}
    );
    
    /**
     * @brief Get all recorded conflict diagnostics.
     * 
     * @return Vector of conflict-related diagnostics
     */
    [[nodiscard]] std::vector<GovernanceDiagnostic> getConflictDiagnostics() const;
    
    /**
     * @brief Clear all recorded conflict diagnostics.
     */
    void clearConflictHistory();
    
    /**
     * @brief Get current resolution strategy.
     * 
     * @return Current ResolutionStrategy
     */
    [[nodiscard]] ResolutionStrategy getCurrentStrategy() const;
    
    /**
     * @brief Set resolution strategy.
     * 
     * @param strategy New strategy to apply
     */
    void setResolutionStrategy(ResolutionStrategy strategy);

    // Safety check methods (Phase 3B Extended)
    
    /**
     * @brief Check for conflicting classifications.
     * @param classifications List of classifications assigned
     * @return true if conflicting classifications detected
     */
    bool hasConflictingClassifications(
        const std::vector<std::string>& classifications
    ) const;
    
    /**
     * @brief Validate CCPA compliance path.
     * @param context Request context with CCPA indicators
     * @return true if CCPA opt-out properly respected
     */
    bool validateCCPACompliancePath(
        const std::unordered_map<std::string, std::string>& context
    ) const;
    
    /**
     * @brief Detect privilege escalation attempts.
     * @param user_tier User's access tier
     * @param required_tier Tier required for operation
     * @return true if escalation attempt detected
     */
    bool detectPrivilegeEscalation(
        const std::string& user_tier,
        const std::string& required_tier
    ) const;
    
    /**
     * @brief Detect temporal policy violations.
     * @param policy Policy object (serialized as context map)
     * @return Vector of temporal issues (empty if none)
     */
    std::vector<struct TemporalIssue> detectTemporalViolations(
        const std::unordered_map<std::string, std::string>& policy
    ) const;
    
    /**
     * @brief Validate masking rule consistency.
     * @param mask_rules Masking rules to validate
     * @return Vector of violations (empty if valid)
     */
    std::vector<struct MaskingRuleViolation> validateMaskingRuleConsistency(
        const std::vector<std::unordered_map<std::string, std::string>>& mask_rules
    ) const;
    
    /**
     * @brief Validate whitelist policy.
     * @param whitelist_policy Whitelist policy context
     * @return true if whitelist is valid (non-empty)
     */
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

/**
 * @brief High-level access safety validator orchestrator.
 *
 * Composes multiple safety checks in fail-closed order to validate
 * complete access paths. Returns structured result with all violations
 * and remediation steps.
 *
 * Thread-safe. Intended for pre-evaluation checks before policy engine.
 */
class SafeAccessValidator {
public:
    /**
     * @brief Create validator with optional external aggregator.
     * 
     * @param aggregator Optional external DiagnosticAggregator
     */
    explicit SafeAccessValidator(DiagnosticAggregator* aggregator = nullptr);
    ~SafeAccessValidator();
    
    // Non-copyable, non-movable
    SafeAccessValidator(const SafeAccessValidator&) = delete;
    SafeAccessValidator& operator=(const SafeAccessValidator&) = delete;
    SafeAccessValidator(SafeAccessValidator&&) = delete;
    SafeAccessValidator& operator=(SafeAccessValidator&&) = delete;
    
    /**
     * @brief Validate an access request comprehensively.
     *
     * Runs all 8 safety checks in fail-closed order:
     * 1. Classification conflicts (S1)
     * 2. CCPA compliance (S2)
     * 3. Privilege escalation (S3)
     * 4. Temporal violations (S4)
     * 5. Cross-border conflicts (S5)
     * 6. Masking rule consistency (S6)
     * 7. Whitelist exhaustion (S7)
     * 8. Cascading denial detection (S8)
     *
     * @param request AccessRequest to validate
     * @return SafeAccessResult with detailed findings
     */
    SafeAccessResult validateAccessRequest(const AccessRequest& request);
    
    /**
     * @brief Get all recorded safety diagnostics.
     * @return Vector of SafetyViolations recorded
     */
    std::vector<SafetyViolation> getAllViolations() const;
    
    /**
     * @brief Clear violation history.
     */
    void clearViolationHistory();
    
    /**
     * @brief Get total count of violations recorded.
     * @return Number of violations
     */
    size_t getViolationCount() const;

private:
    DiagnosticAggregator* aggregator_;
    bool owns_aggregator_ = false;
    mutable std::mutex mutex_;
    std::vector<SafetyViolation> violation_history_;
    std::shared_ptr<ConflictDiagnosticHelper> conflict_helper_;
    
    // Helper methods for each scenario check
    SafetyViolation checkConflictingClassifications(const AccessRequest& req);
    SafetyViolation checkCCPACompliance(const AccessRequest& req);
    SafetyViolation checkPrivilegeEscalation(const AccessRequest& req);
    SafetyViolation checkTemporalViolations(const AccessRequest& req);
    SafetyViolation checkCrossBorderConflicts(const AccessRequest& req);
    SafetyViolation checkMaskingRuleConsistency(const AccessRequest& req);
    SafetyViolation checkWhitelistExhaustion(const AccessRequest& req);
    SafetyViolation checkCascadingDenials(const AccessRequest& req);
};

} // namespace themis::governance

