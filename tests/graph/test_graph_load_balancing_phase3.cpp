#include <gtest/gtest.h>
#include "graph/graph_load_balancer.h"
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::graph;

// ============================================================================
// P3-04 Group 1: GraphQueryScheduler – basic submit and dispatch
// ============================================================================

TEST(QuerySchedulerTest, InitiallyNoPendingTasks) {
    GraphQueryScheduler sched;
    EXPECT_EQ(0u, sched.pending());
}

TEST(QuerySchedulerTest, SubmitIncreasesPendingCount) {
    GraphQueryScheduler sched;
    sched.submit("q1", GraphQueryScheduler::Priority::NORMAL, [] {});
    EXPECT_EQ(1u, sched.pending());
}

TEST(QuerySchedulerTest, SubmittedCountIncrements) {
    GraphQueryScheduler sched;
    sched.submit("q1", GraphQueryScheduler::Priority::NORMAL, [] {});
    sched.submit("q2", GraphQueryScheduler::Priority::LOW,    [] {});
    EXPECT_EQ(2u, sched.submittedCount());
}

TEST(QuerySchedulerTest, HighPriorityDispatchedBeforeLow) {
    GraphQueryScheduler sched;
    std::vector<std::string> order;
    sched.submit("low",  GraphQueryScheduler::Priority::LOW,  [&] { order.push_back("low"); });
    sched.submit("high", GraphQueryScheduler::Priority::HIGH, [&] { order.push_back("high"); });
    sched.submit("norm", GraphQueryScheduler::Priority::NORMAL,[&]{ order.push_back("norm"); });

    sched.executeNext();
    sched.executeNext();
    sched.executeNext();

    ASSERT_EQ(3u, order.size());
    EXPECT_EQ("high", order[0]);
    EXPECT_EQ("norm", order[1]);
    EXPECT_EQ("low",  order[2]);
}

TEST(QuerySchedulerTest, UrgentPriorityDispatchedFirst) {
    GraphQueryScheduler sched;
    std::vector<int> order;
    sched.submit("norm",   GraphQueryScheduler::Priority::NORMAL, [&] { order.push_back(1); });
    sched.submit("urgent", GraphQueryScheduler::Priority::URGENT, [&] { order.push_back(0); });
    sched.executeNext();
    ASSERT_FALSE(order.empty());
    EXPECT_EQ(0, order[0]);
}

TEST(QuerySchedulerTest, FIFOWithinSamePriority) {
    GraphQueryScheduler sched;
    std::vector<int> ids;
    for (int i = 0; i < 5; ++i)
        sched.submit("q" + std::to_string(i),
                     GraphQueryScheduler::Priority::NORMAL,
                     [i, &ids] { ids.push_back(i); });
    while (sched.pending() > 0) {
      sched.executeNext();
    }
    // FIFO ordering expected within NORMAL
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ(i, ids[i]);
    }
}

TEST(QuerySchedulerTest, CompletedCountAfterExecute) {
    GraphQueryScheduler sched;
    for (int i = 0; i < 3; ++i)
        sched.submit("q", GraphQueryScheduler::Priority::NORMAL, [] {});
    sched.executeNext();
    sched.executeNext();
    EXPECT_EQ(2u, sched.completedCount());
}

TEST(QuerySchedulerTest, NextNonBlockingOnEmptyReturnsNullopt) {
    GraphQueryScheduler sched;
    auto task = sched.next(/*block=*/false);
    EXPECT_FALSE(task.has_value());
}

TEST(QuerySchedulerTest, ClearPendingRemovesAllTasks) {
    GraphQueryScheduler sched;
    for (int i = 0; i < 5; ++i)
        sched.submit("q", GraphQueryScheduler::Priority::LOW, [] {});
    size_t cleared = sched.clearPending();
    EXPECT_EQ(5u, cleared);
    EXPECT_EQ(0u, sched.pending());
}

TEST(QuerySchedulerTest, QueueDepthLimitEnforcedOnOverflow) {
    GraphQueryScheduler sched(3);
    sched.submit("q1", GraphQueryScheduler::Priority::NORMAL, [] {});
    sched.submit("q2", GraphQueryScheduler::Priority::NORMAL, [] {});
    sched.submit("q3", GraphQueryScheduler::Priority::NORMAL, [] {});
    EXPECT_THROW(
        sched.submit("q4", GraphQueryScheduler::Priority::NORMAL, [] {}),
        std::overflow_error);
}

TEST(QuerySchedulerTest, StopUnblocksWaitingNext) {
    GraphQueryScheduler sched;
    std::atomic<bool> returned{false};
    std::thread t([&] {
        sched.next(/*block=*/true);
        returned.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    sched.stop();
    t.join();
    EXPECT_TRUE(returned.load());
}

// ============================================================================
// P3-04 Group 2: GraphShardBalancer – construction and shard management
// ============================================================================

TEST(ShardBalancerTest, EmptyShardListThrows) {
    EXPECT_THROW(
        GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {}),
        std::invalid_argument);
}

TEST(ShardBalancerTest, ShardCountMatchesInitialList) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2", "s3"});
    EXPECT_EQ(3u, b.shardCount());
}

TEST(ShardBalancerTest, AddShardIncreasesCount) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1"});
    b.addShard("s2");
    EXPECT_EQ(2u, b.shardCount());
}

TEST(ShardBalancerTest, RemoveShardDecreasesCount) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2"});
    EXPECT_TRUE(b.removeShard("s1"));
    EXPECT_EQ(1u, b.shardCount());
}

TEST(ShardBalancerTest, RemoveNonexistentShardReturnsFalse) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1"});
    EXPECT_FALSE(b.removeShard("ghost"));
}

// ============================================================================
// P3-04 Group 3: Round-robin selection
// ============================================================================

TEST(ShardBalancerTest, RoundRobinCyclesThroughAllShards) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2", "s3"});
    std::vector<std::string> selected;
    for (int i = 0; i < 6; ++i) {
      selected.push_back(b.selectShard());
    }
    // Every shard must appear exactly twice in 6 round-robin selections
    for (const auto& s : {"s1", "s2", "s3"}) {
        size_t count = std::count(selected.begin(), selected.end(), s);
        EXPECT_EQ(2u, count) << "Shard " << s << " not selected exactly 2 times";
    }
}

TEST(ShardBalancerTest, RoundRobinSkipsUnhealthyShard) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2", "s3"});
    b.setShardHealth("s2", false);
    std::string sel = b.selectShard();
    // s2 should never be selected
    EXPECT_NE("s2", sel);
}

// ============================================================================
// P3-04 Group 4: Least-loaded selection
// ============================================================================

TEST(ShardBalancerTest, LeastLoadedSelectsIdleShard) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::LEAST_LOADED, {"s1", "s2", "s3"});
    // Load s1 and s2
    b.onQueryStarted("s1");
    b.onQueryStarted("s1");
    b.onQueryStarted("s2");
    const std::string sel = b.selectShard();
    EXPECT_EQ("s3", sel); // s3 has 0 in-flight
}

TEST(ShardBalancerTest, LeastLoadedTracksInflight) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::LEAST_LOADED, {"s1", "s2"});
    b.onQueryStarted("s1");
    auto stats = b.shardStats("s1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(1u, stats->inflight);
}

TEST(ShardBalancerTest, QueryCompletedDecrementsInflight) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::LEAST_LOADED, {"s1"});
    b.onQueryStarted("s1");
    b.onQueryCompleted("s1", 10.0);
    auto stats = b.shardStats("s1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(0u, stats->inflight);
}

// ============================================================================
// P3-04 Group 5: Latency-aware selection
// ============================================================================

TEST(ShardBalancerTest, LatencyAwareSelectsLowestEMA) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::LATENCY_AWARE, {"s1", "s2", "s3"});
    // Drive latency observations
    for (int i = 0; i < 10; ++i) {
        b.onQueryStarted("s1");
        b.onQueryCompleted("s1", 100.0); // high latency
        b.onQueryStarted("s2");
        b.onQueryCompleted("s2", 5.0);   // low latency
        b.onQueryStarted("s3");
        b.onQueryCompleted("s3", 50.0);
    }
    const std::string sel = b.selectShard();
    EXPECT_EQ("s2", sel); // s2 has lowest EMA latency
}

TEST(ShardBalancerTest, LatencyRecordedInShardStats) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::LATENCY_AWARE, {"s1"});
    b.onQueryStarted("s1");
    b.onQueryCompleted("s1", 25.0);
    auto stats = b.shardStats("s1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_GT(stats->ema_latency_ms, 0.0);
}

TEST(ShardBalancerTest, SetStrategyAtRuntime) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2"});
    b.setStrategy(GraphShardBalancer::Strategy::LEAST_LOADED);
    EXPECT_EQ(GraphShardBalancer::Strategy::LEAST_LOADED, b.strategy());
}

TEST(ShardBalancerTest, AllStatsReturnsOnePerShard) {
    GraphShardBalancer b(GraphShardBalancer::Strategy::ROUND_ROBIN, {"s1", "s2", "s3"});
    auto all = b.allStats();
    EXPECT_EQ(3u, all.size());
}
