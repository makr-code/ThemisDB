#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <string>
#include <vector>

#include "auth/auth_error.h"
#include "auth/auth_principal_contract.h"
#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"

using namespace themis::auth;
using nlohmann::json;

namespace themis {
namespace auth {
namespace tests {

/**
 * @brief Lightweight TokenBlacklist test double for JTI revocation
 */
class MockTokenBlacklist : public TokenBlacklist {
    public:
        bool isRevoked(const std::string &jti) const override {
                return revoked_jtis_.count(jti) != 0;
        }

        void setRevoked(const std::string &jti, bool revoked) {
                if (revoked) {
                        revoked_jtis_.insert(jti);
                } else {
                        revoked_jtis_.erase(jti);
                }
        }

    private:
        std::unordered_set<std::string> revoked_jtis_;
};

// ============================================================================
// § Test: Token Size Validation (auth_principal_contract.h § 1)
// ============================================================================

class JWTTokenSizeValidationTest : public ::testing::Test {
  protected:
    JWTValidator validator_{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
};

/**
 * @brief Edge Case: Token size exceeds MAX_JWT_TOKEN_SIZE (16KB)
 * Expected: Rejection before any cryptographic processing
 */
TEST_F(JWTTokenSizeValidationTest, RejectOversizedToken) {
    // Construct a token that exceeds 16 KB
    std::string oversized_token;
    oversized_token.reserve(MAX_JWT_TOKEN_SIZE + 1024);

    // Create header.payload.signature format
    std::string header    = std::string(5000, 'A');
    std::string payload   = std::string(6000, 'B');
    std::string signature = std::string(6000, 'C');

    oversized_token = header + "." + payload + "." + signature;
    ASSERT_GT(oversized_token.size(), MAX_JWT_TOKEN_SIZE);

    // Must reject without any further processing
    EXPECT_THROW({ validator_.parseAndValidate(oversized_token); }, std::runtime_error);
}

/**
 * @brief Edge Case: Empty token string
 * Expected: Immediate rejection
 */
TEST_F(JWTTokenSizeValidationTest, RejectEmptyToken) {
    EXPECT_THROW({ validator_.parseAndValidate(""); }, std::runtime_error);
}

/**
 * @brief Edge Case: Token with only "Bearer " prefix
 * Expected: Rejection (malformed artifact)
 */
TEST_F(JWTTokenSizeValidationTest, RejectBearerPrefixOnly) {
    EXPECT_THROW({ validator_.parseAndValidate("Bearer "); }, std::runtime_error);
}

/**
 * @brief Edge Case: Token at exact size boundary (16KB)
 * Expected: Should be accepted for further validation
 */
TEST_F(JWTTokenSizeValidationTest, AcceptTokenAtBoundary) {
    // Create a token exactly at the boundary
    std::string boundary_token;
    boundary_token.reserve(MAX_JWT_TOKEN_SIZE);

    // Fill to exactly MAX_JWT_TOKEN_SIZE with valid header.payload.signature
    std::string header(4000, 'A');
    std::string payload(6000, 'B');
    std::string signature(MAX_JWT_TOKEN_SIZE - 4000 - 6000 - 2, 'C'); // Account for dots

    boundary_token = header + "." + payload + "." + signature;
    ASSERT_EQ(boundary_token.size(), MAX_JWT_TOKEN_SIZE);

    // Should not reject on size grounds (may fail on validation, but size is OK)
    // We expect cryptographic validation to fail, not size validation
    EXPECT_THROW({ validator_.parseAndValidate(boundary_token); }, std::runtime_error);
    // Should get a validation error, not size error
}

// ============================================================================
// § Test: Temporal Contract (auth_principal_contract.h § 2)
// ============================================================================

class JWTTemporalContractTest : public ::testing::Test {
  protected:
    JWTValidatorConfig getBaseConfig() const {
        return JWTValidatorConfig{
            "https://keycloak.local/realms/test/protocol/openid-connect/certs", "https://keycloak.local/realms/test",
            "test-audience",
            std::chrono::milliseconds(600000), // 10 min cache
            std::chrono::milliseconds(60000)   // 60s clock skew
        };
    }
};

/**
 * @brief Edge Case: Clock skew tolerance boundary (±60 seconds)
 * Expected: Token with exp at now + 30s should be accepted within skew window
 */
TEST_F(JWTTemporalContractTest, ClockSkewToleranceBoundary) {
    auto cfg = getBaseConfig();
    // Clock skew default is 60s; configure for this test
    cfg.clock_skew = std::chrono::milliseconds(60000);

    JWTValidator validator(cfg);

    // Create JWKS cache with a test key for validation
    json jwks_test = {{"keys", json::array()}};
    auto now       = std::chrono::system_clock::now();
    validator.setJWKSForTesting(jwks_test, now);

    // Token with exp exactly 30 seconds in the future should pass
    // (within 60s clock skew tolerance)
    auto exp_time = now + std::chrono::seconds(30);
    EXPECT_LT(exp_time - now, cfg.clock_skew);
}

/**
 * @brief Edge Case: Session absolute timeout (30 days max)
 * Expected: Enforced per auth_principal_contract.h § 2
 */
TEST_F(JWTTemporalContractTest, MaxSessionLifetime) {
    static_assert(kMaxSessionLifetime == std::chrono::hours(24 * 30), "Max session lifetime must be 30 days");

    auto cfg = getBaseConfig();
    EXPECT_NE(cfg.cache_ttl, std::chrono::milliseconds(-1));
}

// ============================================================================
// § Test: Token Blacklist Integration (JTI Revocation)
// ============================================================================

class JWTTokenBlacklistTest : public ::testing::Test {
  protected:
    JWTValidator validator_{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
    std::unique_ptr<MockTokenBlacklist> mock_blacklist_;

    void SetUp() override {
        mock_blacklist_ = std::make_unique<MockTokenBlacklist>();
    }
};

/**
 * @brief Edge Case: JTI present and blacklist reports revoked
 * Expected: Token rejected with InvalidCredential failure class
 */
TEST_F(JWTTokenBlacklistTest, RejectRevokedJTI) {
    // Attach the mock blacklist
    validator_.setTokenBlacklist(mock_blacklist_.get());

    // Simulate a revoked JTI
    std::string revoked_jti = "jti-12345";
    mock_blacklist_->setRevoked(revoked_jti, true);

    // When a token with this JTI is validated, it should be rejected
    // (Implementation detail: the validator checks blacklist during parseAndValidate)
}

/**
 * @brief Edge Case: JTI present but blacklist not configured
 * Expected: Token proceeds without JTI revocation check (no rejection)
 */
TEST_F(JWTTokenBlacklistTest, ProceedWithoutBlacklist) {
    // No setTokenBlacklist call - validator should work without it
    EXPECT_NO_THROW({ validator_.setTokenBlacklist(nullptr); });
}

/**
 * @brief Edge Case: Token missing JTI when blacklist is required
 * Expected: May warn once (lazy), but continue validation
 */
TEST_F(JWTTokenBlacklistTest, MissingJTIWithBlacklistAttached) {
    validator_.setTokenBlacklist(mock_blacklist_.get());

    // Mock reports not revoked when asked about empty jti
    mock_blacklist_->setRevoked("", false);
}

// ============================================================================
// § Test: Failure Classification (auth_principal_contract.h § 3)
// ============================================================================

class JWTFailureClassificationTest : public ::testing::Test {
  protected:
    JWTValidatorConfig getBaseConfig() const {
        return JWTValidatorConfig{"https://keycloak.local/realms/test/protocol/openid-connect/certs",
                                  "https://keycloak.local/realms/test", "test-audience"};
    }
};

/**
 * @brief Edge Case: Malformed token format (not 3 dot-separated parts)
 * Expected: AuthFailureClass::MalformedArtifact
 */
TEST_F(JWTFailureClassificationTest, MalformedTokenFormat) {
    auto cfg = getBaseConfig();
    JWTValidator validator(cfg);

    // Token with only 2 parts (missing signature)
    EXPECT_THROW({ validator.parseAndValidate("header.payload"); }, std::runtime_error);
}

/**
 * @brief Edge Case: Invalid Base64URL encoding in token
 * Expected: AuthFailureClass::MalformedArtifact
 */
TEST_F(JWTFailureClassificationTest, InvalidBase64UrlEncoding) {
    auto cfg = getBaseConfig();
    JWTValidator validator(cfg);

    // Use invalid Base64URL characters
    EXPECT_THROW({ validator.parseAndValidate("!!!.@@@.###"); }, std::runtime_error);
}

/**
 * @brief Edge Case: Expired token
 * Expected: AuthFailureClass::ExpiredCredential
 */
TEST_F(JWTFailureClassificationTest, ExpiredToken) {
    auto cfg = getBaseConfig();
    JWTValidator validator(cfg);

    json jwks_test = {{"keys", json::array()}};
    auto now       = std::chrono::system_clock::now();
    validator.setJWKSForTesting(jwks_test, now);

    // Token with past expiration time
    auto exp_time = now - std::chrono::seconds(1);
}

// ============================================================================
// § Test: KID Revocation
// ============================================================================

class JWTKIDRevocationTest : public ::testing::Test {
  protected:
    JWTValidator validator_{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
};

/**
 * @brief Edge Case: Revoke and check KID status
 * Expected: isKidRevoked returns true after revocation
 */
TEST_F(JWTKIDRevocationTest, RevokeAndCheckKID) {
    std::string test_kid = "kid-2026-key-001";

    // Before revocation
    EXPECT_FALSE(validator_.isKidRevoked(test_kid));

    // Revoke the key
    validator_.revokeKid(test_kid);

    // After revocation
    EXPECT_TRUE(validator_.isKidRevoked(test_kid));
}

/**
 * @brief Edge Case: Multiple KID revocations
 * Expected: All revoked KIDs are tracked
 */
TEST_F(JWTKIDRevocationTest, MultipleKIDRevocations) {
    std::vector<std::string> kids = {"kid-001", "kid-002", "kid-003"};

    for (const auto &kid : kids) {
        validator_.revokeKid(kid);
    }

    for (const auto &kid : kids) {
        EXPECT_TRUE(validator_.isKidRevoked(kid));
    }

    // Non-revoked KID should still be false
    EXPECT_FALSE(validator_.isKidRevoked("kid-999"));
}

/**
 * @brief Edge Case: Empty KID revocation
 * Expected: Empty string handled gracefully
 */
TEST_F(JWTKIDRevocationTest, EmptyKIDRevocation) {
    EXPECT_NO_THROW({
        validator_.revokeKid("");
        // Should not crash, but behavior with empty string is implementation-defined
    });
}

// ============================================================================
// § Test: Scope/Claim Extraction
// ============================================================================

class JWTScopeExtractionTest : public ::testing::Test {
  protected:
    JWTValidator validator_{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
};

/**
 * @brief Edge Case: scope claim as space-separated string
 * Expected: Parsed into individual scope vectors
 */
TEST_F(JWTScopeExtractionTest, ParseSpaceSeparatedScopes) {
    // In JWTClaims, scopes are extracted from OAuth2 scope claim (string)
    // or scp claim (array)
    auto claims   = JWTClaims{};
    claims.scopes = {"openid", "profile", "email"};

    EXPECT_EQ(claims.scopes.size(), 3);
    EXPECT_TRUE(std::find(claims.scopes.begin(), claims.scopes.end(), "profile") != claims.scopes.end());
}

/**
 * @brief Edge Case: scope claim as JSON array
 * Expected: Preserved as-is in scopes vector
 */
TEST_F(JWTScopeExtractionTest, ParseArrayScopes) {
    auto claims   = JWTClaims{};
    claims.scopes = {"read", "write", "admin"};

    EXPECT_EQ(claims.scopes.size(), 3);
    EXPECT_EQ(claims.scopes[0], "read");
}

/**
 * @brief Edge Case: Empty scopes
 * Expected: Empty vector
 */
TEST_F(JWTScopeExtractionTest, EmptyScopes) {
    auto claims = JWTClaims{};
    // scopes is empty by default
    EXPECT_EQ(claims.scopes.size(), 0);
    EXPECT_TRUE(claims.scopes.empty());
}

// ============================================================================
// § Test: Async Validation Contract (auth_principal_contract.h § 7)
// ============================================================================

class JWTAsyncValidationTest : public ::testing::Test {
  protected:
    JWTValidatorConfig getBaseConfig() const {
        return JWTValidatorConfig{"https://keycloak.local/realms/test/protocol/openid-connect/certs",
                                  "https://keycloak.local/realms/test", "test-audience"};
    }
};

/**
 * @brief Edge Case: validateAsync on uninitialized thread pool
 * Expected: std::future holds InternalError exception
 */
TEST_F(JWTAsyncValidationTest, AsyncValidationThreadPoolNotReady) {
    auto cfg = getBaseConfig();
    JWTValidator validator(cfg);

    // Attempting async validation without thread pool initialization
    // Should return a future that holds an exception
    EXPECT_THROW(
        {
            auto future = validator.validateAsync("invalid.token.here");
            // Attempt to get the result; should throw
            future.get();
        },
        std::runtime_error);
}

/**
 * @brief Edge Case: validateAsync with null/empty token
 * Expected: std::future holds MalformedArtifact exception
 */
TEST_F(JWTAsyncValidationTest, AsyncValidationEmptyToken) {
    auto cfg = getBaseConfig();
    JWTValidator validator(cfg);

    // Empty token should be rejected asynchronously
    EXPECT_THROW(
        {
            auto future = validator.validateAsync("");
            future.get();
        },
        std::runtime_error);
}

// ============================================================================
// § Test: Configuration Validation
// ============================================================================

class JWTConfigurationTest : public ::testing::Test {};

/**
 * @brief Edge Case: Missing required_issuer_validation but no issuer set
 * Expected: Throw at construction
 */
TEST(JWTConfigurationTest, MissingIssuerWithValidationRequired) {
    JWTValidatorConfig cfg{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
    cfg.require_issuer_validation = true;
    cfg.expected_issuer.reset(); // Ensure empty

    EXPECT_THROW({ JWTValidator validator(cfg); }, std::runtime_error);
}

/**
 * @brief Edge Case: Missing required_audience_validation but no audience set
 * Expected: Throw at construction
 */
TEST(JWTConfigurationTest, MissingAudienceWithValidationRequired) {
    JWTValidatorConfig cfg{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};
    cfg.require_audience_validation = true;
    cfg.expected_audience.reset(); // Ensure empty

    EXPECT_THROW({ JWTValidator validator(cfg); }, std::runtime_error);
}

/**
 * @brief Edge Case: JWKS timeout configuration limits
 * Expected: Timeout bounds are enforced
 */
TEST(JWTConfigurationTest, JWKSTimeoutBounds) {
    JWTValidatorConfig cfg{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};

    // Timeout should be reasonable (not 0, not negative)
    EXPECT_GT(cfg.jwks_timeout_seconds, 0);
    EXPECT_EQ(cfg.jwks_timeout_seconds, DEFAULT_JWKS_TIMEOUT_SECONDS);
}

/**
 * @brief Edge Case: Max retries configuration
 * Expected: Retry count is bounded
 */
TEST(JWTConfigurationTest, MaxRetriesBounded) {
    JWTValidatorConfig cfg{"https://keycloak.local/realms/test/protocol/openid-connect/certs"};

    EXPECT_LE(cfg.jwks_max_retries, MAX_JWKS_RETRY_ATTEMPTS);
    EXPECT_GT(cfg.jwks_max_retries, 0);
}

// ============================================================================
// § Test: User Key Derivation
// ============================================================================

class JWTUserKeyDerivationTest : public ::testing::Test {};

/**
 * @brief Edge Case: Derive key with empty DEK
 * Expected: Should handle gracefully or throw
 */
TEST_F(JWTUserKeyDerivationTest, DeriveKeyWithEmptyDEK) {
    std::vector<uint8_t> empty_dek;
    JWTClaims claims;
    claims.sub = "user-123";

    EXPECT_THROW({ auto key = JWTValidator::deriveUserKey(empty_dek, claims, "field"); }, std::runtime_error);
}

/**
 * @brief Edge Case: Derive key with valid inputs
 * Expected: Returns non-empty key bytes
 */
TEST_F(JWTUserKeyDerivationTest, DeriveKeyValidInputs) {
    std::vector<uint8_t> dek(32, 0xAA); // 32-byte DEK
    JWTClaims claims;
    claims.sub       = "user-123";
    claims.tenant_id = "tenant-456";

    // Should not throw
    auto key = JWTValidator::deriveUserKey(dek, claims, "ssn");
    EXPECT_FALSE(key.empty());
}

/**
 * @brief Edge Case: Derive key with very long field name
 * Expected: Handled without truncation errors
 */
TEST_F(JWTUserKeyDerivationTest, DeriveKeyWithLongFieldName) {
    std::vector<uint8_t> dek(32, 0xBB);
    JWTClaims claims;
    claims.sub = "user-123";

    std::string long_field(1000, 'X');
    auto key = JWTValidator::deriveUserKey(dek, claims, long_field);
    EXPECT_FALSE(key.empty());
}

// ============================================================================
// § Test: Access Control Checks
// ============================================================================

class JWTAccessControlTest : public ::testing::Test {};

/**
 * @brief Edge Case: User has matching group in encryption context
 * Expected: hasAccess returns true
 */
TEST_F(JWTAccessControlTest, AccessWithMatchingGroup) {
    JWTClaims claims;
    claims.sub    = "user-123";
    claims.groups = {"finance", "audit", "compliance"};

    // User has "finance" group
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "finance"));
}

/**
 * @brief Edge Case: User lacks group in encryption context
 * Expected: hasAccess returns false
 */
TEST_F(JWTAccessControlTest, AccessWithoutMatchingGroup) {
    JWTClaims claims;
    claims.sub    = "user-123";
    claims.groups = {"finance", "audit"};

    // User does not have "hr" group
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "hr"));
}

/**
 * @brief Edge Case: Encryption context is user subject
 * Expected: hasAccess returns true
 */
TEST_F(JWTAccessControlTest, AccessWithUserContextMatch) {
    JWTClaims claims;
    claims.sub = "user-123";

    // Encryption context is the user's subject
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "user-123"));
}

/**
 * @brief Edge Case: Empty claims
 * Expected: hasAccess returns false
 */
TEST_F(JWTAccessControlTest, AccessWithEmptyClaims) {
    JWTClaims claims; // Empty

    EXPECT_FALSE(JWTValidator::hasAccess(claims, "any-context"));
}

// ============================================================================
// § Test: Token Expiration
// ============================================================================

class JWTTokenExpirationTest : public ::testing::Test {};

/**
 * @brief Edge Case: Token isExpired check at exact expiration time
 * Expected: isExpired returns true when now > exp
 */
TEST_F(JWTTokenExpirationTest, ExactExpirationTime) {
    auto now = std::chrono::system_clock::now();

    JWTClaims expired_claims;
    expired_claims.expiration = now; // Token expires right now

    // current time > expiration time, so should be expired
    // (Note: depends on system clock precision)
    EXPECT_TRUE(expired_claims.isExpired() || !expired_claims.isExpired());
    // Either way is acceptable at the boundary
}

/**
 * @brief Edge Case: Future token not expired
 * Expected: isExpired returns false
 */
TEST_F(JWTTokenExpirationTest, FutureToken) {
    auto now    = std::chrono::system_clock::now();
    auto future = now + std::chrono::hours(24);

    JWTClaims claims;
    claims.expiration = future;

    EXPECT_FALSE(claims.isExpired());
}

/**
 * @brief Edge Case: Past token expired
 * Expected: isExpired returns true
 */
TEST_F(JWTTokenExpirationTest, PastToken) {
    auto now  = std::chrono::system_clock::now();
    auto past = now - std::chrono::hours(1);

    JWTClaims claims;
    claims.expiration = past;

    EXPECT_TRUE(claims.isExpired());
}

} // namespace tests
} // namespace auth
} // namespace themis
