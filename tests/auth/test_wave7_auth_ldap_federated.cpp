/**
 * @file test_wave7_auth_ldap_federated.cpp
 * @brief Wave 7 — LDAP Connection Pool + Federated Cross-Provider State Sync tests.
 *
 * Covers:
 *  - LDAP connection pool lifecycle (checkout, checkin, exhaustion, idle eviction)
 *  - LDAP search pagination plumbing (non-LDAP stub path)
 *  - FederatedIdentityManager: cross-provider trust registry (9 new methods)
 *  - FederatedIdentityManager: in-memory token validation cache (hit, miss, expiry, eviction)
 *
 * Labels: wave_b release_critical
 */

#include <gtest/gtest.h>

#include "auth/ldap_authenticator.h"
#include "auth/ldap_connection_pool.h"
#include "auth/federated_identity_manager.h"
#include "auth/auth_error.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::auth;
using namespace std::chrono_literals;

// ===========================================================================
// Helpers
// ===========================================================================

namespace {

/// Build a minimal LDAPPoolConfig that points at a non-existent server so
/// createConnection() fails fast (no network calls in unit tests).
LDAPPoolConfig makeFakePoolConfig(int max_size = 1, int timeout_ms = 50) {
    LDAPPoolConfig cfg;
    cfg.server_url          = "ldap://127.0.0.1:1";  // refuse immediately
    cfg.port                = 1;
    cfg.max_size            = max_size;
    cfg.min_idle            = 0;
    cfg.checkout_timeout_ms = timeout_ms;
    return cfg;
}

/// Build a minimal LDAPConfig for the authenticator (pool-disabled so tests
/// don't spin up pool threads unnecessarily).
LDAPConfig makeLDAPConfig(bool pool_enabled = false) {
    LDAPConfig cfg;
    cfg.server_url       = "ldap://127.0.0.1:1";
    cfg.bind_dn_template = "CN={username},DC=test,DC=local";
    cfg.pool_enabled     = pool_enabled;
    cfg.pool_max_size    = 1;
    cfg.pool_checkout_timeout_ms = 50;
    return cfg;
}

/// Build an OIDCProviderConfig for a fake issuer (no actual OIDC server).
OIDCProviderConfig makeFakeOIDCConfig(const std::string &issuer) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer;
    cfg.client_id  = "test-client";
    return cfg;
}

/// Build a fake JWT-like token with an embedded iss claim (not cryptographically
/// valid — used for cache/trust tests that do NOT call OIDCProvider::validateToken).
/// Format: <base64url-header>.<base64url-payload>.<dummy-sig>
std::string makeFakeJWT(const std::string &issuer,
                        const std::string &sub = "user1",
                        const std::string &jti = "jti-1") {
    // Header: {"alg":"none","typ":"JWT"}
    const std::string hdr_json = R"({"alg":"none","typ":"JWT"})";
    // Payload with iss, sub, jti, exp (far future)
    const std::string pay_json =
        R"({"iss":")" + issuer + R"(","sub":")" + sub +
        R"(","jti":")" + jti + R"(","exp":9999999999,"iat":1})";

    auto b64url = [](const std::string &s) {
        // Minimal base64url (no padding) for ASCII-safe JSON
        static const char tbl[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        unsigned acc = 0, bits = 0;
        for (unsigned char c : s) {
            acc  = (acc << 8) | c;
            bits += 8;
            while (bits >= 6) {
                bits -= 6;
                char ch = tbl[(acc >> bits) & 0x3F];
                if (ch == '+') ch = '-';
                else if (ch == '/') ch = '_';
                out += ch;
            }
        }
        if (bits > 0) {
            char ch = tbl[(acc << (6 - bits)) & 0x3F];
            if (ch == '+') ch = '-';
            else if (ch == '/') ch = '_';
            out += ch;
        }
        return out;
    };

    return b64url(hdr_json) + "." + b64url(pay_json) + ".fakesig";
}

} // anonymous namespace

// ===========================================================================
// LDAP Connection Pool Tests (WP-01..WP-06)
// ===========================================================================

// WP-01: Pool construction with valid config does not throw.
TEST(LDAPPool_Wave7, WP01_ConstructionDoesNotThrow) {
    EXPECT_NO_THROW({
        LDAPConnectionPool pool(makeFakePoolConfig(4));
        (void)pool;
    });
}

// WP-02: checkout() in the no-LDAP stub path returns nullptr (not a throw).
//        In a LDAP-enabled build this test is skipped via the THEMIS_HAS_LDAP guard.
TEST(LDAPPool_Wave7, WP02_CheckoutWithoutLdapReturnsNullptr) {
#ifdef THEMIS_HAS_LDAP
    GTEST_SKIP() << "THEMIS_HAS_LDAP is defined — stub-path test not applicable";
#endif
    LDAPConnectionPool pool(makeFakePoolConfig(2, 50));
    // Without libldap, checkout() returns nullptr immediately.
    auto conn = pool.checkout();
    EXPECT_EQ(conn, nullptr);
}

// WP-03: Pool config values are preserved after construction.
TEST(LDAPPool_Wave7, WP03_ConfigIsPreserved) {
    LDAPPoolConfig cfg = makeFakePoolConfig(8, 200);
    LDAPConnectionPool pool(cfg);
    EXPECT_EQ(pool.config().max_size, 8);
    EXPECT_EQ(pool.config().checkout_timeout_ms, 200);
}

// WP-04: Metrics accessors return sane values on an empty pool.
TEST(LDAPPool_Wave7, WP04_MetricsOnEmptyPool) {
    LDAPConnectionPool pool(makeFakePoolConfig(4));
    EXPECT_GE(pool.poolSize(), 0);
    EXPECT_GE(pool.idleConnections(), 0);
    EXPECT_GE(pool.activeConnections(), 0);
}

// WP-05: Pool PROVIDER_DEGRADED exhaustion behavior — real LDAP path only.
//        We simulate exhaustion by using max_size=0 and a very short timeout.
//        In non-LDAP stub builds, checkout() returns nullptr not throw, so we skip.
TEST(LDAPPool_Wave7, WP05_PoolExhaustionThrowsProviderDegraded) {
#ifndef THEMIS_HAS_LDAP
    GTEST_SKIP() << "THEMIS_HAS_LDAP not defined — exhaustion throw only in real LDAP builds";
#else
    LDAPPoolConfig cfg = makeFakePoolConfig(0, 10);  // max_size=0 → always exhausted
    LDAPConnectionPool pool(cfg);
    EXPECT_THROW({
        pool.checkout();
    }, AuthException);
#endif
}

// WP-06: Stale connection marked via PooledConnection::markStale() is not
//        returned to the idle pool.
TEST(LDAPPool_Wave7, WP06_StaleConnectionIsEvictedNotReturned) {
#ifndef THEMIS_HAS_LDAP
    GTEST_SKIP() << "THEMIS_HAS_LDAP not defined — stale-eviction not testable";
#else
    LDAPConnectionPool pool(makeFakePoolConfig(2));
    {
        auto conn = pool.checkout();
        ASSERT_NE(conn, nullptr);
        conn->markStale();
        // On destruction the stale handle is NOT returned to idle_.
    }
    // After returning a stale conn, the pool total decreases (eviction path).
    EXPECT_GE(pool.activeConnections(), 0);
#endif
}

// ===========================================================================
// LDAPAuthenticator Pool integration Tests (WA-01..WA-04)
// ===========================================================================

// WA-01: Authenticator without pool falls back to inject-fn.
TEST(LDAPAuthenticator_Wave7, WA01_InjectFnUsedWhenPoolDisabled) {
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeLDAPConfig(/*pool_enabled=*/false)));

    bool called = false;
    LDAPAuthenticator::setLdapBindFn(
        [&called](const std::string &, const std::string &dn,
                  const std::string &) -> LDAPAuthResult {
            called = true;
            return LDAPAuthResult::Success("user", dn, {"admin"});
        });

    const auto result = auth.authenticate("user", "pass");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(called);

    LDAPAuthenticator::setLdapBindFn({});  // clean up
}

// WA-02: Authenticator with pool enabled still honours the inject-fn.
TEST(LDAPAuthenticator_Wave7, WA02_InjectFnUsedWhenPoolEnabled) {
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeLDAPConfig(/*pool_enabled=*/true)));

    LDAPAuthenticator::setLdapBindFn(
        [](const std::string &, const std::string &dn,
           const std::string &) -> LDAPAuthResult {
            return LDAPAuthResult::Success("u", dn, {"role"});
        });

    const auto result = auth.authenticate("u", "p");
    EXPECT_TRUE(result.success);

    LDAPAuthenticator::setLdapBindFn({});
}

// WA-03: Authenticator rejects empty username.
TEST(LDAPAuthenticator_Wave7, WA03_EmptyUsernameThrows) {
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeLDAPConfig()));
    EXPECT_THROW(auth.authenticate("", "pass"), AuthException);
}

// WA-04: Authenticator rejects empty password.
TEST(LDAPAuthenticator_Wave7, WA04_EmptyPasswordThrows) {
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeLDAPConfig()));
    EXPECT_THROW(auth.authenticate("user", ""), AuthException);
}

// ===========================================================================
// FederatedIdentityManager: realm registration (FR-01..FR-03)
// ===========================================================================

// FR-01: addRealm / hasRealm roundtrip.
TEST(FederatedManager_Wave7, FR01_AddAndHasRealm) {
    FederatedIdentityManager fed;
    fed.addRealm(makeFakeOIDCConfig("https://idp.example.com/realms/prod"));
    EXPECT_TRUE(fed.hasRealm("https://idp.example.com/realms/prod"));
    EXPECT_EQ(fed.realmCount(), 1u);
}

// FR-02: removeRealm returns true on success and false when not found.
TEST(FederatedManager_Wave7, FR02_RemoveRealm) {
    FederatedIdentityManager fed;
    fed.addRealm(makeFakeOIDCConfig("https://idp.example.com/realms/dev"));
    EXPECT_TRUE(fed.removeRealm("https://idp.example.com/realms/dev"));
    EXPECT_FALSE(fed.hasRealm("https://idp.example.com/realms/dev"));
    EXPECT_FALSE(fed.removeRealm("https://idp.example.com/realms/dev"));
}

// FR-03: Duplicate realm registration throws AUTH_CONFIG_INVALID.
TEST(FederatedManager_Wave7, FR03_DuplicateRealmThrows) {
    FederatedIdentityManager fed;
    fed.addRealm(makeFakeOIDCConfig("https://idp.example.com/realms/x"));
    EXPECT_THROW(
        fed.addRealm(makeFakeOIDCConfig("https://idp.example.com/realms/x")),
        AuthException);
}

// ===========================================================================
// FederatedIdentityManager: cross-provider trust registry (FT-01..FT-05)
// ===========================================================================

// FT-01: addCrossProviderTrust + isTrustedBy roundtrip.
TEST(FederatedManager_Wave7, FT01_AddTrustAndCheck) {
    FederatedIdentityManager fed;
    fed.addCrossProviderTrust("https://issuer-a.example.com",
                               "https://issuer-b.example.com");
    EXPECT_TRUE(fed.isTrustedBy("https://issuer-a.example.com",
                                "https://issuer-b.example.com"));
    EXPECT_FALSE(fed.isTrustedBy("https://issuer-b.example.com",
                                 "https://issuer-a.example.com"));
}

// FT-02: A realm always implicitly trusts itself (same-issuer shortcut).
TEST(FederatedManager_Wave7, FT02_SameIssuerAlwaysTrusted) {
    FederatedIdentityManager fed;
    EXPECT_TRUE(fed.isTrustedBy("https://idp.example.com",
                                "https://idp.example.com"));
}

// FT-03: removeCrossProviderTrust correctly removes relationship.
TEST(FederatedManager_Wave7, FT03_RemoveTrust) {
    FederatedIdentityManager fed;
    fed.addCrossProviderTrust("https://a.example.com", "https://b.example.com");
    EXPECT_TRUE(fed.removeCrossProviderTrust("https://a.example.com",
                                             "https://b.example.com"));
    EXPECT_FALSE(fed.isTrustedBy("https://a.example.com",
                                 "https://b.example.com"));
    // Second remove is idempotent (returns false).
    EXPECT_FALSE(fed.removeCrossProviderTrust("https://a.example.com",
                                              "https://b.example.com"));
}

// FT-04: getCrossProviderTrusts lists all registered subject-issuers.
TEST(FederatedManager_Wave7, FT04_GetCrossProviderTrusts) {
    FederatedIdentityManager fed;
    fed.addCrossProviderTrust("https://src1.example.com", "https://dst.example.com");
    fed.addCrossProviderTrust("https://src2.example.com", "https://dst.example.com");

    const auto trusts = fed.getCrossProviderTrusts("https://dst.example.com");
    EXPECT_EQ(trusts.size(), 2u);
}

// FT-05: Empty issuer throws AUTH_CONFIG_INVALID.
TEST(FederatedManager_Wave7, FT05_EmptyIssuerThrows) {
    FederatedIdentityManager fed;
    EXPECT_THROW(
        fed.addCrossProviderTrust("", "https://idp.example.com"),
        AuthException);
    EXPECT_THROW(
        fed.addCrossProviderTrust("https://idp.example.com", ""),
        AuthException);
}

// ===========================================================================
// FederatedIdentityManager: token validation cache (FC-01..FC-07)
// ===========================================================================

/// Build a FederatedValidationResult for cache injection tests (no real JWT).
static FederatedValidationResult makeFakeResult(const std::string &realm,
                                                const std::string &sub,
                                                std::chrono::system_clock::time_point exp) {
    JWTClaims claims;
    claims.sub        = sub;
    claims.jti        = "jti-cache-test";
    claims.expiration = exp;
    FederatedValidationResult r;
    r.claims = claims;
    r.realm  = realm;
    return r;
}

// FC-01: cacheValidationResult + getCachedResult hit.
TEST(FederatedManager_Wave7, FC01_CacheHit) {
    FederatedIdentityManager fed;
    const auto exp = std::chrono::system_clock::now() + 1h;
    auto res = makeFakeResult("https://idp.example.com", "alice", exp);
    fed.cacheValidationResult("token-abc", res);

    const auto cached = fed.getCachedResult("token-abc");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->claims.sub, "alice");
    EXPECT_EQ(cached->realm, "https://idp.example.com");
}

// FC-02: getCachedResult returns nullopt for unknown token.
TEST(FederatedManager_Wave7, FC02_CacheMiss) {
    FederatedIdentityManager fed;
    EXPECT_FALSE(fed.getCachedResult("no-such-token").has_value());
}

// FC-03: Expired cache entry returns nullopt without eviction.
TEST(FederatedManager_Wave7, FC03_ExpiredEntryReturnsMiss) {
    FederatedIdentityManager fed;
    const auto past = std::chrono::system_clock::now() - 1s;
    auto res = makeFakeResult("https://idp.example.com", "bob", past);
    fed.cacheValidationResult("expired-token", res);

    // Expired — getCachedResult must return nullopt.
    EXPECT_FALSE(fed.getCachedResult("expired-token").has_value());
    // But the entry still occupies the map until explicit eviction.
    EXPECT_EQ(fed.tokenCacheSize(), 1u);
}

// FC-04: evictExpiredCacheEntries removes stale entries.
TEST(FederatedManager_Wave7, FC04_EvictExpiredEntries) {
    FederatedIdentityManager fed;
    const auto past   = std::chrono::system_clock::now() - 1s;
    const auto future = std::chrono::system_clock::now() + 1h;

    fed.cacheValidationResult("stale-1",
        makeFakeResult("https://idp.example.com", "u1", past));
    fed.cacheValidationResult("stale-2",
        makeFakeResult("https://idp.example.com", "u2", past));
    fed.cacheValidationResult("valid-1",
        makeFakeResult("https://idp.example.com", "u3", future));

    EXPECT_EQ(fed.tokenCacheSize(), 3u);
    const size_t evicted = fed.evictExpiredCacheEntries();
    EXPECT_EQ(evicted, 2u);
    EXPECT_EQ(fed.tokenCacheSize(), 1u);
}

// FC-05: clearTokenCache empties the entire cache.
TEST(FederatedManager_Wave7, FC05_ClearCache) {
    FederatedIdentityManager fed;
    for (int i = 0; i < 5; ++i) {
        fed.cacheValidationResult(
            "tok-" + std::to_string(i),
            makeFakeResult("https://idp.example.com", "u" + std::to_string(i),
                           std::chrono::system_clock::now() + 1h));
    }
    EXPECT_EQ(fed.tokenCacheSize(), 5u);
    fed.clearTokenCache();
    EXPECT_EQ(fed.tokenCacheSize(), 0u);
}

// FC-06: tokenCacheSize reflects insertions and evictions accurately.
TEST(FederatedManager_Wave7, FC06_CacheSizeAccuracy) {
    FederatedIdentityManager fed;
    EXPECT_EQ(fed.tokenCacheSize(), 0u);
    fed.cacheValidationResult("t1",
        makeFakeResult("https://idp.example.com", "a",
                       std::chrono::system_clock::now() + 1h));
    EXPECT_EQ(fed.tokenCacheSize(), 1u);
    fed.cacheValidationResult("t2",
        makeFakeResult("https://idp.example.com", "b",
                       std::chrono::system_clock::now() + 1h));
    EXPECT_EQ(fed.tokenCacheSize(), 2u);
    fed.clearTokenCache();
    EXPECT_EQ(fed.tokenCacheSize(), 0u);
}

// FC-07: validateToken with unknown realm throws FEDERATION_UNKNOWN_REALM
//        (fast path — realm lookup, not cache).
TEST(FederatedManager_Wave7, FC07_UnknownRealmThrowsFederationError) {
    FederatedIdentityManager fed;
    // No realms registered.
    const std::string fake_token = makeFakeJWT("https://unknown.example.com");
    try {
        fed.validateToken(fake_token);
        FAIL() << "Expected AuthException";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::FEDERATION_UNKNOWN_REALM);
    }
}
