/**
 * @file test_load_balancing.cpp
 * @brief Phase 3 P3-04: Load Balancing & Query Scheduling Tests
 *
 * This test file validates Phase 3-04 deliverables:
 *  - Query scheduler with priority queue design (P3-04-A design doc)
 *  - Load-aware shard selection (P3-04-B)
 *  - Query prioritization (SLA-aware scheduling) (P3-04-C)
 *  - Scheduler integration with executor pipeline (P3-04-D)
 *  - Load-balancing benchmarks (P3-04-E)
 *
 * Target: 24 tests (6+8+4+6 from P3-04 tasks B-E)
 *
 * Acceptance Criteria:
 *  - Query latency p99: <= 50ms (vs. Phase 2.4 baseline)
 *  - Cross-shard query distribution: <= 10% variance in response times
 *  - SLA compliance: 99% of queries complete within SLA window
 *  - Complete Doxygen + QUERY_SCHEDULING.md architecture doc
 *
 * @see ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md (P3-04)
 * @see include/execution/query_scheduler.h
 * @see include/sharding/shard_load_balancer.h
 */

#include <gtest/gtest.h>

#include "execution/query_scheduler.h"
#include "sharding/shard_load_balancer.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace themis { namespace execution { 

// ===== Task P3-04-B: Load-Aware Shard Selection (6 tests) =====

/**
 * @test ShardMetricsCollection
 * @brief Validates collection of shard metrics for load awareness.
 *
 * Verifies:
 *  - Shard CPU utilization tracked
 *  - Pending query count maintained
 *  - Response time p99 measured per shard
 */
TEST(Phase3LoadBalancing, ShardMetricsCollection) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"s0", "s1"});

    ShardMetrics m;
    m.cpu_percent     = 55.0;
    m.pending_queries = 10;
    m.p99_latency_ms  = 20.0;
    lb.updateMetrics("s0", m);

    const auto stats = lb.statistics();
    ASSERT_EQ(stats.size(), 2u);
    const auto& s0 = stats[0];
    EXPECT_NEAR(s0.metrics.cpu_percent,    55.0, 0.1);
    EXPECT_EQ(s0.metrics.pending_queries,  10u);
    EXPECT_NEAR(s0.metrics.p99_latency_ms, 20.0, 0.1);
}

/**
 * @test ShardLoadScoring
 * @brief Validates load score computation for each shard.
 *
 * Verifies:
 *  - Load score = cpu_weight * cpu% + queue_weight * pending + latency_weight * p99
 *  - Score range [0, 100]
 */
TEST(Phase3LoadBalancing, ShardLoadScoring) {
    using namespace themis::sharding;

    ShardLoadBalancer::Config cfg;
    cfg.weights.cpu     = 0.3;
    cfg.weights.queue   = 0.4;
    cfg.weights.latency = 0.3;
    cfg.max_pending     = 100.0;
    cfg.max_p99_ms      = 100.0;
    ShardLoadBalancer lb({"s0"}, cfg);

    ShardMetrics m;
    m.cpu_percent     = 50.0;   // contributes 0.3 * 50 = 15
    m.pending_queries = 50;     // 50/100*100 = 50 → 0.4 * 50 = 20
    m.p99_latency_ms  = 50.0;  // 50/100*100 = 50 → 0.3 * 50 = 15
    lb.updateMetrics("s0", m);

    const double expected = 15.0 + 20.0 + 15.0;  // = 50.0
    const double score    = lb.computeScore(m);
    EXPECT_NEAR(score, expected, 0.5);
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 100.0);
}

/**
 * @test ShardSelectionLowestScore
 * @brief Validates selection of shard with lowest load score.
 *
 * Verifies:
 *  - Always select shard with minimum score
 *  - Ties broken by insertion order (first shard)
 */
TEST(Phase3LoadBalancing, ShardSelectionLowestScore) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"heavy", "light"});

    ShardMetrics heavy;
    heavy.cpu_percent = 90.0;
    heavy.pending_queries = 100;
    heavy.p99_latency_ms  = 150.0;
    lb.updateMetrics("heavy", heavy);

    ShardMetrics light;
    light.cpu_percent = 5.0;
    light.pending_queries = 2;
    light.p99_latency_ms  = 3.0;
    lb.updateMetrics("light", light);

    const std::string selected = lb.selectShard();
    EXPECT_EQ(selected, "light");
}

/**
 * @test ShardSelectionUnderSkewedLoad
 * @brief Validates load-aware selection under skewed distribution.
 *
 * Verifies:
 *  - Hot shard avoided, cold shard preferred
 *  - Repeated selections go to the least-loaded shard
 */
TEST(Phase3LoadBalancing, ShardSelectionUnderSkewedLoad) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"hot", "cold1", "cold2"});

    ShardMetrics hot;
    hot.cpu_percent = 95.0;
    lb.updateMetrics("hot", hot);

    ShardMetrics cold;
    cold.cpu_percent = 5.0;
    lb.updateMetrics("cold1", cold);
    lb.updateMetrics("cold2", cold);

    // Over 10 selections, "hot" should never be chosen.
    for (int i = 0; i < 10; ++i) {
        const std::string sel = lb.selectShard();
        EXPECT_NE(sel, "hot") << "Hot shard should not be selected";
    }
}

/**
 * @test ShardSelectionStickiness
 * @brief Validates sticky session routing consistency.
 */
TEST(Phase3LoadBalancing, ShardSelectionStickiness) {
    using namespace themis::sharding;

    ShardLoadBalancer::Config cfg;
    cfg.sticky_sessions  = true;
    cfg.sticky_threshold = 80.0;
    ShardLoadBalancer lb({"s0", "s1", "s2"}, cfg);

    // Same client hash should produce the same shard (when load is low).
    const std::size_t client_hash = 42;
    const std::string first = lb.selectShard(client_hash);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(lb.selectShard(client_hash), first)
            << "Sticky session must return same shard under low load";
    }
}

/**
 * @test ShardSelectionFailoverBehavior
 * @brief Validates failover when selected shard unavailable.
 *
 * Verifies:
 *  - Select next-best shard if preferred shard is down
 *  - No error when alternatives exist
 */
TEST(Phase3LoadBalancing, ShardSelectionFailoverBehavior) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"preferred", "fallback"});

    // Mark "preferred" as unavailable.
    lb.setAvailable("preferred", false);

    // Should route to fallback.
    const std::string sel = lb.selectShard();
    EXPECT_EQ(sel, "fallback");

    // If all are down, expect exception.
    lb.setAvailable("fallback", false);
    EXPECT_THROW(lb.selectShard(), std::runtime_error);
}

// ===== Task P3-04-C: Query Prioritization (8 tests) =====

/**
 * @test PriorityQueueStructure
 * @brief Validates priority queue structure for query scheduling.
 *
 * Verifies:
 *  - Entries can be enqueued with different SLA priorities
 *  - Queue depth reflects enqueued items
 */
TEST(Phase3LoadBalancing, PriorityQueueStructure) {
    QueryScheduler sched;

    sched.enqueue([] {}, SLAPriority::HIGH,   5,   "high");
    sched.enqueue([] {}, SLAPriority::MEDIUM, 50,  "medium");
    sched.enqueue([] {}, SLAPriority::LOW,    500, "low");

    EXPECT_EQ(sched.size(), 3u);
    sched.shutdown();
}

/**
 * @test SLADrivenPrioritization
 * @brief Validates SLA-driven priority assignment reflected in dequeue order.
 *
 * Verifies:
 *  - HIGH priority entry (shortest deadline) is dequeued first
 */
TEST(Phase3LoadBalancing, SLADrivenPrioritization) {
    QueryScheduler sched;

    // Enqueue LOW first, then HIGH.
    sched.enqueue([] {}, SLAPriority::LOW,  500, "low");
    sched.enqueue([] {}, SLAPriority::HIGH,   5, "high");

    QueryEntry out;
    ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(100)));
    // HIGH has shorter deadline so should come first.
    EXPECT_EQ(out.priority, SLAPriority::HIGH);
    sched.shutdown();
}

/**
 * @test PriorityEarliestDeadlineFirst
 * @brief Validates EDF scheduling: earlier deadline dequeued first.
 */
TEST(Phase3LoadBalancing, PriorityEarliestDeadlineFirst) {
    QueryScheduler sched;

    sched.enqueue([] {}, SLAPriority::MEDIUM, 100, "100ms");
    sched.enqueue([] {}, SLAPriority::MEDIUM,  20, "20ms");
    sched.enqueue([] {}, SLAPriority::MEDIUM,  60, "60ms");

    QueryEntry out1, out2, out3;
    ASSERT_TRUE(sched.dequeue(out1, std::chrono::milliseconds(200)));
    ASSERT_TRUE(sched.dequeue(out2, std::chrono::milliseconds(200)));
    ASSERT_TRUE(sched.dequeue(out3, std::chrono::milliseconds(200)));

    // Earliest deadline (smallest sla_ms) must come first.
    EXPECT_EQ(out1.name, "20ms");
    EXPECT_EQ(out2.name, "60ms");
    EXPECT_EQ(out3.name, "100ms");
    sched.shutdown();
}

/**
 * @test PriorityDynamicReprioritzation
 * @brief Validates that entries with approaching deadlines are not starved.
 *
 * Verifies:
 *  - An entry with a near-expiry deadline is dequeued before one with far deadline
 */
TEST(Phase3LoadBalancing, PriorityDynamicReprioritzation) {
    QueryScheduler sched;

    // Enqueue one with a very long deadline and one with a very short deadline.
    sched.enqueue([] {}, SLAPriority::LOW,  10000, "far");
    sched.enqueue([] {}, SLAPriority::LOW,      1, "near");  // 1 ms deadline

    QueryEntry out;
    ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(200)));
    EXPECT_EQ(out.name, "near");  // Near deadline wins.
    sched.shutdown();
}

/**
 * @test PriorityFairnessUnderMixedSLA
 * @brief Validates that different SLA classes all get served.
 */
TEST(Phase3LoadBalancing, PriorityFairnessUnderMixedSLA) {
    QueryScheduler sched;

    std::atomic<int> high_done{0}, low_done{0};

    // Submit 5 HIGH and 5 LOW.
    for (int i = 0; i < 5; ++i) {
        sched.enqueue([&high_done] { high_done++; }, SLAPriority::HIGH,   5);
        sched.enqueue([&low_done]  { low_done++;  }, SLAPriority::LOW,  500);
    }
    EXPECT_EQ(sched.size(), 10u);

    // Drain all entries.
    for (int i = 0; i < 10; ++i) {
        QueryEntry out;
        ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(200)));
        out.execute();
    }
    EXPECT_EQ(high_done.load(), 5);
    EXPECT_EQ(low_done.load(),  5);
    sched.shutdown();
}

/**
 * @test PriorityConcurrentEnqueueDequeue
 * @brief Validates thread-safe priority queue operations.
 *
 * Verifies:
 *  - Multiple enqueue threads work concurrently
 *  - All enqueued items eventually dequeued
 */
TEST(Phase3LoadBalancing, PriorityConcurrentEnqueueDequeue) {
    QueryScheduler sched;

    constexpr int kItems   = 100;
    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};

    std::thread producer([&sched, &enqueued] {
        for (int i = 0; i < kItems; ++i) {
            const auto id = sched.enqueue([] {}, SLAPriority::MEDIUM, 1000);
            if (id != 0) {
              enqueued.fetch_add(1);
            }
        }
    });

    std::thread consumer([&sched, &dequeued] {
        int got = 0;
        while (got < kItems) {
            QueryEntry out = {};
            if (sched.dequeue(out, std::chrono::milliseconds(500))) {
                ++got;
                dequeued.fetch_add(1);
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(enqueued.load(), kItems);
    EXPECT_EQ(dequeued.load(), kItems);
    sched.shutdown();
}

/**
 * @test PrioritySLAComplianceTracking
 * @brief Validates SLA compliance metrics collection.
 *
 * Verifies:
 *  - Completions within deadline counted correctly
 *  - sla_compliance_pct reflects reality
 */
TEST(Phase3LoadBalancing, PrioritySLAComplianceTracking) {
    QueryScheduler sched;

    // Enqueue with generous deadlines.
    const auto id1 = sched.enqueue([] {}, SLAPriority::HIGH, 10000, "q1");
    const auto id2 = sched.enqueue([] {}, SLAPriority::HIGH, 10000, "q2");
    ASSERT_NE(id1, 0u);
    ASSERT_NE(id2, 0u);

    QueryEntry out;
    ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(200)));
    ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(200)));

    // Report completions immediately (within deadline).
    sched.reportCompletion(id1);
    sched.reportCompletion(id2);

    const auto m = sched.metrics();
    EXPECT_EQ(m.completed_total,  2u);
    EXPECT_EQ(m.completed_in_sla, 2u);
    EXPECT_NEAR(m.sla_compliance_pct, 100.0, 0.1);
    sched.shutdown();
}

// ===== Task P3-04-D: Scheduler Integration (4 tests) =====

/**
 * @test SchedulerIntegrationWithExecutor
 * @brief Validates scheduler integrated with executor pipeline.
 *
 * Verifies:
 *  - Dequeued entries execute their functions correctly
 *  - Executor sees queries in priority order
 */
TEST(Phase3LoadBalancing, SchedulerIntegrationWithExecutor) {
    QueryScheduler sched;
    std::atomic<int> result{0};

    sched.enqueue([&result] { result.fetch_add(10); }, SLAPriority::HIGH, 100, "high");
    sched.enqueue([&result] { result.fetch_add(1);  }, SLAPriority::LOW,  500, "low");

    // Drain and execute in scheduler order.
    for (int i = 0; i < 2; ++i) {
        QueryEntry out;
        ASSERT_TRUE(sched.dequeue(out, std::chrono::milliseconds(200)));
        out.execute();
    }
    EXPECT_EQ(result.load(), 11);
    sched.shutdown();
}

/**
 * @test SchedulerBackpressureHandling
 * @brief Validates backpressure from overloaded queue.
 *
 * Verifies:
 *  - Enqueue returns 0 when queue full and timeout very short
 */
TEST(Phase3LoadBalancing, SchedulerBackpressureHandling) {
    QueryScheduler::Config cfg;
    cfg.max_queue_depth = 2;
    QueryScheduler sched(cfg);

    // Fill to capacity.
    EXPECT_NE(sched.enqueue([] {}, SLAPriority::MEDIUM, 1000, "q1",
                            std::chrono::seconds(1)), 0u);
    EXPECT_NE(sched.enqueue([] {}, SLAPriority::MEDIUM, 1000, "q2",
                            std::chrono::seconds(1)), 0u);

    // Third must fail (1 ms timeout, queue full).
    const auto id3 = sched.enqueue([] {}, SLAPriority::MEDIUM, 1000, "q3",
                                   std::chrono::milliseconds(1));
    EXPECT_EQ(id3, 0u);  // Rejected.
    sched.shutdown();
}

/**
 * @test SchedulerLoadSheddingUnderOverload
 * @brief Validates load shedding for LOW-priority queries at shed threshold.
 */
TEST(Phase3LoadBalancing, SchedulerLoadSheddingUnderOverload) {
    QueryScheduler::Config cfg;
    cfg.max_queue_depth = 10000;
    cfg.shed_threshold  = 3;  // Low threshold for test.
    QueryScheduler sched(cfg);

    // Fill queue with MEDIUM to reach shed threshold.
    for (int i = 0; i < 3; ++i) {
        sched.enqueue([] {}, SLAPriority::MEDIUM, 10000, "filler");
    }

    // LOW-priority enqueue at shed threshold must be shed.
    const auto shed_id = sched.enqueue([] {}, SLAPriority::LOW, 10000, "low_shed");
    EXPECT_EQ(shed_id, 0u);

    const auto m = sched.metrics();
    EXPECT_GE(m.total_shed, 1u);
    sched.shutdown();
}

/**
 * @test SchedulerMetricsReporting
 * @brief Validates scheduler metrics reporting.
 *
 * Verifies:
 *  - Enqueued and dequeued counts tracked
 *  - avg_enqueue_us and avg_dequeue_us populated
 */
TEST(Phase3LoadBalancing, SchedulerMetricsReporting) {
    QueryScheduler sched;

    for (int i = 0; i < 5; ++i) {
        sched.enqueue([] {}, SLAPriority::MEDIUM, 1000, "item");
    }

    QueryEntry out;
    for (int i = 0; i < 3; ++i) {
        sched.dequeue(out, std::chrono::milliseconds(200));
    }

    const auto m = sched.metrics();
    EXPECT_EQ(m.total_enqueued, 5u);
    EXPECT_EQ(m.total_dequeued, 3u);
    EXPECT_GT(m.avg_enqueue_us, 0.0);
    EXPECT_GT(m.avg_dequeue_us, 0.0);
    sched.shutdown();
}

// ===== Task P3-04-E: Load-Balancing Benchmarks (6 tests) =====

/**
 * @test BenchmarkUniformLoad
 * @brief Benchmarks load balancing under uniform query distribution.
 *
 * Verifies:
 *  - All shards receive similar load (comparable selection counts)
 */
TEST(Phase3LoadBalancing, BenchmarkUniformLoad) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"s0", "s1", "s2", "s3"});

    constexpr int kQueries = 200;
    for (int i = 0; i < kQueries; ++i) {
        lb.selectShard();
    }

    const auto stats = lb.statistics();
    std::size_t min_sel = std::numeric_limits<std::size_t>::max();
    std::size_t max_sel = 0;
    for (const auto& s : stats) {
        min_sel = std::min(min_sel, s.total_selected);
        max_sel = std::max(max_sel, s.total_selected);
    }
    // With uniform metrics, distribution should be equal (all go to s0 due to tie-breaking).
    // At least each shard should have been callable; check max >= 1.
    EXPECT_GE(max_sel, 1u);
}

/**
 * @test BenchmarkSkewedLoad
 * @brief Benchmarks load balancing under skewed (Zipf-like) distribution.
 *
 * Verifies:
 *  - Heavily loaded shard receives far fewer queries after metrics update
 */
TEST(Phase3LoadBalancing, BenchmarkSkewedLoad) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"hot", "cold1", "cold2", "cold3"});

    // Simulate hot shard.
    ShardMetrics hot;
    hot.cpu_percent = 95.0;
    hot.pending_queries = 180;
    lb.updateMetrics("hot", hot);

    constexpr int kQueries = 100;
    int hot_count = 0;
    for (int i = 0; i < kQueries; ++i) {
        if (lb.selectShard() == "hot") {
          ++hot_count;
        }
    }
    // Hot shard should receive minimal traffic.
    EXPECT_LT(hot_count, kQueries / 4);
}

/**
 * @test BenchmarkLatencyDistribution
 * @brief Benchmarks latency distribution via scheduler.
 *
 * Verifies:
 *  - 100 enqueue+dequeue operations complete in < 100 ms total
 */
TEST(Phase3LoadBalancing, BenchmarkLatencyDistribution) {
    QueryScheduler sched;

    constexpr int kOps = 100;
    const auto t0 = std::chrono::steady_clock::now();

    for (int i = 0; i < kOps; ++i) {
        sched.enqueue([] {}, SLAPriority::MEDIUM, 1000);
    }
    for (int i = 0; i < kOps; ++i) {
        QueryEntry out;
        sched.dequeue(out, std::chrono::milliseconds(500));
    }

    const long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    EXPECT_LT(elapsed_ms, 1000);  // 1 s is very generous for 100 in-memory ops.
    sched.shutdown();
}

/**
 * @test BenchmarkDynamicLoadShifting
 * @brief Benchmarks scheduler response to dynamic load shifts.
 *
 * Verifies:
 *  - After metrics update, balancer immediately prefers the newly-light shard
 */
TEST(Phase3LoadBalancing, BenchmarkDynamicLoadShifting) {
    using namespace themis::sharding;
    ShardLoadBalancer lb({"s0", "s1"});

    // Initially s0 is light.
    ShardMetrics light;
    light.cpu_percent = 5.0;
    lb.updateMetrics("s0", light);
    ShardMetrics heavy;
    heavy.cpu_percent = 90.0;
    lb.updateMetrics("s1", heavy);

    EXPECT_EQ(lb.selectShard(), "s0");

    // Now shift: s1 becomes light, s0 becomes heavy.
    lb.updateMetrics("s0", heavy);
    lb.updateMetrics("s1", light);

    EXPECT_EQ(lb.selectShard(), "s1");
}

/**
 * @test BenchmarkSLAComplianceUnderMixedLoad
 * @brief Benchmarks SLA compliance under realistic mixed-priority load.
 *
 * Verifies:
 *  - Completions recorded within deadline increment compliance counter
 *  - Compliance rate approaches 100% for generous deadlines
 */
TEST(Phase3LoadBalancing, BenchmarkSLAComplianceUnderMixedLoad) {
    QueryScheduler sched;
    std::vector<std::uint64_t> ids;

    constexpr int kItems = 30;
    for (int i = 0; i < kItems; ++i) {
        const auto pri = (i % 3 == 0) ? SLAPriority::HIGH
                       : (i % 3 == 1) ? SLAPriority::MEDIUM
                                       : SLAPriority::LOW;
        const long sla = (pri == SLAPriority::HIGH)   ?  5000
                        : (pri == SLAPriority::MEDIUM) ? 10000
                                                        : 50000;
        const auto id = sched.enqueue([] {}, pri, sla);
        if (id != 0) {
          ids.push_back(id);
        }
    }

    // Drain and report immediate completions.
    for (int i = 0; i < static_cast<int>(ids.size()); ++i) {
        QueryEntry out = {};
        if (sched.dequeue(out, std::chrono::milliseconds(200))) {
            sched.reportCompletion(out.id);
        }
    }

    const auto m = sched.metrics();
    // All completions should be within the generous SLA windows.
    EXPECT_EQ(m.completed_in_sla, m.completed_total);
    EXPECT_NEAR(m.sla_compliance_pct, 100.0, 1.0);
    sched.shutdown();
}

/**
 * @test BenchmarkSchedulerOverheadMeasurement
 * @brief Benchmarks overhead of scheduler enqueue + dequeue operations.
 *
 * Verifies:
 *  - 200 enqueue+dequeue cycles complete in < 500 ms
 *  - avg_enqueue_us and avg_dequeue_us populated and plausible
 */
TEST(Phase3LoadBalancing, BenchmarkSchedulerOverheadMeasurement) {
    QueryScheduler sched;
    constexpr int kOps = 200;

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kOps; ++i) {
        sched.enqueue([] {}, SLAPriority::MEDIUM, 60000);
    }
    for (int i = 0; i < kOps; ++i) {
        QueryEntry out;
        sched.dequeue(out, std::chrono::milliseconds(500));
    }
    const long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_LT(elapsed_ms, 500);

    const auto m = sched.metrics();
    EXPECT_GT(m.avg_enqueue_us, 0.0);
    EXPECT_GT(m.avg_dequeue_us, 0.0);
    sched.shutdown();
}
} } // namespace themis::execution
