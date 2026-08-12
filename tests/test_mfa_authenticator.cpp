#include <gtest/gtest.h>
#include "auth/mfa_authenticator.h"
#include "auth/auth_audit_logger.h"
#include "auth/auth_metrics.h"
#include <chrono>
#include <thread>

using namespace themis::auth;

/**
 * @brief Test MFA enrollment generation
 */
TEST(MFAAuthenticatorTest, GenerateEnrollment) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    
    EXPECT_EQ(enrollment.user_id, "test_user");
    EXPECT_FALSE(enrollment.secret_base32.empty());
    EXPECT_FALSE(enrollment.enabled);
    EXPECT_EQ(enrollment.recovery_codes.size(), 8);
    
    // Check recovery codes are unique
    std::set<std::string> unique_codes(enrollment.recovery_codes.begin(), 
                                      enrollment.recovery_codes.end());
    EXPECT_EQ(unique_codes.size(), enrollment.recovery_codes.size());
}

/**
 * @brief Test provisioning URI generation
 */
TEST(MFAAuthenticatorTest, GenerateProvisioningURI) {
    MFAAuthenticator::Config config;
    config.issuer = "ThemisDB";
    MFAAuthenticator mfa(config);
    
    auto enrollment = mfa.generateEnrollment("alice@example.com");
    std::string uri = mfa.generateProvisioningURI(enrollment);
    
    EXPECT_TRUE(uri.find("otpauth://totp/") == 0);
    EXPECT_TRUE(uri.find("ThemisDB:alice@example.com") != std::string::npos);
    EXPECT_TRUE(uri.find("secret=") != std::string::npos);
    EXPECT_TRUE(uri.find("issuer=ThemisDB") != std::string::npos);
    EXPECT_TRUE(uri.find("digits=6") != std::string::npos);
    EXPECT_TRUE(uri.find("period=30") != std::string::npos);
}

/**
 * @brief Test TOTP validation with current code
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_CurrentCode) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    
    // Generate current TOTP code
    std::string current_code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    // Validate the code
    bool valid = mfa.validateTOTP(enrollment.secret_base32, current_code);
    EXPECT_TRUE(valid) << "Current TOTP code should be valid";
}

/**
 * @brief Test TOTP validation with wrong code
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_WrongCode) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    
    // Use wrong code
    bool valid = mfa.validateTOTP(enrollment.secret_base32, "000000");
    EXPECT_FALSE(valid) << "Wrong code should be invalid";
}

/**
 * @brief Test TOTP validation with wrong length
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_WrongLength) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    
    // Use code with wrong length
    bool valid1 = mfa.validateTOTP(enrollment.secret_base32, "12345");    // Too short
    bool valid2 = mfa.validateTOTP(enrollment.secret_base32, "1234567");  // Too long
    
    EXPECT_FALSE(valid1);
    EXPECT_FALSE(valid2);
}

/**
 * @brief Test TOTP validation with time window
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_TimeWindow) {
    MFAAuthenticator::Config config;
    config.time_step_seconds = 30;
    config.time_window = 1;
    MFAAuthenticator mfa(config);
    
    auto enrollment = mfa.generateEnrollment("test_user");
    auto now = std::chrono::system_clock::now();
    
    // Generate code for previous time step
    auto past = now - std::chrono::seconds(30);
    std::string past_code = mfa.getCurrentTOTP(enrollment.secret_base32, past);
    
    // Should still be valid due to time window
    bool valid = mfa.validateTOTP(enrollment.secret_base32, past_code, now);
    EXPECT_TRUE(valid) << "Code from previous time step should be valid within time window";
}

/**
 * @brief Test TOTP validation outside time window
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_OutsideTimeWindow) {
    MFAAuthenticator::Config config;
    config.time_step_seconds = 30;
    config.time_window = 1;
    MFAAuthenticator mfa(config);
    
    auto enrollment = mfa.generateEnrollment("test_user");
    auto now = std::chrono::system_clock::now();
    
    // Generate code for 2 time steps ago (outside window)
    auto past = now - std::chrono::seconds(60);
    std::string old_code = mfa.getCurrentTOTP(enrollment.secret_base32, past);
    
    // Should be invalid (outside time window)
    bool valid = mfa.validateTOTP(enrollment.secret_base32, old_code, now);
    EXPECT_FALSE(valid) << "Code from 2 time steps ago should be invalid";
}

/**
 * @brief Test recovery code validation
 */
TEST(MFAAuthenticatorTest, ValidateRecoveryCode_Valid) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    std::string recovery_code = enrollment.recovery_codes[0];
    
    // Should validate successfully
    bool valid = mfa.validateRecoveryCode(enrollment, recovery_code);
    EXPECT_TRUE(valid);
    
    // Code should be removed after use
    EXPECT_EQ(enrollment.recovery_codes.size(), 7);
    
    // Same code should not work again
    valid = mfa.validateRecoveryCode(enrollment, recovery_code);
    EXPECT_FALSE(valid);
}

/**
 * @brief Test recovery code validation with invalid code
 */
TEST(MFAAuthenticatorTest, ValidateRecoveryCode_Invalid) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    
    // Try invalid recovery code
    bool valid = mfa.validateRecoveryCode(enrollment, "INVALID");
    EXPECT_FALSE(valid);
    
    // Recovery codes should not be modified
    EXPECT_EQ(enrollment.recovery_codes.size(), 8);
}

/**
 * @brief Test recovery codes generation
 */
TEST(MFAAuthenticatorTest, GenerateRecoveryCodes) {
    MFAAuthenticator::Config config;
    config.recovery_codes_count = 10;
    MFAAuthenticator mfa(config);
    
    auto codes = mfa.generateRecoveryCodes("test_user");
    
    EXPECT_EQ(codes.size(), 10);
    
    // Check all codes are 8 characters
    for (const auto& code : codes) {
        EXPECT_EQ(code.length(), 8);
    }
    
    // Check codes are unique
    std::set<std::string> unique_codes(codes.begin(), codes.end());
    EXPECT_EQ(unique_codes.size(), codes.size());
}

/**
 * @brief Test enrollment JSON serialization
 */
TEST(MFAAuthenticatorTest, EnrollmentSerialization) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    enrollment.enabled = true;
    
    // Serialize to JSON
    nlohmann::json j = enrollment.to_json();
    
    EXPECT_EQ(j["user_id"], "test_user");
    EXPECT_EQ(j["secret_base32"], enrollment.secret_base32);
    EXPECT_EQ(j["enabled"], true);
    EXPECT_EQ(j["recovery_codes"].size(), 8);
    
    // Deserialize from JSON
    auto enrollment2 = MFAAuthenticator::EnrollmentData::from_json(j);
    
    EXPECT_EQ(enrollment2.user_id, enrollment.user_id);
    EXPECT_EQ(enrollment2.secret_base32, enrollment.secret_base32);
    EXPECT_EQ(enrollment2.enabled, enrollment.enabled);
    EXPECT_EQ(enrollment2.recovery_codes.size(), enrollment.recovery_codes.size());
}

/**
 * @brief Test TOTP with 8-digit codes
 */
TEST(MFAAuthenticatorTest, TOTP_8Digits) {
    MFAAuthenticator::Config config;
    config.code_length = 8;
    MFAAuthenticator mfa(config);
    
    auto enrollment = mfa.generateEnrollment("test_user");
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    EXPECT_EQ(code.length(), 8);
    
    // Validate the code
    bool valid = mfa.validateTOTP(enrollment.secret_base32, code);
    EXPECT_TRUE(valid);
}

/**
 * @brief Test TOTP consistency - same secret and time should produce same code
 */
TEST(MFAAuthenticatorTest, TOTP_Consistency) {
    MFAAuthenticator mfa;
    
    auto enrollment = mfa.generateEnrollment("test_user");
    auto timestamp = std::chrono::system_clock::now();
    
    std::string code1 = mfa.getCurrentTOTP(enrollment.secret_base32, timestamp);
    std::string code2 = mfa.getCurrentTOTP(enrollment.secret_base32, timestamp);
    
    EXPECT_EQ(code1, code2) << "Same secret and time should produce same code";
}

/**
 * @brief Test TOTP changes over time
 */
TEST(MFAAuthenticatorTest, TOTP_ChangesOverTime) {
    MFAAuthenticator::Config config;
    config.time_step_seconds = 30;
    MFAAuthenticator mfa(config);
    
    auto enrollment = mfa.generateEnrollment("test_user");
    auto now = std::chrono::system_clock::now();
    
    std::string code_now = mfa.getCurrentTOTP(enrollment.secret_base32, now);
    
    // Code 30 seconds later should be different
    auto later = now + std::chrono::seconds(30);
    std::string code_later = mfa.getCurrentTOTP(enrollment.secret_base32, later);
    
    EXPECT_NE(code_now, code_later) << "TOTP should change after time step";
}

/**
 * @brief Test multiple users have different secrets
 */
TEST(MFAAuthenticatorTest, MultipleUsers_DifferentSecrets) {
    MFAAuthenticator mfa;
    
    auto enrollment1 = mfa.generateEnrollment("user1");
    auto enrollment2 = mfa.generateEnrollment("user2");
    auto enrollment3 = mfa.generateEnrollment("user3");
    
    EXPECT_NE(enrollment1.secret_base32, enrollment2.secret_base32);
    EXPECT_NE(enrollment1.secret_base32, enrollment3.secret_base32);
    EXPECT_NE(enrollment2.secret_base32, enrollment3.secret_base32);
}

/**
 * @brief Test that time_window > 2 throws std::invalid_argument
 */
TEST(MFAAuthenticatorTest, Config_TimeWindowTooLarge_Throws) {
    MFAAuthenticator::Config config;
    config.time_window = 3;
    EXPECT_THROW(MFAAuthenticator mfa(config), std::invalid_argument)
        << "Expected std::invalid_argument for time_window=3";
}

/**
 * @brief Test that time_window == 2 is accepted (boundary value)
 */
TEST(MFAAuthenticatorTest, Config_TimeWindowAtMaxBoundary_Accepted) {
    MFAAuthenticator::Config config;
    config.time_window = 2;
    EXPECT_NO_THROW(MFAAuthenticator mfa(config))
        << "time_window == 2 must be accepted";
}

/**
 * @brief Test that drift metrics are recorded when TOTP validates at offset != 0
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_DriftMetricsRecorded) {
    MFAAuthenticator::Config config;
    config.time_step_seconds = 30;
    config.time_window = 1;
    MFAAuthenticator mfa(config);

    AuthMetrics metrics;
    mfa.setMetrics(&metrics);

    auto enrollment = mfa.generateEnrollment("drift_user");
    auto now = std::chrono::system_clock::now();

    // Generate a code from the previous time step (offset -1)
    auto past = now - std::chrono::seconds(config.time_step_seconds);
    std::string past_code = mfa.getCurrentTOTP(enrollment.secret_base32, past);

    // Validate against "now" — this should match at offset -1
    bool valid = mfa.validateTOTP(enrollment.secret_base32, past_code, now, "drift_user");
    EXPECT_TRUE(valid) << "Code from previous time step should be valid within window";
    EXPECT_EQ(metrics.getTOTPDriftCount(), 1u)
        << "Drift counter must be incremented exactly once for an offset-1 match";
}

/**
 * @brief Test that drift metrics are NOT recorded when TOTP validates at offset 0
 */
TEST(MFAAuthenticatorTest, ValidateTOTP_NoDriftMetricsForCurrentStep) {
    MFAAuthenticator::Config config;
    config.time_step_seconds = 30;
    config.time_window = 1;
    MFAAuthenticator mfa(config);

    AuthMetrics metrics;
    mfa.setMetrics(&metrics);

    auto enrollment = mfa.generateEnrollment("nodrift_user");
    auto now = std::chrono::system_clock::now();

    std::string current_code = mfa.getCurrentTOTP(enrollment.secret_base32, now);
    bool valid = mfa.validateTOTP(enrollment.secret_base32, current_code, now, "nodrift_user");
    EXPECT_TRUE(valid) << "Current code should be valid";
    EXPECT_EQ(metrics.getTOTPDriftCount(), 0u)
        << "Drift counter must not be incremented for a zero-offset match";
}

// ===========================================================================
// Constant-time comparison microbenchmark (opt-in)
// ===========================================================================

/**
 * @brief Verify that recovery-code validation latency does not vary by more
 *        than 100 µs based on the match position within a 10-code list.
 *
 * This test measures wall-clock timing and is therefore sensitive to CPU
 * frequency scaling, scheduler jitter, sanitizer overhead, and CI load.
 * It is opt-in: set the environment variable THEMIS_RUN_PERF_TESTS=1 to
 * enable it.  In production builds on dedicated hardware the implementation
 * targets < 100 ns variance; the 100 µs gate here accommodates sanitizer and
 * CI overhead while still detecting gross regressions.
 *
 * The implementation always iterates all codes (constant-time linear scan), so
 * the median latency for matching code[0] vs code[9] must be within the CI
 * tolerance.
 */
TEST(MFAAuthenticatorTest, RecoveryCode_ConstantTimeByPosition) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping timing microbenchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable)";
    }

    MFAAuthenticator::Config config;
    config.recovery_codes_count = 10;
    MFAAuthenticator mfa(config);

    const int iterations = 200;
    const int64_t tolerance_ns = 100000; // 100 µs – CI-safe gate

    // Collect median latency for first vs last code position.
    std::vector<int64_t> first_ns, last_ns;
    first_ns.reserve(iterations);
    last_ns.reserve(iterations);

    for (int iter = 0; iter < iterations; ++iter) {
        // Fresh enrollment per iteration so codes are not consumed.
        auto enrollment_first = mfa.generateEnrollment("bench_user");
        std::string code_first = enrollment_first.recovery_codes[0];

        auto t0 = std::chrono::steady_clock::now();
        bool ok_first = mfa.validateRecoveryCode(enrollment_first, code_first);
        auto t1 = std::chrono::steady_clock::now();
        ASSERT_TRUE(ok_first);
        first_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        auto enrollment_last = mfa.generateEnrollment("bench_user");
        std::string code_last = enrollment_last.recovery_codes[9];

        auto t2 = std::chrono::steady_clock::now();
        bool ok_last = mfa.validateRecoveryCode(enrollment_last, code_last);
        auto t3 = std::chrono::steady_clock::now();
        ASSERT_TRUE(ok_last);
        last_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count());
    }

    // Compare medians to avoid outlier sensitivity.
    std::sort(first_ns.begin(), first_ns.end());
    std::sort(last_ns.begin(), last_ns.end());
    int64_t median_first = first_ns[iterations / 2];
    int64_t median_last  = last_ns[iterations / 2];
    int64_t diff = std::abs(median_last - median_first);

    EXPECT_LT(diff, tolerance_ns)
        << "Median latency difference between first and last code position ("
        << diff << " ns) exceeds CI tolerance (" << tolerance_ns << " ns). "
        << "Recovery-code lookup must be constant-time regardless of match position.";
}
