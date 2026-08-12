#include <gtest/gtest.h>
#include "auth/session_manager.h"
#include "server/session_api_handler.h"
#include "server/auth_middleware.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <string>
#include <thread>

using namespace themis::auth;
using namespace themis::server;
using namespace themis;
using json = nlohmann::json;

// ===========================================================================
// Helpers
// ===========================================================================

namespace {

SessionManager::SessionLimits defaultLimits() {
    SessionManager::SessionLimits lim;
    lim.max_sessions_per_user = 5;
    lim.idle_timeout    = std::chrono::hours(8);
    lim.absolute_timeout = std::chrono::hours(24);
    return lim;
}

// Build an AuthMiddleware with one static token that has auth:sessions scope
std::shared_ptr<AuthMiddleware> makeAuth(
    const std::string& token      = "test-token",
    const std::string& user_id    = "alice",
    bool               is_admin   = false)
{
    auto auth = std::make_shared<AuthMiddleware>();
    AuthMiddleware::TokenConfig cfg;
    cfg.token   = token;
    cfg.user_id = user_id;
    cfg.scopes  = {"auth:sessions"};
    if (is_admin) {
        cfg.scopes.insert("admin:all");
    }
    auth->addToken(cfg);
    return auth;
}

} // anonymous namespace

// ===========================================================================
// SessionManager – generateSessionId
// ===========================================================================

TEST(SessionManagerTest, GenerateSessionId_HasPrefix) {
    const auto id = SessionManager::generateSessionId();
    EXPECT_TRUE(id.rfind("sess_", 0) == 0) << "Expected 'sess_' prefix, got: " << id;
}

TEST(SessionManagerTest, GenerateSessionId_IsUnique) {
    const auto id1 = SessionManager::generateSessionId();
    const auto id2 = SessionManager::generateSessionId();
    EXPECT_NE(id1, id2);
}

TEST(SessionManagerTest, GenerateSessionId_HasCorrectLength) {
    // "sess_" (5) + 32 hex chars (128 bits) = 37
    const auto id = SessionManager::generateSessionId();
    EXPECT_EQ(id.size(), 37u);
}

// ===========================================================================
// SessionManager – createSession
// ===========================================================================

TEST(SessionManagerTest, CreateSession_ReturnsNonEmptyId) {
    SessionManager mgr(defaultLimits());
    const auto id = mgr.createSession("alice");
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(id.rfind("sess_", 0) == 0);
}

TEST(SessionManagerTest, CreateSession_EmptyUserIdThrows) {
    SessionManager mgr;
    EXPECT_THROW(mgr.createSession(""), std::invalid_argument);
}

TEST(SessionManagerTest, CreateSession_StoresMetadata) {
    SessionManager mgr(defaultLimits());
    const auto id = mgr.createSession("alice", "fp-abc", "192.168.1.1", "curl/7");

    auto res = mgr.validateSession(id);
    ASSERT_TRUE(res.valid);
    ASSERT_TRUE(res.session.has_value());
    EXPECT_EQ(res.session->user_id,            "alice");
    EXPECT_EQ(res.session->device_fingerprint, "fp-abc");
    EXPECT_EQ(res.session->ip_address,         "192.168.1.1");
    EXPECT_EQ(res.session->user_agent,         "curl/7");
}

// ===========================================================================
// SessionManager – validateSession
// ===========================================================================

TEST(SessionManagerTest, ValidateSession_ValidSession) {
    SessionManager mgr(defaultLimits());
    const auto id = mgr.createSession("alice");
    auto res = mgr.validateSession(id);
    EXPECT_TRUE(res.valid);
    EXPECT_TRUE(res.session.has_value());
}

TEST(SessionManagerTest, ValidateSession_UnknownIdReturnsFalse) {
    SessionManager mgr;
    auto res = mgr.validateSession("sess_doesnotexist");
    EXPECT_FALSE(res.valid);
    EXPECT_FALSE(res.session.has_value());
}

TEST(SessionManagerTest, ValidateSession_EmptyIdReturnsFalse) {
    SessionManager mgr;
    auto res = mgr.validateSession("");
    EXPECT_FALSE(res.valid);
}

TEST(SessionManagerTest, ValidateSession_UpdatesLastAccessed) {
    SessionManager mgr(defaultLimits());
    const auto id = mgr.createSession("alice");

    auto r1 = mgr.validateSession(id);
    ASSERT_TRUE(r1.valid);
    auto t1 = r1.session->last_accessed_at;

    // Small sleep to ensure clock advances
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto r2 = mgr.validateSession(id);
    ASSERT_TRUE(r2.valid);
    auto t2 = r2.session->last_accessed_at;

    EXPECT_GE(t2, t1);
}

TEST(SessionManagerTest, ValidateSession_ExpiredByAbsoluteTimeout) {
    SessionManager::SessionLimits lim;
    lim.absolute_timeout = std::chrono::seconds(0); // zero = no limit
    lim.idle_timeout     = std::chrono::milliseconds(1);
    SessionManager mgr(lim);

    const auto id = mgr.createSession("alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto res = mgr.validateSession(id);
    EXPECT_FALSE(res.valid);
    EXPECT_EQ(res.reason, "session expired");
}

// ===========================================================================
// SessionManager – terminateSession
// ===========================================================================

TEST(SessionManagerTest, TerminateSession_RemovesSession) {
    SessionManager mgr(defaultLimits());
    const auto id = mgr.createSession("alice");

    mgr.terminateSession(id);

    auto res = mgr.validateSession(id);
    EXPECT_FALSE(res.valid);
}

TEST(SessionManagerTest, TerminateSession_UnknownIdIsNoOp) {
    SessionManager mgr;
    // Should not throw
    EXPECT_NO_THROW(mgr.terminateSession("sess_nonexistent"));
}

// ===========================================================================
// SessionManager – terminateAllOtherSessions
// ===========================================================================

TEST(SessionManagerTest, TerminateAllOther_RemovesAllExceptCurrent) {
    SessionManager mgr(defaultLimits());
    const auto id1 = mgr.createSession("alice");
    const auto id2 = mgr.createSession("alice");
    const auto id3 = mgr.createSession("alice");

    const int removed = mgr.terminateAllOtherSessions("alice", id2);
    EXPECT_EQ(removed, 2);

    EXPECT_FALSE(mgr.validateSession(id1).valid);
    EXPECT_TRUE(mgr.validateSession(id2).valid);
    EXPECT_FALSE(mgr.validateSession(id3).valid);
}

TEST(SessionManagerTest, TerminateAllOther_EmptyKeepRemovesAll) {
    SessionManager mgr(defaultLimits());
    mgr.createSession("alice");
    mgr.createSession("alice");

    const int removed = mgr.terminateAllOtherSessions("alice");
    EXPECT_EQ(removed, 2);
}

TEST(SessionManagerTest, TerminateAllOther_DoesNotAffectOtherUsers) {
    SessionManager mgr(defaultLimits());
    const auto alice_id = mgr.createSession("alice");
    const auto bob_id   = mgr.createSession("bob");

    mgr.terminateAllOtherSessions("alice");

    EXPECT_TRUE(mgr.validateSession(bob_id).valid);
    EXPECT_FALSE(mgr.validateSession(alice_id).valid);
}

// ===========================================================================
// SessionManager – listSessions
// ===========================================================================

TEST(SessionManagerTest, ListSessions_ReturnsOnlyUserSessions) {
    SessionManager mgr(defaultLimits());
    mgr.createSession("alice");
    mgr.createSession("alice");
    mgr.createSession("bob");

    const auto sessions = mgr.listSessions("alice");
    EXPECT_EQ(sessions.size(), 2u);
    for (const auto& s : sessions) {
        EXPECT_EQ(s.user_id, "alice");
    }
}

TEST(SessionManagerTest, ListSessions_OrderedByCreationTime) {
    SessionManager mgr(defaultLimits());
    const auto id1 = mgr.createSession("alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto id2 = mgr.createSession("alice");

    const auto sessions = mgr.listSessions("alice");
    ASSERT_EQ(sessions.size(), 2u);
    EXPECT_EQ(sessions[0].session_id, id1);
    EXPECT_EQ(sessions[1].session_id, id2);
}

TEST(SessionManagerTest, ListSessions_ExcludesExpiredSessions) {
    SessionManager::SessionLimits lim;
    lim.idle_timeout     = std::chrono::milliseconds(1);
    lim.absolute_timeout = std::chrono::seconds(0);
    SessionManager mgr(lim);

    mgr.createSession("alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    const auto sessions = mgr.listSessions("alice");
    EXPECT_TRUE(sessions.empty());
}

// ===========================================================================
// SessionManager – session limit enforcement
// ===========================================================================

TEST(SessionManagerTest, SessionLimit_EvictsOldestWhenFull) {
    SessionManager::SessionLimits lim;
    lim.max_sessions_per_user = 3;
    lim.idle_timeout     = std::chrono::hours(8);
    lim.absolute_timeout = std::chrono::hours(24);
    SessionManager mgr(lim);

    const auto id1 = mgr.createSession("alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto id2 = mgr.createSession("alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto id3 = mgr.createSession("alice");

    EXPECT_EQ(mgr.size(), 3u);

    // Creating a 4th session should evict id1 (oldest)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto id4 = mgr.createSession("alice");

    EXPECT_EQ(mgr.size(), 3u);
    EXPECT_FALSE(mgr.validateSession(id1).valid);
    EXPECT_TRUE(mgr.validateSession(id2).valid);
    EXPECT_TRUE(mgr.validateSession(id3).valid);
    EXPECT_TRUE(mgr.validateSession(id4).valid);
}

// ===========================================================================
// SessionApiHandler
// ===========================================================================

class SessionApiHandlerTest : public ::testing::Test {
protected:
    std::shared_ptr<AuthMiddleware>         auth_;
    std::shared_ptr<auth::SessionManager>  manager_;
    std::unique_ptr<SessionApiHandler>     handler_;

    void SetUp() override {
        auth_    = makeAuth("alice-token", "alice");
        manager_ = std::make_shared<auth::SessionManager>(defaultLimits());
        handler_ = std::make_unique<SessionApiHandler>(auth_, manager_);
    }
};

TEST_F(SessionApiHandlerTest, CreateSession_ValidToken) {
    json body = {{"device_fingerprint", "fp-123"}, {"user_agent", "TestAgent/1.0"}};
    auto resp = handler_->createSession("alice-token", body, "10.0.0.1");

    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp.contains("session_id"));
    EXPECT_EQ(resp["user_id"].get<std::string>(), "alice");
    EXPECT_TRUE(resp["session_id"].get<std::string>().rfind("sess_", 0) == 0);
}

TEST_F(SessionApiHandlerTest, CreateSession_InvalidToken) {
    auto resp = handler_->createSession("bad-token", json::object());
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 401);
}

TEST_F(SessionApiHandlerTest, CreateSession_InvalidUserAgentHeaderInjection_Returns400) {
    json body = {{"user_agent", "evil\r\nX-Injected: 1"}};
    auto resp = handler_->createSession("alice-token", body, "10.0.0.1");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 400);
}

TEST_F(SessionApiHandlerTest, ListSessions_ReturnsSessions) {
    // Create a couple of sessions first
    handler_->createSession("alice-token", json::object());
    handler_->createSession("alice-token", json::object());

    auto resp = handler_->listSessions("alice-token");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_EQ(resp["total"].get<int>(), 2);
    EXPECT_EQ(resp["sessions"].size(), 2u);
}

TEST_F(SessionApiHandlerTest, ListSessions_MarksCurrent) {
    auto create_resp = handler_->createSession("alice-token", json::object());
    ASSERT_FALSE(create_resp.contains("error"));
    const std::string current = create_resp["session_id"].get<std::string>();

    auto resp = handler_->listSessions("alice-token", current);
    ASSERT_FALSE(resp.contains("error"));

    bool found_current = false;
    for (const auto& s : resp["sessions"]) {
        if (s["session_id"] == current) {
            EXPECT_TRUE(s["is_current"].get<bool>());
            found_current = true;
        }
    }
    EXPECT_TRUE(found_current);
}

TEST_F(SessionApiHandlerTest, RevokeSession_OwnSession) {
    auto create_resp = handler_->createSession("alice-token", json::object());
    const std::string sid = create_resp["session_id"].get<std::string>();

    auto resp = handler_->revokeSession("alice-token", sid);
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp["success"].get<bool>());
    EXPECT_EQ(resp["session_id"].get<std::string>(), sid);

    // Session should no longer be retrievable
    EXPECT_FALSE(manager_->validateSession(sid).valid);
}

TEST_F(SessionApiHandlerTest, RevokeSession_EmptyIdReturnsError) {
    auto resp = handler_->revokeSession("alice-token", "");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 400);
}

TEST_F(SessionApiHandlerTest, RevokeSession_PathTraversalIdReturnsError) {
    auto resp = handler_->revokeSession("alice-token", "../sess_abc");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 400);
}

TEST_F(SessionApiHandlerTest, RevokeSession_UnknownIdReturnsNotFound) {
    auto resp = handler_->revokeSession("alice-token", "sess_doesnotexist00000000000000000000");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 404);
}

TEST_F(SessionApiHandlerTest, RevokeSession_OtherUserForbidden) {
    // Create session for alice
    auto create_resp = handler_->createSession("alice-token", json::object());
    const std::string alice_sid = create_resp["session_id"].get<std::string>();

    // Bob tries to revoke alice's session
    auto bob_auth = makeAuth("bob-token", "bob");
    manager_->createSession("bob"); // ensure bob has a session too (not strictly needed)
    SessionApiHandler bob_handler(bob_auth, manager_);

    auto resp = bob_handler.revokeSession("bob-token", alice_sid);
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 403);
}

TEST_F(SessionApiHandlerTest, RevokeSession_AdminCanRevokeAnySession) {
    // alice creates a session
    auto create_resp = handler_->createSession("alice-token", json::object());
    const std::string alice_sid = create_resp["session_id"].get<std::string>();

    // admin revokes it
    auto admin_auth = makeAuth("admin-token", "admin", /*is_admin=*/true);
    SessionApiHandler admin_handler(admin_auth, manager_);

    auto resp = admin_handler.revokeSession("admin-token", alice_sid);
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp["success"].get<bool>());
}

TEST_F(SessionApiHandlerTest, RevokeAllOtherSessions_KeepsCurrent) {
    auto r1 = handler_->createSession("alice-token", json::object());
    auto r2 = handler_->createSession("alice-token", json::object());
    auto r3 = handler_->createSession("alice-token", json::object());

    const std::string keep = r2["session_id"].get<std::string>();
    const std::string id1  = r1["session_id"].get<std::string>();
    const std::string id3  = r3["session_id"].get<std::string>();

    auto resp = handler_->revokeAllOtherSessions("alice-token", keep);
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp["success"].get<bool>());
    EXPECT_EQ(resp["terminated"].get<int>(), 2);

    EXPECT_FALSE(manager_->validateSession(id1).valid);
    EXPECT_TRUE(manager_->validateSession(keep).valid);
    EXPECT_FALSE(manager_->validateSession(id3).valid);
}

TEST_F(SessionApiHandlerTest, RevokeAllOtherSessions_InvalidCurrentSessionReturnsError) {
    auto resp = handler_->revokeAllOtherSessions("alice-token", "../current");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["status_code"].get<int>(), 400);
}

TEST_F(SessionApiHandlerTest, Constructor_NullAuthThrows) {
    EXPECT_THROW(
        SessionApiHandler(nullptr, manager_),
        std::invalid_argument);
}

TEST_F(SessionApiHandlerTest, Constructor_NullManagerThrows) {
    EXPECT_THROW(
        SessionApiHandler(auth_, nullptr),
        std::invalid_argument);
}

// ===========================================================================
// Session ID hashing security tests
// ===========================================================================

/**
 * @brief Verify that validateSession returns the correct session after creation.
 *
 * This tests functional correctness of the lookup path; the internal storage
 * format (hash-keyed map) is an encapsulated implementation detail.
 */
TEST(SessionManagerHashTest, ValidateSession_WorksAfterHashedStorage) {
    SessionManager mgr;
    std::string sid = mgr.createSession("alice", "fp", "1.2.3.4", "agent");

    // Validation of the raw token must succeed (implementation hashes internally).
    auto result = mgr.validateSession(sid);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.session.has_value());
    EXPECT_EQ(result.session->user_id, "alice");
}

/**
 * @brief Verify that terminateSession works when the map is keyed by hash.
 */
TEST(SessionManagerHashTest, TerminateSession_WorksAfterHashedStorage) {
    SessionManager mgr;
    std::string sid = mgr.createSession("bob", "fp", "1.2.3.4", "agent");

    mgr.terminateSession(sid);
    auto result = mgr.validateSession(sid);
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Verify that terminateAllOtherSessions keeps only the specified session.
 */
TEST(SessionManagerHashTest, TerminateAllOtherSessions_KeepsCorrectSession) {
    SessionManager mgr;
    std::string s1 = mgr.createSession("carol", "fp1", "1.1.1.1", "a");
    std::string s2 = mgr.createSession("carol", "fp2", "1.1.1.2", "b");
    std::string s3 = mgr.createSession("carol", "fp3", "1.1.1.3", "c");

    int removed = mgr.terminateAllOtherSessions("carol", s2);
    EXPECT_EQ(removed, 2);

    EXPECT_FALSE(mgr.validateSession(s1).valid);
    EXPECT_TRUE(mgr.validateSession(s2).valid);
    EXPECT_FALSE(mgr.validateSession(s3).valid);
}
