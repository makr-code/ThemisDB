/**
 * @file evaluation_metrics.cc
 * @brief Evaluation metrics collector implementation stub.
 *
 * Skeleton: in-memory accumulator.  Replace with time-series persistence
 * and ablation runner in sub-issue #5439.
 */

#include "evaluation/include/evaluation_metrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis::evaluation {

namespace {

class EvaluationMetricsImpl final : public IEvaluationMetrics {
public:
    void record(const MetricObservation& obs) override {
        data_[obs.experiment_id][obs.dimension].push_back(obs.value);
    }

    MetricReport report(MetricDimension dim,
                         const std::string& experiment_id) const override {
        MetricReport r{.dimension = dim, .experiment_id = experiment_id};
        auto eit = data_.find(experiment_id);
        if (eit == data_.end()) return r;
        auto dit = eit->second.find(dim);
        if (dit == eit->second.end()) return r;

        const auto& vals = dit->second;
        r.n = vals.size();
        if (r.n == 0) return r;

        r.mean = std::accumulate(vals.begin(), vals.end(), 0.0) / r.n;

        double sq = 0.0;
        for (double v : vals) sq += (v - r.mean) * (v - r.mean);
        r.stddev = std::sqrt(sq / r.n);

        auto sorted = vals;
        std::sort(sorted.begin(), sorted.end());
        r.p50 = sorted[sorted.size() / 2];
        r.p99 = sorted[static_cast<std::size_t>(sorted.size() * 0.99)];
        return r;
    }

    std::vector<MetricReport> reportAll(
        const std::string& experiment_id) const override {
        std::vector<MetricReport> out;
        auto eit = data_.find(experiment_id);
        if (eit == data_.end()) return out;
        for (const auto& [dim, _] : eit->second)
            out.push_back(report(dim, experiment_id));
        return out;
    }

    void registerVariant(AblationVariant variant) override {
        variants_[variant.id] = std::move(variant);
    }

    std::vector<std::string> listVariants() const override {
        std::vector<std::string> ids;
        for (const auto& [id, _] : variants_) ids.push_back(id);
        return ids;
    }

    void reset() override { data_.clear(); }

private:
    // experiment_id → dimension → observations
    std::unordered_map<std::string,
        std::unordered_map<MetricDimension,
            std::vector<double>>> data_;
    std::unordered_map<std::string, AblationVariant> variants_;
};

} // namespace

std::unique_ptr<IEvaluationMetrics> makeEvaluationMetrics() {
    return std::make_unique<EvaluationMetricsImpl>();
}

} // namespace themis::evaluation
