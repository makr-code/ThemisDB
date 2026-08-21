// SIMULATION NOTE:
// Purpose: Synthetic distributed merge benchmark for shard aggregation and ranking escalation.
// Activation: Always active in benchmark builds; it does not exercise the production distributed merge pipeline.
// Production Delta: Shards are generated in-memory and merged with a deterministic sort, rather than the live distributed coordinator/runtime path.
// Removal Plan: Replace with real distributed merge benchmarks once the coordinator and ranker APIs are exposed for benchmark instrumentation.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

struct SearchResult {
    std::string document_id;
    double score = 0.0;
};

std::vector<std::vector<SearchResult>> makeShardResults(std::size_t shard_count, std::size_t results_per_shard) {
    std::vector<std::vector<SearchResult>> shards;
    shards.reserve(shard_count);

    for (std::size_t shard = 0; shard < shard_count; ++shard) {
        std::vector<SearchResult> results;
        results.reserve(results_per_shard);
        for (std::size_t index = 0; index < results_per_shard; ++index) {
            results.push_back(SearchResult{
                "doc_" + std::to_string(shard) + '_' + std::to_string(index),
                1.0 / static_cast<double>(index + 1U) + static_cast<double>(shard) * 0.001});
        }
        shards.push_back(std::move(results));
    }

    return shards;
}

void BM_DistributedMergeGates(benchmark::State& state) {
    const std::size_t shard_count = static_cast<std::size_t>(state.range(0));
    const std::size_t results_per_shard = static_cast<std::size_t>(state.range(1));
    const auto shards = makeShardResults(shard_count, results_per_shard);

    for (auto _ : state) {
        std::vector<SearchResult> merged;
        merged.reserve(shard_count * results_per_shard);
        for (const auto& shard : shards) {
            merged.insert(merged.end(), shard.begin(), shard.end());
        }

        std::sort(merged.begin(), merged.end(), [](const SearchResult& lhs, const SearchResult& rhs) {
            return lhs.score > rhs.score;
        });

        benchmark::DoNotOptimize(merged.data());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(shard_count * results_per_shard));
}

BENCHMARK(BM_DistributedMergeGates)
    ->Args({16, 256})
    ->Args({32, 512})
    ->Args({64, 1024})
    ->UseRealTime();

}  // namespace