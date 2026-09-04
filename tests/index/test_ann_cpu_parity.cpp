// Phase B gate test for ANN + CPU parity on Category A kernels.

#include "index/ann_frontdoor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

using namespace themis::index;

namespace {

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

        std::vector<AnnSearchResult> scored = {};

        scored.reserve(entries_.size());
        for (const auto& e : entries_) {
            if (e.values.size() != dim) {
                continue;
            }
            float distance = 0.0F;
            for (size_t i = 0; i < dim; ++i) {
                const float diff = e.values[i] - query[i];
                distance += diff * diff;
            }
            scored.push_back({e.id, distance});
        }

        std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
            if (a.distance == b.distance) {
                return a.id < b.id;
            }
            return a.distance < b.distance;
        });

        if (k < static_cast<int>(scored.size())) {
            scored.resize(static_cast<size_t>(k));
        }
        return scored;
    }

    [[nodiscard]] size_t size() const override { return entries_.size(); }

private:
    std::vector<Entry> entries_;
};

std::shared_ptr<DeterministicFlatIndex> makeDeterministicIndex() {
    std::vector<DeterministicFlatIndex::Entry> entries = {
        {101, {0.10F, 0.10F, 0.00F, 0.00F}},
        {102, {0.95F, 0.05F, 0.00F, 0.00F}},
        {103, {0.85F, 0.15F, 0.00F, 0.00F}},
        {104, {0.20F, 0.80F, 0.00F, 0.00F}},
        {105, {0.00F, 1.00F, 0.00F, 0.00F}},
    };
    return std::make_shared<DeterministicFlatIndex>(std::move(entries));
}

void expectParity(const AnnFrontdoorResult& lhs,
                  const AnnFrontdoorResult& rhs,
                  float tolerance = 1e-6F) {
    ASSERT_EQ(lhs.candidates.size(), rhs.candidates.size());
    for (size_t i = 0; i < lhs.candidates.size(); ++i) {
        EXPECT_EQ(lhs.candidates[i].id, rhs.candidates[i].id);
        EXPECT_NEAR(lhs.candidates[i].distance, rhs.candidates[i].distance, tolerance);
    }
}

} // namespace

TEST(AnnCpuParityPhaseB, MatchesFlatBaselineForTopK) {
    AnnFrontdoor frontdoor;
    frontdoor.registerBackend("cpu", makeDeterministicIndex(), AnnScopeKind::Document);
    frontdoor.registerBackend("flat", makeDeterministicIndex(), AnnScopeKind::Document);

    static constexpr float kQuery[] = {1.0F, 0.0F, 0.0F, 0.0F};
    static constexpr size_t kDim = 4;
    static constexpr int kTopK = 3;

    AnnQueryContext cpu_context;
    cpu_context.scope_id = "cpu";
    cpu_context.dataset_size = 200'000;
    cpu_context.hot_tier = true;

    AnnQueryContext flat_context;
    flat_context.scope_id = "flat";
    flat_context.dataset_size = 200'000;
    flat_context.hot_tier = true;

    const auto cpu_result = frontdoor.search(kQuery, kDim, kTopK, cpu_context);
    const auto flat_result = frontdoor.search(kQuery, kDim, kTopK, flat_context);

    expectParity(cpu_result, flat_result);
}

TEST(AnnCpuParityPhaseB, EnsuresOutputCardinalityAndDistanceOrdering) {
    AnnFrontdoor frontdoor;
    frontdoor.registerBackend("cpu", makeDeterministicIndex(), AnnScopeKind::Document);

    static constexpr float kQuery[] = {0.90F, 0.10F, 0.00F, 0.00F};
    static constexpr size_t kDim = 4;
    static constexpr int kTopK = 4;

    AnnQueryContext context;
    context.scope_id = "cpu";
    context.dataset_size = 200'000;
    context.hot_tier = true;

    const auto result = frontdoor.search(kQuery, kDim, kTopK, context);
    ASSERT_EQ(result.candidates.size(), static_cast<size_t>(kTopK));

    EXPECT_TRUE(std::is_sorted(result.candidates.begin(), result.candidates.end(),
                               [](const auto& a, const auto& b) {
                                   if (a.distance == b.distance) {
                                       return a.id <= b.id;
                                   }
                                   return a.distance < b.distance;
                               }));

    for (const auto& candidate : result.candidates) {
        EXPECT_GE(candidate.distance, 0.0F);
        EXPECT_TRUE(std::isfinite(candidate.distance));
        EXPECT_LT(candidate.distance, std::numeric_limits<float>::max());
    }
}

