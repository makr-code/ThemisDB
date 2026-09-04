/**
 * @file test_auth_wavec_token_lifecycle.cpp
 * @brief Wave C unit tests — AUTH-Token-01 through AUTH-Token-08
 *
 * Covers session creation, validation, expiry, termination, pruning, and
 * the DistributedTokenBlacklist JTI revocation primitives.
 *
 * Test IDs: AUTH-Token-01 … AUTH-Token-08
 */

#include <gtest/gtest.h>

#include "auth/session_manager.h"
#include "auth/distributed_token_blacklist.h"
#include "auth/auth_error.h"

#include <chrono>
#include <string>
#include <thread>

namespace themis {
namespace auth {
namespace tests {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Return a temporary directory path suitable for a test RocksDB instance.
/// Each test invocation receives its own unique path via the test name.
std::string testDbPath(const std::string& suffix) {
    return "/tmp/themis_wavec_blacklist_" + suffix;
}

/// Build a minimal DistributedBlacklistConfig that uses an isolated RocksDB
/// directory and disables cluster sync (no real TCP peers needed).
DistributedBlacklistConfig makeLocalBlacklistConfig(const std::string& tag) {
    DistributedBlacklistConfig cfg;
    cfg.db_path              = testDbPath(tag);
    cfg.enable_cluster_sync  = false;
    cfg.purge_interval_seconds = 3600;  // no background purge during test
    cfg.local_node.node_id   = "test-node-" + tag;
    return cfg;
}

} // namespace

// ---------------------------------------------------------------------------
// SessionManager fixture
// ---------------------------------------------------------------------------

class TokenLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default limits — generous for most tests.
        mgr_ = std::make_unique<SessionManager>();
    }

    std::unique_ptr<SessionManager> mgr_;
};

// ---------------------------------------------------------------------------
// AUTH-Token-01: SessionManager creates a valid session with non-empty ID
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-01 — createSession() must return a non-empty, "sess_"-
 *        prefixed session identifier for a valid user.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_01_CreateSessionReturnsNonEmptyId) {
    const std::string session_id = mgr_->createSession("user_alice");

    EXPECT_FALSE(session_id.empty()) << "Session ID must not be empty";
    EXPECT_EQ(session_id.substr(0, 5), "sess_")
        << "Session ID must be prefixed with 'sess_'";
}

// ---------------------------------------------------------------------------
// AUTH-Token-02: validateSession returns valid=true for a fresh session
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-02 — A freshly created session must validate as valid
 *        and carry the correct user_id.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_02_ValidateSessionReturnsTrueForFreshSession) {
    const std::string session_id = mgr_->createSession("user_bob");

    const auto result = mgr_->validateSession(session_id);

    EXPECT_TRUE(result.valid) << "Fresh session must be valid; reason: " << result.reason;
    ASSERT_TRUE(result.session.has_value());
    EXPECT_EQ(result.session->user_id, "user_bob");
}

// ---------------------------------------------------------------------------
// AUTH-Token-03: validateSession returns valid=false for expired session
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-03 — A session whose absolute_timeout is exceeded must
 *        be treated as expired (valid=false) on subsequent validation.
 *
 * Strategy: construct a SessionManager with a 1 ms absolute_timeout, create
 * a session, sleep briefly, then validate — it must be expired.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_03_ValidateSessionReturnsFalseForExpired) {
    SessionManager::SessionLimits limits;
    limits.absolute_timeout = std::chrono::milliseconds(1);
    limits.idle_timeout     = std::chrono::milliseconds(0);  // disabled

    SessionManager short_mgr(limits);

    const std::string session_id = short_mgr.createSession("user_carol");

    // Let the 1 ms absolute lifetime elapse.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    const auto result = short_mgr.validateSession(session_id);

    EXPECT_FALSE(result.valid)
        << "Session past absolute_timeout must be invalid";
}

// ---------------------------------------------------------------------------
// AUTH-Token-04: terminateSession makes session invalid
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-04 — After terminateSession() the session must no longer
 *        validate as valid.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_04_TerminateSessionMakesItInvalid) {
    const std::string session_id = mgr_->createSession("user_dave");

    // Confirm it is valid before termination.
    ASSERT_TRUE(mgr_->validateSession(session_id).valid);

    mgr_->terminateSession(session_id);

    const auto result = mgr_->validateSession(session_id);
    EXPECT_FALSE(result.valid) << "Terminated session must not be valid";
}

// ---------------------------------------------------------------------------
// AUTH-Token-05: pruneExpired removes expired sessions
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-05 — pruneExpired() must remove sessions whose absolute
 *        lifetime has elapsed and reduce size() accordingly.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_05_PruneExpiredRemovesDeadSessions) {
    SessionManager::SessionLimits limits;
    limits.absolute_timeout = std::chrono::milliseconds(1);
    limits.idle_timeout     = std::chrono::milliseconds(0);

    SessionManager short_mgr(limits);

    short_mgr.createSession("user_eve");
    short_mgr.createSession("user_frank");

    EXPECT_EQ(short_mgr.size(), 2u);

    // Let both sessions expire.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    const size_t removed = short_mgr.pruneExpired();

    EXPECT_EQ(removed, 2u) << "Both expired sessions should have been pruned";
    EXPECT_EQ(short_mgr.size(), 0u);
}

// ---------------------------------------------------------------------------
// AUTH-Token-06: DistributedTokenBlacklist add() then isRevoked() returns true
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-06 — After add(jti, future_expiry) the blacklist must
 *        return isRevoked(jti) == true for that JTI.
 */
TEST(DistributedTokenBlacklistTest, AUTH_Token_06_AddThenIsRevokedReturnsTrue) {
    DistributedBlacklistConfig cfg = makeLocalBlacklistConfig("tok06");
    DistributedTokenBlacklist blacklist(cfg);

    const std::string jti    = "test-jti-revoked-abc123";
    const auto        expiry = std::chrono::system_clock::now() + std::chrono::hours(1);

    blacklist.add(jti, expiry);

    EXPECT_TRUE(blacklist.isRevoked(jti))
        << "JTI must be revoked immediately after add()";
}

// ---------------------------------------------------------------------------
// AUTH-Token-07: DistributedTokenBlacklist returns false for unknown JTI
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-07 — isRevoked() must return false for a JTI that was
 *        never added to the blacklist.
 */
TEST(DistributedTokenBlacklistTest, AUTH_Token_07_IsRevokedReturnsFalseForUnknownJti) {
    DistributedBlacklistConfig cfg = makeLocalBlacklistConfig("tok07");
    DistributedTokenBlacklist blacklist(cfg);

    EXPECT_FALSE(blacklist.isRevoked("totally-unknown-jti-xyz987"))
        << "Unknown JTI must not be reported as revoked";
}

// ---------------------------------------------------------------------------
// AUTH-Token-08: SessionManager enforces max_sessions_per_user
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-Token-08 — When max_sessions_per_user is exceeded the oldest
 *        session must be evicted so the total for that user stays at the
 *        limit; the new session is always created successfully.
 */
TEST_F(TokenLifecycleTest, AUTH_Token_08_EnforcesMaxSessionsPerUser) {
    constexpr uint32_t kMaxSessions = 3u;

    SessionManager::SessionLimits limits;
    limits.max_sessions_per_user = kMaxSessions;

    SessionManager bounded_mgr(limits);

    // Create one more than the limit.
    std::string last_session_id = {};
    for (uint32_t i = 0; i <= kMaxSessions; ++i) {
        last_session_id = bounded_mgr.createSession("user_grace");
    }

    // The most-recently-created session must still be valid.
    EXPECT_TRUE(bounded_mgr.validateSession(last_session_id).valid)
        << "Newest session must be valid after eviction";

    // Total sessions for the user must not exceed the limit.
    const auto sessions = bounded_mgr.listSessions("user_grace");
    EXPECT_LE(sessions.size(), static_cast<size_t>(kMaxSessions))
        << "Active session count must not exceed max_sessions_per_user";
}

} // namespace tests
} // namespace auth
} // namespace themis
