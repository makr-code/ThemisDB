/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mfa_authenticator.cpp                         ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     310                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "auth/mfa_authenticator.h"
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
