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

using themis::auth::LDAPAuthenticator;
using themis::auth::LDAPAuthResult;

// ── Fixture ───────────────────────────────────────────────────────────────────

class LdapBindBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        LDAPAuthenticator::setLdapBindFn({});
    }
};

// ── LDAP-BIND-01 ──────────────────────────────────────────────────────────────
// With no fn injected the stub returns Failed with the "not compiled in" message.
TEST_F(LdapBindBridgeTest, NoFnReturnsFailed) {
    LDAPAuthenticator::LDAPConfig cfg;
    cfg.server_url  = "ldap://localhost";
    cfg.base_dn     = "dc=example,dc=com";
    cfg.user_dn_template = "uid={username},dc=example,dc=com";
    LDAPAuthenticator authenticator(cfg);

    auto result = authenticator.authenticate("alice", "secret");
    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.error_message, ::testing::HasSubstr("THEMIS_ENABLE_LDAP"));
}

// ── LDAP-BIND-02 ──────────────────────────────────────────────────────────────
// With fn injected, fn is called and its LDAPAuthResult is returned.
TEST_F(LdapBindBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    LDAPAuthenticator::setLdapBindFn(
        [&](const std::string& username,
            const std::string& /*dn*/,
            const std::string& password) -> LDAPAuthResult {
            fn_called = true;
            EXPECT_EQ(username, "alice");
            EXPECT_EQ(password, "secret");
            return LDAPAuthResult::Success(username, {"role_admin"});
        });

    LDAPAuthenticator::LDAPConfig cfg;
    cfg.server_url  = "ldap://localhost";
    cfg.base_dn     = "dc=example,dc=com";
    cfg.user_dn_template = "uid={username},dc=example,dc=com";
    LDAPAuthenticator authenticator(cfg);

    auto result = authenticator.authenticate("alice", "secret");
    EXPECT_TRUE(fn_called);
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.roles.size(), 1u);
    EXPECT_EQ(result.roles[0], "role_admin");
}

// ── LDAP-BIND-03 ──────────────────────────────────────────────────────────────
// When fn throws, performBind returns Failed (fail-closed).
TEST_F(LdapBindBridgeTest, ThrowingFnIsFailClosed) {
    LDAPAuthenticator::setLdapBindFn(
        [](const std::string&, const std::string&, const std::string&)
                -> LDAPAuthResult {
            throw std::runtime_error("simulated ldap error");
        });

    LDAPAuthenticator::LDAPConfig cfg;
    cfg.server_url  = "ldap://localhost";
    cfg.base_dn     = "dc=example,dc=com";
    cfg.user_dn_template = "uid={username},dc=example,dc=com";
    LDAPAuthenticator authenticator(cfg);

    LDAPAuthResult result;
    EXPECT_NO_THROW({ result = authenticator.authenticate("alice", "secret"); });
    EXPECT_FALSE(result.success);
#include <gtest/gtest.h>

#include "auth/ldap_authenticator.h"

using namespace themis::auth;

namespace {

LDAPConfig makeLdapBridgeConfig()
{
    LDAPConfig cfg;
    cfg.server_url = "ldap://example.local:389";
    cfg.bind_dn_template = "CN={username},OU=Users,DC=example,DC=local";
    return cfg;
}

} // namespace

TEST(LDAPBindBridgeTest, InjectedBindFunctionIsUsedByAuthenticate)
{
    LDAPAuthenticator::setLdapBindFn(nullptr);

    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeLdapBridgeConfig()));

    LDAPAuthenticator::setLdapBindFn(
        [](const std::string& username, const std::string& dn, const std::string& password) {
            if (username == "bridge-user" &&
                dn == "CN=bridge-user,OU=Users,DC=example,DC=local" &&
                password == "bridge-pass") {
                return LDAPAuthResult::Success(username, dn, {"admin"});
            }
            return LDAPAuthResult::Failed("unexpected credentials");
        });

    const auto result = auth.authenticate("bridge-user", "bridge-pass");
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.roles.size(), 1u);
    EXPECT_EQ(result.roles[0], "admin");

    LDAPAuthenticator::setLdapBindFn(nullptr);
}
