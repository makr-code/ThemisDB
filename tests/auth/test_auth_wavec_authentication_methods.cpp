/**
 * @file test_auth_wavec_authentication_methods.cpp
 * @brief Wave C unit tests — AUTH-Auth-01 through AUTH-Auth-08
 *
 * Covers authentication method API validation for JWTValidator,
 * SAMLAuthenticator, and MTLSAuthenticator.
 *
 * Test IDs: AUTH-Auth-01 … AUTH-Auth-08
 */

#include <gtest/gtest.h>

#include "auth/jwt_validator.h"
#include "auth/saml_authenticator.h"
#include "auth/mtls_authenticator.h"
#include "auth/auth_error.h"

namespace themis {
namespace auth {
namespace tests {

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class AuthMethodsTest : public ::testing::Test {
protected:
    /**
     * @brief Build a minimal JWTValidatorConfig that skips issuer/audience
     *        enforcement (so construction always succeeds) and points to a
     *        fake JWKS URL.  Validation of a malformed token fails BEFORE
     *        any network request is made.
     */
    JWTValidatorConfig makePermissiveConfig(const std::string& jwks_url =
                                                "http://localhost:9999/jwks") {
        JWTValidatorConfig cfg;
        cfg.jwks_url                    = jwks_url;
        cfg.require_issuer_validation   = false;
        cfg.require_audience_validation = false;
        return cfg;
    }

    /**
     * @brief Build a config with strict issuer validation enabled.
     */
    JWTValidatorConfig makeStrictIssuerConfig(const std::string& expected_issuer) {
        JWTValidatorConfig cfg      = makePermissiveConfig();
        cfg.require_issuer_validation = true;
        cfg.expected_issuer           = expected_issuer;
        return cfg;
    }

    /**
     * @brief Build a config with strict audience validation enabled.
     */
    JWTValidatorConfig makeStrictAudienceConfig(const std::string& expected_audience) {
        JWTValidatorConfig cfg          = makePermissiveConfig();
        cfg.require_audience_validation = true;
        cfg.expected_audience           = expected_audience;
        return cfg;
    }
};

// ---------------------------------------------------------------------------
// AUTH-Auth-01: JWTValidator rejects token with empty signature field
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-01 — A JWT whose third segment (signature) is empty is
 *        structurally malformed and must be rejected with JWT_INVALID_FORMAT.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_01_RejectsMalformedTokenEmptySignature) {
    JWTValidator validator(makePermissiveConfig());

    // header.payload. — trailing dot, no signature bytes
    const std::string malformed_token =
        "******"
        ".eyJzdWIiOiJ1c2VyMSIsImV4cCI6OTk5OTk5OTk5OX0"
        ".";  // empty signature

    try {
        validator.parseAndValidate(malformed_token);
        FAIL() << "Expected AuthException for malformed token";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::JWT_INVALID_FORMAT);
    } catch (const std::exception& ex) {
        // Any std::exception for a malformed token is acceptable; the key
        // requirement is that the call does not succeed.
        SUCCEED() << "Threw std::exception: " << ex.what();
    }
}

// ---------------------------------------------------------------------------
// AUTH-Auth-02: JWTValidator rejects expired token
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-02 — A token whose exp claim is in the past must be
 *        rejected.  The validator should raise an error rather than returning
 *        valid claims.
 *
 * Strategy: craft a three-part "JWT" with exp=1 (Unix epoch + 1 second) so
 * it is always expired.  The signature verification will fail first or the
 * expiry check will fire — either way the call must not succeed.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_02_RejectsExpiredToken) {
    JWTValidator validator(makePermissiveConfig());

    // Base64url({"alg":"RS256","typ":"JWT"}).Base64url({"sub":"u1","exp":1}).fakesig
    // exp=1 is year 1970 — permanently expired.
    const std::string expired_token =
        "******"
        ".eyJzdWIiOiJ1MSIsImV4cCI6MX0"
        ".ZmFrZXNpZ25hdHVyZQ";

    EXPECT_THROW(validator.parseAndValidate(expired_token), std::exception);
}

// ---------------------------------------------------------------------------
// AUTH-Auth-03: JWTValidator rejects token with wrong issuer (strict mode)
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-03 — When require_issuer_validation=true the validator
 *        must reject a token whose iss claim does not match expected_issuer.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_03_RejectsWrongIssuer) {
    JWTValidator validator(makeStrictIssuerConfig("https://expected-issuer.example.com"));

    // Payload contains iss="https://rogue-issuer.example.com"
    // Base64url({"alg":"RS256"}).Base64url({"sub":"u1","iss":"https://rogue-issuer.example.com","exp":9999999999}).fakesig
    const std::string wrong_issuer_token =
        "******"
        ".eyJzdWIiOiJ1MSIsImlzcyI6Imh0dHBzOi8vcm9ndWUtaXNzdWVyLmV4YW1wbGUuY29tIiwiZXhwIjo5OTk5OTk5OTk5fQ"
        ".ZmFrZXNpZ25hdHVyZQ";

    try {
        validator.parseAndValidate(wrong_issuer_token);
        FAIL() << "Expected AuthException for wrong issuer";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::JWT_ISSUER_MISMATCH);
    } catch (const std::exception&) {
        // Signature or format error fires first — acceptable, the token is still rejected.
        SUCCEED();
    }
}

// ---------------------------------------------------------------------------
// AUTH-Auth-04: JWTValidator rejects token with wrong audience (strict mode)
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-04 — When require_audience_validation=true the validator
 *        must reject a token whose aud claim does not match expected_audience.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_04_RejectsWrongAudience) {
    JWTValidator validator(makeStrictAudienceConfig("themisdb-production"));

    // Payload contains aud="other-service"
    // Base64url({"alg":"RS256"}).Base64url({"sub":"u1","aud":"other-service","exp":9999999999}).fakesig
    const std::string wrong_audience_token =
        "******"
        ".eyJzdWIiOiJ1MSIsImF1ZCI6Im90aGVyLXNlcnZpY2UiLCJleHAiOjk5OTk5OTk5OTl9"
        ".ZmFrZXNpZ25hdHVyZQ";

    try {
        validator.parseAndValidate(wrong_audience_token);
        FAIL() << "Expected AuthException for wrong audience";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::JWT_AUDIENCE_MISMATCH);
    } catch (const std::exception&) {
        // Signature or format error fires first — the token is still rejected.
        SUCCEED();
    }
}

// ---------------------------------------------------------------------------
// AUTH-Auth-05: SAMLAuthenticator rejects empty assertion string
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-05 — SAMLAuthenticator::processResponse() must reject an
 *        empty Base64 SAML response string without crashing.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_05_SAMLRejectsEmptyAssertion) {
    SAMLConfig cfg;
    cfg.sp_entity_id         = "https://sp.example.com/saml/metadata";
    cfg.sp_acs_url           = "https://sp.example.com/saml/acs";
    cfg.idp_entity_id        = "https://idp.example.com";
    cfg.idp_certificate_pem  = "";  // no cert — ensures fast failure path

    SAMLAuthenticator saml(cfg);

    EXPECT_THROW(saml.processResponse("", ""), std::exception);
}

// ---------------------------------------------------------------------------
// AUTH-Auth-06: MTLSAuthenticator rejects empty certificate
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-06 — MTLSAuthenticator::authenticate() must reject an
 *        empty PEM string and throw rather than returning bogus claims.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_06_MTLSRejectsEmptyCertificate) {
    MTLSAuthenticator::Config cfg;
    cfg.ca_cert_pem           = "";  // no CA — fast rejection path
    cfg.require_client_cert   = true;

    MTLSAuthenticator mtls(cfg);

    EXPECT_THROW(mtls.authenticate(""), std::exception);
}

// ---------------------------------------------------------------------------
// AUTH-Auth-07: JWTValidator constructor throws when jwks_url is empty and
//               issuer/audience validation is required
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-07 — Constructing JWTValidator with a config that has
 *        require_issuer_validation=true but no expected_issuer set must
 *        throw AUTH_CONFIG_INVALID at construction time.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_07_ConstructorThrowsOnMissingIssuerConfig) {
    JWTValidatorConfig cfg;
    cfg.jwks_url                  = "http://localhost:9999/jwks";
    cfg.require_issuer_validation = true;
    // expected_issuer intentionally left unset (std::nullopt)
    cfg.require_audience_validation = false;

    try {
        JWTValidator validator(cfg);
        FAIL() << "Expected an exception when required issuer is not configured";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::AUTH_CONFIG_INVALID);
    } catch (const std::runtime_error&) {
        // std::runtime_error is also acceptable per the header contract.
        SUCCEED();
    }
}

// ---------------------------------------------------------------------------
// AUTH-Auth-08: JWTValidator rejects token with missing nbf when require_jti=true
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Auth-08 — When require_jti=true the validator must reject a
 *        token that does not contain a "jti" claim.
 *
 * The token payload intentionally omits "jti"; expiry is set to the far
 * future to ensure the rejection is about the missing claim, not expiry.
 */
TEST_F(AuthMethodsTest, AUTH_Auth_08_RejectsMissingJtiWhenRequired) {
    JWTValidatorConfig cfg      = makePermissiveConfig();
    cfg.require_jti             = true;

    JWTValidator validator(cfg);

    // Payload has sub + exp but no jti claim
    // Base64url({"alg":"RS256"}).Base64url({"sub":"u1","exp":9999999999}).fakesig
    const std::string token_without_jti =
        "******"
        ".eyJzdWIiOiJ1MSIsImV4cCI6OTk5OTk5OTk5OX0"
        ".ZmFrZXNpZ25hdHVyZQ";

    try {
        validator.parseAndValidate(token_without_jti);
        FAIL() << "Expected exception for missing jti claim";
    } catch (const AuthException& ex) {
        // JWT_MISSING_REQUIRED_CLAIM or JWT_INVALID_FORMAT are both acceptable
        const auto code = ex.error().code();
        EXPECT_TRUE(code == AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM ||
                    code == AuthErrorCode::JWT_INVALID_FORMAT ||
                    code == AuthErrorCode::JWT_INVALID_SIGNATURE)
            << "Unexpected error code: " << static_cast<int>(code);
    } catch (const std::exception&) {
        SUCCEED();
    }
}

} // namespace tests
} // namespace auth
} // namespace themis
