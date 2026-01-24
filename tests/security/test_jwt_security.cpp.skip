#include <gtest/gtest.h>
#include "auth/jwt_validator.h"
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

using namespace themis::auth;

/**
 * @brief Security tests for JWT validation
 * 
 * These tests validate that ThemisDB is protected against common
 * JWT vulnerabilities including:
 * - Algorithm confusion attacks (HS256/RS256)
 * - None algorithm bypass
 * - Token forgery
 * - Expired token acceptance
 * - Signature verification bypass
 */
class JWTSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with secure configuration
        config_.secret = "test_secret_key_minimum_256_bits_for_hs256_algorithm";
        config_.algorithm = "HS256";
        config_.issuer = "ThemisDB";
        config_.audience = "ThemisDB-API";
        config_.expiration_seconds = 3600;
        
        validator_ = std::make_unique<JWTValidator>(config_);
    }
    
    JWTValidator::Config config_;
    std::unique_ptr<JWTValidator> validator_;
};

/**
 * @brief Test: Algorithm confusion attack (HS256 -> None)
 * 
 * Attack: Attacker changes algorithm from HS256 to "none" to bypass signature verification
 * Expected: Token should be rejected
 */
TEST_F(JWTSecurityTest, AlgorithmConfusion_NoneAlgorithm) {
    // Create a token with "none" algorithm
    auto token = jwt::create()
        .set_issuer(config_.issuer)
        .set_type("JWT")
        .set_algorithm("none")
        .set_payload_claim("user_id", jwt::claim(std::string("test_user")))
        .sign(jwt::algorithm::none{});
    
    // Attempt to validate - should fail
    auto result = validator_->validate(token);
    EXPECT_FALSE(result.valid) << "Token with 'none' algorithm should be rejected";
    EXPECT_FALSE(result.error_message.empty());
}

/**
 * @brief Test: Token forgery without signature
 * 
 * Attack: Attacker creates token with valid claims but invalid/missing signature
 * Expected: Token should be rejected
 */
TEST_F(JWTSecurityTest, TokenForgery_InvalidSignature) {
    // Create a valid token first
    auto valid_token = jwt::create()
        .set_issuer(config_.issuer)
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(std::string("test_user")))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(3600))
        .sign(jwt::algorithm::hs256{config_.secret});
    
    // Tamper with signature (change last character)
    std::string tampered_token = valid_token;
    if (!tampered_token.empty()) {
        tampered_token[tampered_token.length() - 1] = 'X';
    }
    
    // Attempt to validate tampered token - should fail
    auto result = validator_->validate(tampered_token);
    EXPECT_FALSE(result.valid) << "Token with invalid signature should be rejected";
}

/**
 * @brief Test: Expired token acceptance
 * 
 * Attack: Attacker uses old expired token
 * Expected: Token should be rejected
 */
TEST_F(JWTSecurityTest, ExpiredToken_Rejected) {
    // Create token that expired 1 hour ago
    auto expired_time = std::chrono::system_clock::now() - std::chrono::hours(1);
    
    auto token = jwt::create()
        .set_issuer(config_.issuer)
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(std::string("test_user")))
        .set_issued_at(expired_time - std::chrono::hours(1))
        .set_expires_at(expired_time)
        .sign(jwt::algorithm::hs256{config_.secret});
    
    // Attempt to validate expired token - should fail
    auto result = validator_->validate(token);
    EXPECT_FALSE(result.valid) << "Expired token should be rejected";
    EXPECT_TRUE(result.error_message.find("expired") != std::string::npos ||
                result.error_message.find("Expired") != std::string::npos);
}

/**
 * @brief Test: Invalid issuer claim
 * 
 * Attack: Attacker creates token with wrong issuer
 * Expected: Token should be rejected
 */
TEST_F(JWTSecurityTest, InvalidIssuer_Rejected) {
    // Create token with wrong issuer
    auto token = jwt::create()
        .set_issuer("EvilIssuer")
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(std::string("test_user")))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(3600))
        .sign(jwt::algorithm::hs256{config_.secret});
    
    // Attempt to validate token with wrong issuer - should fail
    auto result = validator_->validate(token);
    EXPECT_FALSE(result.valid) << "Token with invalid issuer should be rejected";
}

/**
 * @brief Test: Malformed token structure
 * 
 * Attack: Attacker sends malformed JWT
 * Expected: Token should be rejected gracefully without crashes
 */
TEST_F(JWTSecurityTest, MalformedToken_Rejected) {
    std::vector<std::string> malformed_tokens = {
        "",                           // Empty token
        "not.a.token",               // Invalid format
        "header.payload",            // Missing signature
        "....",                      // Only dots
        "header.payload.signature.extra",  // Too many parts
        "invalid_base64!@#$.invalid.invalid"  // Invalid base64
    };
    
    for (const auto& token : malformed_tokens) {
        auto result = validator_->validate(token);
        EXPECT_FALSE(result.valid) 
            << "Malformed token should be rejected: " << token;
        EXPECT_FALSE(result.error_message.empty());
    }
}

/**
 * @brief Test: Valid token acceptance
 * 
 * Positive test: Ensure valid tokens are accepted
 */
TEST_F(JWTSecurityTest, ValidToken_Accepted) {
    // Create properly signed token
    auto token = jwt::create()
        .set_issuer(config_.issuer)
        .set_audience(std::set<std::string>{config_.audience})
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(std::string("test_user")))
        .set_payload_claim("role", jwt::claim(std::string("admin")))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(3600))
        .sign(jwt::algorithm::hs256{config_.secret});
    
    // Validate token - should succeed
    auto result = validator_->validate(token);
    EXPECT_TRUE(result.valid) << "Valid token should be accepted: " << result.error_message;
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_EQ(result.user_id, "test_user");
}
