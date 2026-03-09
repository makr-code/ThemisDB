/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ldap_authenticator.h                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:52:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 79129146f  2026-02-24  feat(auth): implement LDAP/Active Directory direct bind a... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

// Input validation limits for LDAP
constexpr size_t MAX_LDAP_USERNAME_LENGTH = 256;   ///< Maximum username length
constexpr size_t MAX_LDAP_PASSWORD_LENGTH = 512;   ///< Maximum password length
constexpr size_t MAX_LDAP_DN_LENGTH       = 1024;  ///< Maximum Distinguished Name length
constexpr int    DEFAULT_LDAP_PORT        = 389;   ///< Default LDAP port (plain)
constexpr int    DEFAULT_LDAPS_PORT       = 636;   ///< Default LDAPS port (TLS)
constexpr int    DEFAULT_LDAP_TIMEOUT     = 10;    ///< Default connection/search timeout (s)

/**
 * @brief Configuration for LDAP/Active Directory direct bind authentication
 */
struct LDAPConfig {
    // Connection settings
    std::string server_url;          ///< e.g., "ldap://dc.example.com:389" or "ldaps://..."
    int         port{DEFAULT_LDAP_PORT}; ///< Override port (0 = derive from URL scheme)
    bool        use_tls{false};      ///< Upgrade plain connection with StartTLS
    int         connection_timeout_seconds{DEFAULT_LDAP_TIMEOUT};
    int         search_timeout_seconds{DEFAULT_LDAP_TIMEOUT};

    // User authentication
    /// DN template with {username} placeholder, e.g.:
    ///   "CN={username},OU=Users,DC=example,DC=com"
    /// or for AD UPN-style bind: "{username}@EXAMPLE.COM"
    std::string bind_dn_template;

    // Optional user-search (resolves group membership)
    bool        enable_group_search{false};
    std::string base_dn;             ///< Search base, e.g., "DC=example,DC=com"
    /// LDAP filter with {username} placeholder, e.g.:
    ///   "(&(objectClass=user)(sAMAccountName={username}))"
    std::string user_search_filter;
    std::string group_search_base;   ///< Optional separate base for group queries
    /// LDAP filter with {dn} placeholder for group membership lookup
    std::string group_search_filter; ///< e.g., "(&(objectClass=group)(member={dn}))"
    std::string group_attribute{"cn"};  ///< Attribute name for group name

    // Role mapping: LDAP group → ThemisDB role
    struct GroupMapping {
        std::string group_name;  ///< LDAP group name (exact match)
        std::string role;        ///< ThemisDB role to assign
    };
    std::vector<GroupMapping> group_mappings;

    // Fallback role assigned when no group mapping matches
    std::string default_role{"readonly"};
};

/**
 * @brief Result of LDAP direct-bind authentication
 */
struct LDAPAuthResult {
    bool        success{false};
    std::string username;           ///< Authenticated username
    std::string dn;                 ///< Resolved Distinguished Name
    std::vector<std::string> roles; ///< Mapped ThemisDB roles
    std::vector<std::string> groups; ///< LDAP groups the user belongs to
    std::string error_message;

    static LDAPAuthResult Success(const std::string& user,
                                  const std::string& distinguished_name,
                                  const std::vector<std::string>& mapped_roles,
                                  const std::vector<std::string>& ldap_groups = {})
    {
        LDAPAuthResult r;
        r.success = true;
        r.username = user;
        r.dn      = distinguished_name;
        r.roles   = mapped_roles;
        r.groups  = ldap_groups;
        return r;
    }

    static LDAPAuthResult Failed(const std::string& error)
    {
        LDAPAuthResult r;
        r.success       = false;
        r.error_message = error;
        return r;
    }
};

/**
 * @brief LDAP/Active Directory direct-bind authenticator
 *
 * Authenticates users by binding to an LDAP or Active Directory server
 * using the user's credentials directly (username + password).
 *
 * Authentication flow:
 * 1. Build user DN from bind_dn_template by substituting {username}.
 * 2. Open connection to the LDAP server.
 * 3. Optionally upgrade to TLS (StartTLS) when use_tls is true.
 * 4. Bind with the user DN and provided password.
 * 5. Optionally search for the user entry and enumerate group memberships.
 * 6. Map LDAP groups to ThemisDB roles via group_mappings.
 * 7. Return LDAPAuthResult with roles.
 *
 * Platform notes:
 * - Unix/Linux: uses OpenLDAP (libldap) when THEMIS_HAS_LDAP is defined.
 * - Windows: uses the built-in WinLDAP/WLDAP32 when compiled on Windows.
 * - If neither library is available, authenticate() returns a LDAP_NOT_INITIALIZED
 *   error explaining that LDAP support is not compiled in.
 *
 * Security properties:
 * - Username and password lengths are validated before any network I/O.
 * - TLS is strongly recommended for production deployments to prevent
 *   credential interception.
 * - Passwords are never stored or logged.
 *
 * Compliance: NIST SP 800-63B, SOC 2 CC6.1
 */
class LDAPAuthenticator {
public:
    /**
     * @brief Construct an uninitialised authenticator.
     */
    LDAPAuthenticator();

    /**
     * @brief Destructor — releases any open LDAP connections.
     */
    ~LDAPAuthenticator();

    // Non-copyable, non-movable
    LDAPAuthenticator(const LDAPAuthenticator&)            = delete;
    LDAPAuthenticator& operator=(const LDAPAuthenticator&) = delete;
    LDAPAuthenticator(LDAPAuthenticator&&)                 = delete;
    LDAPAuthenticator& operator=(LDAPAuthenticator&&)      = delete;

    /**
     * @brief Attach an AuditLogger to receive LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Initialise with the given LDAP configuration.
     *
     * Validates the configuration (non-empty server_url and bind_dn_template)
     * but does NOT open a network connection — connections are opened per
     * authenticate() call to keep the authenticator stateless.
     *
     * @param config  LDAP configuration
     * @return true on success, false if the configuration is invalid
     */
    bool initialize(const LDAPConfig& config);

    /**
     * @brief Return true if initialize() completed successfully.
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Authenticate a user with username + password via LDAP direct bind.
     *
     * @param username  Plain username (e.g., "jdoe" or "jdoe@EXAMPLE.COM")
     * @param password  User password in plain text (transmitted over TLS)
     * @return LDAPAuthResult — check .success and .roles
     * @throws AuthException on invalid input (oversized username/password)
     */
    LDAPAuthResult authenticate(const std::string& username,
                                const std::string& password);

    /**
     * @brief Return the current configuration (after initialize()).
     */
    const LDAPConfig& getConfig() const { return config_; }

    /**
     * @brief Build a user DN by substituting {username} in the template.
     *
     * Public for unit-testing; callers normally use authenticate().
     *
     * @param username  Username to substitute
     * @return DN string with {username} replaced
     */
    std::string buildUserDN(const std::string& username) const;

    /**
     * @brief Map a list of LDAP group names to ThemisDB roles.
     *
     * Public for unit-testing; callers normally use authenticate().
     *
     * @param groups  List of LDAP group names
     * @return List of ThemisDB roles (may include default_role)
     */
    std::vector<std::string> mapGroupsToRoles(
        const std::vector<std::string>& groups) const;

private:
    bool         initialized_{false};
    LDAPConfig   config_;
    utils::AuditLogger* audit_logger_{nullptr}; ///< Non-owning, optional.

    /**
     * @brief Perform the LDAP bind and optional group search.
     *
     * Called by authenticate() after input validation.
     */
    LDAPAuthResult performBind(const std::string& username,
                               const std::string& dn,
                               const std::string& password);
};

} // namespace auth
} // namespace themis
