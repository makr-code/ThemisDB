/**
 * @file updates_operator_diagnostics.h
 * @brief Operator-facing diagnostics module for update incident detection and recovery
 * @version 1.0.0
 * @since 2.4.0 (Q4 2026)
 *
 * Provides structured failure detection, alerting rules, and recovery procedures for
 * common update scenarios:
 *  - Coordinator unreachability and timeout cycles
 *  - Partial migration failures with recovery ambiguity
 *  - Canary rollout failures and timeout loops
 *  - Node failures during blue-green rollback
 *  - Resource exhaustion scenarios
 *  - Manifest corruption and degradation
 *  - Cluster partition handling
 *  - Deadlock/race condition detection
 *
 * Supports machine-parseable alerting rules for observability tools (Prometheus, Grafana).
 *
 * Doxygen maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "updates/updates_diagnostics.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

// ============================================================================
// Failure Scenario Detection
// ============================================================================

/**
 * @brief Enumeration of common failure scenarios
 *
 * Each scenario has diagnostic symptoms, root causes, and proven recovery procedures.
 */
enum class FailureScenario {
    COORDINATOR_UNREACHABLE,        ///< Coordinator timeout and canary cycles
    PARTIAL_MIGRATION_FAILURE,      ///< Migration fails on subset of nodes
    CANARY_TIMEOUT_CYCLE,           ///< Canary repeatedly times out
    BLUE_GREEN_ROLLBACK_FAILURE,    ///< Node failure during rollback
    RESOURCE_EXHAUSTION,            ///< Memory, disk, file handle limits hit
    MANIFEST_CORRUPTION,            ///< Update artifact corrupted
    CLUSTER_PARTITION,              ///< Network partition in cluster
    DEADLOCK_RACE_CONDITION         ///< Mutual dependency or concurrent access issue
};

/**
 * @brief Recovery action type for a failure scenario
 */
enum class RecoveryAction {
    IMMEDIATE_ABORT,               ///< Abort current operation immediately
    ISOLATED_ROLLBACK,             ///< Rollback on affected node only
    FULL_CLUSTER_ROLLBACK,         ///< Rollback all nodes to prior state
    RESOURCE_CLEANUP,              ///< Free resources and retry
    MANUAL_INTERVENTION,           ///< Requires operator investigation
    GRACEFUL_DEGRADE,              ///< Degrade to read-only or partial mode
    RESTART_COORDINATOR            ///< Restart coordinator and retry
};

/**
 * @brief Alerting rule for observability tools
 *
 * Machine-parseable rule definitions for Prometheus, Grafana, or other alerting systems.
 */
struct AlertingRule {
    /// Unique identifier for this alert rule
    std::string rule_id;
    
    /// Human-readable rule name
    std::string rule_name;
    
    /// Failure scenario this rule detects
    FailureScenario scenario;
    
    /// Alert severity (INFO, WARN, ERROR, CRITICAL)
    std::string severity;
    
    /// Alert condition in Prometheus query language or equivalent
    std::string condition;
    
    /// Alert message template (supports {{variable}} substitution)
    std::string message_template;
    
    /// Runbook URL for operator reference
    std::string runbook_url;
    
    /// Recommended recovery action
    RecoveryAction recommended_action;
    
    /// Convert to JSON for serialization
    json to_json() const;
};

/**
 * @brief Recovery procedure for a failure scenario
 *
 * Documented steps that have been validated through Phase 4-5 testing.
 */
struct RecoveryProcedure {
    /// Failure scenario addressed by this procedure
    FailureScenario scenario;
    
    /// Brief description of symptoms
    std::string symptoms;
    
    /// Root cause analysis steps
    std::vector<std::string> root_cause_analysis;
    
    /// Recovery steps in order
    std::vector<std::string> recovery_steps;
    
    /// Prevention recommendations
    std::vector<std::string> prevention_tips;
    
    /// Expected outcomes after recovery
    std::string expected_outcome;
    
    /// Convert to JSON for documentation
    json to_json() const;
};

// ============================================================================
// Operator Diagnostics Module
// ============================================================================

/**
 * @brief Central diagnostics module for operator visibility into update incidents
 *
 * Provides:
 *  - Failure scenario detection (8+ patterns)
 *  - Alerting rule generation for observability tools
 *  - Log pattern recommendations
 *  - Recovery procedure lookup
 *  - Structured error context enrichment
 *
 * Thread-safe. All methods can be called concurrently from multiple threads.
 *
 * Example usage:
 * @code
 *   OperatorDiagnostics diagnostics;
 *   
 *   // Detect coordinator timeout pattern
 *   ErrorContext ctx;
 *   ctx.error_code = DiagnosticErrorCode::COORDINATION_TIMEOUT;
 *   ctx.operation = "coordinate_update";
 *   
 *   auto scenario = diagnostics.detectScenario(ctx);
 *   if (scenario == FailureScenario::COORDINATOR_UNREACHABLE) {
 *       auto procedure = diagnostics.getRecoveryProcedure(scenario);
 *       std::cout << "Recovery steps:" << std::endl;
 *       for (const auto& step : procedure.recovery_steps) {
 *           std::cout << "  - " << step << std::endl;
 *       }
 *   }
 *   
 *   // Generate alerting rules for observability tools
 *   auto rules = diagnostics.getAllAlertingRules();
 *   for (const auto& rule : rules) {
 *       std::cout << rule.rule_name << ": " << rule.condition << std::endl;
 *   }
 * @endcode
 */
class OperatorDiagnostics {
public:
    /**
     * @brief Construct diagnostics module
     */
    explicit OperatorDiagnostics();

    ~OperatorDiagnostics() = default;

    // Non-copyable
    OperatorDiagnostics(const OperatorDiagnostics&) = delete;
    OperatorDiagnostics& operator=(const OperatorDiagnostics&) = delete;

    // Movable
    OperatorDiagnostics(OperatorDiagnostics&&) = default;
    OperatorDiagnostics& operator=(OperatorDiagnostics&&) = default;

    // ========================================================================
    // Failure Scenario Detection
    // ========================================================================

    /**
     * @brief Detect which failure scenario an error context matches
     *
     * Analyzes error code, operation, phase, and context to classify the failure.
     * Uses pattern matching against Phase 4-5 hardening test scenarios.
     *
     * @param context Error context from DiagnosticEmitter
     * @return Detected failure scenario, or returns DEADLOCK_RACE_CONDITION for unclassified
     */
    FailureScenario detectScenario(const ErrorContext& context) const;

    /**
     * @brief Get human-readable name for a failure scenario
     *
     * @param scenario The failure scenario
     * @return Descriptive name (e.g., "Coordinator Unreachable")
     */
    std::string getScenarioName(FailureScenario scenario) const;

    /**
     * @brief Check if an error pattern matches a specific scenario
     *
     * @param scenario The scenario to check against
     * @param context Error context
     * @return true if pattern matches
     */
    bool matchesScenario(FailureScenario scenario, const ErrorContext& context) const;

    // ========================================================================
    // Recovery Procedures
    // ========================================================================

    /**
     * @brief Get recovery procedure for a failure scenario
     *
     * Recovery procedures are documented and validated through Phase 4-5 testing.
     *
     * @param scenario The failure scenario
     * @return Recovery procedure with steps and prevention tips
     */
    RecoveryProcedure getRecoveryProcedure(FailureScenario scenario) const;

    /**
     * @brief Recommend recovery action for an error context
     *
     * Returns the safest recovery action based on the detected scenario and
     * current operation state.
     *
     * @param context Error context
     * @return Recommended recovery action
     */
    RecoveryAction recommendRecoveryAction(const ErrorContext& context) const;

    // ========================================================================
    // Alerting Rules for Observability Tools
    // ========================================================================

    /**
     * @brief Get alerting rule for a failure scenario
     *
     * Rules are suitable for use in Prometheus, Grafana, or similar alerting systems.
     * Includes Prometheus query syntax and message templates.
     *
     * @param scenario The failure scenario
     * @return Alerting rule with condition and message template
     */
    AlertingRule getAlertingRule(FailureScenario scenario) const;

    /**
     * @brief Get all alerting rules for integration with observability tools
     *
     * @return Vector of alerting rules for all 8+ scenarios
     */
    std::vector<AlertingRule> getAllAlertingRules() const;

    // ========================================================================
    // Log Pattern Recommendations
    // ========================================================================

    /**
     * @brief Get recommended log patterns for a failure scenario
     *
     * Helps operators configure log aggregation, filtering, and alerts.
     *
     * @param scenario The failure scenario
     * @return Vector of regular expressions or exact patterns to watch for
     */
    std::vector<std::string> getLogPatterns(FailureScenario scenario) const;

    /**
     * @brief Get recommended metrics to track for a failure scenario
     *
     * @param scenario The failure scenario
     * @return Vector of metric names (e.g., "updates_coordinator_timeout_count")
     */
    std::vector<std::string> getMetricsToTrack(FailureScenario scenario) const;

    // ========================================================================
    // Error Context Enrichment
    // ========================================================================

    /**
     * @brief Enrich an error context with recovery recommendations
     *
     * Adds recovery suggestions, alerting metadata, and scenario classification
     * to the error context for structured logging.
     *
     * @param context Input error context (modified in place)
     * @return true if enrichment was successful
     */
    bool enrichErrorContext(ErrorContext& context) const;

    // ========================================================================
    // JSON Export for Documentation and Tools
    // ========================================================================

    /**
     * @brief Export all failure scenarios as JSON
     *
     * Suitable for generating documentation, configuration files, or API responses.
     *
     * @return JSON array of failure scenario descriptions
     */
    json exportScenariosAsJson() const;

    /**
     * @brief Export all recovery procedures as JSON
     *
     * @return JSON array of recovery procedures
     */
    json exportProceduresAsJson() const;

    /**
     * @brief Export all alerting rules as JSON
     *
     * @return JSON array of alerting rules
     */
    json exportAlertingRulesAsJson() const;

private:
    // Internal helper methods
    FailureScenario detectCoordinatorScenario(const ErrorContext& context) const;
    FailureScenario detectMigrationScenario(const ErrorContext& context) const;
    FailureScenario detectCanaryScenario(const ErrorContext& context) const;
    FailureScenario detectRollbackScenario(const ErrorContext& context) const;
    FailureScenario detectResourceScenario(const ErrorContext& context) const;
    FailureScenario detectManifestScenario(const ErrorContext& context) const;
    FailureScenario detectPartitionScenario(const ErrorContext& context) const;
    FailureScenario detectDeadlockScenario(const ErrorContext& context) const;

    // Initialize recovery procedures and alerting rules
    void initializeRecoveryProcedures();
    void initializeAlertingRules();
};

}  // namespace updates
}  // namespace themis

#endif  // THEMIS_UPDATES_OPERATOR_DIAGNOSTICS_H
