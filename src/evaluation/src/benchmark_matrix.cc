/**
 * @file benchmark_matrix.cc
 * @brief Benchmark matrix implementation stub.
 *
 * Skeleton: pre-populated scenario catalogue with no-op execution.
 * Replace with live retrieval calls in sub-issue #5438.
 */

#include "evaluation/include/benchmark_matrix.h"

namespace themis::evaluation {

namespace {

class BenchmarkMatrixImpl final : public IBenchmarkMatrix {
public:
    BenchmarkMatrixImpl() {
        // Register a representative starting set of EPIC 2 scenarios.
        registerScenario({
            .id = "ann-latency-1m",
            .description = "ANN latency on 1 M vectors, top-10",
            .category = BenchmarkCategory::AnnLatency,
            .dataset_vectors = 1'000'000,
            .query_count = 10'000,
            .top_k = 10,
            .target_latency_p99_ms = 20.0,
        });
        registerScenario({
            .id = "recall-at-10-1m",
            .description = "Recall@10 accuracy, 1 M vectors",
            .category = BenchmarkCategory::RecallAtK,
            .dataset_vectors = 1'000'000,
            .query_count = 5'000,
            .top_k = 10,
            .target_recall = 0.95f,
        });
        registerScenario({
            .id = "e2e-full-pipeline",
            .description = "End-to-end latency: ANN + tensor + graph",
            .category = BenchmarkCategory::EndToEnd,
            .dataset_vectors = 1'000'000,
            .query_count = 1'000,
            .top_k = 5,
            .target_latency_p99_ms = 500.0,
        });
    }

    void registerScenario(BenchmarkScenario scenario) override {
        scenarios_[scenario.id] = std::move(scenario);
    }

    BenchmarkResult run(const std::string& scenario_id,
                         const HardwareProfile& hw) override {
        // TODO(#5438): Execute the scenario against the live retrieval stack.
        auto it = scenarios_.find(scenario_id);
        BenchmarkResult result{
            .scenario_id = scenario_id,
            .hardware_profile_id = hw.id,
            .passed = false,
            .failure_reason = "not implemented yet",
        };
        return result;
    }

    std::vector<BenchmarkResult> runAll(const HardwareProfile& hw) override {
        std::vector<BenchmarkResult> results;
        for (const auto& [id, _] : scenarios_) {
            results.push_back(run(id, hw));
        }
        return results;
    }

    std::vector<std::string> listScenarios() const override {
        std::vector<std::string> ids;
        for (const auto& [id, _] : scenarios_) ids.push_back(id);
        return ids;
    }

    void onResult(ResultObserver obs) override {
        observer_ = std::move(obs);
    }

private:
    std::unordered_map<std::string, BenchmarkScenario> scenarios_;
    ResultObserver observer_;
};

} // namespace

std::unique_ptr<IBenchmarkMatrix> makeBenchmarkMatrix() {
    return std::make_unique<BenchmarkMatrixImpl>();
}

} // namespace themis::evaluation
