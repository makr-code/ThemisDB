// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_wave_a_fencing.cpp
 * @brief Wave A gate FO-Promote-04: fencing verification before promotion.
 *
 * Validates gap FO-IMPL-003: the auto-failover promotion path now verifies
 * epoch fencing regardless of the enable_split_brain_prevention flag when a
 * fencing manager is configured.
 *
 * Test matrix:
 *   FO-Promote-04-NO-FENCE           : fencing_manager_=null, prevention=true
 *                                      → preventSplitBrain returns false,
 *                                        QUORUM_CHECK_FAILED event emitted.
 *   FO-Promote-04-FENCE-OK           : StubEpochFencingManager returns epoch=42
 *                                      → preventSplitBrain returns true,
 *                                        split_brain_preventions stat incremented.
 *   FO-Promote-04-FENCE-INVALID-EPOCH: StubEpochFencingManager returns epoch=0
 *                                      → preventSplitBrain returns false,
 *                                        FAILOVER_CANCELLED (SPLIT_BRAIN_DETECTED)
 *                                        event emitted.
 *   FO-Promote-04-PREVENTION-DISABLED-NO-FENCE:
 *                                      prevention=false, fencing_manager_=null
 *                                      → processFailover does NOT invoke
 *                                        preventSplitBrain; no FAILOVER_CANCELLED
 *                                        (split-brain) event fired.
 *   FO-Promote-06-SPARE-ACTIVATION-READBACK:
 *                                      real HotSpareManager activation
 *                                      → ACTIVE state independently verified.
 *   FO-Promote-07-SPARE-ACK-WITHOUT-STATE:
 *                                      adapter ACK with spare still AVAILABLE
 *                                      → activation rejected.
 *
 * Tests use no network I/O; persistence-related test cases may use temporary files.
 * Test seed constant: kFencingTestSeed = 42.
 *
 * @see include/failover/auto_failover_manager.h
 * @see src/failover/auto_failover_manager.cpp — FO-IMPL-003 changes
 * @see include/sharding/epoch_fencing.h
 */

#ifdef THEMIS_TEST_BUILD

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"
#include "failover/failover_api_contract.h"
#include "sharding/consistent_hash.h"
#include "sharding/epoch_fencing.h"
#include "sharding/hot_spare_manager.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/shard_topology.h"

using namespace std::chrono_literals;
using namespace themis::failover;

namespace {

// ---------------------------------------------------------------------------
// Canonical seed
// ---------------------------------------------------------------------------
constexpr uint32_t kFencingTestSeed = 42;

// ---------------------------------------------------------------------------
// StubEpochFencingManager
//
// Overrides bumpEpoch() to return a caller-specified epoch value, enabling
// controlled testing of the invalid-epoch guard in preventSplitBrain().
// ---------------------------------------------------------------------------
class StubEpochFencingManager : public themis::sharding::EpochFencingManager {
public:
    explicit StubEpochFencingManager(themis::sharding::EpochNumber epoch_to_return)
        : themis::sharding::EpochFencingManager(themis::sharding::EpochFencingConfig{})
        , epoch_to_return_(epoch_to_return)
    {}

    [[nodiscard]] themis::sharding::EpochToken bumpEpoch(
            const std::string& /*reason*/) override {
        ++bump_call_count;
        themis::sharding::EpochToken tok;
        tok.epoch     = epoch_to_return_;
        tok.issued_at = std::chrono::system_clock::now();
        return tok;
    }

    std::atomic<int> bump_call_count{0};

private:
    themis::sharding::EpochNumber epoch_to_return_;
};

// ---------------------------------------------------------------------------
// Config helpers
// ---------------------------------------------------------------------------

/// Minimal AutoFailoverConfig with fast timers and all side-effects disabled.
AutoFailoverConfig makeFencingTestConfig() {
    AutoFailoverConfig cfg;
    cfg.health_check_interval              = 10ms;
    cfg.failure_detection_interval         = 10ms;
    cfg.failover_timeout                   = 50ms;
    cfg.spare_activation_timeout           = 50ms;
    cfg.leader_election_timeout            = 50ms;
    cfg.recovery_retry_interval            = 0ms;
    cfg.max_recovery_attempts              = 1;
    cfg.enable_automatic_failover          = true;
    cfg.enable_automatic_recovery          = false;  // no 5-second delay in tests
    cfg.enable_spare_activation            = false;
    cfg.enable_network_partition_detection = false;
    cfg.enable_split_brain_prevention      = true;   // default-on; overridden per test
    cfg.max_concurrent_failovers           = 4;
    return cfg;
}

}  // namespace

// ===========================================================================
// FO-Promote-04-NO-FENCE
// preventSplitBrain fails closed when fencing manager is null and prevention
// is enabled.  QUORUM_CHECK_FAILED event must be emitted.
// ===========================================================================

TEST(WaveAFencing, FO_Promote_04_NO_FENCE) {
    (void)kFencingTestSeed;

    AutoFailoverConfig cfg = makeFencingTestConfig();
    cfg.enable_split_brain_prevention = true;

    // null fencing manager (last arg)
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> quorum_failed_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::QUORUM_CHECK_FAILED) {
                quorum_failed_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // preventSplitBrain must fail closed — no fencing manager, prevention=true
    const bool result = mgr.testPreventSplitBrain("node-failed");

    EXPECT_FALSE(result) << "Expected fail-closed when no fencing manager is configured "
                            "and enable_split_brain_prevention=true";

    // Diagnostic QUORUM_UNAVAILABLE maps to QUORUM_CHECK_FAILED event.
    EXPECT_GE(quorum_failed_events.load(), 1)
        << "Expected QUORUM_CHECK_FAILED event from split-brain prevention failure";
}

// ===========================================================================
// FO-Promote-04-FENCE-OK
// When a fencing manager returns a valid epoch (42), preventSplitBrain must
// succeed and the split_brain_preventions statistic must be incremented.
// ===========================================================================

TEST(WaveAFencing, FO_Promote_04_FENCE_OK) {
    AutoFailoverConfig cfg = makeFencingTestConfig();
    cfg.enable_split_brain_prevention = true;

    auto stub_fencing = std::make_shared<StubEpochFencingManager>(
        static_cast<themis::sharding::EpochNumber>(kFencingTestSeed)  // epoch = 42
    );

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, stub_fencing);

    const bool result = mgr.testPreventSplitBrain("node-primary");

    EXPECT_TRUE(result) << "Expected preventSplitBrain to succeed with a valid epoch (42)";
    EXPECT_EQ(stub_fencing->bump_call_count.load(), 1)
        << "bumpEpoch should be called exactly once";

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.split_brain_preventions, 1u)
        << "split_brain_preventions counter must be incremented on success";
}

// ===========================================================================
// FO-Promote-04-FENCE-INVALID-EPOCH
// When fencing returns epoch=0 (invalid sentinel), preventSplitBrain must
// fail closed and emit a SPLIT_BRAIN_DETECTED (→ FAILOVER_CANCELLED) event.
// ===========================================================================

TEST(WaveAFencing, FO_Promote_04_FENCE_INVALID_EPOCH) {
    AutoFailoverConfig cfg = makeFencingTestConfig();
    cfg.enable_split_brain_prevention = true;

    // epoch=0 is kInvalidEpoch — simulates a broken fencing back-end
    auto stub_fencing = std::make_shared<StubEpochFencingManager>(
        static_cast<themis::sharding::EpochNumber>(0)
    );

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, stub_fencing);

    std::atomic<int> failover_cancelled_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::FAILOVER_CANCELLED) {
                failover_cancelled_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    const bool result = mgr.testPreventSplitBrain("node-primary");

    EXPECT_FALSE(result) << "Expected fail-closed when bumpEpoch returns invalid epoch 0";
    EXPECT_EQ(stub_fencing->bump_call_count.load(), 1)
        << "bumpEpoch should be called exactly once before the guard fires";

    // SPLIT_BRAIN_DETECTED maps to FAILOVER_CANCELLED in emitDiagnostic.
    EXPECT_GE(failover_cancelled_events.load(), 1)
        << "Expected FAILOVER_CANCELLED (SPLIT_BRAIN_DETECTED) event on invalid epoch";

    // Stat must NOT be incremented for a failed fencing attempt.
    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.split_brain_preventions, 0u)
        << "split_brain_preventions must not be incremented when fencing failed";
}

// ===========================================================================
// FO-Promote-04-PREVENTION-DISABLED-BLOCKED
// Disabling fencing must never permit failover, even when quorum is present.
// ===========================================================================

TEST(WaveAFencing, FO_Promote_04_PREVENTION_DISABLED_BLOCKED) {
    AutoFailoverConfig cfg = makeFencingTestConfig();
    cfg.enable_split_brain_prevention = false;  // prevention explicitly disabled

    // Test-only quorum override lets the request reach the fencing safety gate.
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetQuorumOverride(true);

    std::atomic<int> split_brain_cancel_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::FAILOVER_CANCELLED) {
                split_brain_cancel_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    const auto result = mgr.testProcessFailover("node-down");

    EXPECT_FALSE(result.success)
        << "Failover must fail when fencing is disabled";
    EXPECT_EQ(result.error_message, "fencing cannot be disabled for failover");

    EXPECT_EQ(split_brain_cancel_events.load(), 1)
        << "A fail-safe diagnostic must be emitted when fencing is disabled";

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.split_brain_preventions, 0u)
        << "split_brain_preventions counts actual fencing attempts only";
}

TEST(WaveAFencing, FO_Promote_05_SPARE_FAILURE_LEAVES_DEGRADED_STATE) {
    auto config = makeFencingTestConfig();
    config.enable_spare_activation = true;
    auto fencing = std::make_shared<StubEpochFencingManager>(42);
    AutoFailoverManager manager(config, nullptr, nullptr, nullptr, fencing);
    manager.testSetQuorumOverride(true);
    manager.testSetPromotionOverride(
        [](const std::string&, std::string& promoted_id) {
            promoted_id = "replica-1";
            return true;
        });

    std::atomic<int> degraded_events{0};
    manager.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::FAILOVER_DEGRADED) {
                ++degraded_events;
            }
        });

    const auto result = manager.testProcessFailover("node-down");

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.degraded);
    EXPECT_EQ(manager.getState(), FailoverOrchestratorState::DEGRADED);
    EXPECT_EQ(degraded_events.load(), 1);
    EXPECT_EQ(manager.getStatistics().degraded_failovers, 1u);
}

TEST(WaveAFencing, FO_Promote_06_SPARE_ACTIVATION_REQUIRES_ACTIVE_READBACK) {
    themis::sharding::RedundancyConfig redundancy_config;
    themis::sharding::RedundancyStrategy redundancy_strategy(redundancy_config);
    themis::sharding::ShardTopology topology;

    themis::sharding::HotSpareConfig spare_config;
    spare_config.enable = true;
    spare_config.spare_shards = {"spare-1"};
    spare_config.auto_rebuild = false;
    auto spare_manager = std::make_shared<themis::sharding::HotSpareManager>(
        spare_config, redundancy_strategy, topology);
    themis::sharding::ConsistentHashRing ring;
    ring.addShard("node-down");

    auto config = makeFencingTestConfig();
    config.enable_spare_activation = true;
    AutoFailoverManager manager(config, nullptr, nullptr, spare_manager, nullptr);
    std::atomic<int> activated_events{0};
    manager.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::SPARE_ACTIVATED) {
                ++activated_events;
            }
        });

    manager.setSpareActivationHook(
        [&](const std::string& failed_shard, const std::string&) {
            return spare_manager->activateSpare(
                failed_shard, ring,
                [](const std::string&, const std::string&)
                    -> std::optional<std::vector<uint8_t>> { return std::nullopt; },
                [](const std::string&, const std::string&,
                   const std::vector<uint8_t>&) { return true; },
                [](const std::string&) { return std::vector<std::string>{}; });
        });

    std::string activated_spare_id;
    EXPECT_TRUE(manager.testActivateSpareIfNeeded("node-down", activated_spare_id));
    EXPECT_EQ(activated_spare_id, "spare-1");
    const auto spare_info = spare_manager->getSpareInfo(activated_spare_id);
    ASSERT_TRUE(spare_info.has_value());
    EXPECT_EQ(spare_info->state, themis::sharding::SpareState::ACTIVE);
    EXPECT_EQ(activated_events.load(), 1);
}

TEST(WaveAFencing, FO_Promote_07_SPARE_ACK_WITHOUT_ACTIVE_STATE_IS_REJECTED) {
    themis::sharding::RedundancyConfig redundancy_config;
    themis::sharding::RedundancyStrategy redundancy_strategy(redundancy_config);
    themis::sharding::ShardTopology topology;

    themis::sharding::HotSpareConfig spare_config;
    spare_config.enable = true;
    spare_config.spare_shards = {"spare-1"};
    spare_config.auto_rebuild = false;
    auto spare_manager = std::make_shared<themis::sharding::HotSpareManager>(
        spare_config, redundancy_strategy, topology);

    auto config = makeFencingTestConfig();
    config.enable_spare_activation = true;
    AutoFailoverManager manager(config, nullptr, nullptr, spare_manager, nullptr);
    manager.setSpareActivationHook(
        [](const std::string&, const std::string&) { return true; });

    std::string activated_spare_id;
    EXPECT_FALSE(manager.testActivateSpareIfNeeded("node-down", activated_spare_id));
    EXPECT_TRUE(activated_spare_id.empty());
    const auto spare_info = spare_manager->getSpareInfo("spare-1");
    ASSERT_TRUE(spare_info.has_value());
    EXPECT_EQ(spare_info->state, themis::sharding::SpareState::AVAILABLE);
}

TEST(WaveAFencing, FO_Promote_08_MISSING_SPARE_ADAPTER_IS_REJECTED) {
    themis::sharding::RedundancyConfig redundancy_config;
    themis::sharding::RedundancyStrategy redundancy_strategy(redundancy_config);
    themis::sharding::ShardTopology topology;

    themis::sharding::HotSpareConfig spare_config;
    spare_config.enable = true;
    spare_config.spare_shards = {"spare-1"};
    spare_config.auto_rebuild = false;
    auto spare_manager = std::make_shared<themis::sharding::HotSpareManager>(
        spare_config, redundancy_strategy, topology);

    auto config = makeFencingTestConfig();
    config.enable_spare_activation = true;
    AutoFailoverManager manager(config, nullptr, nullptr, spare_manager, nullptr);

    std::string activated_spare_id;
    EXPECT_FALSE(manager.testActivateSpareIfNeeded("node-down", activated_spare_id));
    EXPECT_TRUE(activated_spare_id.empty());
    const auto spare_info = spare_manager->getSpareInfo("spare-1");
    ASSERT_TRUE(spare_info.has_value());
    EXPECT_EQ(spare_info->state, themis::sharding::SpareState::AVAILABLE);
}

#endif  // THEMIS_TEST_BUILD
