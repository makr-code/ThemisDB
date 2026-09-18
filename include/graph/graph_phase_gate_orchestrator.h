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

enum class GateStatus {
    PASS,         ///< All criteria met; phase may advance.
    FAIL,         ///< One or more criteria not met; see `gaps()`.
    PENDING,      ///< Phase has not yet been evaluated.
    BLOCKED,      ///< A prerequisite phase has not passed its own gate.
};

// ---------------------------------------------------------------------------
// GateCriterion
// ---------------------------------------------------------------------------

struct GateCriterion {
    std::string metric_key;
    float threshold = 0.0f;
    std::string description;
};

// ---------------------------------------------------------------------------
// PhaseGapReport
// ---------------------------------------------------------------------------

struct PhaseGapReport {
    std::string phase_name;
    GateStatus status = GateStatus::PENDING;
    std::vector<std::string> unsatisfied_criteria;
    std::vector<std::string> blocked_by_phases;
    std::string summary;
};

// ---------------------------------------------------------------------------
// GraphPhaseGateOrchestrator
// ---------------------------------------------------------------------------

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
     * @brief Register Phase.
     * @param[in] phase_name Name of the phase.
     * @param[in] prerequisites Input parameter.
     * @return True when the operation succeeds.
     */
    bool registerPhase(const std::string&              phase_name,
                       const std::vector<std::string>& prerequisites);

    /**
     * @brief Set Gate Criteria.
     * @param[in] phase_name Name of the phase.
     * @param[in] criteria Input parameter.
     * @return True when the operation succeeds.
     */
    bool setGateCriteria(const std::string&             phase_name,
                         const std::vector<GateCriterion>& criteria);

    // -----------------------------------------------------------------------
    // Metric management
    // -----------------------------------------------------------------------

    /**
     * @brief Attach Metric.
     * @param[in] phase_name Name of the phase.
     * @param[in] metric_key Input parameter.
     * @param[in] value Input parameter.
     * @return True when the operation succeeds.
     */
    bool attachMetric(const std::string& phase_name,
                      const std::string& metric_key,
                      float              value);

    [[nodiscard]]
    /**
     * @brief Get Metric.
     * @param[in] phase_name Name of the phase.
     * @param[in] metric_key Input parameter.
     * @return Return value.
     */
    std::optional<float> getMetric(const std::string& phase_name,
                                   const std::string& metric_key) const;

    // -----------------------------------------------------------------------
    // Gate evaluation
    // -----------------------------------------------------------------------

    [[nodiscard]]
    /**
     * @brief Gate Status.
     * @param[in] phase_name Name of the phase.
     * @return Return value.
     */
    GateStatus gateStatus(const std::string& phase_name) const;

    [[nodiscard]]
    /**
     * @brief Gaps.
     * @param[in] phase_name Name of the phase.
     * @return Return value.
     */
    PhaseGapReport gaps(const std::string& phase_name) const;

    [[nodiscard]]
    /**
     * @brief All Gaps.
     * @return Return value.
     */
    std::vector<PhaseGapReport> allGaps() const;

    // -----------------------------------------------------------------------
    // Graph introspection
    // -----------------------------------------------------------------------

    [[nodiscard]]
    /**
     * @brief Phase Names.
     * @return Return value.
     */
    std::vector<std::string> phaseNames() const;

    [[nodiscard]]
    /**
     * @brief Phase Count.
     * @return Return value.
     */
    std::size_t phaseCount() const;

    [[nodiscard]]
    /**
     * @brief Has Phase.
     * @param[in] phase_name Name of the phase.
     * @return True when the operation succeeds.
     */
    bool hasPhase(const std::string& phase_name) const;

    [[nodiscard]]
    /**
     * @brief Prerequisites.
     * @param[in] phase_name Name of the phase.
     * @return Return value.
     */
    std::vector<std::string> prerequisites(const std::string& phase_name) const;

    [[nodiscard]]
    /**
     * @brief Topological Order.
     * @return Return value.
     */
    std::vector<std::string> topologicalOrder() const;

    [[nodiscard]]
    /**
     * @brief Passed Phase Count.
     * @return Return value.
     */
    std::size_t passedPhaseCount() const;

    [[nodiscard]]
    /**
     * @brief Completion Ratio.
     * @return Return value.
     */
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

    /**
     * @brief ----------------------------------------------------------------------- Helpers (called under the mutex) -----------------------------------------------------------------------
     * @param[in] start Input parameter.
     * @param[in,out] visited Input/output parameter.
     * @param[in,out] rec_stack Input/output parameter.
     * @return True when the operation succeeds.
     */

    bool hasCycleDFS(const std::string&              start,
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& rec_stack) const;

    /**
     * @brief Evaluate Gate.
     * @param[in] phase_name Name of the phase.
     * @param[in,out] visiting Input/output parameter.
     * @return Return value.
     */
    GateStatus evaluateGate(const std::string&              phase_name,
                            std::unordered_set<std::string>& visiting) const;

    /**
     * @brief Compute Gaps.
     * @param[in] phase_name Name of the phase.
     * @return Return value.
     */
    PhaseGapReport computeGaps(const std::string& phase_name) const;

    /**
     * @brief Transfer From.
     * @param[in,out] other Input/output parameter.
     * @note Exception safety: noexcept.
     */
    void transferFrom(GraphPhaseGateOrchestrator& other) noexcept;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    mutable std::shared_mutex                          mutex_;
    std::unordered_map<std::string, PhaseNode>         phases_;
};

}  // namespace graph
}  // namespace themis
