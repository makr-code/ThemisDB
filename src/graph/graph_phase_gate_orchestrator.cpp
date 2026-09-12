/**
 * @file graph_phase_gate_orchestrator.cpp
 * @brief Wave C C3 — Graph Phase Gate Orchestrator implementation.
 *
 * See `include/graph/graph_phase_gate_orchestrator.h` for the full API
 * contract and design notes.
 *
 * Wave C issue: #6287
 */

#include "graph/graph_phase_gate_orchestrator.h"

#include <algorithm>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace graph {

void GraphPhaseGateOrchestrator::transferFrom(
    GraphPhaseGateOrchestrator& other) noexcept
{
    phases_ = std::move(other.phases_);
    other.phases_.clear();
}

GraphPhaseGateOrchestrator::GraphPhaseGateOrchestrator(
    GraphPhaseGateOrchestrator&& other) noexcept
{
    // `this` is still under construction, so only the source mutex needs to be
    // locked while the phase graph is transferred.
    std::unique_lock<std::shared_mutex> lock(other.mutex_);
    transferFrom(other);
}

GraphPhaseGateOrchestrator& GraphPhaseGateOrchestrator::operator=(
    GraphPhaseGateOrchestrator&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    std::unique_lock<std::shared_mutex> this_lock(mutex_, std::defer_lock);
    std::unique_lock<std::shared_mutex> other_lock(other.mutex_, std::defer_lock);
    std::lock(this_lock, other_lock);

    transferFrom(other);
    return *this;
}

// ---------------------------------------------------------------------------
// registerPhase
// ---------------------------------------------------------------------------

bool GraphPhaseGateOrchestrator::registerPhase(
    const std::string&              phase_name,
    const std::vector<std::string>& prerequisites)
{
    if (phase_name.empty()) {
        return false;
    }

    std::unique_lock lock(mutex_);

    if (phases_.count(phase_name) != 0) {
        return false;  // already registered
    }

    // Verify all prerequisites exist.
    for (const auto& pre : prerequisites) {
        if (pre == phase_name) {
            return false;  // self-reference would break the DAG invariant
        }
        if (phases_.count(pre) == 0) {
            return false;  // prerequisite not yet registered
        }
    }

    // Tentatively insert so that cycle detection can traverse the full graph.
    PhaseNode node;
    node.name          = phase_name;
    node.prerequisites = prerequisites;
    phases_.emplace(phase_name, std::move(node));

    // Defensive invariant check: the public API shape already prevents cycle
    // introduction by only allowing prerequisites that are pre-registered.
    // Keep the DFS as a safety net in case future mutations relax that rule.
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> rec_stack;
    for (const auto& [name, _] : phases_) {
        if (visited.count(name) == 0) {
            if (hasCycleDFS(name, visited, rec_stack)) {
                // Roll back the insertion.
                phases_.erase(phase_name);
                return false;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// setGateCriteria
// ---------------------------------------------------------------------------

bool GraphPhaseGateOrchestrator::setGateCriteria(
    const std::string&                phase_name,
    const std::vector<GateCriterion>& criteria)
{
    std::unique_lock lock(mutex_);

    auto it = phases_.find(phase_name);
    if (it == phases_.end()) {
        return false;
    }

    it->second.criteria = criteria;
    return true;
}

// ---------------------------------------------------------------------------
// attachMetric
// ---------------------------------------------------------------------------

bool GraphPhaseGateOrchestrator::attachMetric(const std::string& phase_name,
                                               const std::string& metric_key,
                                               float              value)
{
    std::unique_lock lock(mutex_);

    auto it = phases_.find(phase_name);
    if (it == phases_.end()) {
        return false;
    }

    it->second.metrics[metric_key] = value;
    return true;
}

// ---------------------------------------------------------------------------
// getMetric
// ---------------------------------------------------------------------------

std::optional<float> GraphPhaseGateOrchestrator::getMetric(
    const std::string& phase_name,
    const std::string& metric_key) const
{
    std::shared_lock lock(mutex_);

    auto pit = phases_.find(phase_name);
    if (pit == phases_.end()) {
        return std::nullopt;
    }

    const auto& metrics = pit->second.metrics;
    auto mit = metrics.find(metric_key);
    if (mit == metrics.end()) {
        return std::nullopt;
    }

    return mit->second;
}

// ---------------------------------------------------------------------------
// gateStatus
// ---------------------------------------------------------------------------

GateStatus GraphPhaseGateOrchestrator::gateStatus(const std::string& phase_name) const
{
    std::shared_lock lock(mutex_);

    if (phases_.count(phase_name) == 0) {
        return GateStatus::PENDING;
    }

    std::unordered_set<std::string> visiting;
    return evaluateGate(phase_name, visiting);
}

// ---------------------------------------------------------------------------
// gaps
// ---------------------------------------------------------------------------

PhaseGapReport GraphPhaseGateOrchestrator::gaps(const std::string& phase_name) const
{
    std::shared_lock lock(mutex_);

    if (phases_.count(phase_name) == 0) {
        PhaseGapReport r;
        r.phase_name = phase_name;
        r.status     = GateStatus::PENDING;
        r.summary    = "Phase '" + phase_name + "' is not registered.";
        return r;
    }

    return computeGaps(phase_name);
}

// ---------------------------------------------------------------------------
// allGaps
// ---------------------------------------------------------------------------

std::vector<PhaseGapReport> GraphPhaseGateOrchestrator::allGaps() const
{
    std::shared_lock lock(mutex_);

    std::vector<PhaseGapReport> result;
    for (const auto& [name, _] : phases_) {
        auto r = computeGaps(name);
        if (r.status != GateStatus::PASS) {
            result.push_back(std::move(r));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// phaseNames
// ---------------------------------------------------------------------------

std::vector<std::string> GraphPhaseGateOrchestrator::phaseNames() const
{
    std::shared_lock lock(mutex_);

    std::vector<std::string> names;
    names.reserve(phases_.size());
    for (const auto& [name, _] : phases_) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// ---------------------------------------------------------------------------
// phaseCount
// ---------------------------------------------------------------------------

std::size_t GraphPhaseGateOrchestrator::phaseCount() const
{
    std::shared_lock lock(mutex_);
    return phases_.size();
}

// ---------------------------------------------------------------------------
// hasPhase
// ---------------------------------------------------------------------------

bool GraphPhaseGateOrchestrator::hasPhase(const std::string& phase_name) const
{
    std::shared_lock lock(mutex_);
    return phases_.count(phase_name) != 0;
}

// ---------------------------------------------------------------------------
// prerequisites
// ---------------------------------------------------------------------------

std::vector<std::string> GraphPhaseGateOrchestrator::prerequisites(
    const std::string& phase_name) const
{
    std::shared_lock lock(mutex_);

    auto it = phases_.find(phase_name);
    if (it == phases_.end()) {
        return {};
    }
    return it->second.prerequisites;
}

// ---------------------------------------------------------------------------
// topologicalOrder
// ---------------------------------------------------------------------------

std::vector<std::string> GraphPhaseGateOrchestrator::topologicalOrder() const
{
    std::shared_lock lock(mutex_);

    // Kahn's algorithm — deterministic by sorting each round.
    std::unordered_map<std::string, int> in_degree;
    for (const auto& [name, node] : phases_) {
        if (in_degree.count(name) == 0) {
            in_degree[name] = 0;
        }
        for (const auto& pre : node.prerequisites) {
            in_degree[name]++;  // edge pre → name
            if (in_degree.count(pre) == 0) {
                in_degree[pre] = 0;
            }
        }
    }

    // Seed the queue with in-degree-0 nodes (sorted for determinism).
    std::vector<std::string> ready;
    for (const auto& [name, deg] : in_degree) {
        if (deg == 0) {
            ready.push_back(name);
        }
    }
    std::sort(ready.begin(), ready.end());

    std::vector<std::string> order;
    order.reserve(phases_.size());

    while (!ready.empty()) {
        std::string node = ready.front();
        ready.erase(ready.begin());
        order.push_back(node);

        // Decrease in-degree for nodes that have `node` as prerequisite.
        std::vector<std::string> newly_ready;
        for (const auto& [name, phase] : phases_) {
            for (const auto& pre : phase.prerequisites) {
                if (pre == node) {
                    in_degree[name]--;
                    if (in_degree[name] == 0) {
                        newly_ready.push_back(name);
                    }
                }
            }
        }
        std::sort(newly_ready.begin(), newly_ready.end());
        ready.insert(ready.end(), newly_ready.begin(), newly_ready.end());
        std::sort(ready.begin(), ready.end());
    }

    if (order.size() != phases_.size()) {
        // Cycle detected — should not happen if registerPhase is used correctly.
        return {};
    }
    return order;
}

// ---------------------------------------------------------------------------
// passedPhaseCount
// ---------------------------------------------------------------------------

std::size_t GraphPhaseGateOrchestrator::passedPhaseCount() const
{
    std::shared_lock lock(mutex_);

    std::size_t count = 0;
    for (const auto& [name, _] : phases_) {
        std::unordered_set<std::string> visiting;
        if (evaluateGate(name, visiting) == GateStatus::PASS) {
            ++count;
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// completionRatio
// ---------------------------------------------------------------------------

float GraphPhaseGateOrchestrator::completionRatio() const
{
    std::shared_lock lock(mutex_);

    if (phases_.empty()) {
        return 0.0f;
    }

    std::size_t passed = 0;
    for (const auto& [name, _] : phases_) {
        std::unordered_set<std::string> visiting;
        if (evaluateGate(name, visiting) == GateStatus::PASS) {
            ++passed;
        }
    }
    return static_cast<float>(passed) / static_cast<float>(phases_.size());
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool GraphPhaseGateOrchestrator::hasCycleDFS(
    const std::string&              start,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& rec_stack) const
{
    visited.insert(start);
    rec_stack.insert(start);

    auto it = phases_.find(start);
    if (it != phases_.end()) {
        for (const auto& pre : it->second.prerequisites) {
            if (visited.count(pre) == 0) {
                if (hasCycleDFS(pre, visited, rec_stack)) {
                    return true;
                }
            } else if (rec_stack.count(pre) != 0) {
                return true;
            }
        }
    }

    rec_stack.erase(start);
    return false;
}

GateStatus GraphPhaseGateOrchestrator::evaluateGate(
    const std::string&              phase_name,
    std::unordered_set<std::string>& visiting) const
{
    auto pit = phases_.find(phase_name);
    if (pit == phases_.end()) {
        return GateStatus::PENDING;
    }

    // Guard against infinite recursion (should not happen in a valid DAG).
    if (visiting.count(phase_name) != 0) {
        return GateStatus::BLOCKED;
    }
    visiting.insert(phase_name);

    const PhaseNode& node = pit->second;

    // 1. Check prerequisite gates first.
    for (const auto& pre : node.prerequisites) {
        GateStatus pre_status = evaluateGate(pre, visiting);
        if (pre_status != GateStatus::PASS) {
            visiting.erase(phase_name);
            return GateStatus::BLOCKED;
        }
    }

    // 2. Check metric criteria.
    for (const auto& criterion : node.criteria) {
        auto mit = node.metrics.find(criterion.metric_key);
        if (mit == node.metrics.end()) {
            visiting.erase(phase_name);
            return GateStatus::FAIL;  // metric absent
        }
        if (mit->second < criterion.threshold) {
            visiting.erase(phase_name);
            return GateStatus::FAIL;  // metric below threshold
        }
    }

    visiting.erase(phase_name);
    return GateStatus::PASS;
}

PhaseGapReport GraphPhaseGateOrchestrator::computeGaps(const std::string& phase_name) const
{
    PhaseGapReport report;
    report.phase_name = phase_name;

    auto pit = phases_.find(phase_name);
    if (pit == phases_.end()) {
        report.status  = GateStatus::PENDING;
        report.summary = "Phase '" + phase_name + "' is not registered.";
        return report;
    }

    const PhaseNode& node = pit->second;

    // Prerequisite gaps.
    for (const auto& pre : node.prerequisites) {
        std::unordered_set<std::string> visiting;
        GateStatus pre_status = evaluateGate(pre, visiting);
        if (pre_status != GateStatus::PASS) {
            report.blocked_by_phases.push_back(pre);
        }
    }

    // Metric criteria gaps.
    for (const auto& criterion : node.criteria) {
        auto mit = node.metrics.find(criterion.metric_key);
        bool satisfied = false;
        if (mit != node.metrics.end() && mit->second >= criterion.threshold) {
            satisfied = true;
        }
        if (!satisfied) {
            std::ostringstream oss;
            oss << criterion.metric_key
                << " (required ≥ " << criterion.threshold;
            if (mit != node.metrics.end()) {
                oss << ", current " << mit->second;
            } else {
                oss << ", not yet attached";
            }
            if (!criterion.description.empty()) {
                oss << " — " << criterion.description;
            }
            oss << ")";
            report.unsatisfied_criteria.push_back(oss.str());
        }
    }

    if (report.blocked_by_phases.empty() && report.unsatisfied_criteria.empty()) {
        report.status  = GateStatus::PASS;
        report.summary = "Phase '" + phase_name + "' gate PASS.";
    } else if (!report.blocked_by_phases.empty()) {
        report.status = GateStatus::BLOCKED;
        std::ostringstream oss;
        oss << "Phase '" << phase_name << "' blocked by: ";
        for (std::size_t i = 0; i < report.blocked_by_phases.size(); ++i) {
            if (i != 0) oss << ", ";
            oss << report.blocked_by_phases[i];
        }
        oss << ".";
        if (!report.unsatisfied_criteria.empty()) {
            oss << "  Also has " << report.unsatisfied_criteria.size()
                << " unsatisfied criterion(a).";
        }
        report.summary = oss.str();
    } else {
        report.status = GateStatus::FAIL;
        std::ostringstream oss;
        oss << "Phase '" << phase_name << "' gate FAIL — "
            << report.unsatisfied_criteria.size() << " unsatisfied criterion(a).";
        report.summary = oss.str();
    }

    return report;
}

}  // namespace graph
}  // namespace themis
