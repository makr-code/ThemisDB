// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_wave_a_fencing.cpp
 * @brief Wave A gate FO-Promote-04: fencing verification before promotion.
 *
 * Validates gap FO-IMPL-003: the auto-failover promotion path now verifies
 * epoch fencing before promotion regardless of the enable_split_brain_prevention
 * flag, including when no fencing manager is configured.
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
 *                                      → processFailover blocks promotion and
 *                                        reports a quorum/safety failure.
 *
 * Tests use no network I/O; the no-fence case uses a temporary WAL directory.
 * Test seed constant: kFencingTestSeed = 42.
 *
 * @see include/failover/auto_failover_manager.h
 * @see src/failover/auto_failover_manager.cpp — FO-IMPL-003 changes
 * @see include/sharding/epoch_fencing.h
 */

#ifdef THEMIS_TEST_BUILD

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"
#include "failover/failover_api_contract.h"
#include "sharding/epoch_fencing.h"

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
// FO-Promote-04-PREVENTION-DISABLED-NO-FENCE
// Disabling optional split-brain detection cannot bypass fencing before a
// write-side promotion.
// ===========================================================================

TEST(WaveAFencing, FO_Promote_04_PREVENTION_DISABLED_NO_FENCE) {
    AutoFailoverConfig cfg = makeFencingTestConfig();
    cfg.enable_split_brain_prevention = false;  // prevention explicitly disabled
    cfg.quorum_timeout_ms             = 50ms;

    themisdb::replication::ReplicationConfig replication_config;
    replication_config.wal_directory =
        (std::filesystem::temp_directory_path() /
         ("themisdb_failover_fencing_test_" +
          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
            .string();
    std::filesystem::remove_all(replication_config.wal_directory);
    auto replication_mgr =
        std::make_shared<themisdb::replication::ReplicationManager>(replication_config);
    ASSERT_TRUE(replication_mgr->initialize());
    themisdb::replication::ReplicaInfo replica{};
    replica.node_id = "replica-1";
    replica.endpoint = "127.0.0.1:9000";
    replica.role = themisdb::replication::ReplicationRole::FOLLOWER;
    replica.is_voting_member = true;
    replica.health_status = themisdb::replication::HealthStatus::HEALTHY;
    replica.last_heartbeat = std::chrono::system_clock::now();
    replication_mgr->addReplica(replica);

    // Quorum exists, but fencing_manager is intentionally absent.
    AutoFailoverManager mgr(cfg, replication_mgr, nullptr, nullptr, nullptr);

    std::atomic<int> split_brain_cancel_events{0};
    std::atomic<int> quorum_failed_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::FAILOVER_CANCELLED) {
                split_brain_cancel_events.fetch_add(1, std::memory_order_relaxed);
            }
            if (type == FailoverEventType::QUORUM_CHECK_FAILED) {
                quorum_failed_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // Drive processFailover directly; the missing fencing manager must block
    // promotion even though the optional detection flag is disabled.
    const auto result = mgr.testProcessFailover("node-down");

    EXPECT_FALSE(result.success)
        << "Expected fail-closed behavior when promotion lacks an epoch fence";
    EXPECT_EQ(result.error_message, "epoch fencing is unavailable or invalid");

    EXPECT_GE(quorum_failed_events.load(), 1)
        << "Expected the missing-fencing diagnostic to be surfaced";
    EXPECT_EQ(split_brain_cancel_events.load(), 0)
        << "Missing fencing is a quorum/safety failure, not a split-brain detection";

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.split_brain_preventions, 0u)
        << "No successful fencing operation may be recorded without a fencing manager";

    replication_mgr->shutdown();
    std::error_code ec;
    std::filesystem::remove_all(replication_config.wal_directory, ec);
}

#endif  // THEMIS_TEST_BUILD
