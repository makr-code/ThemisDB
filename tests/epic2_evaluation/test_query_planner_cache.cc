/**
 * @file test_query_planner_cache.cc
 * @brief Phase 3 P3-01: Query optimizer hardening tests for plan cache and cost model.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "query/optimizer_cost_model.h"
#include "query/plan_cache.h"

using namespace themis;
using namespace themis::query;
using namespace std::chrono_literals;

namespace {

QueryOptimizer::Plan makePlan(double complexity = 1.0,
                              std::vector<std::string> indexes = {}) {
    QueryOptimizer::Plan plan;
    plan.nlp_complexity = complexity;
    plan.nlp_suggested_indexes = std::move(indexes);
    return plan;
}

PlanCache::Statistics makeStats(
    std::initializer_list<std::pair<const std::string, size_t>> rows) {
    return PlanCache::Statistics{std::unordered_map<std::string, size_t>(rows)};
}

std::vector<long long> measure_latencies(size_t iterations,
                                         const std::function<void(size_t)>& fn) {
    std::vector<long long> latencies;
    latencies.reserve(iterations);
    for (size_t i = 0; i < iterations; ++i) {
        const auto start = std::chrono::steady_clock::now();
        fn(i);
        const auto end = std::chrono::steady_clock::now();
        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }
    std::sort(latencies.begin(), latencies.end());
    return latencies;
}

long long percentile(const std::vector<long long>& values, double pct) {
    if (values.empty()) {
        return 0;
    }
    const auto idx = static_cast<size_t>(std::clamp(pct, 0.0, 1.0) *
                                         static_cast<double>(values.size() - 1));
    return values[idx];
}

size_t query_index_for_skew(size_t i) {
    return (i % 10 < 8) ? (i % 2) : (2 + (i % 8));
}

class Phase3PlanCacheOptimization : public ::testing::Test {
protected:
    void SetUp() override {
        PlanCache::Config cfg;
        cfg.max_entries = 3;
        cfg.max_plan_age = 1h;
        cfg.statistics_drift_factor = 10.0;
        cfg.max_memory_bytes = 4096;
        cfg.memory_eviction_threshold = 0.8;
        cfg.max_consecutive_failures = 2;
        cache_ = std::make_unique<PlanCache>(cfg);
    }

    OptimizerCostModel makeCostModel() const {
        OptimizerCostModel::CostConstants constants;
        constants.availableMemory = 4096;
        constants.memoryThresholdRatio = 0.5;
        return OptimizerCostModel(constants);
    }

    std::unique_ptr<PlanCache> cache_;
};

TEST_F(Phase3PlanCacheOptimization, PlanCacheLRUBasic) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(1.0), stats, {}, {"users"});
    cache_->put("SELECT * FROM users WHERE email = 'alice@example.com'", makePlan(2.0), stats, {}, {"users"});
    cache_->put("SELECT * FROM users WHERE status = 'active'", makePlan(3.0), stats, {}, {"users"});
    ASSERT_TRUE(cache_->get("SELECT * FROM users WHERE id = 999", stats).has_value());

    cache_->put("SELECT * FROM users WHERE created_at > 42", makePlan(4.0), stats, {}, {"users"});

    EXPECT_TRUE(cache_->get("SELECT * FROM users WHERE id = 7", stats).has_value());
    EXPECT_FALSE(cache_->get("SELECT * FROM users WHERE email = 'bob@example.com'", stats).has_value());
    EXPECT_TRUE(cache_->get("SELECT * FROM users WHERE created_at > 99", stats).has_value());
}

TEST_F(Phase3PlanCacheOptimization, PlanCacheEvictionUnderMemoryPressure) {
    PlanCache::Config cfg;
    cfg.max_entries = 10;
    cfg.max_memory_bytes = 1400;
    cfg.memory_eviction_threshold = 0.8;
    PlanCache cache(cfg);
    const auto stats = makeStats({{"users", 1000}});
    const std::string large_value(500, 'x');
    const std::vector<PlanCache::ParameterInfo> params = {
        {"@tenant", "string", large_value}
    };

    cache.put("SELECT * FROM users WHERE tenant_a = 'a'", makePlan(1.0), stats, params, {"users"});
    cache.put("SELECT * FROM users WHERE tenant_b = 'b'", makePlan(1.0), stats, params, {"users"});
    cache.put("SELECT * FROM users WHERE tenant_c = 'c'", makePlan(1.0), stats, params, {"users"});

    const auto cache_stats = cache.getStats();
    EXPECT_LE(cache_stats.current_memory_bytes, static_cast<size_t>(1140));
    EXPECT_LT(cache_stats.current_size, 3u);
}

TEST_F(Phase3PlanCacheOptimization, PlanCacheReinsertionOfEvictedPlan) {
    PlanCache::Config cfg;
    cfg.max_entries = 1;
    PlanCache cache(cfg);
    const auto stats = makeStats({{"users", 1000}});

    cache.put("SELECT * FROM users WHERE id = 1", makePlan(1.0), stats, {}, {"users"});
    cache.put("SELECT * FROM users WHERE email = 'alice@example.com'", makePlan(2.0), stats, {}, {"users"});
    EXPECT_FALSE(cache.get("SELECT * FROM users WHERE id = 2", stats).has_value());

    cache.put("SELECT * FROM users WHERE id = 1", makePlan(7.0), stats, {}, {"users"});
    const auto result = cache.get("SELECT * FROM users WHERE id = 2", stats);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->plan.nlp_complexity, 7.0);
}

TEST_F(Phase3PlanCacheOptimization, PlanCacheConcurrentAccessWithoutDeadlock) {
    const auto stats = makeStats({{"users", 1000}});
    std::atomic<int> hits{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 6; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 40; ++i) {
                const std::string query = "SELECT * FROM users WHERE shard_" + std::to_string((t + i) % 3) + " = 1";
                cache_->put(query, makePlan(i), stats, {}, {"users"});
                if (cache_->get(query, stats).has_value()) {
                    ++hits;
                }
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(hits.load(), 0);
    EXPECT_GT(cache_->getStats().hits, 0u);
}

TEST_F(Phase3PlanCacheOptimization, PlanCacheHitRatioTracking) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(), stats, {}, {"users"});

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(cache_->get("SELECT * FROM users WHERE id = 999", stats).has_value());
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(cache_->get("SELECT * FROM users WHERE email = 'nobody@example.com'", stats).has_value());
    }

    const auto cache_stats = cache_->getStats();
    EXPECT_EQ(cache_stats.hits, 5u);
    EXPECT_EQ(cache_stats.misses, 3u);
    EXPECT_NEAR(cache_stats.hitRate(), 5.0 / 8.0, 1e-9);
}

TEST_F(Phase3PlanCacheOptimization, PlanCacheInvalidationOnSchemaChange) {
    const auto stats = makeStats({{"users", 1000}, {"orders", 500}});
    cache_->put("SELECT * FROM users", makePlan(1.0), stats, {}, {"users"});
    cache_->put("SELECT * FROM orders", makePlan(2.0), stats, {}, {"orders"});

    EXPECT_EQ(cache_->invalidateTable("users"), 1u);
    EXPECT_FALSE(cache_->get("SELECT * FROM users", stats).has_value());
    EXPECT_TRUE(cache_->get("SELECT * FROM orders", stats).has_value());
}

TEST_F(Phase3PlanCacheOptimization, CostModelCardinalityEstimation) {
    const auto model = makeCostModel();
    const auto scan = model.estimateIndexScan(
        {"users", 1000, 100, 64.0, false, 0},
        {"idx_users_age", "btree", 1000, 2, 0.25},
        0.25);
    EXPECT_EQ(scan.estimatedRows, 250u);
}

TEST_F(Phase3PlanCacheOptimization, CostModelSelectivityComputation) {
    const auto model = makeCostModel();
    OptimizerCostModel::ColumnStatistics column_stats;
    column_stats.distinctValues = 10;
    column_stats.nullFraction = 0.2;

    EXPECT_NEAR(model.estimateSelectivity("status", column_stats), 0.08, 1e-9);
}

TEST_F(Phase3PlanCacheOptimization, CostModelJoinCostCalculation) {
    const auto model = makeCostModel();
    const auto nested = model.estimateNestedLoopJoin(100, 1000, 0.1);
    const auto hash = model.estimateHashJoin(100, 1000, 0.1);
    const auto sort_merge = model.estimateSortMergeJoin(100, 1000, 0.1);

    EXPECT_GT(nested.totalCost, hash.totalCost);
    EXPECT_GT(sort_merge.totalCost, 0.0);
    EXPECT_EQ(hash.type, OptimizerCostModel::JoinCost::JoinType::HASH_JOIN);
}

TEST_F(Phase3PlanCacheOptimization, CostModelIndexUsageOptimization) {
    const auto model = makeCostModel();
    OptimizerCostModel::TableStatistics table{"users", 100000, 1000, 128.0, false, 0};
    OptimizerCostModel::IndexStatistics index{"idx_users_age", "btree", 100000, 3, 0.01};

    const auto table_scan = model.estimateTableScan(table);
    const auto index_scan = model.estimateIndexScan(table, index, 0.01);

    EXPECT_LT(index_scan.totalCost, table_scan.totalCost);
    EXPECT_LT(index_scan.estimatedRows, table_scan.estimatedRows);
}

TEST_F(Phase3PlanCacheOptimization, CostModelMultiTableOptimization) {
    const auto model = makeCostModel();
    const auto order_ab = model.estimateHashJoin(100, 1000, 0.05).totalCost +
                          model.estimateHashJoin(50, 10000, 0.02).totalCost;
    const auto order_ac = model.estimateHashJoin(100, 10000, 0.05).totalCost +
                          model.estimateHashJoin(500, 1000, 0.02).totalCost;

    EXPECT_LT(order_ab, order_ac);
}

TEST_F(Phase3PlanCacheOptimization, CostModelAggregationCost) {
    const auto model = makeCostModel();
    const auto aggregation = model.estimateAggregation(10000, 100, 3);

    EXPECT_EQ(aggregation.inputRows, 10000u);
    EXPECT_EQ(aggregation.outputRows, 100u);
    EXPECT_GT(aggregation.totalCost, aggregation.cpuCost - 1e-9);
}

TEST_F(Phase3PlanCacheOptimization, CostModelRegressionVsBaseline) {
    auto model = makeCostModel();
    const auto before = model.estimateIndexScan({"users", 50000, 500, 128.0, false, 0},
                                                {"idx", "btree", 50000, 3, 0.02},
                                                0.02)
                            .totalCost;
    model.calibrateCosts({{"cpuCostPerRow", 0.02}, {"pageReadCost", 0.8}});
    const auto after = model.estimateIndexScan({"users", 50000, 500, 128.0, false, 0},
                                               {"idx", "btree", 50000, 3, 0.02},
                                               0.02)
                           .totalCost;

    EXPECT_GT(after, 0.0);
    EXPECT_GT(before, 0.0);
}

TEST_F(Phase3PlanCacheOptimization, CostModelFilterSelectivityProduct) {
    const auto model = makeCostModel();
    std::map<std::string, OptimizerCostModel::ColumnStatistics> columns;
    columns["country"].distinctValues = 5;
    columns["country"].nullFraction = 0.0;
    columns["status"].distinctValues = 10;
    columns["status"].nullFraction = 0.0;

    const auto filter = model.estimateFilter(1000, {"country", "status"}, columns);
    EXPECT_NEAR(filter.selectivity, 0.02, 1e-9);
    EXPECT_EQ(filter.outputRows, 20u);
}

TEST_F(Phase3PlanCacheOptimization, CostModelNetworkTransferHandlesZeroBandwidthCalibration) {
    auto model = makeCostModel();
    model.calibrateCosts({{"networkBandwidth", 0.0}, {"networkLatency", -1.0}});
    const auto network = model.estimateNetworkTransfer(10 * 1024 * 1024, 2);

    EXPECT_TRUE(std::isfinite(network.transferCost));
    EXPECT_GE(network.transferCost, 0.0);
    EXPECT_GE(network.latencyCost, 0.0);
}

TEST_F(Phase3PlanCacheOptimization, CostModelSelectivityClampsInvalidNullFraction) {
    const auto model = makeCostModel();
    OptimizerCostModel::ColumnStatistics column_stats;
    column_stats.distinctValues = 10;
    column_stats.nullFraction = 1.5;
    EXPECT_NEAR(model.estimateSelectivity("status", column_stats), 0.001, 1e-12);

    column_stats.nullFraction = -0.5;
    EXPECT_NEAR(model.estimateSelectivity("status", column_stats), 0.1, 1e-9);
}

TEST_F(Phase3PlanCacheOptimization, CostModelSerializationAdviceAlwaysUsesAtLeastOneThread) {
    auto model = makeCostModel();
    model.calibrateCosts({
        {"cpu_batch_thread_low", 0.0},
        {"cpu_batch_thread_high", 0.0},
        {"msgpack_row_threshold", 1.0}
    });

    const auto advice = model.adviseSerializationStrategy(
        10'000,
        128,
        false,
        0,
        WorkloadType::VECTOR_SEARCH);

    EXPECT_GE(advice.recommended_thread_count, 1u);
}

TEST_F(Phase3PlanCacheOptimization, CacheIntegrationExecutionPath) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE age > 30", makePlan(1.0), stats, {}, {"users"});

    const auto literal_variant = cache_->get("SELECT * FROM users WHERE age > 45", stats);
    ASSERT_TRUE(literal_variant.has_value());
    EXPECT_EQ(literal_variant->query_fingerprint,
              PlanCache::fingerprint("SELECT * FROM users WHERE age > 30"));
}

TEST_F(Phase3PlanCacheOptimization, PlanCachePreservesQuotedIdentifiers) {
    const auto stats = makeStats({{"orders", 1000}});
    cache_->put("SELECT \"user\" FROM \"orders\" WHERE id = 1", makePlan(1.0), stats, {}, {"orders"});

    EXPECT_TRUE(cache_->get("SELECT \"user\" FROM \"orders\" WHERE id = 99", stats).has_value());
    EXPECT_FALSE(cache_->get("SELECT \"account\" FROM \"orders\" WHERE id = 99", stats).has_value());
}

TEST_F(Phase3PlanCacheOptimization, CacheIntegrationParameterizedQueries) {
    const auto stats = makeStats({{"users", 1000}});
    const std::vector<PlanCache::ParameterInfo> params = {{"@age", "int", "30"}};
    cache_->put("SELECT * FROM users WHERE age > @age", makePlan(2.0), stats, params, {"users"});

    const auto result = cache_->get("SELECT * FROM users WHERE age > @age", stats);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->parameters.size(), 1u);
    EXPECT_EQ(result->parameters.front().name, "@age");
}

TEST_F(Phase3PlanCacheOptimization, CacheIntegrationDistributedQueries) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users", makePlan(3.0), stats, {}, {"users"}, "topology-a");

    EXPECT_TRUE(cache_->get("SELECT * FROM users", stats, "topology-a").has_value());
    EXPECT_FALSE(cache_->get("SELECT * FROM users", stats, "topology-b").has_value());
}

TEST_F(Phase3PlanCacheOptimization, CacheIntegrationErrorRecovery) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users", makePlan(3.0), stats, {}, {"users"});

    EXPECT_FALSE(cache_->recordExecutionFailure("SELECT * FROM users"));
    EXPECT_TRUE(cache_->recordExecutionFailure("SELECT * FROM users"));
    EXPECT_FALSE(cache_->get("SELECT * FROM users", stats).has_value());
}

TEST_F(Phase3PlanCacheOptimization, CacheBenchmarkYCSBWorkload) {
    PlanCache::Config cfg;
    cfg.max_entries = 64;
    PlanCache cache(cfg);
    const auto stats = makeStats({{"users", 1000}});

    for (size_t i = 0; i < 20; ++i) {
        cache.put("SELECT * FROM users WHERE bucket_" + std::to_string(i) + " = @v", makePlan(), stats, {}, {"users"});
    }
    for (size_t i = 0; i < 1000; ++i) {
        ASSERT_TRUE(cache.get("SELECT * FROM users WHERE bucket_" + std::to_string(i % 20) + " = @v", stats).has_value());
    }

    EXPECT_GE(cache.getStats().hitRate(), 0.8);
}

TEST_F(Phase3PlanCacheOptimization, CacheBenchmarkHotQueryWorkload) {
    PlanCache::Config cfg;
    cfg.max_entries = 16;
    PlanCache cache(cfg);
    const auto stats = makeStats({{"users", 1000}});

    for (size_t i = 0; i < 10; ++i) {
        cache.put("SELECT * FROM users WHERE segment_" + std::to_string(i) + " = @v", makePlan(), stats, {}, {"users"});
    }
    for (size_t i = 0; i < 500; ++i) {
        ASSERT_TRUE(cache.get("SELECT * FROM users WHERE segment_" + std::to_string(query_index_for_skew(i)) + " = @v", stats).has_value());
    }

    EXPECT_GE(cache.getStats().hitRate(), 0.85);
}

TEST_F(Phase3PlanCacheOptimization, CacheBenchmarkMemoryFootprint) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(1.0), stats, {}, {"users"});
    const auto first = cache_->estimateCurrentMemoryBytes();
    cache_->put("SELECT * FROM users WHERE email = 'memory@example.com'", makePlan(2.0), stats,
                {{"@payload", "string", std::string(128, 'z')}}, {"users"});
    const auto second = cache_->estimateCurrentMemoryBytes();

    EXPECT_GT(first, 0u);
    EXPECT_GT(second, first);
}

TEST_F(Phase3PlanCacheOptimization, CacheBenchmarkEvictionLatency) {
    PlanCache::Config cfg;
    cfg.max_entries = 2;
    PlanCache cache(cfg);
    const auto stats = makeStats({{"users", 1000}});
    const auto latencies = measure_latencies(64, [&](size_t i) {
        cache.put("SELECT * FROM users WHERE field_" + std::to_string(i) + " = 1", makePlan(), stats, {}, {"users"});
    });

    EXPECT_LT(percentile(latencies, 0.99), 5'000'000LL);
}

TEST_F(Phase3PlanCacheOptimization, RegressionQueryLatencyP50) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(), stats, {}, {"users"});

    const auto warm = measure_latencies(200, [&](size_t) {
        ASSERT_TRUE(cache_->get("SELECT * FROM users WHERE id = 999", stats).has_value());
    });
    const auto cold = measure_latencies(200, [&](size_t i) {
        const auto query = "SELECT * FROM users WHERE cold_" + std::to_string(i) + " = 1";
        const auto result = cache_->get(query, stats);
        if (!result.has_value()) {
            cache_->put(query, makePlan(), stats, {}, {"users"});
        }
    });

    EXPECT_LE(percentile(warm, 0.50), percentile(cold, 0.50));
}

TEST_F(Phase3PlanCacheOptimization, RegressionQueryLatencyP95) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(), stats, {}, {"users"});

    const auto warm = measure_latencies(200, [&](size_t) {
        ASSERT_TRUE(cache_->get("SELECT * FROM users WHERE id = 999", stats).has_value());
    });
    const auto cold = measure_latencies(200, [&](size_t i) {
        const auto query = "SELECT * FROM users WHERE cold_p95_" + std::to_string(i) + " = 1";
        const auto result = cache_->get(query, stats);
        if (!result.has_value()) {
            cache_->put(query, makePlan(), stats, {}, {"users"});
        }
    });

    EXPECT_LE(percentile(warm, 0.95), percentile(cold, 0.95));
}

TEST_F(Phase3PlanCacheOptimization, RegressionQueryLatencyP99) {
    const auto stats = makeStats({{"users", 1000}});
    cache_->put("SELECT * FROM users WHERE id = 1", makePlan(), stats, {}, {"users"});

    const auto warm = measure_latencies(200, [&](size_t) {
        ASSERT_TRUE(cache_->get("SELECT * FROM users WHERE id = 999", stats).has_value());
    });
    const auto cold = measure_latencies(200, [&](size_t i) {
        const auto query = "SELECT * FROM users WHERE cold_p99_" + std::to_string(i) + " = 1";
        const auto result = cache_->get(query, stats);
        if (!result.has_value()) {
            cache_->put(query, makePlan(), stats, {}, {"users"});
        }
    });

    EXPECT_LE(percentile(warm, 0.99), percentile(cold, 0.99));
}

TEST_F(Phase3PlanCacheOptimization, RegressionThroughputUnderConcurrentLoad) {
    const auto stats = makeStats({{"users", 1000}});
    for (int i = 0; i < 8; ++i) {
        cache_->put("SELECT * FROM users WHERE hot_" + std::to_string(i) + " = 1", makePlan(), stats, {}, {"users"});
    }

    auto run_workload = [&](bool cached) {
        const auto start = std::chrono::steady_clock::now();
        std::vector<std::thread> threads = {};

        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < 100; ++i) {
                    const std::string query = cached
                        ? "SELECT * FROM users WHERE hot_" + std::to_string(i % 8) + " = 1"
                        : "SELECT * FROM users WHERE cold_throughput_" + std::to_string(t * 100 + i) + " = 1";
                    const auto result = cache_->get(query, stats);
                    if (!cached && !result.has_value()) {
                        cache_->put(query, makePlan(), stats, {}, {"users"});
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        return std::chrono::steady_clock::now() - start;
    };

    const auto cached_time = run_workload(true);
    const auto cold_time = run_workload(false);
    EXPECT_LE(cached_time, cold_time);
}

TEST_F(Phase3PlanCacheOptimization, RegressionMemoryUsageStability) {
    const auto stats = makeStats({{"users", 1000}});
    for (int round = 0; round < 20; ++round) {
        for (int i = 0; i < 8; ++i) {
            cache_->put("SELECT * FROM users WHERE slot_" + std::to_string(i) + " = 1", makePlan(i), stats, {}, {"users"});
            cache_->get("SELECT * FROM users WHERE slot_" + std::to_string(i) + " = 2", stats);
        }
        cache_->invalidateTable("users");
    }

    EXPECT_LE(cache_->estimateCurrentMemoryBytes(), 4096u);
}

TEST_F(Phase3PlanCacheOptimization, RegressionWave7GatesRepass) {
    const auto stats = makeStats({{"users", 1000}});
    for (int i = 0; i < 16; ++i) {
        cache_->put("SELECT * FROM users WHERE gate_" + std::to_string(i) + " = 1", makePlan(), stats, {}, {"users"});
    }

    const auto read_latencies = measure_latencies(200, [&](size_t i) {
        cache_->get("SELECT * FROM users WHERE gate_" + std::to_string(i % 16) + " = 2", stats);
    });
    const auto write_latencies = measure_latencies(200, [&](size_t i) {
        cache_->put("SELECT * FROM users WHERE mut_" + std::to_string(i) + " = 1", makePlan(), stats, {}, {"users"});
    });

    EXPECT_LT(percentile(read_latencies, 0.99), 5'000'000LL);
    EXPECT_LT(percentile(write_latencies, 0.99), 5'000'000LL);
}

}  // namespace
