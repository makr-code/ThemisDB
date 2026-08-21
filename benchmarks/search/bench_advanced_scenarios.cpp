// SIMULATION NOTE:
// Purpose: Synthetic search benchmark covering multimodal fusion, learning-to-rank, and concurrent indexing scenarios.
// Activation: Benchmark-only synthetic data path for release-gate estimation.
// Production Delta: Uses generated scores and in-memory merges instead of the live retrieval/runtime stack.
// Removal Plan: Replace with production-path benchmarks once these search flows are exposed through the real runtime APIs.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ScenarioItem {
    std::string id;
    double text_score = 0.0;
    double image_score = 0.0;
    double code_score = 0.0;
};

std::vector<ScenarioItem> makeItems(std::size_t count) {
    std::vector<ScenarioItem> items;
    items.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        items.push_back(ScenarioItem{
            "item_" + std::to_string(index),
            1.0 / static_cast<double>(index + 1U),
            0.8 / static_cast<double>(index + 1U),
            0.6 / static_cast<double>(index + 1U)});
    }
    return items;
}

void BM_AdvancedSearchMultimodalFusion(benchmark::State& state) {
    const std::size_t item_count = static_cast<std::size_t>(state.range(0));
    const auto items = makeItems(item_count);

    for (auto _ : state) {
        double total = 0.0;
        for (const auto& item : items) {
            total += item.text_score * 0.5 + item.image_score * 0.3 + item.code_score * 0.2;
        }
        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(item_count));
}

void BM_AdvancedSearchLearningToRank(benchmark::State& state) {
    const std::size_t item_count = static_cast<std::size_t>(state.range(0));
    auto items = makeItems(item_count);

    for (auto _ : state) {
        std::sort(items.begin(), items.end(), [](const ScenarioItem& lhs, const ScenarioItem& rhs) {
            const double lhs_rank = lhs.text_score + lhs.image_score * 1.5 + lhs.code_score * 2.0;
            const double rhs_rank = rhs.text_score + rhs.image_score * 1.5 + rhs.code_score * 2.0;
            return lhs_rank > rhs_rank;
        });
        benchmark::DoNotOptimize(items.data());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(item_count));
}

void BM_AdvancedSearchConcurrentIndexing(benchmark::State& state) {
    const std::size_t item_count = static_cast<std::size_t>(state.range(0));
    const auto items = makeItems(item_count);
    std::mutex mutex;

    for (auto _ : state) {
        std::vector<ScenarioItem> merged;
        merged.reserve(item_count);

        auto worker = [&items, &merged, &mutex](std::size_t begin, std::size_t end) {
            std::vector<ScenarioItem> local;
            local.reserve(end - begin);
            for (std::size_t index = begin; index < end; ++index) {
                local.push_back(items[index]);
            }
            std::lock_guard<std::mutex> lock(mutex);
            merged.insert(merged.end(), local.begin(), local.end());
        };

        const std::size_t midpoint = item_count / 2U;
        std::thread first(worker, 0U, midpoint);
        std::thread second(worker, midpoint, item_count);
        first.join();
        second.join();

        benchmark::DoNotOptimize(merged.size());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(item_count));
}

BENCHMARK(BM_AdvancedSearchMultimodalFusion)->Arg(1000)->Arg(5000)->UseRealTime();
BENCHMARK(BM_AdvancedSearchLearningToRank)->Arg(1000)->Arg(5000)->UseRealTime();
BENCHMARK(BM_AdvancedSearchConcurrentIndexing)->Arg(1000)->Arg(5000)->UseRealTime();

}  // namespace