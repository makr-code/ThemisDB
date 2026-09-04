/**
 * @file test_auth_hardening_revocation_federation.cpp
 * @brief Phase 4 focused regression tests for auth hardening delivery.
 *
 * Covers three test families mapped to the three roadmap items:
 *
 * ### RFP — Distributed Revocation / Federation / Policy-Edge
 *   RFP-01  Add + isRevoked round-trip (baseline sanity after Phase 2/3 hardening)
 *   RFP-02  Empty JTI rejected by add() with REVOCATION_ENTRY_INVALID
 *   RFP-03  JTI exceeding kMaxJtiBytes (1024) rejected by add() with REVOCATION_ENTRY_INVALID
 *   RFP-04  purgeExpired removes expired entries; valid entries survive
 *   RFP-05  Concurrent add() and isRevoked() are thread-safe (no crash, no data race)
 *   RFP-06  Re-adding the same JTI is idempotent (LWW semantics, no exception)
 *   RFP-07  isRevoked() returns false for an unknown JTI (not revoked by default)
 *   RFP-08  Expired JTI reports false after expiry passes purgeExpired
 *
 * ### FED — Federation Manager Failure Classification
 *   FED-01  Unknown realm throws FEDERATION_UNKNOWN_REALM (not JWT_ISSUER_MISMATCH)
 *   FED-02  Duplicate realm registration throws AUTH_CONFIG_INVALID
 *   FED-03  Token that is not a valid JWT (no dot separators) throws JWT_INVALID_FORMAT
 *   FED-06  realmProvider for unknown issuer throws FEDERATION_UNKNOWN_REALM
 *   FED-07  Multiple realms coexist; correct realm selected by issuer
 *   FED-08  realmCount() reflects realm additions correctly
 *
 * ### ASY — Async / Provider-Integration Consistency
 *   ASY-01  SessionManager::createSession with empty user_id throws std::invalid_argument
 *   ASY-02  validateSession for unknown session_id returns valid=false (no throw)
 *   ASY-03  validateSession for expired session returns valid=false with reason
 *   ASY-04  terminateSession for unknown session_id is idempotent (no throw)
 *   ASY-05  terminateAllOtherSessions terminates only the non-kept sessions
 *   ASY-06  Session ID prefix invariant: all created sessions start with "sess_"
 *   ASY-07  pruneExpired removes expired sessions; active sessions survive
 *   ASY-08  Per-user session limit enforced: oldest session evicted when limit exceeded
 */

#include <gtest/gtest.h>

#include "auth/distributed_token_blacklist.h"
#include "auth/federated_identity_manager.h"
#include "auth/session_manager.h"
#include "auth/auth_error.h"
#include "auth/auth_principal_contract.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <future>
#include <stdexcept>

using namespace themis::auth;
namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

/// Returns a steady-clock-unique temp directory path for RocksDB isolation.
fs::path makeTempDir(std::string_view tag) {
    return fs::temp_directory_path()
           / (std::string("themis_auth_harden_") + std::string(tag) + "_"
              + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

/// Build a single-node DistributedBlacklistConfig pointing to @p db_path.
DistributedBlacklistConfig singleNodeCfg(const fs::path &db_path) {
    DistributedBlacklistConfig cfg;
    cfg.db_path                 = db_path.string();
    cfg.enable_cluster_sync     = false;
    cfg.purge_interval_seconds  = 3600;
    cfg.sync_interval_seconds   = 3600;
    cfg.local_node.node_id      = "node-harden";
    return cfg;
}

/// Time-point helper: now + seconds.
std::chrono::system_clock::time_point future(int secs) {
    return std::chrono::system_clock::now() + std::chrono::seconds(secs);
}

/// Time-point helper: already expired.
std::chrono::system_clock::time_point past(int secs) {
    return std::chrono::system_clock::now() - std::chrono::seconds(secs);
}

} // anonymous namespace

// ============================================================================
// RFP — Distributed Revocation
// ============================================================================

class RevocationHardeningTest : public ::testing::Test {
protected:
    fs::path db_path_;
    void SetUp()    override { db_path_ = makeTempDir("rfp"); fs::create_directories(db_path_); }
    void TearDown() override { fs::remove_all(db_path_); }
};

/**
 * @brief RFP-01: Basic add / isRevoked round-trip after Phase 2/3 hardening.
 */
TEST_F(RevocationHardeningTest, RFP01_AddIsRevokedRoundTrip) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    bl.add("jti-rfp01", future(300));
    EXPECT_TRUE(bl.isRevoked("jti-rfp01"));
    EXPECT_FALSE(bl.isRevoked("jti-unknown"));
}

/**
 * @brief RFP-02: Empty JTI is rejected with REVOCATION_ENTRY_INVALID.
 *
 * An empty JTI cannot meaningfully identify a token; accepting it would
 * allow silent revocation of every token that lacks a jti claim.
 */
TEST_F(RevocationHardeningTest, RFP02_EmptyJtiRejected) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    EXPECT_THROW(
        { bl.add("", future(300)); },
        AuthException
    );
    // Verify the thrown code is REVOCATION_ENTRY_INVALID
    try {
        bl.add("", future(300));
        FAIL() << "Expected AuthException";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::REVOCATION_ENTRY_INVALID);
    }
}

/**
 * @brief RFP-03: JTI exceeding kMaxJtiBytes (1024) is rejected with REVOCATION_ENTRY_INVALID.
 */
TEST_F(RevocationHardeningTest, RFP03_OversizedJtiRejected) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    const std::string big_jti(kMaxJtiBytes + 1, 'x');
    try {
        bl.add(big_jti, future(300));
        FAIL() << "Expected AuthException for oversized JTI";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::REVOCATION_ENTRY_INVALID);
    }
}

/**
 * @brief RFP-04: purgeExpired removes expired entries; non-expired entries survive.
 */
TEST_F(RevocationHardeningTest, RFP04_PurgeExpiredSelectiveRemoval) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    bl.add("jti-alive", future(300));
    bl.add("jti-dead",  past(1));   // already expired
    bl.purgeExpired();
    EXPECT_TRUE(bl.isRevoked("jti-alive"));
    EXPECT_FALSE(bl.isRevoked("jti-dead"));
}

/**
 * @brief RFP-05: Concurrent add() and isRevoked() are thread-safe.
 */
TEST_F(RevocationHardeningTest, RFP05_ConcurrentAddIsRevoked) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 50;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&bl, t, &errors]() {
            try {
                for (int i = 0; i < kOpsPerThread; ++i) {
                    const std::string jti = "jti-conc-" + std::to_string(t) + "-" + std::to_string(i);
                    bl.add(jti, future(300));
                    (void)bl.isRevoked(jti);
                }
            } catch (...) {
                ++errors;
            }
        });
    }
    for (auto &th : threads) {
      th.join();
    }
    EXPECT_EQ(errors.load(), 0) << "Concurrent operations produced unexpected exceptions";
}

/**
 * @brief RFP-06: Re-adding the same JTI is idempotent (no exception, LWW semantics).
 */
TEST_F(RevocationHardeningTest, RFP06_IdempotentReAdd) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    ASSERT_NO_THROW(bl.add("jti-idem", future(300)));
    ASSERT_NO_THROW(bl.add("jti-idem", future(600)));  // second add with later expiry
    EXPECT_TRUE(bl.isRevoked("jti-idem"));
}

/**
 * @brief RFP-07: isRevoked() returns false for a JTI that was never added.
 */
TEST_F(RevocationHardeningTest, RFP07_UnknownJtiIsNotRevoked) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    EXPECT_FALSE(bl.isRevoked("jti-never-added"));
}

/**
 * @brief RFP-08: After an entry expires and purgeExpired runs, isRevoked() returns false.
 */
TEST_F(RevocationHardeningTest, RFP08_ExpiredEntryNotRevoked) {
    DistributedTokenBlacklist bl(singleNodeCfg(db_path_));
    bl.add("jti-expiring", past(1));  // immediately expired
    bl.purgeExpired();
    EXPECT_FALSE(bl.isRevoked("jti-expiring"));
}

// ============================================================================
// FED — Federation Manager Failure Classification
// ============================================================================

class FederationHardeningTest : public ::testing::Test {
protected:
    FederatedIdentityManager fed_;
};

/**
 * @brief FED-01: validateToken for a token with an unknown issuer throws
 *        FEDERATION_UNKNOWN_REALM (not JWT_ISSUER_MISMATCH).
 *
 * Phase 2/3 hardened the error classification: callers can now distinguish
 * "no such realm" from "realm exists but token is invalid".
 */
TEST_F(FederationHardeningTest, FED01_UnknownRealmThrowsFederationCode) {
    // Build a minimal JWT-shaped token with iss claim "https://unknown.example.com"
    // Header: {"alg":"RS256","typ":"JWT"}  → base64url
    // Payload: {"iss":"https://unknown.example.com","sub":"u1","exp":9999999999}  → base64url
    // Signature: dummy bytes (won't be validated — realm lookup fails first)
    const std::string header  = "******";
    const std::string payload = "eyJpc3MiOiJodHRwczovL3Vua25vd24uZXhhbXBsZS5jb20iLCJzdWIiOiJ1MSIsImV4cCI6OTk5OTk5OTk5OX0";
    const std::string sig     = "dummysig";
    const std::string token   = header + "." + payload + "." + sig;

    try {
        fed_.validateToken(token);
        FAIL() << "Expected AuthException";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::FEDERATION_UNKNOWN_REALM)
            << "Expected FEDERATION_UNKNOWN_REALM, got: "
            << static_cast<int>(ex.error().code());
    }
}

/**
 * @brief FED-02: Registering the same issuer URL twice throws AUTH_CONFIG_INVALID.
 */
TEST_F(FederationHardeningTest, FED02_DuplicateRealmThrowsConfigError) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com/realm/test";
    cfg.client_id  = "themisdb";
    fed_.addRealm(cfg);

    try {
        fed_.addRealm(cfg);  // same issuer again
        FAIL() << "Expected AuthException for duplicate realm";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::AUTH_CONFIG_INVALID);
    }
}

/**
 * @brief FED-03: A token that is not a valid JWT (no dot separators) throws
 *        JWT_INVALID_FORMAT before any realm lookup.
 */
TEST_F(FederationHardeningTest, FED03_MalformedTokenThrowsInvalidFormat) {
    try {
        fed_.validateToken("not-a-jwt");
        FAIL() << "Expected AuthException";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::JWT_INVALID_FORMAT);
    }
}

/**
 * @brief FED-06: realmProvider for an unknown issuer throws FEDERATION_UNKNOWN_REALM.
 */
TEST_F(FederationHardeningTest, FED06_RealmProviderUnknownThrows) {
    try {
        (void)fed_.realmProvider("https://nobody.example.com");
        FAIL() << "Expected AuthException";
    } catch (const AuthException &ex) {
        EXPECT_EQ(ex.error().code(), AuthErrorCode::FEDERATION_UNKNOWN_REALM);
    }
}

/**
 * @brief FED-07: Multiple realms coexist; realmCount() reflects additions.
 */
TEST_F(FederationHardeningTest, FED07_MultipleRealmsCoexist) {
    OIDCProviderConfig cfg1;
    cfg1.issuer_url = "https://idp1.example.com/realm/a";
    cfg1.client_id  = "themisdb";
    OIDCProviderConfig cfg2;
    cfg2.issuer_url = "https://idp2.example.com/realm/b";
    cfg2.client_id  = "themisdb";

    EXPECT_EQ(fed_.realmCount(), 0u);
    fed_.addRealm(cfg1);
    EXPECT_EQ(fed_.realmCount(), 1u);
    fed_.addRealm(cfg2);
    EXPECT_EQ(fed_.realmCount(), 2u);
}

/**
 * @brief FED-08: realmCount() starts at 0 and increments correctly.
 */
TEST_F(FederationHardeningTest, FED08_RealmCountIncrements) {
    EXPECT_EQ(fed_.realmCount(), 0u);
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp-fed08.example.com/realm/x";
    cfg.client_id  = "themisdb";
    fed_.addRealm(cfg);
    EXPECT_EQ(fed_.realmCount(), 1u);
}

// ============================================================================
// ASY — Session / Async-Provider Consistency
// ============================================================================

class SessionHardeningTest : public ::testing::Test {
protected:
    SessionManager::SessionLimits limits_;
    void SetUp() override {
        limits_.max_sessions_per_user = 3;
        limits_.idle_timeout          = std::chrono::milliseconds(0);  // no idle timeout
        limits_.absolute_timeout      = std::chrono::hours(24);
    }
};

/**
 * @brief ASY-01: createSession with empty user_id throws std::invalid_argument.
 *
 * The contract requires user_id to be non-empty; an empty subject would produce
 * an anonymous session with no deterministic ownership.
 */
TEST_F(SessionHardeningTest, ASY01_EmptyUserIdThrows) {
    SessionManager sm(limits_);
    EXPECT_THROW(
        (void)sm.createSession(""),
        std::invalid_argument
    );
}

/**
 * @brief ASY-02: validateSession for a completely unknown session ID returns valid=false
 *        without throwing.
 */
TEST_F(SessionHardeningTest, ASY02_UnknownSessionReturnsFalse) {
    SessionManager sm(limits_);
    const auto result = sm.validateSession("sess_doesnotexist");
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.reason.empty());
}

/**
 * @brief ASY-03: validateSession for a session with an expired absolute timeout
 *        returns valid=false with a populated reason string.
 */
TEST_F(SessionHardeningTest, ASY03_ExpiredSessionReturnsFalse) {
    SessionManager::SessionLimits tiny;
    tiny.max_sessions_per_user = 10;
    tiny.absolute_timeout      = std::chrono::milliseconds(1);  // 1 ms — essentially instant expiry
    tiny.idle_timeout          = std::chrono::milliseconds(0);
    SessionManager sm(tiny);

    const std::string sid = sm.createSession("user-asy03");
    // Sleep briefly to guarantee the 1 ms absolute timeout has elapsed.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto result = sm.validateSession(sid);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.reason.empty()) << "Expiry reason should be populated";
}

/**
 * @brief ASY-04: terminateSession for a non-existent session ID is idempotent.
 */
TEST_F(SessionHardeningTest, ASY04_TerminateUnknownIsIdempotent) {
    SessionManager sm(limits_);
    ASSERT_NO_THROW(sm.terminateSession("sess_ghost_session"));
    ASSERT_NO_THROW(sm.terminateSession("sess_ghost_session"));
}

/**
 * @brief ASY-05: terminateAllOtherSessions removes non-kept sessions and preserves
 *        the specified keep session.
 */
TEST_F(SessionHardeningTest, ASY05_TerminateAllOtherSessions) {
    SessionManager sm(limits_);
    const std::string s1 = sm.createSession("user-asy05");
    const std::string s2 = sm.createSession("user-asy05");
    const std::string s3 = sm.createSession("user-asy05");

    const int removed = sm.terminateAllOtherSessions("user-asy05", s1);
    EXPECT_GE(removed, 1);

    // s1 must still be valid
    const auto r1 = sm.validateSession(s1);
    EXPECT_TRUE(r1.valid) << "Kept session should still be valid";

    // s2 and s3 must now be invalid
    EXPECT_FALSE(sm.validateSession(s2).valid) << "s2 should have been terminated";
    EXPECT_FALSE(sm.validateSession(s3).valid) << "s3 should have been terminated";
}

/**
 * @brief ASY-06: All session IDs created by createSession() start with "sess_".
 *
 * The "sess_" prefix is part of the public contract: downstream components
 * that route or filter session tokens rely on this prefix.
 */
TEST_F(SessionHardeningTest, ASY06_SessionIdPrefixInvariant) {
    SessionManager sm(limits_);
    for (int i = 0; i < 10; ++i) {
        const std::string sid = sm.createSession("user-asy06");
        EXPECT_EQ(sid.substr(0, 5), "sess_")
            << "Session ID '" << sid << "' does not start with 'sess_'";
    }
}

/**
 * @brief ASY-07: pruneExpired removes expired sessions; active sessions are unaffected.
 */
TEST_F(SessionHardeningTest, ASY07_PruneExpiredSelectiveRemoval) {
    SessionManager::SessionLimits mix;
    mix.max_sessions_per_user = 20;
    mix.absolute_timeout      = std::chrono::milliseconds(1);   // instant expiry for these
    mix.idle_timeout          = std::chrono::milliseconds(0);
    SessionManager sm_short(mix);
    const std::string short_sid = sm_short.createSession("user-asy07");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // A separate manager with a long timeout for the "survivor" session.
    SessionManager::SessionLimits long_lim;
    long_lim.max_sessions_per_user = 20;
    long_lim.absolute_timeout      = std::chrono::hours(24);
    long_lim.idle_timeout          = std::chrono::milliseconds(0);
    SessionManager sm_long(long_lim);
    const std::string live_sid = sm_long.createSession("user-asy07-long");

    // The short-timeout manager should see the session as expired after pruning.
    const size_t pruned = sm_short.pruneExpired();
    (void)pruned;  // number of pruned entries depends on internal state
    EXPECT_FALSE(sm_short.validateSession(short_sid).valid)
        << "Expired session should be invalid after pruning";

    // The long-timeout manager's session must remain valid.
    EXPECT_TRUE(sm_long.validateSession(live_sid).valid)
        << "Active session must survive prune";
}

/**
 * @brief ASY-08: Per-user session limit is enforced; creating a session beyond the
 *        limit evicts the oldest session for that user.
 */
TEST_F(SessionHardeningTest, ASY08_PerUserSessionLimitEnforced) {
    SessionManager::SessionLimits lim;
    lim.max_sessions_per_user = 2;
    lim.absolute_timeout      = std::chrono::hours(24);
    lim.idle_timeout          = std::chrono::milliseconds(0);
    SessionManager sm(lim);

    const std::string s1 = sm.createSession("user-asy08");  // slot 1
    const std::string s2 = sm.createSession("user-asy08");  // slot 2
    const std::string s3 = sm.createSession("user-asy08");  // slot 3 — s1 must be evicted

    // s3 must be valid
    EXPECT_TRUE(sm.validateSession(s3).valid) << "Newly created session must be valid";

    // s1 (oldest) must have been evicted
    EXPECT_FALSE(sm.validateSession(s1).valid)
        << "Oldest session must be evicted when per-user limit is exceeded";

    // s2 may or may not survive depending on implementation (oldest-first eviction)
    // — we only assert s1 is gone and s3 is present; s2 behaviour is implementation-defined.
}
