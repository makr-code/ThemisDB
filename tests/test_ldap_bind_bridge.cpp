/**
 * @file test_ldap_bind_bridge.cpp
 * @brief Unit tests for LDAPAuthenticator injectable LdapBindFn bridge (STUB #35).
 *
 * Tests verify the performBind injectable callback slot in the non-libldap
 * stub path:
 *   LDAP-BIND-01  no fn injected → performBind returns Failed (stub default)
 *   LDAP-BIND-02  fn injected → fn is called, its LDAPAuthResult is returned
 *   LDAP-BIND-03  fn throws → performBind returns Failed (fail-closed)
 */

#include <gtest/gtest.h>
#include "auth/ldap_authenticator.h"

#include <stdexcept>

using themis::auth::LDAPAuthResult;
using themis::auth::LDAPAuthenticator;
using themis::auth::LDAPConfig;

namespace {

LDAPConfig makeLdapBridgeConfig() {
    LDAPConfig cfg;
    cfg.server_url = "ldap://example.local:389";
    cfg.bind_dn_template = "CN={username},OU=Users,DC=example,DC=local";
    cfg.base_dn = "DC=example,DC=local";
    return cfg;
}

} // namespace

class LdapBindBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        LDAPAuthenticator::setLdapBindFn({});
        ASSERT_TRUE(auth_.initialize(makeLdapBridgeConfig()));
    }

    void TearDown() override {
        LDAPAuthenticator::setLdapBindFn({});
    }

    LDAPAuthenticator auth_;
};

// ── LDAP-BIND-01 ──────────────────────────────────────────────────────────────
// With no fn injected the fallback path must return Failed.
TEST_F(LdapBindBridgeTest, NoFnReturnsFailed) {
    const auto result = auth_.authenticate("alice", "secret");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ── LDAP-BIND-02 ──────────────────────────────────────────────────────────────
// With fn injected, fn is called and its LDAPAuthResult is returned.
TEST_F(LdapBindBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    LDAPAuthenticator::setLdapBindFn(
        [&](const std::string& username,
            const std::string& dn,
            const std::string& password) -> LDAPAuthResult {
            fn_called = true;
            EXPECT_EQ(username, "alice");
            EXPECT_EQ(dn, "CN=alice,OU=Users,DC=example,DC=local");
            EXPECT_EQ(password, "secret");
            return LDAPAuthResult::Success(username, dn, {"role_admin"});
        });

    const auto result = auth_.authenticate("alice", "secret");
    EXPECT_TRUE(fn_called);
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.roles.size(), 1u);
    EXPECT_EQ(result.roles[0], "role_admin");
}

// ── LDAP-BIND-03 ──────────────────────────────────────────────────────────────
// Throwing callback is fail-closed and does not propagate exceptions.
TEST_F(LdapBindBridgeTest, ThrowingFnIsFailClosed) {
    LDAPAuthenticator::setLdapBindFn(
        [](const std::string&, const std::string&, const std::string&) -> LDAPAuthResult {
            throw std::runtime_error("simulated ldap error");
        });

    LDAPAuthResult result;
    EXPECT_NO_THROW({ result = auth_.authenticate("alice", "secret"); });
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}
