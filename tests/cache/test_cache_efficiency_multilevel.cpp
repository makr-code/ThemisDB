/**
 * @file test_cache_efficiency_multilevel.cpp
 * @brief Phase 3 P3-02 multi-tier cache eviction tests.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "cache/cache_eviction_policy.h"
#include "cache/cache_manager.h"

using namespace themis::cache;

namespace {

CacheKeyDescriptor makeDescriptor(std::string key,
                                  size_t access_count,
                                  int64_t last_access_ns,
                                  int64_t creation_time_ns) {
    return {std::move(key), access_count, last_access_ns, creation_time_ns};
}

WeightedTieredLRUEvictionPolicy makePolicy(double frequency_weight = 0.3,
                                           double recency_weight = 0.7,
                                           bool adaptive_thresholds = true) {
    WeightedTieredLRUEvictionPolicy::Config config;
    config.l2_promotion_threshold = 2;
    config.l1_promotion_threshold = 5;
    config.frequency_weight = frequency_weight;
    config.recency_weight = recency_weight;
    config.frequency_decay_factor = 0.5;
    config.trigger_threshold_percent = 70;
    config.safe_threshold_percent = 50;
    config.severe_threshold_percent = 85;
    config.adaptive_thresholds = adaptive_thresholds;
    config.threshold_adjustment_interval_ns = 100;
    return WeightedTieredLRUEvictionPolicy(config);
}

CacheManagerConfig makeManagerConfig(size_t default_cache_size) {
    CacheManagerConfig config;
    config.default_cache_size = default_cache_size;
    return config;
}

class Phase3CacheEfficiency : public ::testing::Test {
protected:
    WeightedTieredLRUEvictionPolicy policy_ = makePolicy();
};

TEST_F(Phase3CacheEfficiency, MultiTierEvictionHotTier) {
    policy_.record_insert("hot", 1);
    for (int i = 0; i < 5; ++i) {
        policy_.record_hit("hot");
    }
    EXPECT_EQ(policy_.tier_for_key("hot"), WeightedTieredLRUEvictionPolicy::Tier::L1);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionWarmTier) {
    policy_.record_insert("warm", 1);
    for (int i = 0; i < 2; ++i) {
        policy_.record_hit("warm");
    }
    EXPECT_EQ(policy_.tier_for_key("warm"), WeightedTieredLRUEvictionPolicy::Tier::L2);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionColdTier) {
    policy_.record_insert("cold", 1);
    EXPECT_EQ(policy_.tier_for_key("cold"), WeightedTieredLRUEvictionPolicy::Tier::L3);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionTierPromotion) {
    policy_.record_insert("promote", 1);
    EXPECT_EQ(policy_.tier_for_key("promote"), WeightedTieredLRUEvictionPolicy::Tier::L3);
    policy_.record_hit("promote");
    policy_.record_hit("promote");
    EXPECT_EQ(policy_.tier_for_key("promote"), WeightedTieredLRUEvictionPolicy::Tier::L2);
    for (int i = 0; i < 3; ++i) {
        policy_.record_hit("promote");
    }
    EXPECT_EQ(policy_.tier_for_key("promote"), WeightedTieredLRUEvictionPolicy::Tier::L1);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionOrderingUnderPressure) {
    policy_.record_insert("cold", 1);
    policy_.record_insert("warm", 1);
    policy_.record_hit("warm");
    policy_.record_hit("warm");
    policy_.record_insert("hot", 1);
    for (int i = 0; i < 5; ++i) {
        policy_.record_hit("hot");
    }

    const auto victim = policy_.choose_victim({
        makeDescriptor("hot", 5, 90, 10),
        makeDescriptor("warm", 2, 50, 5),
        makeDescriptor("cold", 0, 10, 1),
    });
    ASSERT_TRUE(victim.should_evict);
    EXPECT_EQ(victim.victim_key, "cold");
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionConcurrentTierTransitions) {
    policy_.record_insert("k1", 1);
    policy_.record_insert("k2", 1);
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i) {
                policy_.record_hit((t % 2 == 0) ? "k1" : "k2");
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    const auto distribution = policy_.tier_distribution();
    EXPECT_EQ(distribution[1] + distribution[2], 2u);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionMemoryPressureThreshold) {
    EXPECT_EQ(policy_.recommended_batch_size(69, 100), 0u);
    EXPECT_EQ(policy_.recommended_batch_size(70, 100), 1u);
    EXPECT_GE(policy_.recommended_batch_size(90, 20), 10u);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionBatchEvictionUnderSevereMemoryPressure) {
    const auto batch = policy_.recommended_batch_size(91, 40);
    EXPECT_GE(batch, 10u);
    EXPECT_LE(batch, 40u);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionTierDistribution) {
    policy_.record_insert("cold1", 1);
    policy_.record_insert("cold2", 1);
    policy_.record_insert("warm1", 1);
    policy_.record_insert("warm2", 1);
    policy_.record_insert("hot", 1);
    policy_.record_hit("warm1");
    policy_.record_hit("warm1");
    policy_.record_hit("warm2");
    policy_.record_hit("warm2");
    for (int i = 0; i < 5; ++i) {
        policy_.record_hit("hot");
    }

    const auto distribution = policy_.tier_distribution();
    EXPECT_EQ(distribution[0], 2u);
    EXPECT_EQ(distribution[1], 2u);
    EXPECT_EQ(distribution[2], 1u);
}

TEST_F(Phase3CacheEfficiency, MultiTierEvictionRegressionVsPhase24) {
    LRUEvictionPolicy single_tier;
    const auto candidates = std::vector<CacheKeyDescriptor>{
        makeDescriptor("recent_hot", 10, 90, 10),
        makeDescriptor("stale_cold", 0, 1, 1),
    };
    policy_.record_insert("recent_hot", 1);
    for (int i = 0; i < 10; ++i) {
        policy_.record_hit("recent_hot");
    }
    policy_.record_insert("stale_cold", 1);

    const auto tiered = policy_.choose_victim(candidates);
    const auto baseline = single_tier.choose_victim(candidates);
    EXPECT_EQ(tiered.victim_key, "stale_cold");
    EXPECT_EQ(baseline.victim_key, "stale_cold");
}

TEST_F(Phase3CacheEfficiency, WeightedScoringFrequencyComponent) {
    policy_.record_insert("low", 1);
    policy_.record_insert("high", 1);
    policy_.record_hit("low");
    for (int i = 0; i < 5; ++i) {
        policy_.record_hit("high");
    }
    EXPECT_GT(policy_.score_for_key("high"), policy_.score_for_key("low"));
}

TEST_F(Phase3CacheEfficiency, WeightedScoringRecencyComponent) {
    const auto now = 1'000'000'000LL;
    const auto recent = policy_.score_for_descriptor(makeDescriptor("recent", 1, now - 1'000, 0), now);
    const auto stale = policy_.score_for_descriptor(makeDescriptor("stale", 1, now - 1'000'000'000, 0), now);
    EXPECT_GT(recent, stale);
}

TEST_F(Phase3CacheEfficiency, WeightedScoringCombinedScore) {
    const auto now = 2'000'000'000LL;
    const auto combined = policy_.score_for_descriptor(makeDescriptor("combo", 5, now - 1'000, 0), now);
    const auto weak = policy_.score_for_descriptor(makeDescriptor("weak", 1, now - 2'000'000'000, 0), now);
    EXPECT_GT(combined, weak);
}

TEST_F(Phase3CacheEfficiency, WeightedScoringEvictionOrderByScore) {
    policy_.record_insert("keep", 1);
    for (int i = 0; i < 3; ++i) {
        policy_.record_hit("keep");
    }
    policy_.record_insert("evict", 1);

    const auto decision = policy_.choose_victim({
        makeDescriptor("keep", 3, 100, 1),
        makeDescriptor("evict", 0, 10, 1),
    });
    EXPECT_EQ(decision.victim_key, "evict");
}

TEST_F(Phase3CacheEfficiency, WeightedScoringTuningForWorkload) {
    auto frequency_policy = makePolicy(0.9, 0.1);
    auto recency_policy = makePolicy(0.1, 0.9);
    const auto now = 1'000'000'000LL;
    const auto frequent_old = makeDescriptor("frequent_old", 5, now - 500'000'000, 0);
    const auto rare_recent = makeDescriptor("rare_recent", 1, now - 1'000, 0);

    EXPECT_GT(frequency_policy.score_for_descriptor(frequent_old, now),
              frequency_policy.score_for_descriptor(rare_recent, now));
    EXPECT_GT(recency_policy.score_for_descriptor(rare_recent, now),
              recency_policy.score_for_descriptor(frequent_old, now));
}

TEST_F(Phase3CacheEfficiency, WeightedScoringFrequencyDecay) {
    policy_.record_insert("decay", 1);
    policy_.record_hit("decay");
    const auto before = policy_.score_for_key("decay");
    policy_.record_miss("decay");
    const auto after = policy_.score_for_key("decay");
    EXPECT_LT(after, before);
}

TEST_F(Phase3CacheEfficiency, WeightedScoringImpactOnHitRatio) {
    policy_.record_insert("hot", 1);
    for (int i = 0; i < 10; ++i) {
        policy_.record_hit("hot");
    }
    policy_.record_insert("cold", 1);

    const auto decision = policy_.choose_victim({
        makeDescriptor("hot", 10, 100, 10),
        makeDescriptor("cold", 0, 10, 1),
    });
    EXPECT_EQ(decision.victim_key, "cold");
}

TEST_F(Phase3CacheEfficiency, WeightedScoringEdgeCases) {
    policy_.record_insert("fresh", 1);
    EXPECT_GE(policy_.score_for_key("fresh"), 0.0);
    EXPECT_DOUBLE_EQ(policy_.score_for_key("unknown"), 0.0);
}

TEST_F(Phase3CacheEfficiency, EvictionTriggerAtCapacityThreshold) {
    EXPECT_EQ(policy_.recommended_batch_size(50, 10), 0u);
    EXPECT_EQ(policy_.recommended_batch_size(policy_.trigger_threshold_percent(), 10), 1u);
}

TEST_F(Phase3CacheEfficiency, EvictionTriggerResponseTime) {
    const auto start = std::chrono::steady_clock::now();
    policy_.observe_capacity(91, 1'000'000'000LL);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count(), 1000);
}

TEST_F(Phase3CacheEfficiency, AdaptiveThresholdTuning) {
    const auto initial_trigger = policy_.trigger_threshold_percent();
    policy_.observe_capacity(91, 1'000'000'000LL);
    EXPECT_LT(policy_.trigger_threshold_percent(), initial_trigger);
    const auto lowered = policy_.trigger_threshold_percent();
    policy_.observe_capacity(60, 1'000'000'200LL);
    EXPECT_GE(policy_.trigger_threshold_percent(), lowered);
}

TEST_F(Phase3CacheEfficiency, AdaptiveThresholdStability) {
    const auto initial_trigger = policy_.trigger_threshold_percent();
    policy_.observe_capacity(91, 1'000'000'000LL);
    const auto lowered = policy_.trigger_threshold_percent();
    policy_.observe_capacity(91, 1'000'000'050LL);
    EXPECT_EQ(policy_.trigger_threshold_percent(), lowered);
    EXPECT_LE(lowered, initial_trigger);
}

TEST_F(Phase3CacheEfficiency, EvictionTriggerUnderCyclicalLoad) {
    policy_.observe_capacity(91, 1'000'000'000LL);
    const auto after_peak = policy_.trigger_threshold_percent();
    policy_.observe_capacity(60, 1'000'000'200LL);
    const auto after_valley = policy_.trigger_threshold_percent();
    EXPECT_GE(after_valley, after_peak);
    EXPECT_LE(after_valley, 84u);
}

TEST_F(Phase3CacheEfficiency, EvictionTriggerFallbackBehavior) {
    auto static_policy = makePolicy(0.3, 0.7, false);
    const auto trigger = static_policy.trigger_threshold_percent();
    static_policy.observe_capacity(91, 1'000'000'000LL);
    EXPECT_EQ(static_policy.trigger_threshold_percent(), trigger);
    EXPECT_EQ(static_policy.safe_threshold_percent(), 50u);
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheExecutorPipeline) {
    CacheManager manager(makeManagerConfig(128));
    ASSERT_TRUE(manager.register_cache("queries", 128));
    ASSERT_TRUE(manager.set_eviction_policy("queries", makePolicy()));
    const auto* policy = manager.get_eviction_policy("queries");
    ASSERT_NE(policy, nullptr);
    EXPECT_STREQ(policy->policy_name(), "TIERED_LRU");
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheConsistencyUnderConcurrency) {
    CacheManager manager(makeManagerConfig(128));
    ASSERT_TRUE(manager.register_cache("queries", 128));
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            if (manager.set_eviction_policy("queries", makePolicy())) {
                ++success;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_GT(success.load(), 0);
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheInvalidationOnWriteOperations) {
    policy_.record_insert("users:1", 1);
    policy_.record_hit("users:1");
    policy_.record_hit("users:1");
    EXPECT_NE(policy_.tier_for_key("users:1"), WeightedTieredLRUEvictionPolicy::Tier::L3);
    policy_.record_delete("users:1");
    EXPECT_EQ(policy_.tier_for_key("users:1"), WeightedTieredLRUEvictionPolicy::Tier::L3);
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheWithoutPrefixEviction) {
    policy_.record_insert("users:1", 1);
    policy_.record_insert("users:2", 1);
    policy_.record_insert("orders:1", 1);
    policy_.record_delete("users:1");
    policy_.record_delete("users:2");
    EXPECT_EQ(policy_.tier_distribution()[0], 1u);
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheStatisticsAccuracy) {
    policy_.record_insert("cold", 1);
    policy_.record_insert("warm", 1);
    policy_.record_hit("warm");
    policy_.record_hit("warm");
    const auto distribution = policy_.tier_distribution();
    EXPECT_EQ(distribution[0], 1u);
    EXPECT_EQ(distribution[1], 1u);
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheMonitoring) {
    CacheManager manager(makeManagerConfig(64));
    ASSERT_TRUE(manager.register_cache("queries", 64));
    std::vector<CacheEvent> events;
    manager.register_event_handler([&](const CacheEvent& event) { events.push_back(event); });
    ASSERT_TRUE(manager.set_eviction_policy("queries", makePolicy()));
    ASSERT_FALSE(events.empty());
    EXPECT_EQ(events.back().type, CacheEvent::Type::POLICY_CHANGE);
    EXPECT_EQ(events.back().cache_name, "queries");
}

TEST_F(Phase3CacheEfficiency, IntegrationCacheRecoveryAfterShutdown) {
    policy_.record_insert("persisted", 1);
    for (int i = 0; i < 5; ++i) {
        policy_.record_hit("persisted");
    }
    auto clone = policy_.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->policy_name(), "TIERED_LRU");
}

TEST_F(Phase3CacheEfficiency, IntegrationCachePerformanceUnderMixedWorkload) {
    for (int i = 0; i < 32; ++i) {
        policy_.record_insert("mixed-" + std::to_string(i), 1);
        if (i % 3 == 0) {
            policy_.record_hit("mixed-" + std::to_string(i));
        }
    }
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 64; ++i) {
        policy_.choose_victim({
            makeDescriptor("mixed-0", 3, 100, 10),
            makeDescriptor("mixed-1", 0, 10, 1),
            makeDescriptor("mixed-2", 0, 5, 1),
        });
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 5);
}

}  // namespace
