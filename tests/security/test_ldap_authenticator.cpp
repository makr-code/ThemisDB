#include <gtest/gtest.h>
#include "auth/ldap_authenticator.h"
#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <algorithm>
#include <future>
#include <chrono>

using namespace themis::auth;

// ===========================================================================
// Helper: build a valid test configuration
// ===========================================================================

namespace {

LDAPConfig makeConfig(bool with_groups = false)
{
    LDAPConfig cfg;
    cfg.server_url        = "ldap://dc.example.com:389";
    cfg.bind_dn_template  = "CN={username},OU=Users,DC=example,DC=com";
    cfg.default_role      = "readonly";

    if (with_groups) {
        cfg.enable_group_search = true;
        cfg.base_dn             = "DC=example,DC=com";
        cfg.user_search_filter  = "(&(objectClass=user)(sAMAccountName={username}))";
        cfg.group_search_filter = "(&(objectClass=group)(member={dn}))";
        cfg.group_attribute     = "cn";
        cfg.group_mappings.push_back({"Admins",   "admin"});
        cfg.group_mappings.push_back({"Engineers", "engineer"});
    }

    return cfg;
}

} // anonymous namespace

// ===========================================================================
// Construction / initialization
// ===========================================================================

TEST(LDAPAuthenticatorTest, DefaultConstructedIsNotInitialized)
{
    LDAPAuthenticator auth;
    EXPECT_FALSE(auth.isInitialized());
}

TEST(LDAPAuthenticatorTest, InitializeSucceedsWithValidConfig)
{
    LDAPAuthenticator auth;
    EXPECT_TRUE(auth.initialize(makeConfig()));
    EXPECT_TRUE(auth.isInitialized());
}

TEST(LDAPAuthenticatorTest, InitializeFailsWhenServerUrlEmpty)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.server_url = "";
    EXPECT_FALSE(auth.initialize(cfg));
    EXPECT_FALSE(auth.isInitialized());
}

TEST(LDAPAuthenticatorTest, InitializeFailsWhenBindDnTemplateEmpty)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.bind_dn_template = "";
    EXPECT_FALSE(auth.initialize(cfg));
}

TEST(LDAPAuthenticatorTest, InitializeFailsWhenGroupSearchEnabledButBaseDnEmpty)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig(/*with_groups=*/true);
    cfg.base_dn = "";
    EXPECT_FALSE(auth.initialize(cfg));
}

// ===========================================================================
// buildUserDN
// ===========================================================================

TEST(LDAPAuthenticatorTest, BuildUserDN_SubstitutesUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("jdoe");
    EXPECT_EQ(dn, "CN=jdoe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_UPNStyleTemplate)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg;
    cfg.server_url       = "ldap://dc.example.com";
    cfg.bind_dn_template = "{username}@EXAMPLE.COM";
    ASSERT_TRUE(auth.initialize(cfg));

    EXPECT_EQ(auth.buildUserDN("alice"), "alice@EXAMPLE.COM");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_NoPlaceholderReturnsTemplateUnchanged)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg;
    cfg.server_url       = "ldap://dc.example.com";
    cfg.bind_dn_template = "CN=service,DC=example,DC=com";
    ASSERT_TRUE(auth.initialize(cfg));

    // No {username} placeholder — template is returned as-is
    EXPECT_EQ(auth.buildUserDN("anyone"), "CN=service,DC=example,DC=com");
}

// ===========================================================================
// mapGroupsToRoles
// ===========================================================================

TEST(LDAPAuthenticatorTest, MapGroupsToRoles_SingleMapping)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig(/*with_groups=*/true)));

    const auto roles = auth.mapGroupsToRoles({"Admins"});
    ASSERT_EQ(roles.size(), 1u);
    EXPECT_EQ(roles[0], "admin");
}

TEST(LDAPAuthenticatorTest, MapGroupsToRoles_MultipleGroupsMapped)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig(/*with_groups=*/true)));

    const auto roles = auth.mapGroupsToRoles({"Admins", "Engineers"});
    EXPECT_EQ(roles.size(), 2u);
    EXPECT_NE(std::find(roles.begin(), roles.end(), "admin"),    roles.end());
    EXPECT_NE(std::find(roles.begin(), roles.end(), "engineer"), roles.end());
}

TEST(LDAPAuthenticatorTest, MapGroupsToRoles_UnknownGroupReturnsDefaultRole)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig(/*with_groups=*/true)));

    const auto roles = auth.mapGroupsToRoles({"UnknownGroup"});
    ASSERT_EQ(roles.size(), 1u);
    EXPECT_EQ(roles[0], "readonly");
}

TEST(LDAPAuthenticatorTest, MapGroupsToRoles_EmptyGroupListReturnsDefaultRole)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig(/*with_groups=*/true)));

    const auto roles = auth.mapGroupsToRoles({});
    ASSERT_EQ(roles.size(), 1u);
    EXPECT_EQ(roles[0], "readonly");
}

TEST(LDAPAuthenticatorTest, MapGroupsToRoles_NoDuplicateRoles)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg;
    cfg.server_url       = "ldap://dc.example.com";
    cfg.bind_dn_template = "{username}@EXAMPLE.COM";
    // Two groups that map to the same role
    cfg.group_mappings.push_back({"GroupA", "admin"});
    cfg.group_mappings.push_back({"GroupB", "admin"});
    ASSERT_TRUE(auth.initialize(cfg));

    const auto roles = auth.mapGroupsToRoles({"GroupA", "GroupB"});
    EXPECT_EQ(roles.size(), 1u);   // deduplication
    EXPECT_EQ(roles[0], "admin");
}

// ===========================================================================
// Input validation in authenticate()
// ===========================================================================

TEST(LDAPAuthenticatorTest, AuthenticateThrowsOnEmptyUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    EXPECT_THROW(
        auth.authenticate("", "password"),
        AuthException
    );
}

TEST(LDAPAuthenticatorTest, AuthenticateThrowsOnEmptyPassword)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    EXPECT_THROW(
        auth.authenticate("jdoe", ""),
        AuthException
    );
}

TEST(LDAPAuthenticatorTest, AuthenticateThrowsWhenUsernameTooLong)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string long_username(MAX_LDAP_USERNAME_LENGTH + 1, 'x');
    EXPECT_THROW(
        auth.authenticate(long_username, "password"),
        AuthException
    );
}

TEST(LDAPAuthenticatorTest, AuthenticateThrowsWhenPasswordTooLong)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string long_password(MAX_LDAP_PASSWORD_LENGTH + 1, 'x');
    EXPECT_THROW(
        auth.authenticate("jdoe", long_password),
        AuthException
    );
}

// ===========================================================================
// Offline failure handling (no LDAP server available)
// ===========================================================================

TEST(LDAPAuthenticatorTest, AuthenticateFailsWhenNotInitialized)
{
    LDAPAuthenticator auth;
    // Do not call initialize()
    const auto result = auth.authenticate("jdoe", "password");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(LDAPAuthenticatorTest, AuthenticateReturnsFailedResultWhenServerUnreachable)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg;
    cfg.server_url       = "ldap://127.0.0.1:39999";  // nothing listening here
    cfg.bind_dn_template = "{username}@EXAMPLE.COM";
    cfg.connection_timeout_seconds = 1;
    cfg.search_timeout_seconds     = 1;
    ASSERT_TRUE(auth.initialize(cfg));

    // The result must be a failure — we just don't assert the exact reason
    // because it varies between platforms (LDAP stub vs real library).
    const auto result = auth.authenticate("jdoe", "secret");
    EXPECT_FALSE(result.success);
}

// ===========================================================================
// LDAPAuthResult helpers
// ===========================================================================

TEST(LDAPAuthResultTest, SuccessFactoryPopulatesFields)
{
    const auto r = LDAPAuthResult::Success(
        "jdoe",
        "CN=jdoe,OU=Users,DC=example,DC=com",
        {"admin", "readonly"},
        {"Admins"}
    );
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.username, "jdoe");
    EXPECT_EQ(r.dn, "CN=jdoe,OU=Users,DC=example,DC=com");
    EXPECT_EQ(r.roles.size(), 2u);
    EXPECT_EQ(r.groups.size(), 1u);
    EXPECT_TRUE(r.error_message.empty());
}

TEST(LDAPAuthResultTest, FailedFactoryPopulatesErrorMessage)
{
    const auto r = LDAPAuthResult::Failed("connection timed out");
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_message, "connection timed out");
    EXPECT_TRUE(r.username.empty());
    EXPECT_TRUE(r.dn.empty());
    EXPECT_TRUE(r.roles.empty());
}

// ===========================================================================
// getConfig
// ===========================================================================

TEST(LDAPAuthenticatorTest, GetConfigReturnsInitializedConfig)
{
    LDAPAuthenticator auth;
    const LDAPConfig cfg = makeConfig();
    ASSERT_TRUE(auth.initialize(cfg));

    EXPECT_EQ(auth.getConfig().server_url, cfg.server_url);
    EXPECT_EQ(auth.getConfig().bind_dn_template, cfg.bind_dn_template);
}

// ===========================================================================
// LDAP DN injection prevention (RFC 4514)
// ===========================================================================

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesCommaInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    // Comma is a DN special character and must be backslash-escaped.
    const std::string dn = auth.buildUserDN("j,doe");
    EXPECT_EQ(dn, "CN=j\\,doe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesPlusInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("j+doe");
    EXPECT_EQ(dn, "CN=j\\+doe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesQuoteInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("j\"doe");
    EXPECT_EQ(dn, "CN=j\\\"doe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesBackslashInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("j\\doe");
    EXPECT_EQ(dn, "CN=j\\\\doe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesAngleBracketsInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("a<b>c");
    EXPECT_EQ(dn, "CN=a\\<b\\>c,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesSemicolonInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("j;doe");
    EXPECT_EQ(dn, "CN=j\\;doe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesLeadingHashInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    // Leading '#' must be escaped; interior '#' is left as-is.
    const std::string dn = auth.buildUserDN("#admin");
    EXPECT_EQ(dn, "CN=\\#admin,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesLeadingSpaceInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN(" jdoe");
    EXPECT_EQ(dn, "CN=\\ jdoe,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesTrailingSpaceInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string dn = auth.buildUserDN("jdoe ");
    EXPECT_EQ(dn, "CN=jdoe\\ ,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_InjectionAttemptWithCommaAndEquals)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    // Classic DN injection: attacker tries to break out and add another RDN.
    // "jdoe,OU=Attackers" should be escaped, not interpreted as a separate RDN.
    const std::string dn = auth.buildUserDN("jdoe,OU=Attackers");
    EXPECT_EQ(dn, "CN=jdoe\\,OU\\=Attackers,OU=Users,DC=example,DC=com");
    // The constructed DN must contain exactly "CN=..." at the start.
    EXPECT_EQ(dn.substr(0, 3), "CN=");
    // And must NOT contain an unescaped "OU=Attackers" component.
    EXPECT_EQ(dn.find(",OU=Attackers"), std::string::npos);
}

TEST(LDAPAuthenticatorTest, BuildUserDN_PlainUsernameUnchanged)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    // Normal alphanumeric username must pass through unmodified.
    EXPECT_EQ(auth.buildUserDN("jdoe123"), "CN=jdoe123,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_EscapesEqualsInUsername)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    // '=' is an RFC 4514 DN special character and must be backslash-escaped.
    const std::string dn = auth.buildUserDN("cn=admin");
    EXPECT_EQ(dn, "CN=cn\\=admin,OU=Users,DC=example,DC=com");
}

TEST(LDAPAuthenticatorTest, BuildUserDN_ReplacesAllUsernamePlaceholdersEscaped)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.bind_dn_template = "CN={username},OU={username},DC=example,DC=com";
    ASSERT_TRUE(auth.initialize(cfg));

    const std::string dn = auth.buildUserDN("jdoe,admins");
    EXPECT_EQ(dn, "CN=jdoe\\,admins,OU=jdoe\\,admins,DC=example,DC=com");
}

// ===========================================================================
// LDAP filter injection prevention (RFC 4515) — buildGroupSearchFilter
// ===========================================================================

TEST(LDAPAuthenticatorTest, BuildGroupSearchFilter_EscapesFilterSpecialCharsInDN)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.enable_group_search    = true;
    cfg.base_dn                = "DC=example,DC=com";
    cfg.group_search_filter    = "(&(objectClass=group)(member={dn}))";
    ASSERT_TRUE(auth.initialize(cfg));

    // A DN containing LDAP filter special chars must be hex-escaped per RFC 4515.
    // '*' → \2a, '(' → \28, ')' → \29, '\' → \5c
    const std::string filter =
        auth.buildGroupSearchFilter("CN=j*doe(evil),DC=example,DC=com");
    EXPECT_EQ(filter,
        "(&(objectClass=group)(member=CN=j\\2adoe\\28evil\\29,DC=example,DC=com))");
    // Must not contain unescaped wildcard in substituted member DN.
    const std::string substituted = filter.substr(filter.find("member=") + 7);
    EXPECT_EQ(substituted.find('*'), std::string::npos);
}

TEST(LDAPAuthenticatorTest, BuildGroupSearchFilter_EscapesMaliciousDNPlaceholder)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.enable_group_search = true;
    cfg.base_dn             = "DC=example,DC=com";
    cfg.group_search_filter = "(&(objectClass=group)(member={dn}))";
    ASSERT_TRUE(auth.initialize(cfg));

    // Mapping for "CN=jdoe)(|(member=*))":
    // ')' -> \29, '(' -> \28, '*' -> \2a, remaining bytes unchanged.
    const std::string filter =
        auth.buildGroupSearchFilter("CN=jdoe)(|(member=*))", "unused-username");
    EXPECT_EQ(filter,
        "(&(objectClass=group)(member=CN=jdoe\\29\\28|\\28member=\\2a\\29\\29))");
    EXPECT_EQ(filter.find("member=*)"), std::string::npos);
}

TEST(LDAPAuthenticatorTest, BuildGroupSearchFilter_EscapesMaliciousUsernamePlaceholder)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg = makeConfig();
    cfg.enable_group_search = true;
    cfg.base_dn             = "DC=example,DC=com";
    cfg.group_search_filter = "(&(objectClass=group)(memberUid={username}))";
    ASSERT_TRUE(auth.initialize(cfg));

    const std::string expected_escaped_filter =
        "(&(objectClass=group)(memberUid=jdoe\\2a\\29\\28|\\28member=\\2a\\29\\29))"; // \2a='*', \29=')', \28='('
    const std::string filter =
        auth.buildGroupSearchFilter("CN=safe,DC=example,DC=com", "jdoe*)(|(member=*))");
    EXPECT_EQ(filter, expected_escaped_filter);
    EXPECT_EQ(filter.find("member=*)"), std::string::npos);
}

/**
 * @brief Integration test — requires an actual LDAP / Active Directory server.
 *
 * To run this test:
 * 1. Set up an OpenLDAP or Active Directory test server.
 * 2. Create a test user, e.g., "testuser" with password "testpass".
 * 3. Build ThemisDB with -DTHEMIS_ENABLE_LDAP=ON.
 * 4. Run: ctest -R LDAPAuthenticator.DISABLED_IntegrationWithLDAPServer
 */
TEST(LDAPAuthenticatorTest, DISABLED_IntegrationWithLDAPServer)
{
    LDAPAuthenticator auth;
    LDAPConfig cfg;
    cfg.server_url             = "ldap://localhost:389";
    cfg.bind_dn_template       = "CN={username},OU=Users,DC=test,DC=local";
    cfg.connection_timeout_seconds = 5;
    cfg.search_timeout_seconds     = 5;

    ASSERT_TRUE(auth.initialize(cfg));

    // Authenticate with valid credentials
    const auto result = auth.authenticate("testuser", "testpass");
    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.username, "testuser");
    EXPECT_FALSE(result.dn.empty());

    // Authenticate with wrong password
    const auto bad_result = auth.authenticate("testuser", "wrongpassword");
    EXPECT_FALSE(bad_result.success);
}

// ===========================================================================
// authenticateAsync() — thread pool dispatch
// ===========================================================================

// authenticateAsync() must throw synchronously for empty username (same as
// authenticate()), because input validation runs on the calling thread.
TEST(LDAPAuthenticatorAsyncTest, ThrowsOnEmptyUsernameSync)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    EXPECT_THROW(
        (void)auth.authenticateAsync("", "password"),
        AuthException
    );
}

// authenticateAsync() must throw synchronously for empty password.
TEST(LDAPAuthenticatorAsyncTest, ThrowsOnEmptyPasswordSync)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    EXPECT_THROW(
        (void)auth.authenticateAsync("user", ""),
        AuthException
    );
}

// authenticateAsync() must throw synchronously for oversized username.
TEST(LDAPAuthenticatorAsyncTest, ThrowsOnOversizedUsernameSync)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeConfig()));

    const std::string long_user(MAX_LDAP_USERNAME_LENGTH + 1, 'a');
    EXPECT_THROW(
        (void)auth.authenticateAsync(long_user, "password"),
        AuthException
    );
}

// authenticateAsync() returns a valid std::future (not initialized) even
// when the authenticator is not initialized — the error is returned through
// the future rather than thrown synchronously.
TEST(LDAPAuthenticatorAsyncTest, ReturnsFailedFutureWhenNotInitialized)
{
    LDAPAuthenticator auth;
    // Do NOT call initialize()

    auto fut = auth.authenticateAsync("jdoe", "secret");
    ASSERT_TRUE(fut.valid());

    // Should be ready quickly (no real LDAP call)
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);

    const auto result = fut.get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Submitting multiple concurrent authenticateAsync() calls must not deadlock.
TEST(LDAPAuthenticatorAsyncTest, ConcurrentCallsDoNotDeadlock)
{
    LDAPAuthenticator auth;
    // Intentionally NOT calling initialize() so all async calls return a
    // fast-fail result without any network I/O.  This exercises the thread
    // pool correctness (no deadlock, all futures become ready) without
    // requiring a real LDAP server or accepting a network timeout.

    constexpr int kTasks = 16;
    std::vector<std::future<LDAPAuthResult>> futures;
    futures.reserve(kTasks);

    for (int i = 0; i < kTasks; ++i) {
        futures.push_back(auth.authenticateAsync("user" + std::to_string(i), "pass"));
    }

    for (auto& f : futures) {
        ASSERT_TRUE(f.valid());
        ASSERT_EQ(f.wait_for(std::chrono::seconds(5)),
                  std::future_status::ready);
        // Not-initialized path returns a failed result immediately (no throw).
        const auto result = f.get();
        EXPECT_FALSE(result.success);
        EXPECT_FALSE(result.error_message.empty());
    }
}
