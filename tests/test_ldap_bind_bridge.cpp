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
