// Phase A gate test for degraded query-planner fallback behavior.

#include "index/secondary_index.h"
#include "observability/metrics_collector.h"
#include "query/query_optimizer.h"
#include "storage/rocksdb_wrapper.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

using namespace themis;
using namespace themis::query;

namespace {

std::string makeTempPlannerDbPath() {
    namespace fs = std::filesystem;
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_query_planner_fallback_" + std::to_string(now))).string();
}

class QueryPlannerFallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        db_path_ = makeTempPlannerDbPath();
        cfg.db_path = db_path_;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);

        themis::observability::MetricsCollector::getInstance().reset();
    }

    void TearDown() override {
        secondary_index_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
};

} // namespace

TEST_F(QueryPlannerFallbackTest, EmitsFallbackMetricForDeterministicZeroEstimateMode) {
    auto& metrics = themis::observability::MetricsCollector::getInstance();

    QueryOptimizer optimizer(*secondary_index_, nullptr, &metrics);

    ConjunctiveQuery query;
    query.table = "phase_a_orders";
    query.predicates.push_back({"zeta", "1"});
    query.predicates.push_back({"alpha", "1"});

    const auto plan = optimizer.chooseOrderForAndQuery(query, 32);

    ASSERT_EQ(plan.orderedPredicates.size(), 2U);
    EXPECT_EQ(plan.orderedPredicates[0].column, "alpha");
    EXPECT_EQ(plan.orderedPredicates[1].column, "zeta");

    const auto exported = metrics.getPrometheusMetrics();
    EXPECT_NE(exported.find("query_planner_fallback_total"), std::string::npos);
    EXPECT_NE(exported.find("reason=\"deterministic_zero_estimate\""), std::string::npos);
    EXPECT_NE(exported.find("table=\"phase_a_orders\""), std::string::npos);
}
