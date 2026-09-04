// Copyright 2026 ThemisDB — WorkloadAdaptiveOptimizer focused tests (Issue #230)
#include "performance/workload_adaptive_optimizer.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>

using namespace themis::performance;

namespace {

class WAOTest : public ::testing::Test {
protected:
    WorkloadAdaptiveOptimizer opt;
};

// --- Construction ------------------------------------------------------------

TEST_F(WAOTest, DefaultConstructionSucceeds) {
    auto stats = opt.get_stats();
    EXPECT_EQ(stats.total_queries_recorded, 0u);
}

TEST_F(WAOTest, ClassifyEmptyReturnsUnknown) {
    auto profile = opt.classify_workload();
    EXPECT_EQ(profile.type, WorkloadType::UNKNOWN);
}

// --- record_query ------------------------------------------------------------

TEST_F(WAOTest, RecordQueryIncrementsCounter) {
    opt.record_query(false, 1.0, 10);
    EXPECT_EQ(opt.get_stats().total_queries_recorded, 1u);
}

TEST_F(WAOTest, RecordMultipleQueries) {
    for (int i = 0; i < 10; ++i) {
      opt.record_query(false, 1.0, 5);
    }
    EXPECT_EQ(opt.get_stats().total_queries_recorded, 10u);
}

// --- classify_workload -------------------------------------------------------

TEST_F(WAOTest, ClassifyOLTPHighConcurrencyWrites) {
    opt.set_concurrent_queries(16);
    for (int i = 0; i < 50; ++i) {
      opt.record_query(true, 1.5, 1, "orders");
    }
    auto profile = opt.classify_workload();
    EXPECT_EQ(profile.type, WorkloadType::OLTP);
}

TEST_F(WAOTest, ClassifyOLAPHighComplexity) {
    for (int i = 0; i < 50; ++i) {
      opt.record_query(false, 8.0, 100000);
    }
    auto profile = opt.classify_workload();
    EXPECT_EQ(profile.type, WorkloadType::OLAP);
}

TEST_F(WAOTest, ClassifyMIXED) {
    for (int i = 0; i < 25; ++i) {
      opt.record_query(true,  4.0, 10, "t1");
    }
    for (int i = 0; i < 25; ++i) {
      opt.record_query(false, 4.0, 100, "t1");
    }
    auto profile = opt.classify_workload();
    // Mixed: write_ratio >= 0.3 and complexity >= 3
    EXPECT_EQ(profile.type, WorkloadType::MIXED);
}

TEST_F(WAOTest, ClassifyVECTOR) {
    for (int i = 0; i < 50; ++i) {
      opt.record_query(false, 2.0, 50000);
    }
    auto profile = opt.classify_workload();
    EXPECT_EQ(profile.type, WorkloadType::VECTOR);
}

TEST_F(WAOTest, HotTablesPopulated) {
    for (int i = 0; i < 30; ++i) {
      opt.record_query(false, 1.0, 10, "hot_table");
    }
    for (int i = 0; i < 10; ++i) {
      opt.record_query(false, 1.0, 10, "other_table");
    }
    auto profile = opt.classify_workload();
    EXPECT_FALSE(profile.hot_tables.empty());
    EXPECT_EQ(profile.hot_tables[0], "hot_table");
}

TEST_F(WAOTest, ReadWriteRatioAllReads) {
    for (int i = 0; i < 20; ++i) {
      opt.record_query(false, 1.0, 5);
    }
    auto profile = opt.classify_workload();
    EXPECT_DOUBLE_EQ(profile.read_write_ratio, 1.0);
}

// --- get_strategy ------------------------------------------------------------

TEST_F(WAOTest, StrategyForOLTPUsesHash) {
    WorkloadProfile p;
    p.type = WorkloadType::OLTP;
    p.concurrent_queries = 4;
    auto s = opt.get_strategy(p);
    EXPECT_EQ(s.join_algorithm, "hash");
    EXPECT_EQ(s.index_type, "btree");
}

TEST_F(WAOTest, StrategyForOLAPEnablesJIT) {
    WorkloadProfile p;
    p.type = WorkloadType::OLAP;
    auto s = opt.get_strategy(p);
    EXPECT_TRUE(s.enable_jit_compilation);
    EXPECT_EQ(s.join_algorithm, "sort-merge");
}

TEST_F(WAOTest, StrategyPredictiveScalingBumpsPool) {
    WorkloadProfile p;
    p.type = WorkloadType::OLTP;
    p.concurrent_queries = 100;
    auto s = opt.get_strategy(p);
    EXPECT_GT(s.thread_pool_size, 4u);
}

// --- apply_strategy ----------------------------------------------------------

TEST_F(WAOTest, ApplyStrategyIncrementsAdaptations) {
    WorkloadProfile p;
    p.type = WorkloadType::OLTP;
    opt.apply_strategy(opt.get_strategy(p));
    EXPECT_GE(opt.get_stats().total_adaptations, 1u);
}

TEST_F(WAOTest, CurrentStrategyReflectsApplied) {
    OptimizationStrategy s;
    s.thread_pool_size = 99;
    opt.apply_strategy(s);
    EXPECT_EQ(opt.current_strategy().thread_pool_size, 99u);
}

// --- callback ----------------------------------------------------------------

TEST_F(WAOTest, CallbackInvokedOnApply) {
    std::atomic<int> count{0};
    opt.set_callback([&](const WorkloadProfile&, const WorkloadProfile&,
                         const OptimizationStrategy&) {
        ++count;
    });
    opt.apply_strategy(opt.get_strategy(opt.classify_workload()));
    EXPECT_EQ(count.load(), 1);
}

// --- auto_adapt --------------------------------------------------------------

TEST_F(WAOTest, AutoAdaptEnableDisable) {
    opt.enable_auto_adapt(std::chrono::seconds{1});
    EXPECT_TRUE(opt.is_auto_adapt_enabled());
    opt.disable_auto_adapt();
    EXPECT_FALSE(opt.is_auto_adapt_enabled());
}

TEST_F(WAOTest, DoubleEnableNoCrash) {
    opt.enable_auto_adapt(std::chrono::seconds{1});
    opt.enable_auto_adapt(std::chrono::seconds{1});  // second call should be no-op
    opt.disable_auto_adapt();
}

// --- stats -------------------------------------------------------------------

TEST_F(WAOTest, ResetStatsZeroesCounters) {
    opt.record_query(false, 1.0, 5);
    opt.apply_strategy(opt.get_strategy(opt.classify_workload()));
    opt.reset_stats();
    auto s = opt.get_stats();
    EXPECT_EQ(s.total_queries_recorded, 0u);
    EXPECT_EQ(s.total_adaptations, 0u);
}

// --- set_concurrent_queries --------------------------------------------------

TEST_F(WAOTest, SetConcurrentQueriesReflectsInProfile) {
    opt.set_concurrent_queries(42);
    for (int i = 0; i < 10; ++i) {
      opt.record_query(true, 1.0, 5);
    }
    auto profile = opt.classify_workload();
    EXPECT_EQ(profile.concurrent_queries, 42u);
}

// --- thread safety -----------------------------------------------------------

TEST_F(WAOTest, ConcurrentRecordQueryThreadSafe) {
    constexpr int kThreads = 4;
    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this](){
            for (int i = 0; i < 50; ++i)
                opt.record_query(i % 2 == 0, 2.0, 10, "tbl");
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(opt.get_stats().total_queries_recorded,
              static_cast<uint64_t>(kThreads * 50));
}

} // namespace
