#include <gtest/gtest.h>
#include "auth/auth_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::auth;

/**
 * @brief Test basic metrics recording without Prometheus
 */
TEST(AuthMetricsTest, BasicRecording) {
    AuthMetrics metrics;
    
    // Initial state
    EXPECT_EQ(metrics.getTotalAttempts(), 0);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 0);
    EXPECT_EQ(metrics.getFailedAuths(), 0);
    EXPECT_EQ(metrics.getSuccessRate(), 0.0);
    
    // Record some attempts
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.5);
    metrics.recordAuthSuccess(AuthMethod::JWT, 15.2);
    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 8.3);
    
    // Check counters
    EXPECT_EQ(metrics.getTotalAttempts(), 3);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 2);
    EXPECT_EQ(metrics.getFailedAuths(), 1);
    EXPECT_NEAR(metrics.getSuccessRate(), 2.0/3.0, 0.01);
}

/**
 * @brief Test auth success recording
 */
TEST(AuthMetricsTest, RecordAuthSuccess) {
    AuthMetrics metrics;
    
    metrics.recordAuthSuccess(AuthMethod::JWT, 12.5);
    
    EXPECT_EQ(metrics.getTotalAttempts(), 1);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 1);
    EXPECT_EQ(metrics.getFailedAuths(), 0);
    EXPECT_EQ(metrics.getSuccessRate(), 1.0);
}

/**
 * @brief Test auth failure recording
 */
TEST(AuthMetricsTest, RecordAuthFailure) {
    AuthMetrics metrics;
    
    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 5.0);
    
    EXPECT_EQ(metrics.getTotalAttempts(), 1);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 0);
    EXPECT_EQ(metrics.getFailedAuths(), 1);
    EXPECT_EQ(metrics.getSuccessRate(), 0.0);
}

/**
 * @brief Test multiple auth methods
 */
TEST(AuthMetricsTest, MultipleAuthMethods) {
    AuthMetrics metrics;
    
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    metrics.recordAuthSuccess(AuthMethod::GSSAPI, 50.0);
    metrics.recordAuthSuccess(AuthMethod::MFA, 5.0);
    
    EXPECT_EQ(metrics.getTotalAttempts(), 3);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 3);
}

/**
 * @brief Test JWKS cache metrics
 */
TEST(AuthMetricsTest, JWKSCacheMetrics) {
    AuthMetrics metrics;
    
    // These should not crash even without Prometheus
    EXPECT_NO_THROW(metrics.recordJWKSCacheHit());
    EXPECT_NO_THROW(metrics.recordJWKSCacheMiss());
    EXPECT_NO_THROW(metrics.recordJWKSCacheHit());
    
    EXPECT_NO_THROW(metrics.setJWKSCacheSize(5));
}

/**
 * @brief Test JWKS fetch metrics
 */
TEST(AuthMetricsTest, JWKSFetchMetrics) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordJWKSFetch(125.5, true));
    EXPECT_NO_THROW(metrics.recordJWKSFetch(250.0, false));
}

/**
 * @brief Test rate limiting metrics
 */
TEST(AuthMetricsTest, RateLimitingMetrics) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordRateLimitExceeded("ip"));
    EXPECT_NO_THROW(metrics.recordRateLimitExceeded("user"));
    EXPECT_NO_THROW(metrics.setRateLimitTokens("192.168.1.100", 5.0));
}

/**
 * @brief Test account lockout metrics
 */
TEST(AuthMetricsTest, AccountLockoutMetrics) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordAccountLockout("alice"));
    EXPECT_NO_THROW(metrics.recordAccountUnlock("alice"));
    EXPECT_NO_THROW(metrics.setLockedAccountCount(3));
}

/**
 * @brief Test error recording
 */
TEST(AuthMetricsTest, ErrorRecording) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordError(9302));
    EXPECT_NO_THROW(metrics.recordError(9311));
    EXPECT_NO_THROW(metrics.recordErrorByCategory("jwt"));
    EXPECT_NO_THROW(metrics.recordErrorByCategory("gssapi"));
}

/**
 * @brief Test token validation metrics
 */
TEST(AuthMetricsTest, TokenValidationMetrics) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordTokenValidation(AuthMethod::JWT, 8.5));
    EXPECT_NO_THROW(metrics.recordTokenValidation(AuthMethod::GSSAPI, 45.2));
}

/**
 * @brief Test revoked token checks
 */
TEST(AuthMetricsTest, RevokedTokenChecks) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordRevokedTokenCheck(true));
    EXPECT_NO_THROW(metrics.recordRevokedTokenCheck(false));
}

/**
 * @brief Test success rate calculation
 */
TEST(AuthMetricsTest, SuccessRateCalculation) {
    AuthMetrics metrics;
    
    // Empty metrics
    EXPECT_EQ(metrics.getSuccessRate(), 0.0);
    
    // All successes
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    EXPECT_EQ(metrics.getSuccessRate(), 1.0);
    
    // Add some failures
    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 5.0);
    EXPECT_NEAR(metrics.getSuccessRate(), 2.0/3.0, 0.01);
    
    // Add more failures
    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 5.0);
    EXPECT_EQ(metrics.getSuccessRate(), 0.5);
}

/**
 * @brief Test thread safety of counters
 */
TEST(AuthMetricsTest, ThreadSafety) {
    AuthMetrics metrics;
    
    const int num_threads = 10;
    const int iterations = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&metrics, iterations]() {
            for (int i = 0; i < iterations; i++) {
                if (i % 2 == 0) {
                    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
                } else {
                    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 5.0);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Each thread does 'iterations' attempts
    EXPECT_EQ(metrics.getTotalAttempts(), num_threads * iterations);
    
    // Half successes, half failures
    EXPECT_EQ(metrics.getSuccessfulAuths(), num_threads * iterations / 2);
    EXPECT_EQ(metrics.getFailedAuths(), num_threads * iterations / 2);
}

/**
 * @brief Test AuthDurationTimer RAII helper
 */
TEST(AuthMetricsTest, DurationTimer) {
    AuthMetrics metrics;
    
    {
        AuthDurationTimer timer(metrics, AuthMethod::JWT);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer.recordSuccess();
    }
    
    EXPECT_EQ(metrics.getSuccessfulAuths(), 1);
    
    {
        AuthDurationTimer timer(metrics, AuthMethod::JWT);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer.recordFailure(9302);
    }
    
    EXPECT_EQ(metrics.getFailedAuths(), 1);
    EXPECT_EQ(metrics.getTotalAttempts(), 2);
}

/**
 * @brief Test AuthDurationTimer automatic recording on destruction
 */
TEST(AuthMetricsTest, DurationTimerAutoRecord) {
    AuthMetrics metrics;
    
    {
        AuthDurationTimer timer(metrics, AuthMethod::JWT);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        // Destructor should auto-record as attempt
    }
    
    // Should be recorded as an attempt (but neither success nor failure explicitly)
    EXPECT_EQ(metrics.getTotalAttempts(), 1);
}

/**
 * @brief Test AuthDurationTimer prevents double recording
 */
TEST(AuthMetricsTest, DurationTimerNoDoubleRecord) {
    AuthMetrics metrics;
    
    {
        AuthDurationTimer timer(metrics, AuthMethod::JWT);
        timer.recordSuccess();
        timer.recordSuccess();  // Second call should be ignored
        // Destructor should not record again
    }
    
    EXPECT_EQ(metrics.getTotalAttempts(), 1);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 1);
}

/**
 * @brief Test metrics with custom config
 */
TEST(AuthMetricsTest, CustomConfig) {
    AuthMetrics::Config config;
    config.namespace_prefix = "custom_auth";
    config.enable_histograms = false;
    config.enable_detailed_metrics = false;
    
    AuthMetrics metrics(config);
    
    // Should still work
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    EXPECT_EQ(metrics.getTotalAttempts(), 1);
}

/**
 * @brief Test different auth methods are tracked separately
 */
TEST(AuthMetricsTest, AuthMethodSeparation) {
    AuthMetrics metrics;
    
    metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    metrics.recordAuthSuccess(AuthMethod::GSSAPI, 50.0);
    metrics.recordAuthSuccess(AuthMethod::MFA, 5.0);
    metrics.recordAuthFailure(AuthMethod::JWT, 9302, 8.0);
    
    // Total counters should aggregate all methods
    EXPECT_EQ(metrics.getTotalAttempts(), 4);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 3);
    EXPECT_EQ(metrics.getFailedAuths(), 1);
}

/**
 * @brief Test zero duration is handled correctly
 */
TEST(AuthMetricsTest, ZeroDuration) {
    AuthMetrics metrics;
    
    EXPECT_NO_THROW(metrics.recordAuthSuccess(AuthMethod::JWT, 0.0));
    EXPECT_NO_THROW(metrics.recordAuthFailure(AuthMethod::JWT, 9302, 0.0));
}

/**
 * @brief Test large values don't overflow
 */
TEST(AuthMetricsTest, LargeValues) {
    AuthMetrics metrics;
    
    for (int i = 0; i < 10000; i++) {
        metrics.recordAuthSuccess(AuthMethod::JWT, 10.0);
    }
    
    EXPECT_EQ(metrics.getTotalAttempts(), 10000);
    EXPECT_EQ(metrics.getSuccessfulAuths(), 10000);
}

/**
 * @brief Test getDuration in timer
 */
TEST(AuthMetricsTest, TimerGetDuration) {
    AuthMetrics metrics;
    AuthDurationTimer timer(metrics, AuthMethod::JWT);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    double duration = timer.getDuration();
    EXPECT_GT(duration, 9.0);  // At least 9ms (some slack for timing)
    EXPECT_LT(duration, 50.0); // But not too long
}
