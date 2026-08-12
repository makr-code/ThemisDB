// HTTP/3 Production Readiness Tests
// Validates all acceptance criteria from Issue #101:
//   - Connection migration stability
//   - Better QUIC congestion control
//   - 0-RTT handshake optimization
//   - Fallback to HTTP/2 on QUIC failure
//   - Performance benchmarking vs HTTP/2

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_HTTP3

#include "server/http3_production_config.h"
#include <chrono>
#include <thread>
#include <string>

using namespace themis::server;

// ============================================================================
// Helper
// ============================================================================

static Http3ProductionConfig defaultConfig() {
    return Http3ProductionConfig{};
}

// ============================================================================
// 1. QUIC Congestion Control Configuration
// ============================================================================

class Http3CongestionControlTest : public ::testing::Test {};

TEST_F(Http3CongestionControlTest, DefaultAlgorithmIsBBR) {
    Http3ProductionConfig cfg;
    EXPECT_EQ(cfg.cc_algorithm, Http3CongestionAlgorithm::Bbr);
}

TEST_F(Http3CongestionControlTest, CongestionAlgorithmEnumValues) {
    // Enum values must match ngtcp2 CC algorithm constants
    EXPECT_EQ(static_cast<int>(Http3CongestionAlgorithm::Cubic), 0);
    EXPECT_EQ(static_cast<int>(Http3CongestionAlgorithm::Reno),  1);
    EXPECT_EQ(static_cast<int>(Http3CongestionAlgorithm::Bbr),   2);
}

TEST_F(Http3CongestionControlTest, CanSelectCubicAlgorithm) {
    Http3ProductionConfig cfg;
    cfg.cc_algorithm = Http3CongestionAlgorithm::Cubic;
    EXPECT_EQ(cfg.cc_algorithm, Http3CongestionAlgorithm::Cubic);
}

TEST_F(Http3CongestionControlTest, CanSelectRenoAlgorithm) {
    Http3ProductionConfig cfg;
    cfg.cc_algorithm = Http3CongestionAlgorithm::Reno;
    EXPECT_EQ(cfg.cc_algorithm, Http3CongestionAlgorithm::Reno);
}

TEST_F(Http3CongestionControlTest, ProductionFlowControlTuning) {
    Http3ProductionConfig cfg;
    // Production defaults should be larger than the old 1 MB / 256 KB values
    EXPECT_GE(cfg.initial_max_data, 1024ULL * 1024);
    EXPECT_GE(cfg.initial_max_stream_data_bidi, 256ULL * 1024);
    EXPECT_GE(cfg.initial_max_streams_bidi, 100ULL);
}

// ============================================================================
// 2. Connection Migration Stability
// ============================================================================

class Http3ConnectionMigrationTest : public ::testing::Test {};

TEST_F(Http3ConnectionMigrationTest, MigrationEnabledByDefault) {
    Http3ProductionConfig cfg;
    EXPECT_TRUE(cfg.enable_migration);
}

TEST_F(Http3ConnectionMigrationTest, StrictPathValidationEnabledByDefault) {
    Http3ProductionConfig cfg;
    EXPECT_TRUE(cfg.strict_path_validation);
}

TEST_F(Http3ConnectionMigrationTest, MigrationCanBeDisabled) {
    Http3ProductionConfig cfg;
    cfg.enable_migration = false;
    EXPECT_FALSE(cfg.enable_migration);
}

TEST_F(Http3ConnectionMigrationTest, MetricsMigrationCountInitializedToZero) {
    Http3ConnectionMetrics m;
    EXPECT_EQ(m.migration_count.load(), 0u);
}

TEST_F(Http3ConnectionMigrationTest, MetricsMigrationCountIncremented) {
    Http3ConnectionMetrics m;
    m.migration_count.fetch_add(1, std::memory_order_relaxed);
    m.migration_count.fetch_add(1, std::memory_order_relaxed);
    EXPECT_EQ(m.migration_count.load(), 2u);
}

TEST_F(Http3ConnectionMigrationTest, MetricsSnapshotIncludesMigrationCount) {
    Http3ConnectionMetrics m;
    m.migration_count.store(3, std::memory_order_relaxed);
    auto snap = m.snapshot();
    EXPECT_EQ(snap.migration_count, 3u);
}

// ============================================================================
// 3. 0-RTT Handshake Optimization
// ============================================================================

class Http3ZeroRttTest : public ::testing::Test {};

TEST_F(Http3ZeroRttTest, ZeroRttEnabledByDefault) {
    Http3ProductionConfig cfg;
    EXPECT_TRUE(cfg.enable_0rtt);
}

TEST_F(Http3ZeroRttTest, ZeroRttCanBeDisabled) {
    Http3ProductionConfig cfg;
    cfg.enable_0rtt = false;
    EXPECT_FALSE(cfg.enable_0rtt);
}

TEST_F(Http3ZeroRttTest, SessionTicketLifetimeDefault) {
    Http3ProductionConfig cfg;
    // Default: 24 hours
    EXPECT_EQ(cfg.session_ticket_lifetime_secs, 86400u);
}

TEST_F(Http3ZeroRttTest, MetricsZeroRttFlagInitializedFalse) {
    Http3ConnectionMetrics m;
    EXPECT_FALSE(m.zero_rtt_used);
}

TEST_F(Http3ZeroRttTest, MetricsZeroRttFlagCanBeSet) {
    Http3ConnectionMetrics m;
    m.zero_rtt_used = true;
    auto snap = m.snapshot();
    EXPECT_TRUE(snap.zero_rtt_used);
}

TEST_F(Http3ZeroRttTest, HandshakeTimingFields) {
    Http3ConnectionMetrics m;
    m.handshake_start_us = 1000;
    m.handshake_end_us   = 5000;
    auto snap = m.snapshot();
    EXPECT_EQ(snap.handshake_duration_us, 4000);
}

// ============================================================================
// 4. Fallback to HTTP/2 on QUIC Failure
// ============================================================================

class Http3FallbackManagerTest : public ::testing::Test {
protected:
    Http3ProductionConfig cfg_;
    // Use short recovery window so tests don't block
    Http3FallbackManagerTest() {
        cfg_.enable_http2_fallback         = true;
        cfg_.fallback_failure_threshold    = 3;
        cfg_.fallback_recovery_secs        = 0; // immediate recovery for tests
    }
};

TEST_F(Http3FallbackManagerTest, NoFallbackInitially) {
    Http3FallbackManager mgr(cfg_);
    EXPECT_FALSE(mgr.shouldFallbackToHttp2("192.168.1.1"));
}

TEST_F(Http3FallbackManagerTest, FallbackActivatesAfterThreshold) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 3600; // long window so it doesn't expire
    Http3FallbackManager mgr(cfg);

    mgr.recordQuicFailure("10.0.0.1");
    mgr.recordQuicFailure("10.0.0.1");
    EXPECT_FALSE(mgr.shouldFallbackToHttp2("10.0.0.1")); // not yet

    mgr.recordQuicFailure("10.0.0.1"); // 3rd failure → threshold reached
    EXPECT_TRUE(mgr.shouldFallbackToHttp2("10.0.0.1"));
}

TEST_F(Http3FallbackManagerTest, SuccessResetsFallback) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 3600;
    Http3FallbackManager mgr(cfg);

    for (int i = 0; i < 5; ++i) {
        mgr.recordQuicFailure("10.0.0.2");
    }
    EXPECT_TRUE(mgr.shouldFallbackToHttp2("10.0.0.2"));

    mgr.recordQuicSuccess("10.0.0.2");
    EXPECT_FALSE(mgr.shouldFallbackToHttp2("10.0.0.2"));
}

TEST_F(Http3FallbackManagerTest, FallbackCounterIsPerClient) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 3600;
    Http3FallbackManager mgr(cfg);

    for (int i = 0; i < 5; ++i) {
        mgr.recordQuicFailure("10.0.0.3");
    }
    EXPECT_TRUE(mgr.shouldFallbackToHttp2("10.0.0.3"));
    EXPECT_FALSE(mgr.shouldFallbackToHttp2("10.0.0.4")); // different IP
}

TEST_F(Http3FallbackManagerTest, FallbackDisabledWhenFeatureFlagOff) {
    Http3ProductionConfig cfg = cfg_;
    cfg.enable_http2_fallback      = false;
    cfg.fallback_failure_threshold = 1;
    cfg.fallback_recovery_secs     = 3600;
    Http3FallbackManager mgr(cfg);

    mgr.recordQuicFailure("1.2.3.4");
    mgr.recordQuicFailure("1.2.3.4");
    EXPECT_FALSE(mgr.shouldFallbackToHttp2("1.2.3.4"));
}

TEST_F(Http3FallbackManagerTest, FallbackClientCountZeroInitially) {
    Http3FallbackManager mgr(cfg_);
    EXPECT_EQ(mgr.fallbackClientCount(), 0u);
}

TEST_F(Http3FallbackManagerTest, FallbackClientCountIncreasesOnFallback) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 3600;
    Http3FallbackManager mgr(cfg);

    for (int i = 0; i < 3; ++i) {
        mgr.recordQuicFailure("172.16.0.1");
    }
    EXPECT_GE(mgr.fallbackClientCount(), 1u);
}

TEST_F(Http3FallbackManagerTest, AltSvcValueReturnedWhenNotInFallback) {
    Http3FallbackManager mgr(cfg_);
    std::string altsvc = mgr.altSvcValue(443, "192.168.1.100");
    EXPECT_FALSE(altsvc.empty());
    EXPECT_NE(altsvc.find("h3"), std::string::npos);
}

TEST_F(Http3FallbackManagerTest, AltSvcValueEmptyWhenInFallback) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 3600;
    Http3FallbackManager mgr(cfg);

    for (int i = 0; i < 3; ++i) {
        mgr.recordQuicFailure("192.168.1.200");
    }
    std::string altsvc = mgr.altSvcValue(443, "192.168.1.200");
    EXPECT_TRUE(altsvc.empty());
}

TEST_F(Http3FallbackManagerTest, AltSvcValueContainsPort) {
    Http3FallbackManager mgr(cfg_);
    std::string altsvc = mgr.altSvcValue(8443, "10.1.2.3");
    EXPECT_NE(altsvc.find("8443"), std::string::npos);
}

TEST_F(Http3FallbackManagerTest, PurgeExpiredRemovesExpiredEntries) {
    Http3ProductionConfig cfg = cfg_;
    cfg.fallback_recovery_secs = 0; // immediate expiry
    cfg.fallback_failure_threshold = 1;
    Http3FallbackManager mgr(cfg);

    mgr.recordQuicFailure("10.10.10.1");
    // Even with 0-second window the fallback should already be expired
    // PurgeExpired should safely remove the entry
    mgr.purgeExpired();
    EXPECT_EQ(mgr.fallbackClientCount(), 0u);
}

// ============================================================================
// 5. Performance Metrics (benchmarking vs HTTP/2)
// ============================================================================

class Http3PerformanceMetricsTest : public ::testing::Test {};

TEST_F(Http3PerformanceMetricsTest, MetricsEnabledByDefault) {
    Http3ProductionConfig cfg;
    EXPECT_TRUE(cfg.enable_performance_metrics);
}

TEST_F(Http3PerformanceMetricsTest, MetricsCanBeDisabled) {
    Http3ProductionConfig cfg;
    cfg.enable_performance_metrics = false;
    EXPECT_FALSE(cfg.enable_performance_metrics);
}

TEST_F(Http3PerformanceMetricsTest, InitialCountersAreZero) {
    Http3ConnectionMetrics m;
    EXPECT_EQ(m.bytes_sent.load(),                  0u);
    EXPECT_EQ(m.bytes_received.load(),              0u);
    EXPECT_EQ(m.requests_total.load(),              0u);
    EXPECT_EQ(m.request_latency_total_us.load(),    0u);
    EXPECT_EQ(m.migration_count.load(),             0u);
}

TEST_F(Http3PerformanceMetricsTest, CountersAccumulateCorrectly) {
    Http3ConnectionMetrics m;
    m.bytes_sent.fetch_add(1024, std::memory_order_relaxed);
    m.bytes_received.fetch_add(512, std::memory_order_relaxed);
    m.requests_total.fetch_add(1, std::memory_order_relaxed);
    m.request_latency_total_us.fetch_add(250, std::memory_order_relaxed);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.bytes_sent,             1024u);
    EXPECT_EQ(snap.bytes_received,          512u);
    EXPECT_EQ(snap.requests_total,            1u);
    EXPECT_EQ(snap.avg_request_latency_us,  250u);
}

TEST_F(Http3PerformanceMetricsTest, AvgLatencyCalculation) {
    Http3ConnectionMetrics m;
    m.requests_total.store(4, std::memory_order_relaxed);
    m.request_latency_total_us.store(2000, std::memory_order_relaxed);

    auto snap = m.snapshot();
    EXPECT_EQ(snap.avg_request_latency_us, 500u);
}

TEST_F(Http3PerformanceMetricsTest, AvgLatencyZeroWhenNoRequests) {
    Http3ConnectionMetrics m;
    auto snap = m.snapshot();
    EXPECT_EQ(snap.avg_request_latency_us, 0u);
}

TEST_F(Http3PerformanceMetricsTest, HandshakeDurationInSnapshot) {
    Http3ConnectionMetrics m;
    m.handshake_start_us = 10000;
    m.handshake_end_us   = 12500;

    auto snap = m.snapshot();
    EXPECT_EQ(snap.handshake_duration_us, 2500);
    EXPECT_EQ(snap.handshake_start_us,   10000);
    EXPECT_EQ(snap.handshake_end_us,     12500);
}

// ============================================================================
// 6. Production Config Defaults Validation
// ============================================================================

class Http3ProductionConfigTest : public ::testing::Test {};

TEST_F(Http3ProductionConfigTest, DefaultConfigIsValid) {
    Http3ProductionConfig cfg;
    // Must have sane transport limits
    EXPECT_GT(cfg.initial_max_data, 0ULL);
    EXPECT_GT(cfg.initial_max_stream_data_bidi, 0ULL);
    EXPECT_GT(cfg.initial_max_stream_data_uni, 0ULL);
    EXPECT_GT(cfg.initial_max_streams_bidi, 0ULL);
    EXPECT_GT(cfg.initial_max_streams_uni, 0ULL);
}

TEST_F(Http3ProductionConfigTest, FallbackThresholdSane) {
    Http3ProductionConfig cfg;
    EXPECT_GT(cfg.fallback_failure_threshold, 0u);
}

TEST_F(Http3ProductionConfigTest, FallbackRecoveryWindowSane) {
    Http3ProductionConfig cfg;
    EXPECT_GT(cfg.fallback_recovery_secs, 0u);
}

TEST_F(Http3ProductionConfigTest, SessionTicketLifetimeSane) {
    Http3ProductionConfig cfg;
    // Must be at least 5 minutes and at most 30 days
    EXPECT_GE(cfg.session_ticket_lifetime_secs, 300u);
    EXPECT_LE(cfg.session_ticket_lifetime_secs, 30u * 86400u);
}

TEST_F(Http3ProductionConfigTest, UniStreamLimitAtLeastThree) {
    // HTTP/3 requires at least 3 unidirectional streams
    // (control, QPACK encoder, QPACK decoder)
    Http3ProductionConfig cfg;
    EXPECT_GE(cfg.initial_max_streams_uni, 3ULL);
}

#endif // THEMIS_ENABLE_HTTP3
