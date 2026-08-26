// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_wave_next_analytics_hardening.cpp
 * @brief Targeted regression tests for Wave-A AN1 and AN2 hardening gaps.
 *
 * ## Test families
 *
 * ### AN1 — Federated query coordinator: shard-level retry
 *   AN1-01  Single shard retried on transient failure; succeeds on second call
 *   AN1-02  Shard counted as failed only after max_retries exhausted
 *   AN1-03  Positive backoff delay between retry attempts (mock timer)
 *   AN1-04  Permanent failure (invalid query) does NOT trigger a retry
 *
 * ### AN2 — Forecasting model integrity check
 *   AN2-01  Model with correct checksum deserializes successfully
 *   AN2-02  Model with corrupted checksum throws / returns error
 *   AN2-03  Model without checksum passes with WARN (no hard fail)
 *   AN2-04  Checksum is stored at save time and is verifiable on reload
 *
 * @see include/analytics/distributed_analytics.h — Config::RetryConfig (AN1)
 * @see src/analytics/distributed_analytics.cpp  — executeDistributed retry loop
 * @see src/analytics/forecasting.cpp            — serialize/deserialize AN2 paths
 * @see src/analytics/ROADMAP.md                 — Wave-A AN1/AN2 closure 2026-08-26
 */

#include <gtest/gtest.h>

#include "analytics/distributed_analytics.h"
#include "analytics/forecasting.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace themisdb {
namespace analytics {

// ===========================================================================
// AN1 — Mock infrastructure
// ===========================================================================

class CountingShardExecutor : public ShardQueryExecutor {
public:
    /// Total number of execute() invocations.
    std::atomic<int> call_count{0};

    /// After this many failures the executor succeeds.
    int fail_for_n_calls = 0;

    /// When non-empty the error message is thrown (used to inject "permanent" errors).
    std::string error_message;

    themis::analytics::OLAPResult execute(
        const std::string& /*shard_id*/,
        const themis::analytics::OLAPQuery& /*query*/) override
    {
        const int n = ++call_count;
        if (!error_message.empty()) {
            throw std::runtime_error(error_message);
        }
        if (n <= fail_for_n_calls) {
            throw std::runtime_error("transient network error");
        }
        themis::analytics::OLAPResult ok;
        ok.rows.push_back({});
        return ok;
    }

    bool isHealthy() const override { return true; }
};

// ---------------------------------------------------------------------------
// Fixture: coordinator with retry enabled and zero timeout (fast tests)
// ---------------------------------------------------------------------------
class AN1RetryTest : public ::testing::Test {
protected:
    void SetUp() override {
        DistributedAnalyticsSharding::Config cfg;
        cfg.enable_circuit_breaker      = false; // isolate retry from CB
        cfg.allow_partial_results       = true;
        cfg.shard_timeout_ms            = 0;      // no timeout
        cfg.shard_execution_timeout_ms  = 0;
        cfg.health_check_interval       = std::chrono::milliseconds{0};
        // Wave-A AN1 retry config
        cfg.retry_config.max_retries    = 2;
        cfg.retry_config.base_delay_ms  = 0;  // zero delay so tests run fast
        cfg.retry_config.max_delay_ms   = 0;
        coordinator = std::make_unique<DistributedAnalyticsSharding>(cfg);
    }

    std::unique_ptr<DistributedAnalyticsSharding> coordinator;
};

// ---------------------------------------------------------------------------
// AN1-01: Single shard retried on transient failure; succeeds on second call
// ---------------------------------------------------------------------------
TEST_F(AN1RetryTest, AN1_01_RetryOnTransientFailure) {
    auto exec = std::make_shared<CountingShardExecutor>();
    exec->fail_for_n_calls = 1; // fail once, succeed on attempt #2

    coordinator->addShard("shard-A", exec);

    themis::analytics::OLAPQuery q;
    q.collection = "test";

    auto result = coordinator->executeDistributed(q);

    // The shard must have been called exactly 2 times (1 failure + 1 success).
    EXPECT_EQ(exec->call_count.load(), 2)
        << "Executor should be called once for the initial failure and once for the retry";
    // The result should reflect a successful shard.
    EXPECT_EQ(result.successful_shards, 1u);
}

// ---------------------------------------------------------------------------
// AN1-02: Shard counted as failed only after max_retries exhausted
// ---------------------------------------------------------------------------
TEST_F(AN1RetryTest, AN1_02_FailedAfterMaxRetriesExhausted) {
    auto exec = std::make_shared<CountingShardExecutor>();
    exec->fail_for_n_calls = 999; // always fail

    coordinator->addShard("shard-B", exec);

    themis::analytics::OLAPQuery q;
    q.collection = "test";

    auto result = coordinator->executeDistributed(q);

    // max_retries=2 → 3 total attempts (initial + 2 retries).
    EXPECT_EQ(exec->call_count.load(), 3)
        << "Executor should be called 1 + max_retries = 3 times total";
    // The shard should be counted as failed.
    EXPECT_EQ(result.successful_shards, 0u);
    ASSERT_EQ(result.shard_info.size(), 1u);
    EXPECT_FALSE(result.shard_info[0].success);
}

// ---------------------------------------------------------------------------
// AN1-03: Backoff delay between retries is non-negative (sanity check via
//          observable timing with a non-zero base_delay_ms).
// ---------------------------------------------------------------------------
TEST(AN1BackoffTest, AN1_03_BackoffDelayIsPositive) {
    // Use a non-zero base delay so we can measure real elapsed time.
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker      = false;
    cfg.allow_partial_results       = true;
    cfg.shard_timeout_ms            = 0;
    cfg.shard_execution_timeout_ms  = 0;
    cfg.health_check_interval       = std::chrono::milliseconds{0};
    cfg.retry_config.max_retries    = 1;
    cfg.retry_config.base_delay_ms  = 20;
    cfg.retry_config.max_delay_ms   = 200;

    DistributedAnalyticsSharding coordinator(cfg);

    auto exec = std::make_shared<CountingShardExecutor>();
    exec->fail_for_n_calls = 1; // one transient failure → one retry with delay

    coordinator.addShard("shard-C", exec);

    themis::analytics::OLAPQuery q;
    q.collection = "test";

    const auto t0 = std::chrono::steady_clock::now();
    coordinator.executeDistributed(q);
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // With base_delay_ms=20 and ±20% jitter the minimum possible delay is 16 ms.
    // We assert ≥ 10 ms as a conservative lower bound to avoid flakiness on
    // heavily loaded CI runners.
    EXPECT_GE(elapsed_ms, 10)
        << "Expected non-trivial elapsed time due to retry backoff delay";
    EXPECT_EQ(exec->call_count.load(), 2);
}

// ---------------------------------------------------------------------------
// AN1-04: Permanent failure (invalid query) does NOT trigger a retry
// ---------------------------------------------------------------------------
TEST_F(AN1RetryTest, AN1_04_PermanentFailureNoRetry) {
    auto exec = std::make_shared<CountingShardExecutor>();
    exec->error_message = "invalid query syntax"; // triggers is_permanent path

    coordinator->addShard("shard-D", exec);

    themis::analytics::OLAPQuery q;
    q.collection = "test";

    auto result = coordinator->executeDistributed(q);

    // Permanent failure: executor must be called exactly once (no retry).
    EXPECT_EQ(exec->call_count.load(), 1)
        << "Permanent failure must not trigger any retry attempts";
    EXPECT_EQ(result.successful_shards, 0u);
    ASSERT_EQ(result.shard_info.size(), 1u);
    EXPECT_FALSE(result.shard_info[0].success);
}

// ===========================================================================
// AN2 — Forecasting model integrity check
// ===========================================================================

namespace {

/// Build a minimal fitted ForecastModel, serialize it, and return the string.
std::string makeSerializedModel() {
    using namespace themisdb::analytics;
    ForecastModel m(ForecastMethod::LINEAR);
    TimeSeries ts;
    ts.timestamps = {0, 1, 2, 3, 4};
    ts.values     = {1.0, 2.0, 3.0, 4.0, 5.0};
    m.fit(ts);
    return m.serialize();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// AN2-01: Model with correct checksum deserializes successfully
// ---------------------------------------------------------------------------
TEST(AN2IntegrityTest, AN2_01_CorrectChecksumPassesVerification) {
    const std::string serialized = makeSerializedModel();

    // The serialized string must contain a "checksum=" line.
    ASSERT_NE(serialized.find("checksum="), std::string::npos)
        << "serialize() must embed a checksum line";

    // Deserialize must succeed without throwing.
    EXPECT_NO_THROW({
        ForecastModel loaded = ForecastModel::deserialize(serialized);
        EXPECT_TRUE(loaded.isFitted());
    });
}

// ---------------------------------------------------------------------------
// AN2-02: Model with corrupted checksum returns an error
// ---------------------------------------------------------------------------
TEST(AN2IntegrityTest, AN2_02_CorruptedChecksumThrows) {
    std::string serialized = makeSerializedModel();

    // Corrupt the checksum value by flipping one hex digit.
    const std::string marker = "checksum=";
    const auto pos = serialized.rfind(marker);
    ASSERT_NE(pos, std::string::npos);

    // Flip the first character of the 8-digit hex value.
    const size_t hex_pos = pos + marker.size();
    ASSERT_LT(hex_pos, serialized.size());
    char& c = serialized[hex_pos];
    c = (c == 'A') ? 'B' : 'A'; // guaranteed to change the value

    EXPECT_THROW(
        { ForecastModel::deserialize(serialized); },
        std::runtime_error)
        << "Corrupted checksum must throw std::runtime_error";
}

// ---------------------------------------------------------------------------
// AN2-03: Model without stored checksum passes with WARN (no hard fail)
// ---------------------------------------------------------------------------
TEST(AN2IntegrityTest, AN2_03_MissingChecksumPassesWithWarn) {
    std::string serialized = makeSerializedModel();

    // Strip the checksum line (last non-empty line).
    const std::string marker = "\nchecksum=";
    const auto pos = serialized.rfind(marker);
    ASSERT_NE(pos, std::string::npos) << "Expected a checksum line to strip";
    // Remove from '\nchecksum=...' to the end of string.
    serialized = serialized.substr(0, pos + 1); // keep the trailing '\n'

    // Must succeed without throwing (just logs WARN).
    EXPECT_NO_THROW({
        ForecastModel loaded = ForecastModel::deserialize(serialized);
        EXPECT_TRUE(loaded.isFitted());
    }) << "Legacy model without checksum must not hard-fail on load";
}

// ---------------------------------------------------------------------------
// AN2-04: Checksum is stored at save time and is verifiable on reload
// ---------------------------------------------------------------------------
TEST(AN2IntegrityTest, AN2_04_ChecksumRoundTrip) {
    ForecastModel m(ForecastMethod::EXP_SMOOTHING);
    TimeSeries ts;
    ts.timestamps = {0, 1, 2, 3, 4, 5, 6, 7};
    ts.values     = {10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0};
    m.fit(ts);

    // Serialize → includes checksum.
    const std::string s1 = m.serialize();
    ASSERT_NE(s1.find("checksum="), std::string::npos);

    // Deserialize the serialized form → must succeed.
    ForecastModel m2 = ForecastModel::deserialize(s1);
    EXPECT_TRUE(m2.isFitted());

    // Re-serialize the round-tripped model: the new checksum must be valid.
    const std::string s2 = m2.serialize();
    EXPECT_NO_THROW({ ForecastModel::deserialize(s2); })
        << "Second-generation serialization must also pass integrity check";
}

} // namespace analytics
} // namespace themisdb
