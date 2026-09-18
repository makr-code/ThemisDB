/**
 * @file ldap_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/auth_error.h"
#include "auth/auth_worker_thread_pool.h"
#include "auth/ldap_connection_pool.h"

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <functional>
#include <future>
#include <functional>

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

struct LDAPConfig {
    // Connection settings
    std::string server_url;          ///< e.g., "ldap://dc.example.com:389" or "ldaps://..."
    std::string server_uri;          ///< Backward-compatible alias for older tests and configs.
    int         port{DEFAULT_LDAP_PORT}; ///< Override port (0 = derive from URL scheme)
    bool        use_tls{false};      ///< Upgrade plain connection with StartTLS
    int         connection_timeout_seconds{DEFAULT_LDAP_TIMEOUT};
    int         search_timeout_seconds{DEFAULT_LDAP_TIMEOUT};

    // User authentication
    std::string bind_dn_template;
    std::string bind_dn;             ///< Backward-compatible alias for legacy tests.
    std::string bind_password;       ///< Legacy compatibility field; not stored on the live config.

    // Optional user-search (resolves group membership)
    bool        enable_group_search{false};
    std::string base_dn;             ///< Search base, e.g., "DC=example,DC=com"
    std::string user_search_filter;
    std::string group_search_base;   ///< Optional separate base for group queries
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

    // -----------------------------------------------------------------------
    // Connection pool settings (used when the pool is enabled)
    // -----------------------------------------------------------------------

    bool pool_enabled{true};

    int pool_min_idle{2};

    int pool_max_size{16};

    int pool_checkout_timeout_ms{5000};
};

// Backward-compatible alias used by the auth hardening tests.
using LDAPAuthenticatorConfig = LDAPConfig;

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

    /**
     * @brief Failed.
     * @param[in] error Input parameter.
     * @return Return value.
     */
    static LDAPAuthResult Failed(const std::string& error)
    {
        LDAPAuthResult r;
        r.success       = false;
        r.error_message = error;
        return r;
    }
};

class LDAPAuthenticator {
public:
    using LdapBindFn = std::function<LDAPAuthResult(const std::string& username,
                                                    const std::string& dn,
                                                    const std::string& password)>;

    /**
     * @brief Set Ldap Bind Fn.
     * @param[in] fn Input parameter.
     */
    static void setLdapBindFn(LdapBindFn fn);

    LDAPAuthenticator();

    explicit LDAPAuthenticator(const LDAPConfig& config) : LDAPAuthenticator() {
        initialize(config);
    }

    ~LDAPAuthenticator();

    // Non-copyable, non-movable
    LDAPAuthenticator(const LDAPAuthenticator&)            = delete;
    LDAPAuthenticator& operator=(const LDAPAuthenticator&) = delete;
    LDAPAuthenticator(LDAPAuthenticator&&)                 = delete;
    LDAPAuthenticator& operator=(LDAPAuthenticator&&)      = delete;

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Initialize.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool initialize(const LDAPConfig& config);

    bool isInitialized() const { return initialized_; }

    /**
     * @brief Authenticate.
     * @param[in] username Input parameter.
     * @param[in] password Input parameter.
     * @return Authentication result.
     */
    LDAPAuthResult authenticate(const std::string& username,
                                const std::string& password);

    /**
     * @brief Authenticate Async.
     * @param[in] username Input parameter.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    std::future<LDAPAuthResult> authenticateAsync(const std::string& username,
                                                   const std::string& password);

    const LDAPConfig& getConfig() const { return config_; }

    const LDAPConnectionPool* connectionPool() const noexcept { return pool_.get(); }

    /**
     * @brief Build User DN.
     * @param[in] username Input parameter.
     * @return Return value.
     */
    std::string buildUserDN(const std::string& username) const;

    std::string buildGroupSearchFilter(const std::string& dn,
                                       const std::string& username = "") const;

    /**
     * @brief Map Groups To Roles.
     * @param[in] groups Input parameter.
     * @return Return value.
     */
    std::vector<std::string> mapGroupsToRoles(
        const std::vector<std::string>& groups) const;

private:
    bool         initialized_{false};
    LDAPConfig   config_;
    utils::AuditLogger* audit_logger_{nullptr}; ///< Non-owning, optional.

    std::unique_ptr<LDAPConnectionPool> pool_;       ///< LDAP connection pool (optional)
    std::unique_ptr<AuthWorkerThreadPool> worker_pool_;

    /**
     * @brief Perform Bind.
     * @param[in] username Input parameter.
     * @param[in] dn Input parameter.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    LDAPAuthResult performBind(const std::string& username,
                               const std::string& dn,
                               const std::string& password);

public:
#ifndef THEMIS_HAS_LDAP
#endif // !THEMIS_HAS_LDAP
};

} // namespace auth
} // namespace themis
