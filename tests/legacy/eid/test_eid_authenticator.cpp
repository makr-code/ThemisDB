/*
 * Tests for InMemoryEIDAuthenticator / IEIDAuthenticator
 *
 * Acceptance criteria:
 *   AC-EID-01  isInitialized() returns false before initialize()
 *   AC-EID-02  initialize() returns false when enabled == false
 *   AC-EID-03  initialize() returns false when eid_server_url is empty
 *   AC-EID-04  initialize() returns false when terminal_certificate is empty
 *   AC-EID-05  initialize() returns true with valid config; isInitialized() == true
 *   AC-EID-06  beginAuthSession() returns empty string when not initialized
 *   AC-EID-07  beginAuthSession() returns a non-empty URL when initialized
 *   AC-EID-08  beginAuthSession() URL contains the session ID
 *   AC-EID-09  activeSessions() lists active sessions after beginAuthSession()
 *   AC-EID-10  completeAuthSession() returns failure when not initialized
 *   AC-EID-11  completeAuthSession() returns UNKNOWN_SESSION for unknown session
 *   AC-EID-12  completeAuthSession() returns SAML_INVALID for empty SAML response
 *   AC-EID-13  completeAuthSession() returns success with default identity
 *   AC-EID-14  completeAuthSession() returns pre-configured identity via registerTestIdentity()
 *   AC-EID-15  completeAuthSession() returns pre-configured failure via registerTestFailure()
 *   AC-EID-16  Completed session is removed from activeSessions()
 *   AC-EID-17  revokeSession() removes an active session
 *   AC-EID-18  revokeSession() is a no-op for unknown session
 *   AC-EID-19  EIDIdentity::getAttribute() returns correct value
 *   AC-EID-20  EIDIdentity::getAttribute() returns nullopt for missing attribute
 *   AC-EID-21  EIDIdentity::fullName() returns given + family name
 *   AC-EID-22  EIDIdentity::fullName() handles missing given name
 *   AC-EID-23  EIDIdentity::fullName() handles missing family name
 *   AC-EID-24  EIDIdentity::fullName() returns empty string when both names absent
 *   AC-EID-25  EIDAuthResult::Success() sets success flag and identity
 *   AC-EID-26  EIDAuthResult::Failure() sets error_code and error_message
 *   AC-EID-27  config() returns the current EIDAuthConfig
 *   AC-EID-28  Concurrent beginAuthSession() calls are thread-safe
 *   AC-EID-29  Concurrent completeAuthSession() calls are thread-safe
 *   AC-EID-30  Polymorphic usage via IEIDAuthenticator*
 */

#include <gtest/gtest.h>
#include "auth/eid_authenticator.h"

#include <future>
#include <set>

using namespace themis::auth;

// ── Helpers ───────────────────────────────────────────────────────────────────

static EIDAuthConfig validConfig() {
    EIDAuthConfig c;
    c.enabled              = true;
    c.eid_server_url       = "https://eid-server.example.de/service";
    c.terminal_certificate = "-----BEGIN CERTIFICATE-----\nMIIBtest\n-----END CERTIFICATE-----";
    c.terminal_key_path    = "/etc/themis/eid.key";
    return c;
}

static EIDIdentity makeIdentity(const std::string& transaction_id = "TXN-001") {
    EIDIdentity id;
    id.transaction_id   = transaction_id;
    id.eid_server_id    = "test-server";
    id.assurance        = EIDAssuranceLevel::HIGH;
    id.authenticated_at = std::chrono::system_clock::now();
    id.attributes = {
        {EIDAttributeType::GIVEN_NAMES,  "Max",          true},
        {EIDAttributeType::FAMILY_NAMES, "Mustermann",   true},
        {EIDAttributeType::DATE_OF_BIRTH,"19800101",     true},
        {EIDAttributeType::NATIONALITY,  "DEU",          true},
    };
    return id;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class EIDAuthenticatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(auth_.initialize(validConfig()));
    }

    InMemoryEIDAuthenticator auth_;
};

// ── AC-EID-01 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, IsInitializedFalseBeforeInit) {
    InMemoryEIDAuthenticator auth;
    EXPECT_FALSE(auth.isInitialized());
}

// ── AC-EID-02 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, InitializeReturnsFalseWhenDisabled) {
    InMemoryEIDAuthenticator auth;
    EIDAuthConfig c = validConfig();
    c.enabled = false;
    EXPECT_FALSE(auth.initialize(c));
}

// ── AC-EID-03 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, InitializeReturnsFalseWhenServerUrlEmpty) {
    InMemoryEIDAuthenticator auth;
    EIDAuthConfig c = validConfig();
    c.eid_server_url = "";
    EXPECT_FALSE(auth.initialize(c));
}

// ── AC-EID-04 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, InitializeReturnsFalseWhenCertificateEmpty) {
    InMemoryEIDAuthenticator auth;
    EIDAuthConfig c = validConfig();
    c.terminal_certificate = "";
    EXPECT_FALSE(auth.initialize(c));
}

// ── AC-EID-05 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, InitializeReturnsTrueWithValidConfig) {
    InMemoryEIDAuthenticator auth;
    EXPECT_TRUE(auth.initialize(validConfig()));
    EXPECT_TRUE(auth.isInitialized());
}

// ── AC-EID-06 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, BeginAuthSessionReturnsEmptyWhenNotInit) {
    InMemoryEIDAuthenticator auth;
    EXPECT_TRUE(auth.beginAuthSession("SID-X").empty());
}

// ── AC-EID-07 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, BeginAuthSessionReturnsNonEmptyURL) {
    auto url = auth_.beginAuthSession("SID-001");
    EXPECT_FALSE(url.empty());
}

// ── AC-EID-08 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, BeginAuthSessionURLContainsSessionId) {
    auto url = auth_.beginAuthSession("MY-SESSION-ID");
    EXPECT_NE(url.find("MY-SESSION-ID"), std::string::npos);
}

// ── AC-EID-09 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, ActiveSessionsListsStartedSessions) {
    auth_.beginAuthSession("SESS-A");
    auth_.beginAuthSession("SESS-B");
    auto sessions = auth_.activeSessions();
    EXPECT_EQ(sessions.size(), 2u);
}

// ── AC-EID-10 ────────────────────────────────────────────────────────────────

TEST(EIDAuthenticatorInitTest, CompleteAuthSessionFailsWhenNotInit) {
    InMemoryEIDAuthenticator auth;
    auto r = auth.completeAuthSession("SESS", "response");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_code, EIDAuthErrorCode::INVALID_CONFIGURATION);
}

// ── AC-EID-11 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompleteAuthSessionFailsForUnknownSession) {
    auto r = auth_.completeAuthSession("UNKNOWN-SESSION", "<saml/>");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_code, EIDAuthErrorCode::SESSION_TIMEOUT);
}

// ── AC-EID-12 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompleteAuthSessionFailsForEmptySAMLResponse) {
    auth_.beginAuthSession("SESS-EMPTY");
    auto r = auth_.completeAuthSession("SESS-EMPTY", "");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_code, EIDAuthErrorCode::SAML_SIGNATURE_INVALID);
}

// ── AC-EID-13 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompleteAuthSessionReturnsDefaultIdentity) {
    auth_.beginAuthSession("SESS-DEFAULT");
    auto r = auth_.completeAuthSession("SESS-DEFAULT", "<saml>valid</saml>");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.identity.has_value());
    EXPECT_EQ(r.identity->transaction_id, "SESS-DEFAULT");
}

// ── AC-EID-14 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompleteAuthSessionReturnsPreconfiguredIdentity) {
    auth_.beginAuthSession("SESS-PRE");
    auth_.registerTestIdentity("SESS-PRE", makeIdentity("TXN-PRE"));
    auto r = auth_.completeAuthSession("SESS-PRE", "<saml>ok</saml>");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.identity.has_value());
    EXPECT_EQ(r.identity->transaction_id, "TXN-PRE");
}

// ── AC-EID-15 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompleteAuthSessionReturnsPreconfiguredFailure) {
    auth_.beginAuthSession("SESS-FAIL");
    auth_.registerTestFailure("SESS-FAIL",
                               EIDAuthErrorCode::USER_CANCELLED,
                               "User cancelled");
    auto r = auth_.completeAuthSession("SESS-FAIL", "<saml>ok</saml>");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_code, EIDAuthErrorCode::USER_CANCELLED);
    EXPECT_EQ(r.error_message, "User cancelled");
}

// ── AC-EID-16 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, CompletedSessionRemovedFromActiveSessions) {
    auth_.beginAuthSession("SESS-DONE");
    auth_.completeAuthSession("SESS-DONE", "<saml>ok</saml>");
    auto sessions = auth_.activeSessions();
    EXPECT_TRUE(std::find(sessions.begin(), sessions.end(), "SESS-DONE") ==
                sessions.end());
}

// ── AC-EID-17 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, RevokeSessionRemovesActiveSession) {
    auth_.beginAuthSession("SESS-REV");
    auth_.revokeSession("SESS-REV");
    auto sessions = auth_.activeSessions();
    EXPECT_TRUE(std::find(sessions.begin(), sessions.end(), "SESS-REV") ==
                sessions.end());
}

// ── AC-EID-18 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, RevokeSessionIsNoOpForUnknown) {
    EXPECT_NO_THROW(auth_.revokeSession("GHOST-SESSION"));
}

// ── AC-EID-19 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, GetAttributeReturnsCorrectValue) {
    auto id = makeIdentity();
    auto gn = id.getAttribute(EIDAttributeType::GIVEN_NAMES);
    ASSERT_TRUE(gn.has_value());
    EXPECT_EQ(*gn, "Max");
}

// ── AC-EID-20 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, GetAttributeReturnsNulloptForMissing) {
    EIDIdentity id;
    EXPECT_FALSE(id.getAttribute(EIDAttributeType::GIVEN_NAMES).has_value());
}

// ── AC-EID-21 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, FullNameReturnsCombinedName) {
    auto id = makeIdentity();
    EXPECT_EQ(id.fullName(), "Max Mustermann");
}

// ── AC-EID-22 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, FullNameHandlesMissingGivenName) {
    EIDIdentity id;
    id.attributes = {{EIDAttributeType::FAMILY_NAMES, "Mustermann", true}};
    EXPECT_EQ(id.fullName(), "Mustermann");
}

// ── AC-EID-23 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, FullNameHandlesMissingFamilyName) {
    EIDIdentity id;
    id.attributes = {{EIDAttributeType::GIVEN_NAMES, "Max", true}};
    EXPECT_EQ(id.fullName(), "Max");
}

// ── AC-EID-24 ────────────────────────────────────────────────────────────────

TEST(EIDIdentityTest, FullNameReturnsEmptyWhenBothNamesAbsent) {
    EIDIdentity id;
    EXPECT_EQ(id.fullName(), "");
}

// ── AC-EID-25 ────────────────────────────────────────────────────────────────

TEST(EIDAuthResultTest, SuccessSetsFlagAndIdentity) {
    auto r = EIDAuthResult::Success(makeIdentity());
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.identity.has_value());
}

// ── AC-EID-26 ────────────────────────────────────────────────────────────────

TEST(EIDAuthResultTest, FailureSetsErrorCodeAndMessage) {
    auto r = EIDAuthResult::Failure(EIDAuthErrorCode::EID_SERVER_UNREACHABLE,
                                    "Connection refused");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_code, EIDAuthErrorCode::EID_SERVER_UNREACHABLE);
    EXPECT_EQ(r.error_message, "Connection refused");
}

// ── AC-EID-27 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, ConfigReturnsCurrentConfig) {
    auto cfg = auth_.config();
    EXPECT_EQ(cfg.eid_server_url, "https://eid-server.example.de/service");
    EXPECT_TRUE(cfg.enabled);
}

// ── AC-EID-28 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, ConcurrentBeginAuthSessionIsThreadSafe) {
    constexpr int N = 40;
    std::vector<std::future<std::string>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            return auth_.beginAuthSession("T-" + std::to_string(i));
        }));
    }
    for (auto& f : futs) {
        auto url = f.get();
        EXPECT_FALSE(url.empty());
    }
    EXPECT_EQ(auth_.activeSessions().size(), static_cast<std::size_t>(N));
}

// ── AC-EID-29 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, ConcurrentCompleteIsThreadSafe) {
    constexpr int N = 20;
    for (int i = 0; i < N; ++i) {
        auth_.beginAuthSession("C-" + std::to_string(i));
    }
    std::vector<std::future<EIDAuthResult>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            return auth_.completeAuthSession("C-" + std::to_string(i), "<saml>ok</saml>");
        }));
    }
    int success_count = 0;
    for (auto& f : futs) {
        if (f.get().success) {
          ++success_count;
        }
    }
    EXPECT_EQ(success_count, N);
}

// ── AC-EID-30 ────────────────────────────────────────────────────────────────

TEST_F(EIDAuthenticatorTest, PolymorphicUsageViaInterface) {
    IEIDAuthenticator* auth = &auth_;
    auto begin_result = auth->beginAuthSession("POLY-SESS");
    static_cast<void>(begin_result);
    auto r = auth->completeAuthSession("POLY-SESS", "<saml>ok</saml>");
    EXPECT_TRUE(r.success);
}
