#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct RankedItem {
    std::string document_id;
    double score = 0.0;
};

std::vector<RankedItem> makeRankedList(std::size_t count, double bias) {
    std::vector<RankedItem> items;
    items.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        items.push_back(RankedItem{
            "doc_" + std::to_string(index),
            bias - static_cast<double>(index) * 0.01});
    }
    return items;
}

std::vector<RankedItem> fuseRrf(const std::vector<RankedItem>& left, const std::vector<RankedItem>& right) {
    std::unordered_map<std::string, double> fused;
    fused.reserve(left.size() + right.size());

    for (std::size_t index = 0; index < left.size(); ++index) {
        fused[left[index].document_id] += 1.0 / static_cast<double>(60U + index + 1U);
    }
    for (std::size_t index = 0; index < right.size(); ++index) {
        fused[right[index].document_id] += 1.0 / static_cast<double>(60U + index + 1U);
    }

    std::vector<RankedItem> result;
    result.reserve(fused.size());
    for (const auto& [document_id, score] : fused) {
        result.push_back(RankedItem{document_id, score});
    }

    std::sort(result.begin(), result.end(), [](const RankedItem& lhs, const RankedItem& rhs) {
        return lhs.score > rhs.score;
    });
    return result;
}

void BM_HybridFusionRRF(benchmark::State& state) {
    const std::size_t candidate_count = static_cast<std::size_t>(state.range(0));
    const auto bm25 = makeRankedList(candidate_count, 1.0);
    const auto vector = makeRankedList(candidate_count, 0.9);

    for (auto _ : state) {
        auto fused = fuseRrf(bm25, vector);
        benchmark::DoNotOptimize(fused);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(candidate_count));
}

void BM_HybridFusionNormalizeAndFuse(benchmark::State& state) {
    const std::size_t candidate_count = static_cast<std::size_t>(state.range(0));
    auto bm25 = makeRankedList(candidate_count, 120.0);
    auto vector = makeRankedList(candidate_count, 0.8);

    for (auto _ : state) {
        for (auto& item : bm25) {
            item.score /= 120.0;
        }
        for (auto& item : vector) {
            item.score = (item.score + 1.0) * 0.5;
        }
        auto fused = fuseRrf(bm25, vector);
        benchmark::DoNotOptimize(fused);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(candidate_count));
}

BENCHMARK(BM_HybridFusionRRF)->Arg(1000)->Arg(5000)->Arg(10000)->UseRealTime();
BENCHMARK(BM_HybridFusionNormalizeAndFuse)->Arg(1000)->Arg(5000)->UseRealTime();

}  // namespace