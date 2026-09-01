/**
 * @file test_wave8_auth_hardening.cpp
 * @brief Unit tests for Wave 8 auth hardening items.
 *
 * Test IDs: W8-AUTH-01 .. W8-AUTH-12
 *
 * Covers:
 *  W8-15 — LDAP pagination retry: LDAPAuthenticator audit logger wiring.
 *  W8-16 — Token cache LRU + SHA-256 key: FederatedIdentityManager cache
 *           operations (insert, hit, miss, LRU cap, eviction, clear).
 *  W8-17 — Pool exhaustion audit: LDAPConnectionPool setAuditLogger wiring.
 */

#include <gtest/gtest.h>

#include "auth/auth_audit_logger.h"
#include "auth/federated_identity_manager.h"
#include "auth/ldap_authenticator.h"
#include "auth/ldap_connection_pool.h"
#include "auth/jwt_validator.h"
#include "utils/audit_logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace themis::auth;
using namespace themis::utils;

namespace {

/// Build a minimal FederatedValidationResult with a far-future expiration.
FederatedValidationResult makeFutureResult(const std::string& sub,
                                            const std::string& realm) {
    JWTClaims claims;
    claims.sub        = sub;
    claims.issuer     = realm;
    claims.expiration = std::chrono::system_clock::now() + std::chrono::hours(24);
    FederatedValidationResult r;
    r.claims = claims;
    r.realm  = realm;
    return r;
}

/// Build a result whose JWT expiration is already in the past.
FederatedValidationResult makeExpiredResult(const std::string& sub,
                                             const std::string& realm) {
    JWTClaims claims;
    claims.sub        = sub;
    claims.issuer     = realm;
    claims.expiration = std::chrono::system_clock::now() - std::chrono::seconds(1);
    FederatedValidationResult r;
    r.claims = claims;
    r.realm  = realm;
    return r;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// W8-15: LDAP pagination retry — audit logger wiring on LDAPAuthenticator
// ─────────────────────────────────────────────────────────────────────────────

/// W8-AUTH-01: setAuditLogger(nullptr) on LDAPAuthenticator is safe (no-op).
TEST(Wave8LDAPPaginationRetry, SetNullAuditLoggerIsSafe) {
    LDAPAuthenticatorConfig cfg;
    cfg.server_uri    = "ldap://invalid.local:389";
    cfg.base_dn       = "dc=example,dc=com";
    cfg.bind_dn       = "cn=reader,dc=example,dc=com";
    cfg.bind_password = "secret";
    LDAPAuthenticator auth{cfg};
    EXPECT_NO_THROW(auth.setAuditLogger(nullptr));
}

/// W8-AUTH-02: setAuditLogger() on LDAPAuthenticator accepts a real logger.
TEST(Wave8LDAPPaginationRetry, SetRealAuditLoggerAccepted) {
    const auto log_path = std::filesystem::temp_directory_path() /
                          "w8_auth_ldap_audit.log";
    AuditLogger audit_logger{log_path.string()};
    audit_logger.enable(true);

    LDAPAuthenticatorConfig cfg;
    cfg.server_uri    = "ldap://invalid.local:389";
    cfg.base_dn       = "dc=example,dc=com";
    cfg.bind_dn       = "cn=reader,dc=example,dc=com";
    cfg.bind_password = "secret";
    LDAPAuthenticator auth{cfg};
    EXPECT_NO_THROW(auth.setAuditLogger(&audit_logger));

    std::filesystem::remove(log_path);
}

// ─────────────────────────────────────────────────────────────────────────────
// W8-16: Token cache LRU + SHA-256 key
// ─────────────────────────────────────────────────────────────────────────────

/// W8-AUTH-03: Cache miss returns std::nullopt.
TEST(Wave8TokenCacheLRU, CacheMissReturnsNullopt) {
    FederatedIdentityManager fim;
    auto result = fim.getCachedResult("no_such_token");
    EXPECT_FALSE(result.has_value());
}

/// W8-AUTH-04: Cache hit returns the stored result.
TEST(Wave8TokenCacheLRU, CacheHitReturnsResult) {
    FederatedIdentityManager fim;
    const std::string token = "header.payload.sig_abc123";
    auto fvr = makeFutureResult("alice", "https://idp.example.com");
    fim.cacheValidationResult(token, fvr);

    auto hit = fim.getCachedResult(token);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->claims.sub, "alice");
    EXPECT_EQ(hit->realm, "https://idp.example.com");
}

/// W8-AUTH-05: Two different tokens do not collide in the cache.
TEST(Wave8TokenCacheLRU, DifferentTokensDoNotCollide) {
    FederatedIdentityManager fim;
    fim.cacheValidationResult("token_a", makeFutureResult("alice", "realm1"));
    fim.cacheValidationResult("token_b", makeFutureResult("bob",   "realm2"));

    auto hit_a = fim.getCachedResult("token_a");
    auto hit_b = fim.getCachedResult("token_b");
    ASSERT_TRUE(hit_a.has_value());
    ASSERT_TRUE(hit_b.has_value());
    EXPECT_EQ(hit_a->claims.sub, "alice");
    EXPECT_EQ(hit_b->claims.sub, "bob");
}

/// W8-AUTH-06: Overwriting a token updates the cached result.
TEST(Wave8TokenCacheLRU, OverwriteTokenUpdatesResult) {
    FederatedIdentityManager fim;
    const std::string token = "update_me_token";
    fim.cacheValidationResult(token, makeFutureResult("v1_user", "r1"));
    fim.cacheValidationResult(token, makeFutureResult("v2_user", "r2"));

    auto hit = fim.getCachedResult(token);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->claims.sub, "v2_user");
}

/// W8-AUTH-07: Expired entries are evicted by evictExpiredCacheEntries().
TEST(Wave8TokenCacheLRU, ExpiredEntriesAreEvicted) {
    FederatedIdentityManager fim;
    fim.cacheValidationResult("live_token",    makeFutureResult("alive",   "r"));
    fim.cacheValidationResult("dead_token",    makeExpiredResult("expired", "r"));

    const size_t evicted = fim.evictExpiredCacheEntries();
    EXPECT_GE(evicted, 1u); // at least the one expired entry

    // Live token still present; dead token gone.
    EXPECT_TRUE(fim.getCachedResult("live_token").has_value());
    EXPECT_FALSE(fim.getCachedResult("dead_token").has_value());
}

/// W8-AUTH-08: clearTokenCache removes all entries.
TEST(Wave8TokenCacheLRU, ClearTokenCacheRemovesAll) {
    FederatedIdentityManager fim;
    for (int i = 0; i < 5; ++i) {
        fim.cacheValidationResult("tok_" + std::to_string(i),
                                   makeFutureResult("u" + std::to_string(i), "r"));
    }
    EXPECT_GE(fim.tokenCacheSize(), 5u);
    fim.clearTokenCache();
    EXPECT_EQ(fim.tokenCacheSize(), 0u);
}

/// W8-AUTH-09: tokenCacheSize() reflects the live count.
TEST(Wave8TokenCacheLRU, CacheSizeReflectsLiveCount) {
    FederatedIdentityManager fim;
    EXPECT_EQ(fim.tokenCacheSize(), 0u);
    fim.cacheValidationResult("t1", makeFutureResult("u1", "r"));
    EXPECT_EQ(fim.tokenCacheSize(), 1u);
    fim.cacheValidationResult("t2", makeFutureResult("u2", "r"));
    EXPECT_EQ(fim.tokenCacheSize(), 2u);
    fim.clearTokenCache();
    EXPECT_EQ(fim.tokenCacheSize(), 0u);
}

/// W8-AUTH-10: Cache survives filling kTokenCacheMaxSize entries (no crash).
TEST(Wave8TokenCacheLRU, CacheSurvivesLargeLoad) {
    FederatedIdentityManager fim;
    // Insert more entries than the LRU cap (4096) to exercise eviction path.
    // Using a tight loop of 4100 entries.
    constexpr int kCount = 4100;
    for (int i = 0; i < kCount; ++i) {
        fim.cacheValidationResult("lru_tok_" + std::to_string(i),
                                   makeFutureResult("usr_" + std::to_string(i), "r"));
    }
    // After LRU eviction the cache should be at or below the cap.
    EXPECT_LE(fim.tokenCacheSize(), 4096u);
    // The most recently inserted token should still be present.
    EXPECT_TRUE(fim.getCachedResult("lru_tok_4099").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// W8-17: Pool exhaustion audit — setAuditLogger wiring on LDAPConnectionPool
// ─────────────────────────────────────────────────────────────────────────────

/// W8-AUTH-11: setAuditLogger(nullptr) on LDAPConnectionPool is safe.
TEST(Wave8PoolExhaustionAudit, SetNullAuditLoggerIsSafe) {
    LDAPPoolConfig pcfg;
    pcfg.host              = "ldap://invalid.local";
    pcfg.max_size          = 2;
    pcfg.checkout_timeout_ms = 10; // very short for tests
    LDAPConnectionPool pool{pcfg};
    EXPECT_NO_THROW(pool.setAuditLogger(nullptr));
}

/// W8-AUTH-12: setAuditLogger() on LDAPConnectionPool accepts a real logger.
TEST(Wave8PoolExhaustionAudit, SetRealAuditLoggerAccepted) {
    const auto log_path = std::filesystem::temp_directory_path() /
                          "w8_pool_audit.log";
    AuditLogger audit_logger{log_path.string()};
    audit_logger.enable(true);

    LDAPPoolConfig pcfg;
    pcfg.host              = "ldap://invalid.local";
    pcfg.max_size          = 2;
    pcfg.checkout_timeout_ms = 10;
    LDAPConnectionPool pool{pcfg};
    EXPECT_NO_THROW(pool.setAuditLogger(&audit_logger));

    std::filesystem::remove(log_path);
}
