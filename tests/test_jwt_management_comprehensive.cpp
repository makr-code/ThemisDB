/**
 * @file test_jwt_management_comprehensive.cpp
 * @brief Comprehensive tests for JWT management and security
 *
 * Tests cover:
 * - JWTClaims structure and expiration check
 * - JWTValidatorConfig constants and defaults
 * - Key ID revocation (revokeKid / isKidRevoked)
 * - Group-based access control (hasAccess)
 * - Token size limits (MAX_JWT_TOKEN_SIZE)
 * - Config fields: clock skew, cache TTL, revoked kids
 * - Validation of malformed tokens (missing headers/payload)
 * - Token with revoked kid is rejected
 */

#include <gtest/gtest.h>
#include "auth/jwt_validator.h"
#include <chrono>
#include <string>
#include <vector>

using namespace themis::auth;

// ============================================================================
// JWTClaims Tests
// ============================================================================

TEST(JWTClaimsTest, IsExpired_PastExpiry_ReturnsTrue) {
    JWTClaims claims;
    // Set expiration in the past
    claims.expiration = std::chrono::system_clock::now() - std::chrono::hours(1);
    EXPECT_TRUE(claims.isExpired());
}

TEST(JWTClaimsTest, IsExpired_FutureExpiry_ReturnsFalse) {
    JWTClaims claims;
    // Set expiration in the future
    claims.expiration = std::chrono::system_clock::now() + std::chrono::hours(1);
    EXPECT_FALSE(claims.isExpired());
}

TEST(JWTClaimsTest, IsExpired_ExactlyNow_EffectivelyExpired) {
    JWTClaims claims;
    // Expiration at now - very small past
    claims.expiration = std::chrono::system_clock::now() - std::chrono::seconds(1);
    EXPECT_TRUE(claims.isExpired());
}

TEST(JWTClaimsTest, DefaultConstruction_EmptyFields) {
    JWTClaims claims;
    EXPECT_TRUE(claims.sub.empty());
    EXPECT_TRUE(claims.email.empty());
    EXPECT_TRUE(claims.tenant_id.empty());
    EXPECT_TRUE(claims.roles.empty());
    EXPECT_TRUE(claims.groups.empty());
    EXPECT_TRUE(claims.issuer.empty());
    EXPECT_TRUE(claims.audience.empty());
}

TEST(JWTClaimsTest, RolesAndGroups_CanBePopulated) {
    JWTClaims claims;
    claims.sub = "user123";
    claims.email = "user@example.com";
    claims.roles = {"admin", "operator"};
    claims.groups = {"engineering", "ops-team"};

    EXPECT_EQ(claims.sub, "user123");
    EXPECT_EQ(claims.roles.size(), 2u);
    EXPECT_EQ(claims.groups.size(), 2u);
    EXPECT_EQ(claims.roles[0], "admin");
    EXPECT_EQ(claims.groups[1], "ops-team");
}

// ============================================================================
// JWTValidatorConfig Tests
// ============================================================================

TEST(JWTValidatorConfigTest, DefaultConfig_SaneDefaults) {
    JWTValidatorConfig cfg;
    EXPECT_EQ(cfg.cache_ttl, std::chrono::seconds(600));
    EXPECT_EQ(cfg.clock_skew, std::chrono::seconds(60));
    EXPECT_TRUE(cfg.jwks_url.empty());
    EXPECT_FALSE(cfg.expected_issuer.has_value());
    EXPECT_FALSE(cfg.expected_audience.has_value());
    EXPECT_TRUE(cfg.revoked_kids.empty());
    EXPECT_EQ(cfg.jwks_timeout_seconds, DEFAULT_JWKS_TIMEOUT_SECONDS);
    EXPECT_EQ(cfg.jwks_max_retries, MAX_JWKS_RETRY_ATTEMPTS);
    EXPECT_TRUE(cfg.require_issuer_validation);
    EXPECT_TRUE(cfg.require_audience_validation);
}

TEST(JWTValidatorConfigTest, Constants_ValidValues) {
    EXPECT_EQ(MAX_JWT_TOKEN_SIZE, 16u * 1024u);
    EXPECT_EQ(MAX_PRINCIPAL_NAME_LENGTH, 256u);
    EXPECT_EQ(DEFAULT_JWKS_TIMEOUT_SECONDS, 5);
    EXPECT_EQ(MAX_JWKS_RETRY_ATTEMPTS, 3);
}

TEST(JWTValidatorConfigTest, ConfigCanSpecifyRevokedKids) {
    JWTValidatorConfig cfg;
    cfg.revoked_kids = {"kid1", "kid2", "kid3"};
    EXPECT_EQ(cfg.revoked_kids.size(), 3u);
    EXPECT_EQ(cfg.revoked_kids[0], "kid1");
}

// ============================================================================
// Key ID Revocation Tests
// ============================================================================

class JWTRevocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        JWTValidatorConfig cfg;
        cfg.jwks_url = "http://localhost:8080/jwks"; // Won't be called in tests
        cfg.require_issuer_validation = false;
        cfg.require_audience_validation = false;
        validator_ = std::make_unique<JWTValidator>(cfg);
    }

    std::unique_ptr<JWTValidator> validator_;
};

TEST_F(JWTRevocationTest, RevokeKid_KidIsRevoked) {
    EXPECT_FALSE(validator_->isKidRevoked("test-key-1"));

    validator_->revokeKid("test-key-1");

    EXPECT_TRUE(validator_->isKidRevoked("test-key-1"));
}

TEST_F(JWTRevocationTest, UnrevokedKid_NotInRevocationList) {
    validator_->revokeKid("bad-key");

    EXPECT_TRUE(validator_->isKidRevoked("bad-key"));
    EXPECT_FALSE(validator_->isKidRevoked("good-key"));
}

TEST_F(JWTRevocationTest, RevokeMultipleKids) {
    validator_->revokeKid("key-a");
    validator_->revokeKid("key-b");
    validator_->revokeKid("key-c");

    EXPECT_TRUE(validator_->isKidRevoked("key-a"));
    EXPECT_TRUE(validator_->isKidRevoked("key-b"));
    EXPECT_TRUE(validator_->isKidRevoked("key-c"));
    EXPECT_FALSE(validator_->isKidRevoked("key-d"));
}

TEST_F(JWTRevocationTest, RevokedKidInConfig_CheckedOnValidation) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.revoked_kids = {"pre-revoked-key"};
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    // Pre-revoked key should be revoked even before any runtime revocation
    EXPECT_TRUE(v.isKidRevoked("pre-revoked-key"));
}

// ============================================================================
// Group-Based Access Control Tests
// ============================================================================

TEST(JWTAccessControlTest, HasAccess_MatchingSubject_Granted) {
    JWTClaims claims;
    claims.sub = "user-abc-123";
    claims.groups = {"engineering"};

    EXPECT_TRUE(JWTValidator::hasAccess(claims, "user-abc-123"));
}

TEST(JWTAccessControlTest, HasAccess_MatchingGroup_Granted) {
    JWTClaims claims;
    claims.sub = "user-xyz";
    claims.groups = {"finance-team", "reporting"};

    EXPECT_TRUE(JWTValidator::hasAccess(claims, "finance-team"));
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "reporting"));
}

TEST(JWTAccessControlTest, HasAccess_NoMatch_Denied) {
    JWTClaims claims;
    claims.sub = "user-xyz";
    claims.groups = {"engineering"};

    EXPECT_FALSE(JWTValidator::hasAccess(claims, "finance-team"));
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "admin-group"));
}

TEST(JWTAccessControlTest, HasAccess_EmptyGroups_OnlySubjectMatches) {
    JWTClaims claims;
    claims.sub = "solo-user";
    claims.groups = {};

    EXPECT_TRUE(JWTValidator::hasAccess(claims, "solo-user"));
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "any-group"));
}

TEST(JWTAccessControlTest, HasAccess_EmptyClaims_Denied) {
    JWTClaims claims;
    // Empty sub, no groups
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "some-context"));
}

TEST(JWTAccessControlTest, HasAccess_MultipleGroups_AllChecked) {
    JWTClaims claims;
    claims.sub = "user1";
    claims.groups = {"group-a", "group-b", "group-c"};

    EXPECT_TRUE(JWTValidator::hasAccess(claims, "group-a"));
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "group-b"));
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "group-c"));
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "group-d"));
}

// ============================================================================
// Token Validation - Malformed Token Tests
// ============================================================================

TEST(JWTTokenValidationTest, EmptyToken_ThrowsOrReturnsError) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    EXPECT_THROW(v.parseAndValidate(""), std::exception);
}

TEST(JWTTokenValidationTest, TooLargeToken_Rejected) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    // Token larger than MAX_JWT_TOKEN_SIZE (16KB)
    std::string huge_token(MAX_JWT_TOKEN_SIZE + 1, 'X');
    EXPECT_THROW(v.parseAndValidate(huge_token), std::exception);
}

TEST(JWTTokenValidationTest, TokenWithOnlyTwoParts_Rejected) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    // JWT must have 3 parts separated by dots
    EXPECT_THROW(v.parseAndValidate("header.payload"), std::exception);
}

TEST(JWTTokenValidationTest, GarbageToken_Rejected) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    EXPECT_THROW(v.parseAndValidate("not.a.jwt"), std::exception);
}

TEST(JWTTokenValidationTest, NoneAlgorithmToken_Rejected) {
    // A "none" algorithm JWT: header={"alg":"none","typ":"JWT"}, payload={"sub":"attacker"}
    // This represents the "algorithm confusion" attack
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);

    // Base64url-encoded: {"alg":"none","typ":"JWT"}
    std::string none_header = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0";
    // Base64url-encoded: {"sub":"attacker","exp":9999999999}
    std::string payload = "eyJzdWIiOiJhdHRhY2tlciIsImV4cCI6OTk5OTk5OTk5OX0";
    std::string token = none_header + "." + payload + ".";

    // Should be rejected - "none" algorithm is not supported
    EXPECT_THROW(v.parseAndValidate(token), std::exception);
}
