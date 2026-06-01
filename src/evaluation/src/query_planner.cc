/**
 * @file query_planner.cc
 * @brief Hybrid query planner implementation stub.
 *
 * Skeleton: heuristic path selection based on hardware profile and config.
 * Replace with learned/scoring-based planning in sub-issue #5441.
 */

#include "evaluation/include/query_planner.h"

namespace themis::evaluation {

namespace {

class QueryPlannerImpl final : public IQueryPlanner {
public:
    QueryPlannerImpl(QueryPlannerConfig cfg,
                     std::shared_ptr<IApproximationRules> rules)
        : cfg_(std::move(cfg)), rules_(std::move(rules)) {}

    QueryPlan plan(const PlannerRequest& req,
                   const HardwareProfile& hw) const override {
        QueryPlan p;

        if (req.require_exact) {
            p.path = QueryPath::Exact;
            p.rationale = "Exact result required by caller";
            p.estimated_recall = 1.0f;
            return p;
        }

        if (!cfg_.prefer_local || cfg_.enable_distributed) {
            p.path = cfg_.enable_graph ? QueryPath::DistributedFull
                                       : QueryPath::DistributedAnn;
            p.rationale = "Distributed path selected by config";
        } else if (cfg_.enable_graph && req.require_provenance) {
            p.path = QueryPath::FullLayered;
            p.rationale = "Provenance required; full layered path";
        } else {
            p.path = hw.supports_hot_path ? QueryPath::LocalAnnTensor
                                          : QueryPath::LocalAnn;
            p.rationale = hw.supports_hot_path
                ? "Hot path available; ANN + tensor"
                : "Cold path; ANN only";
        }

        p.estimated_recall = cfg_.min_recall_threshold;
        return p;
    }

    void reconfigure(const QueryPlannerConfig& cfg) override { cfg_ = cfg; }

    QueryPlannerConfig config() const override { return cfg_; }

private:
    QueryPlannerConfig cfg_;
    std::shared_ptr<IApproximationRules> rules_;
};

} // namespace

std::unique_ptr<IQueryPlanner> makeQueryPlanner(
    const QueryPlannerConfig& cfg,
    std::shared_ptr<IApproximationRules> rules) {
    return std::make_unique<QueryPlannerImpl>(cfg, std::move(rules));
}

} // namespace themis::evaluation
