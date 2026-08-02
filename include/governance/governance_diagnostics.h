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
 * and OPA integration.
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

private:
    ResolutionStrategy strategy_;
    DiagnosticAggregator* aggregator_;
    bool owns_aggregator_ = false;
    mutable std::mutex mutex_;
    std::vector<ConflictDetectionResult> conflict_history_;
};

} // namespace themis::governance

