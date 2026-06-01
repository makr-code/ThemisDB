/**
 * @file query_planner.h
 * @brief Hybrid query planner for ANN, tensor, graph, and distributed retrieval.
 *
 * Routes incoming retrieval requests to the appropriate combination of
 * layers and backends, using hardware profiles and approximation policies.
 *
 * Planned in: docs/EPIC2_QUERY_PLANNER.md
 * Sub-issue:   #5441
 */

#pragma once

#include "hardware_profile.h"
#include "approximation_rules.h"

#include <chrono>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace themis::evaluation {

/// The retrieval path selected by the planner for a request.
enum class QueryPath {
    LocalAnn,         ///< ANN-only, in-process, hot path
    LocalAnnTensor,   ///< ANN + tensor mid-layer
    FullLayered,      ///< ANN + tensor + graph + LLM
    DistributedAnn,   ///< Fan-out to remote ANN shards
    DistributedFull,  ///< Distributed version of FullLayered
    Exact,            ///< Brute-force scan (exactness governed zone)
};

/// Request descriptor for the query planner.
struct PlannerRequest {
    std::vector<float>         embedding;
    std::string                query_text;
    std::uint32_t              top_k = 10;
    std::string                namespace_scope;
    std::chrono::milliseconds  budget{500}; ///< Soft latency budget
    bool                       require_provenance = false;
    bool                       require_exact      = false;
};

/// The plan produced for a single request.
struct QueryPlan {
    QueryPath    path;
    std::string  rationale;       ///< Human-readable routing explanation
    double       estimated_latency_ms = 0.0;
    float        estimated_recall    = 1.0f;
    std::vector<std::string> shard_targets; ///< Shards to contact (distributed)
};

/// Configuration for the query planner.
struct QueryPlannerConfig {
    bool  prefer_local  = true;
    bool  enable_graph  = true;
    bool  enable_distributed = false;
    float min_recall_threshold = 0.90f;
};

/**
 * @brief Hybrid query planner interface.
 *
 * Given a request, a hardware profile, and the active approximation policy,
 * produces an executable query plan.
 */
class IQueryPlanner {
public:
    virtual ~IQueryPlanner() = default;

    /// Produce a query plan for the given request.
    virtual QueryPlan plan(const PlannerRequest& req,
                           const HardwareProfile& hw) const = 0;

    /// Update configuration at runtime.
    virtual void reconfigure(const QueryPlannerConfig& cfg) = 0;

    /// Return the current effective configuration.
    virtual QueryPlannerConfig config() const = 0;
};

/// Factory: create a query planner with the given initial configuration.
std::unique_ptr<IQueryPlanner> makeQueryPlanner(
    const QueryPlannerConfig& cfg,
    std::shared_ptr<IApproximationRules> rules = nullptr);

} // namespace themis::evaluation
