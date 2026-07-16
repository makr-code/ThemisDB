// Phase A gate test for single-shard exact-first ANN routing.

#include "index/ann_frontdoor.h"
#include "observability/metrics_collector.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace themis::index;

namespace {

class SingleShardAnnIndex final : public IAnnIndex {
public:
    bool build(const float*, const int64_t*, size_t, size_t) override { return true; }
    [[nodiscard]] bool add(int64_t, const float*, size_t) override { return true; }

    std::vector<AnnSearchResult> search(const float*, size_t, int k) const override {
        std::vector<AnnSearchResult> results = {
            {101, 0.11F},
            {102, 0.22F},
            {103, 0.33F},
        };
        if (k < static_cast<int>(results.size())) {
            results.resize(static_cast<std::size_t>(k));
        }
        return results;
    }

    [[nodiscard]] std::size_t size() const override { return 3; }
};

} // namespace

TEST(AnnFrontdoorSingleShardPhaseA, EmitsRouteMetricForSingleShardExactPath) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();
    metrics.reset();

    AnnFrontdoor frontdoor;
    frontdoor.registerBackend("doc:single-shard",
                              std::make_shared<SingleShardAnnIndex>(),
                              AnnScopeKind::Document);

    AnnQueryContext context;
    context.scope_id = "doc:single-shard";
    context.dataset_size = 128;
    context.hot_tier = true;
    context.correlation_id = "phase-a-single-shard";

    static constexpr float kQuery[] = {1.0F, 0.0F, 0.0F, 0.0F};
    const auto result = frontdoor.search(kQuery, 4, 2, context);

    ASSERT_EQ(result.strategy_used, AnnStrategy::HNSW);
    ASSERT_FALSE(result.is_distributed);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_EQ(result.candidates.front().id, 101);

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("ann_frontdoor_route_type"), std::string::npos);
    EXPECT_NE(exported.find("route_type=\"HNSW\""), std::string::npos);
    EXPECT_NE(exported.find("distributed=\"false\""), std::string::npos);
}
