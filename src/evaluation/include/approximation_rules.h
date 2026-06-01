/**
 * @file approximation_rules.h
 * @brief Approximation boundary rules and governance enforcement.
 *
 * Codifies which retrieval zones may use approximate results and which
 * require exact answers, and enforces these constraints at runtime.
 *
 * Planned in: docs/EPIC2_APPROXIMATION_GOVERNANCE.md
 * Sub-issue:   #5440
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::evaluation {

/// Exactness requirement for a retrieval zone.
enum class ExactnessRequirement {
    Approximate, ///< ANN / probabilistic results acceptable
    Exact,       ///< Must produce deterministic, ground-truth results
    Governed,    ///< Policy-controlled; may vary per request
};

/// A named approximation zone (e.g., "ann_frontdoor", "tensor_midlayer").
struct ApproximationZone {
    std::string          id;
    std::string          description;
    ExactnessRequirement requirement;
    float                max_recall_degradation = 0.05f; ///< Allowed recall drop
    bool                 audit_required = false;
};

/// Violation detected when exactness constraints are breached.
struct ApproximationViolation {
    std::string zone_id;
    float       observed_recall;
    float       allowed_recall;
    std::string request_id;
};

/**
 * @brief Approximation rules engine.
 *
 * Registers zones, evaluates recall constraints at query time, and emits
 * violations for audit and alerting pipelines.
 */
class IApproximationRules {
public:
    virtual ~IApproximationRules() = default;

    /// Register an approximation zone definition.
    virtual void registerZone(ApproximationZone zone) = 0;

    /// Check whether a given recall value satisfies the zone's constraints.
    virtual bool checkRecall(const std::string& zone_id,
                              float observed_recall) const = 0;

    /// Record a violation and invoke registered callbacks.
    virtual void reportViolation(const ApproximationViolation& v) = 0;

    /// Return the exactness requirement for a zone.
    virtual ExactnessRequirement requirement(const std::string& zone_id) const = 0;

    /// Register a violation observer.
    using ViolationCallback = std::function<void(const ApproximationViolation&)>;
    virtual void onViolation(ViolationCallback cb) = 0;

    /// List all registered zone IDs.
    virtual std::vector<std::string> listZones() const = 0;
};

/// Factory: create an approximation rules engine with the default Epic-2 zone set.
std::unique_ptr<IApproximationRules> makeApproximationRules();

} // namespace themis::evaluation
