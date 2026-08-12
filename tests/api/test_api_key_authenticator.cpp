/**
 * @file test_api_key_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <gtest/gtest.h>
#include "auth/api_key_authenticator.h"
#include "auth/auth_error.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::auth;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ApiKeyCredential makeCredential(
    const std::string& key_id = "sk_live_test123",
    const std::string& secret = "super-secret",
    const std::string& principal = "alice@example.com",
    const std::vector<std::string>& scopes = {"data:read", "data:write"},
    const std::vector<std::string>& roles  = {"user"},
    const std::string& tenant_id = "tenant-1")
{
    return ApiKeyAuthenticator::createCredential(
        key_id, secret, principal, scopes, roles, tenant_id);
}

} // anonymous namespace

// ===========================================================================
// hashSecret
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, HashSecret_ProducesSHA256Hex) {
    const std::string hash = ApiKeyAuthenticator::hashSecret("hello");
    // SHA-256("hello") = 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    EXPECT_EQ(hash.size(), 64u);
    EXPECT_EQ(hash, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(ApiKeyAuthenticatorTest, HashSecret_DifferentSecretsDifferentHashes) {
    const std::string h1 = ApiKeyAuthenticator::hashSecret("secret-a");
    const std::string h2 = ApiKeyAuthenticator::hashSecret("secret-b");
    EXPECT_NE(h1, h2);
}

TEST(ApiKeyAuthenticatorTest, HashSecret_EmptyStringIsValid) {
    // SHA-256 of empty string is deterministic
    const std::string hash = ApiKeyAuthenticator::hashSecret("");
    EXPECT_EQ(hash.size(), 64u);
}

// ===========================================================================
// createCredential
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, CreateCredential_SetsAllFields) {
    const auto cred = ApiKeyAuthenticator::createCredential(
        "sk_live_abc", "raw-secret", "bob@example.com",
        {"read"}, {"admin"}, "acme");

    EXPECT_EQ(cred.key_id,    "sk_live_abc");
    EXPECT_EQ(cred.principal, "bob@example.com");
    EXPECT_EQ(cred.tenant_id, "acme");
    EXPECT_EQ(cred.scopes,    std::vector<std::string>{"read"});
    EXPECT_EQ(cred.roles,     std::vector<std::string>{"admin"});
    EXPECT_TRUE(cred.active);
    EXPECT_EQ(cred.secret_hash.size(), 64u);
    EXPECT_EQ(cred.secret_hash, ApiKeyAuthenticator::hashSecret("raw-secret"));
}

TEST(ApiKeyAuthenticatorTest, CreateCredential_DefaultNoExpiry) {
    const auto cred = ApiKeyAuthenticator::createCredential(
        "k", "s", "p");
    static const std::chrono::system_clock::time_point epoch{};
    EXPECT_EQ(cred.expires_at, epoch);
}

// ===========================================================================
// addCredential validation
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, AddCredential_RejectsEmptyKeyId) {
    ApiKeyAuthenticator auth;
    ApiKeyCredential cred = makeCredential();
    cred.key_id.clear();
    EXPECT_THROW(auth.addCredential(cred), AuthException);
}

TEST(ApiKeyAuthenticatorTest, AddCredential_RejectsBadHashLength) {
    ApiKeyAuthenticator auth;
    ApiKeyCredential cred = makeCredential();
    cred.secret_hash = "tooshort";
    EXPECT_THROW(auth.addCredential(cred), AuthException);
}

TEST(ApiKeyAuthenticatorTest, AddCredential_AcceptsValidCredential) {
    ApiKeyAuthenticator auth;
    EXPECT_NO_THROW(auth.addCredential(makeCredential()));
    EXPECT_EQ(auth.credentialCount(), 1u);
}

TEST(ApiKeyAuthenticatorTest, AddCredential_ReplacesExisting) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential("k1", "s1", "alice"));
    auth.addCredential(makeCredential("k1", "s2", "bob"));
    EXPECT_EQ(auth.credentialCount(), 1u);
    // After replacement, "s1" must no longer authenticate
    EXPECT_THROW(auth.authenticate("k1", "s1"), AuthException);
    EXPECT_NO_THROW(auth.authenticate("k1", "s2"));
}

// ===========================================================================
// removeCredential
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, RemoveCredential_RemovesExisting) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());
    EXPECT_EQ(auth.credentialCount(), 1u);
    auth.removeCredential("sk_live_test123");
    EXPECT_EQ(auth.credentialCount(), 0u);
}

TEST(ApiKeyAuthenticatorTest, RemoveCredential_NoopForUnknown) {
    ApiKeyAuthenticator auth;
    EXPECT_NO_THROW(auth.removeCredential("nonexistent"));
}

// ===========================================================================
// authenticate – success paths
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, Authenticate_SuccessReturnsCorrectClaims) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());

    const auto claims = auth.authenticate("sk_live_test123", "super-secret");

    EXPECT_EQ(claims.key_id,    "sk_live_test123");
    EXPECT_EQ(claims.principal, "alice@example.com");
    EXPECT_EQ(claims.tenant_id, "tenant-1");
    EXPECT_EQ(claims.scopes,    (std::vector<std::string>{"data:read", "data:write"}));
    EXPECT_EQ(claims.roles,     (std::vector<std::string>{"user"}));
}

TEST(ApiKeyAuthenticatorTest, Authenticate_NoScopesOrRoles) {
    ApiKeyAuthenticator auth;
    auto cred = ApiKeyAuthenticator::createCredential("k", "s", "p");
    auth.addCredential(cred);
    const auto claims = auth.authenticate("k", "s");
    EXPECT_TRUE(claims.scopes.empty());
    EXPECT_TRUE(claims.roles.empty());
}

// ===========================================================================
// authenticate – failure paths
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, Authenticate_EmptyKeyIdThrows) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());
    EXPECT_THROW(auth.authenticate("", "super-secret"), AuthException);
}

TEST(ApiKeyAuthenticatorTest, Authenticate_EmptySecretThrows) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());
    EXPECT_THROW(auth.authenticate("sk_live_test123", ""), AuthException);
}

TEST(ApiKeyAuthenticatorTest, Authenticate_UnknownKeyIdThrows) {
    ApiKeyAuthenticator auth;
    try {
        auth.authenticate("nonexistent", "secret");
        FAIL() << "Expected AuthException";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::API_KEY_INVALID);
    }
}

TEST(ApiKeyAuthenticatorTest, Authenticate_WrongSecretThrows) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());
    try {
        auth.authenticate("sk_live_test123", "wrong-secret");
        FAIL() << "Expected AuthException";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::API_KEY_SECRET_MISMATCH);
    }
}

TEST(ApiKeyAuthenticatorTest, Authenticate_InactiveKeyThrows) {
    ApiKeyAuthenticator auth;
    auto cred = makeCredential();
    cred.active = false;
    auth.addCredential(cred);
    try {
        auth.authenticate("sk_live_test123", "super-secret");
        FAIL() << "Expected AuthException";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::API_KEY_INACTIVE);
    }
}

TEST(ApiKeyAuthenticatorTest, Authenticate_ExpiredKeyThrows) {
    ApiKeyAuthenticator auth;
    auto cred = makeCredential();
    cred.expires_at = std::chrono::system_clock::now() - std::chrono::seconds(1);
    auth.addCredential(cred);
    try {
        auth.authenticate("sk_live_test123", "super-secret");
        FAIL() << "Expected AuthException";
    } catch (const AuthException& ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::API_KEY_EXPIRED);
    }
}

TEST(ApiKeyAuthenticatorTest, Authenticate_FutureKeyNotExpired) {
    ApiKeyAuthenticator auth;
    auto cred = makeCredential();
    cred.expires_at = std::chrono::system_clock::now() + std::chrono::hours(24);
    auth.addCredential(cred);
    EXPECT_NO_THROW(auth.authenticate("sk_live_test123", "super-secret"));
}

TEST(ApiKeyAuthenticatorTest, Authenticate_ExpiryCheckDisabled_ExpiredKeySucceeds) {
    ApiKeyAuthenticator::Config cfg;
    cfg.check_expiry = false;
    ApiKeyAuthenticator auth(cfg);

    auto cred = makeCredential();
    cred.expires_at = std::chrono::system_clock::now() - std::chrono::seconds(1);
    auth.addCredential(cred);
    // Should not throw even though the key is expired
    EXPECT_NO_THROW(auth.authenticate("sk_live_test123", "super-secret"));
}

TEST(ApiKeyAuthenticatorTest, Authenticate_KeyIdTooLongThrows) {
    ApiKeyAuthenticator::Config cfg;
    cfg.max_key_id_length = 10;
    ApiKeyAuthenticator auth(cfg);

    EXPECT_THROW(
        auth.authenticate("this-is-much-longer-than-10-chars", "secret"),
        AuthException
    );
}

TEST(ApiKeyAuthenticatorTest, Authenticate_SecretTooLongThrows) {
    ApiKeyAuthenticator::Config cfg;
    cfg.max_secret_length = 5;
    ApiKeyAuthenticator auth(cfg);

    auto cred = makeCredential();
    auth.addCredential(cred);
    EXPECT_THROW(
        auth.authenticate("sk_live_test123", "this-is-longer-than-5"),
        AuthException
    );
}

// ===========================================================================
// authenticateCombined
// ===========================================================================

TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_Success) {
    ApiKeyAuthenticator auth;
    auth.addCredential(makeCredential());
    const auto claims = auth.authenticateCombined("sk_live_test123.super-secret");
    EXPECT_EQ(claims.principal, "alice@example.com");
}

TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_NoDotThrows) {
    ApiKeyAuthenticator auth;
    EXPECT_THROW(auth.authenticateCombined("nodothere"), AuthException);
}

TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_LeadingDotThrows) {
    ApiKeyAuthenticator auth;
    EXPECT_THROW(auth.authenticateCombined(".secret"), AuthException);
}

TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_TrailingDotThrows) {
    ApiKeyAuthenticator auth;
    EXPECT_THROW(auth.authenticateCombined("keyid."), AuthException);
}

TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_SplitsOnFirstDot) {
    // Secret itself may contain dots; only the first dot is the separator.
    ApiKeyAuthenticator auth;
    auto cred = ApiKeyAuthenticator::createCredential(
        "sk", "sec.ret.with.dots", "principal");
    auth.addCredential(cred);
    const auto claims = auth.authenticateCombined("sk.sec.ret.with.dots");
    EXPECT_EQ(claims.principal, "principal");
}

// ===========================================================================
// ApiKeyClaims helpers
// ===========================================================================

TEST(ApiKeyClaimsTest, IsExpired_NoExpiry) {
    ApiKeyClaims claims;
    EXPECT_FALSE(claims.isExpired());
}

TEST(ApiKeyClaimsTest, IsExpired_FutureExpiry) {
    ApiKeyClaims claims;
    claims.expires_at = std::chrono::system_clock::now() + std::chrono::hours(1);
    EXPECT_FALSE(claims.isExpired());
}

TEST(ApiKeyClaimsTest, IsExpired_PastExpiry) {
    ApiKeyClaims claims;
    claims.expires_at = std::chrono::system_clock::now() - std::chrono::seconds(1);
    EXPECT_TRUE(claims.isExpired());
}

TEST(ApiKeyClaimsTest, HasScope_Present) {
    ApiKeyClaims claims;
    claims.scopes = {"data:read", "data:write"};
    EXPECT_TRUE(claims.hasScope("data:read"));
    EXPECT_TRUE(claims.hasScope("data:write"));
}

TEST(ApiKeyClaimsTest, HasScope_Absent) {
    ApiKeyClaims claims;
    claims.scopes = {"data:read"};
    EXPECT_FALSE(claims.hasScope("admin"));
}

TEST(ApiKeyClaimsTest, HasScope_EmptyScopes) {
    ApiKeyClaims claims;
    EXPECT_FALSE(claims.hasScope("data:read"));
}
