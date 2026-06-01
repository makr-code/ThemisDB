/**
 * @file approximation_rules.cc
 * @brief Approximation rules engine implementation stub.
 *
 * Skeleton: in-memory zone registry with basic recall checking.
 * Replace with policy persistence and alerting integration in sub-issue #5440.
 */

#include "evaluation/include/approximation_rules.h"

namespace themis::evaluation {

namespace {

class ApproximationRulesImpl final : public IApproximationRules {
public:
    ApproximationRulesImpl() {
        // Register the default EPIC 2 zone set.
        registerZone({"ann_frontdoor", "ANN candidate generation",
                       ExactnessRequirement::Approximate, 0.05f, false});
        registerZone({"tensor_midlayer", "Tensor reranking",
                       ExactnessRequirement::Approximate, 0.03f, false});
        registerZone({"graph_validator", "Graph evidence assembly",
                       ExactnessRequirement::Governed, 0.01f, true});
        registerZone({"llm_final", "LLM answer generation",
                       ExactnessRequirement::Governed, 0.05f, true});
    }

    void registerZone(ApproximationZone zone) override {
        zones_[zone.id] = std::move(zone);
    }

    bool checkRecall(const std::string& zone_id,
                      float observed_recall) const override {
        auto it = zones_.find(zone_id);
        if (it == zones_.end()) return true; // Unknown zone: allow
        if (it->second.requirement == ExactnessRequirement::Exact)
            return observed_recall >= 1.0f;
        return (1.0f - observed_recall) <= it->second.max_recall_degradation;
    }

    void reportViolation(const ApproximationViolation& v) override {
        if (callback_) callback_(v);
    }

    ExactnessRequirement requirement(const std::string& zone_id) const override {
        auto it = zones_.find(zone_id);
        if (it == zones_.end()) return ExactnessRequirement::Approximate;
        return it->second.requirement;
    }

    void onViolation(ViolationCallback cb) override {
        callback_ = std::move(cb);
    }

    std::vector<std::string> listZones() const override {
        std::vector<std::string> ids;
        for (const auto& [id, _] : zones_) ids.push_back(id);
        return ids;
    }

private:
    std::unordered_map<std::string, ApproximationZone> zones_;
    ViolationCallback callback_;
};

} // namespace

std::unique_ptr<IApproximationRules> makeApproximationRules() {
    return std::make_unique<ApproximationRulesImpl>();
}

} // namespace themis::evaluation
