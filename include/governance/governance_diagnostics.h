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

} // namespace themis::governance
