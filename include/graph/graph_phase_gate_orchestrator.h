/**
 * @file graph_phase_gate_orchestrator.h
 * @brief Wave C C3 — Graph Phase Gate Orchestrator for ML pipeline phase tracking.
 *
 * The GraphPhaseGateOrchestrator models an ML training pipeline as a directed
 * acyclic graph (DAG) of phases.  Each node represents a named pipeline phase
 * (e.g. "data_validation", "model_training", "evaluation") and each directed
 * edge encodes a prerequisite relationship.  A phase may only be advanced once
 * all its prerequisite phases have passed their gate criteria.
 *
 * ### Core capabilities
 * - Phase registration with named prerequisite edges (DAG construction).
 * - Per-phase metric attachment (key/value float pairs).
 * - Gate evaluation: a phase gate PASSES when all required metrics meet their
 *   configured thresholds and all prerequisite phases are themselves PASSED.
 * - Gap reporting: returns the set of missing metrics or failing prerequisites
 *   that block a phase from passing ("gap tracking").
 * - DAG invariant enforcement: `registerPhase()` only accepts prerequisites
 *   that are already registered, and rejects duplicate or self-referential
 *   registrations to keep the phase graph acyclic.
 * - Topological ordering: `topologicalOrder()` returns a valid execution
 *   sequence for the full pipeline.
 *
 * ### Thread safety
 * All public methods are protected by a shared mutex.  Multiple threads may
 * concurrently call `gateStatus()` / `gaps()` (shared read lock); writes
 * (`registerPhase`, `attachMetric`, `setGateCriteria`) serialise via an
 * exclusive lock.
 *
 * ### Wave C C3 reference
 * Issue: #6287  |  Wave C ML Strategic Planning  |  C3 acceptance criteria
 *
 * @version 1.0.0
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// GateStatus
// ---------------------------------------------------------------------------

/**
 * @brief Outcome of evaluating a phase gate.
 */
enum class GateStatus {
    PASS,         ///< All criteria met; phase may advance.
    FAIL,         ///< One or more criteria not met; see `gaps()`.
    PENDING,      ///< Phase has not yet been evaluated.
    BLOCKED,      ///< A prerequisite phase has not passed its own gate.
};

// ---------------------------------------------------------------------------
// GateCriterion
// ---------------------------------------------------------------------------

/**
 * @brief A single named metric gate criterion.
 *
 * The gate passes for this criterion when
 * `attached_metric_value >= threshold`.
 */
struct GateCriterion {
    /// Metric key that must be present and satisfy `threshold`.
    std::string metric_key;
    /// Minimum acceptable value (inclusive lower bound).
    float threshold = 0.0f;
    /// Human-readable description for gap reports.
    std::string description;
};

// ---------------------------------------------------------------------------
// PhaseGapReport
// ---------------------------------------------------------------------------

/**
 * @brief Gap report for a single pipeline phase.
 *
 * A gap report is returned for phases whose gate has not passed.  It
 * enumerates both missing/below-threshold metrics and unresolved prerequisite
 * phase blockers.
 */
struct PhaseGapReport {
    /// Name of the phase this report applies to.
    std::string phase_name;
    /// Current gate status at the time of the report.
    GateStatus status = GateStatus::PENDING;
    /// Criteria that are not yet satisfied (metric absent or below threshold).
    std::vector<std::string> unsatisfied_criteria;
    /// Prerequisite phases that have not passed their own gate.
    std::vector<std::string> blocked_by_phases;
    /// Human-readable summary line.
    std::string summary;
};

// ---------------------------------------------------------------------------
// GraphPhaseGateOrchestrator
// ---------------------------------------------------------------------------

/**
 * @brief ML pipeline phase gate orchestrator built on a directed acyclic graph.
 *
 * Typical usage:
 * @code
 * GraphPhaseGateOrchestrator orch;
 *
 * // Register phases (DAG edges = prerequisites)
 * orch.registerPhase("data_validation", {});
 * orch.registerPhase("model_training",  {"data_validation"});
 * orch.registerPhase("evaluation",      {"model_training"});
 *
 * // Define gate criteria
 * orch.setGateCriteria("data_validation", {{"completeness", 0.95f, "≥ 95 % rows present"}});
 * orch.setGateCriteria("model_training",  {{"accuracy",     0.80f, "≥ 80 % training accuracy"}});
 * orch.setGateCriteria("evaluation",      {{"f1_score",     0.75f, "≥ 0.75 F1 on held-out set"}});
 *
 * // Attach observed metrics
 * orch.attachMetric("data_validation", "completeness", 0.98f);
 * orch.attachMetric("model_training",  "accuracy",     0.82f);
 * orch.attachMetric("evaluation",      "f1_score",     0.77f);
 *
 * // Evaluate
 * assert(orch.gateStatus("evaluation") == GateStatus::PASS);
 * @endcode
 */
class GraphPhaseGateOrchestrator {
public:
    GraphPhaseGateOrchestrator()  = default;
    ~GraphPhaseGateOrchestrator() = default;

    // Non-copyable; movable by transferring the phase graph while each
    // instance keeps its own mutex. Moved-from instances remain valid and
    // empty so they can be safely reused.
    GraphPhaseGateOrchestrator(const GraphPhaseGateOrchestrator&)            = delete;
    GraphPhaseGateOrchestrator& operator=(const GraphPhaseGateOrchestrator&) = delete;
    GraphPhaseGateOrchestrator(GraphPhaseGateOrchestrator&& other) noexcept;
    GraphPhaseGateOrchestrator& operator=(GraphPhaseGateOrchestrator&& other) noexcept;

    // -----------------------------------------------------------------------
    // DAG construction
    // -----------------------------------------------------------------------

    /**
     * @brief Register a pipeline phase with its direct prerequisites.
     *
     * @param phase_name   Unique name for the phase.  Must be non-empty.
     * @param prerequisites Names of phases that must PASS before this one can
     *                      advance.  May be empty for root phases.
     * @return `true` on success; `false` if `phase_name` is empty, already
     *         registered, self-referential, or depends on an unknown
     *         prerequisite.
     *
     * @note Prerequisites must have been registered previously (forward
     *       references are not supported), which keeps the public registration
     *       path acyclic.
     */
    bool registerPhase(const std::string&              phase_name,
                       const std::vector<std::string>& prerequisites);

    /**
     * @brief Replace the gate criteria for an already-registered phase.
     *
     * @param phase_name Phase to configure.
     * @param criteria   One or more metric gate criteria (may be empty for an
     *                   unconditional pass once prerequisites are met).
     * @return `false` if `phase_name` is not registered.
     */
    bool setGateCriteria(const std::string&             phase_name,
                         const std::vector<GateCriterion>& criteria);

    // -----------------------------------------------------------------------
    // Metric management
    // -----------------------------------------------------------------------

    /**
     * @brief Attach (or update) a named metric value for a phase.
     *
     * @param phase_name  Target phase.
     * @param metric_key  Metric identifier (must match a registered criterion
     *                    key to influence gate evaluation).
     * @param value       Observed metric value.
     * @return `false` if `phase_name` is not registered.
     */
    bool attachMetric(const std::string& phase_name,
                      const std::string& metric_key,
                      float              value);

    /**
     * @brief Retrieve the current value of a metric, if present.
     *
     * @return `std::nullopt` if the phase or metric is not found.
     */
    [[nodiscard]]
    std::optional<float> getMetric(const std::string& phase_name,
                                   const std::string& metric_key) const;

    // -----------------------------------------------------------------------
    // Gate evaluation
    // -----------------------------------------------------------------------

    /**
     * @brief Evaluate the gate for the given phase.
     *
     * Evaluation is recursive: prerequisite gates are evaluated first.
     *
     * @param phase_name Phase to evaluate.
     * @return `GateStatus::PENDING` if the phase is not registered.
     */
    [[nodiscard]]
    GateStatus gateStatus(const std::string& phase_name) const;

    /**
     * @brief Produce a gap report for a phase whose gate has not PASSED.
     *
     * @param phase_name Phase to diagnose.
     * @return A `PhaseGapReport` with `status == PASS` and empty lists when
     *         all criteria are met; `status == PENDING` when the phase is
     *         not registered.
     */
    [[nodiscard]]
    PhaseGapReport gaps(const std::string& phase_name) const;

    /**
     * @brief Collect gap reports for every phase that has not yet PASSED.
     *
     * @return Vector of gap reports (may be empty if all phases have PASSED).
     */
    [[nodiscard]]
    std::vector<PhaseGapReport> allGaps() const;

    // -----------------------------------------------------------------------
    // Graph introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return all registered phase names.
     */
    [[nodiscard]]
    std::vector<std::string> phaseNames() const;

    /**
     * @brief Return the number of registered phases.
     */
    [[nodiscard]]
    std::size_t phaseCount() const;

    /**
     * @brief Check whether a phase name is registered.
     */
    [[nodiscard]]
    bool hasPhase(const std::string& phase_name) const;

    /**
     * @brief Return the direct prerequisites of a registered phase.
     *
     * @return Empty vector if the phase is not found or has no prerequisites.
     */
    [[nodiscard]]
    std::vector<std::string> prerequisites(const std::string& phase_name) const;

    /**
     * @brief Return a topological ordering of all registered phases.
     *
     * Phases with no prerequisites appear first.  The order is stable
     * (deterministic for a given registration sequence).
     *
     * @return A valid phase order for every normal registration path. Returns
     *         an empty vector only if the internal DAG invariant has been
     *         violated unexpectedly (for example by a future mutation path
     *         bypassing `registerPhase()`). Callers should treat an empty
     *         result as a fatal diagnostic condition rather than a recoverable
     *         scheduling outcome.
     */
    [[nodiscard]]
    std::vector<std::string> topologicalOrder() const;

    /**
     * @brief Return the count of phases whose gate currently has status PASS.
     */
    [[nodiscard]]
    std::size_t passedPhaseCount() const;

    /**
     * @brief Return the fraction of phases that have passed (0.0 – 1.0).
     *
     * Returns 0.0 when no phases are registered.
     */
    [[nodiscard]]
    float completionRatio() const;

private:
    // -----------------------------------------------------------------------
    // Internal phase node
    // -----------------------------------------------------------------------
    struct PhaseNode {
        std::string                    name;
        std::vector<std::string>       prerequisites;
        std::vector<GateCriterion>     criteria;
        std::unordered_map<std::string, float> metrics;
    };

    // -----------------------------------------------------------------------
    // Helpers (called under the mutex)
    // -----------------------------------------------------------------------

    /// @brief DFS cycle check starting from `start`, treating `visited` as
    ///        the already-visited set.  Returns `true` if a cycle is found.
    bool hasCycleDFS(const std::string&              start,
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& rec_stack) const;

    /// @brief Internal gate evaluation (no locking — caller holds at minimum
    ///        a shared lock).
    GateStatus evaluateGate(const std::string&              phase_name,
                            std::unordered_set<std::string>& visiting) const;

    /// @brief Internal gap computation (no locking).
    PhaseGapReport computeGaps(const std::string& phase_name) const;

    /// @brief Transfer phase state from another instance while the caller holds
    ///        the required exclusive lock(s). Leaves `other` valid and empty.
    void transferFrom(GraphPhaseGateOrchestrator& other) noexcept;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    mutable std::shared_mutex                          mutex_;
    std::unordered_map<std::string, PhaseNode>         phases_;
};

}  // namespace graph
}  // namespace themis
