#include "index/ann_frontdoor.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace {

using themis::index::AnnFrontdoor;
using themis::index::AnnQueryContext;
using themis::index::AnnSearchResult;
using themis::index::AnnScopeKind;
using themis::index::IAnnIndex;

class DeterministicFlatIndex final : public IAnnIndex {
public:
    struct Entry {
        int64_t id = 0;
        std::vector<float> values;
    };

    explicit DeterministicFlatIndex(std::vector<Entry> entries)
        : entries_(std::move(entries)) {}

    bool build(const float*, const int64_t*, size_t, size_t) override { return true; }

    [[nodiscard]] bool add(int64_t id, const float* vector, size_t dim) override {
        if (!vector || dim == 0) {
            return false;
        }
        Entry e;
        e.id = id;
        e.values.assign(vector, vector + dim);
        entries_.push_back(std::move(e));
        return true;
    }

    std::vector<AnnSearchResult> search(const float* query, size_t dim, int k) const override {
        if (!query || dim == 0 || k <= 0) {
            return {};
        }
        std::vector<AnnSearchResult> out = {};

        out.reserve(entries_.size());
        for (const auto& e : entries_) {
            if (e.values.size() != dim) {
                continue;
            }
            float distance = 0.0F;
            for (size_t i = 0; i < dim; ++i) {
                const float diff = e.values[i] - query[i];
                distance += diff * diff;
            }
            out.push_back({e.id, distance});
        }
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
            if (a.distance == b.distance) {
                return a.id < b.id;
            }
            return a.distance < b.distance;
        });
        if (k < static_cast<int>(out.size())) {
            out.resize(static_cast<size_t>(k));
        }
        return out;
    }

    [[nodiscard]] size_t size() const override { return entries_.size(); }

private:
    std::vector<Entry> entries_;
};

std::shared_ptr<DeterministicFlatIndex> makeDataset() {
    std::vector<DeterministicFlatIndex::Entry> entries;
    entries.reserve(1024);
    for (int i = 0; i < 1024; ++i) {
        const float x = static_cast<float>(i % 64) / 64.0F;
        const float y = static_cast<float>(i / 64) / 16.0F;
        entries.push_back({1000 + i, {x, y, x * 0.5F, y * 0.5F}});
    }
    return std::make_shared<DeterministicFlatIndex>(std::move(entries));
}

void BM_AnnDistanceCpuVsFlat(benchmark::State& state) {
    const int top_k = static_cast<int>(state.range(0));
    static constexpr float kQuery[] = {0.35F, 0.42F, 0.17F, 0.21F};
    static constexpr size_t kDim = 4;

    AnnFrontdoor frontdoor;
    frontdoor.registerBackend("cpu", makeDataset(), AnnScopeKind::Document);
    frontdoor.registerBackend("flat", makeDataset(), AnnScopeKind::Document);

    AnnQueryContext cpu_ctx;
    cpu_ctx.scope_id = "cpu";
    cpu_ctx.dataset_size = 200'000;
    cpu_ctx.hot_tier = true;

    AnnQueryContext flat_ctx;
    flat_ctx.scope_id = "flat";
    flat_ctx.dataset_size = 200'000;
    flat_ctx.hot_tier = true;

    for (auto _ : state) {
        const auto cpu_result = frontdoor.search(kQuery, kDim, top_k, cpu_ctx);
        const auto flat_result = frontdoor.search(kQuery, kDim, top_k, flat_ctx);

        if (cpu_result.candidates.size() != flat_result.candidates.size()) {
            state.SkipWithError("ANN parity failed: candidate size mismatch");
            break;
        }
        for (size_t i = 0; i < cpu_result.candidates.size(); ++i) {
            if (cpu_result.candidates[i].id != flat_result.candidates[i].id) {
                state.SkipWithError("ANN parity failed: top-k identity mismatch");
                break;
            }
        }

        benchmark::DoNotOptimize(cpu_result);
        benchmark::DoNotOptimize(flat_result);
    }

    state.SetItemsProcessed(state.iterations() * top_k);
}

BENCHMARK(BM_AnnDistanceCpuVsFlat)->Arg(10)->Arg(25)->Arg(50)->Unit(benchmark::kMicrosecond);

} // namespace

