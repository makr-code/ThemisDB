/**
 * @file ldap_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=16; TODO=1, Stub=12, Unimpl=0, Mock=1, Sim=2, Debt=0, C=2, H=8, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/ldap_authenticator.h"
#include <stdexcept>
#include "auth/auth_audit_logger.h"
#include "auth/ldap_connection_pool.h"
#include "utils/audit_logger.h"
#include "security/pii_redaction_policy.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <future>
#include <mutex>
#include <utility>

// ---------------------------------------------------------------------------
// Platform-specific LDAP includes
// ---------------------------------------------------------------------------

#ifdef _WIN32
#  include <windows.h>
#  include <winldap.h>
#  define THEMIS_HAS_LDAP 1
#elif defined(THEMIS_HAS_LDAP) && THEMIS_HAS_LDAP
#  include <ldap.h>
#  include <lber.h>
#endif

namespace themis {
namespace auth {

// ===========================================================================
// LDAP injection-prevention helpers (file-internal)
// ===========================================================================

namespace {

void auditLDAPValidationFailure(themis::utils::AuditLogger* audit_logger,
                                const std::string& username,
                                const std::string& reason)
{
    AuthAuditLogger audit(audit_logger);
    audit.logLDAPFailure(username, reason);
}

void validateLDAPCredentialsOrThrow(themis::utils::AuditLogger* audit_logger,
                                    const std::string& username,
                                    const std::string& password)
{
    if (username.empty()) {
        auditLDAPValidationFailure(audit_logger, username, "empty_username");
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: username must not be empty"
        ));
    }
    if (username.size() > MAX_LDAP_USERNAME_LENGTH) {
        auditLDAPValidationFailure(audit_logger, username, "username_too_long");
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: username exceeds maximum length"
        ));
    }
    if (password.empty()) {
        auditLDAPValidationFailure(audit_logger, username, "empty_password");
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: password must not be empty (anonymous bind not permitted)"
        ));
    }
    if (password.size() > MAX_LDAP_PASSWORD_LENGTH) {
        auditLDAPValidationFailure(audit_logger, username, "password_too_long");
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: password exceeds maximum length"
        ));
    }
}

std::mutex& ldapBindFnMutex()
{
    static std::mutex mutex;
    return mutex;
}

LDAPAuthenticator::LdapBindFn& ldapBindFnStorage()
{
    static LDAPAuthenticator::LdapBindFn callback;
    return callback;
}

LDAPAuthenticator::LdapBindFn getLdapBindFn()
{
    std::lock_guard<std::mutex> lock(ldapBindFnMutex());
    return ldapBindFnStorage();
}

/**
 * @brief Escape a value for use as a DN attribute value component (RFC 4514 §2.4).
 *
 * The following characters are escaped with a preceding backslash:
 *   , + " \ < > ; = (always)
 *   # (only when leading)
 *   space (only when leading or trailing)
 * NUL bytes are encoded as the two-char hex sequence \00.
 * All other characters are left unmodified.
 */
std::string escapeLDAPDNComponent(const std::string& value)
{
    if (value.empty()) {
        return value;
    }

    std::string out;
    out.reserve(value.size() * 2);

    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);

        // Leading '#' must be escaped
        if (i == 0 && c == '#') {
            out += "\\#";
            continue;
        }

        // Leading or trailing space must be escaped
        if (c == ' ' && (i == 0 || i == value.size() - 1)) {
            out += "\\ ";
            continue;
        }

        // Always-escaped DN special characters per RFC 4514
        switch (c) {
            case ',':  out += "\\,";  break;
            case '+':  out += "\\+";  break;
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '<':  out += "\\<";  break;
            case '>':  out += "\\>";  break;
            case ';':  out += "\\;";  break;
            case '=':  out += "\\=";  break;
            // NUL would truncate c_str() passed to LDAP C APIs; encode it.
            case '\0': out += "\\00"; break;
            default:   out += static_cast<char>(c); break;
        }
    }

    return out;
}

/**
 * @brief Escape a value for use inside an LDAP search filter (RFC 4515 §3).
 *
 * The following bytes are backslash-hex escaped as \XX for LDAP filters:
 *   * ( ) \ NUL
 */
std::string escapeLDAPFilterValue(const std::string& value)
{
    std::string out;
    out.reserve(value.size() * 3);

    for (const unsigned char c : value) {
        switch (c) {
            case '*':  out += "\\2a"; break;
            case '(':  out += "\\28"; break;
            case ')':  out += "\\29"; break;
            case '\\': out += "\\5c"; break;
            case '\0': out += "\\00"; break;
            default:   out += static_cast<char>(c); break;
        }
    }

    return out;
}

/**
 * @brief Replace all occurrences of a placeholder token in a template string.
 *
 * @param target       Template string to mutate in-place.
 * @param placeholder  Placeholder token to replace (e.g. "{username}").
 * @param value        Replacement value that is already escaped for LDAP use.
 *
 * @note Security contract: this helper performs substitution only.
 *       It does NOT escape @p value; callers must pre-escape according to the
 *       target context (RFC 4514 for DN values, RFC 4515 for filter values).
 * @warning Passing unescaped user-controlled data to @p value would reintroduce
 *          LDAP injection risk.
 */
void substitutePreEscapedPlaceholderValue(std::string& target,
                                          const std::string& placeholder,
                                          const std::string& value)
{
    // SECURITY: this helper performs pure template replacement only.
    // It does NOT escape values; callers must pass values that were already
    // escaped for the target LDAP context (RFC 4514 for DN, RFC 4515 for filter).
    std::size_t pos = 0;
    while ((pos = target.find(placeholder, pos)) != std::string::npos) {
        target.replace(pos, placeholder.size(), value);
        pos += value.size();
    }
}

} // anonymous namespace

void LDAPAuthenticator::setLdapBindFn(LdapBindFn fn)
{
    std::lock_guard<std::mutex> lock(ldapBindFnMutex());
    ldapBindFnStorage() = std::move(fn);
}

// ===========================================================================
// Construction / destruction
// ===========================================================================

LDAPAuthenticator::LDAPAuthenticator()
    : worker_pool_(std::make_unique<AuthWorkerThreadPool>(
          AuthWorkerThreadPool::kMinThreads,
          AuthWorkerThreadPool::kMaxThreads))
{}

LDAPAuthenticator::~LDAPAuthenticator() = default;

// ===========================================================================
// Initialization
// ===========================================================================

bool LDAPAuthenticator::initialize(const LDAPConfig& config)
{
    if (config.server_url.empty()) {
        spdlog::error("LDAPAuthenticator: server_url must not be empty");
        return false;
    }
    if (config.bind_dn_template.empty()) {
        spdlog::error("LDAPAuthenticator: bind_dn_template must not be empty");
        return false;
    }
    if (config.enable_group_search && config.base_dn.empty()) {
        spdlog::error("LDAPAuthenticator: base_dn is required when enable_group_search=true");
        return false;
    }

    config_      = config;
    initialized_ = true;

    // Create the connection pool if it is enabled.
    if (config_.pool_enabled) {
        LDAPPoolConfig pool_cfg;
        pool_cfg.server_url                 = config_.server_url;
        pool_cfg.port                       = config_.port;
        pool_cfg.use_tls                    = config_.use_tls;
        pool_cfg.connection_timeout_seconds = config_.connection_timeout_seconds;
        pool_cfg.search_timeout_seconds     = config_.search_timeout_seconds;
        pool_cfg.min_idle                   = config_.pool_min_idle;
        pool_cfg.max_size                   = config_.pool_max_size;
        pool_cfg.checkout_timeout_ms        = config_.pool_checkout_timeout_ms;

        pool_ = std::make_unique<LDAPConnectionPool>(pool_cfg);
    }

    spdlog::info("LDAPAuthenticator: initialized for server {} (pool={})",
                 config_.server_url,
                 config_.pool_enabled ? "enabled" : "disabled");
    return true;
}

// ===========================================================================
// Public helpers
// ===========================================================================

std::string LDAPAuthenticator::buildUserDN(const std::string& username) const
{
    std::string dn = config_.bind_dn_template;
    const std::string escaped_username = escapeLDAPDNComponent(username);
    substitutePreEscapedPlaceholderValue(dn, "{username}", escaped_username);
    return dn;
}

std::vector<std::string> LDAPAuthenticator::mapGroupsToRoles(
    const std::vector<std::string>& groups) const
{
    std::vector<std::string> roles;

    for (const auto& group : groups) {
        for (const auto& mapping : config_.group_mappings) {
            if (mapping.group_name == group) {
                // Avoid duplicate roles
                if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
                    roles.push_back(mapping.role);
                }
            }
        }
    }

    // Apply default role if no mapping matched
    if (roles.empty() && !config_.default_role.empty()) {
        roles.push_back(config_.default_role);
    }

    return roles;
}

std::string LDAPAuthenticator::buildGroupSearchFilter(const std::string& dn,
                                                      const std::string& username) const
{
    std::string filter = config_.group_search_filter;
    substitutePreEscapedPlaceholderValue(filter, "{dn}", escapeLDAPFilterValue(dn));
    substitutePreEscapedPlaceholderValue(filter, "{username}", escapeLDAPFilterValue(username));
    return filter;
}

// ===========================================================================
// Authentication
// ===========================================================================

LDAPAuthResult LDAPAuthenticator::authenticate(const std::string& username,
                                               const std::string& password)
{
    validateLDAPCredentialsOrThrow(audit_logger_, username, password);

    if (!initialized_) {
        AuthAuditLogger audit(audit_logger_);
        audit.logLDAPFailure(username, "not_initialized");
        return LDAPAuthResult::Failed("LDAP authenticator not initialized");
    }

    const std::string dn = buildUserDN(username);
    if (dn.size() > MAX_LDAP_DN_LENGTH) {
        AuthAuditLogger audit(audit_logger_);
        audit.logLDAPFailure(username, "dn_too_long");
        return LDAPAuthResult::Failed("Constructed DN exceeds maximum length");
    }

    return performBind(username, dn, password);
}

std::future<LDAPAuthResult> LDAPAuthenticator::authenticateAsync(
    const std::string& username,
    const std::string& password)
{
    // Validate inputs synchronously on the caller's thread to give fast
    // feedback for invalid arguments — no need to dispatch to the pool.
    validateLDAPCredentialsOrThrow(audit_logger_, username, password);

    // Dispatch the blocking LDAP bind to the worker pool so the caller is
    // never stalled by network latency (P99 ≤ 50 ms goal from the roadmap).
    return worker_pool_->submit(
        [this, username, password]() mutable {
            return this->authenticate(username, password);
        });
}

// ===========================================================================
// Platform-specific bind implementation
// ===========================================================================

#if defined(THEMIS_HAS_LDAP) && THEMIS_HAS_LDAP && defined(_WIN32)
// ---------------------------------------------------------------------------
// Windows WinLDAP implementation
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& dn,
                                              const std::string& password)
{
    if (auto bind_fn = getLdapBindFn(); bind_fn) {
        return bind_fn(username, dn, password);
    }

    AuthAuditLogger audit(audit_logger_);

    // -----------------------------------------------------------------------
    // Obtain an LDAP connection — from the pool if available, otherwise open
    // a new per-call connection.
    // -----------------------------------------------------------------------

    std::unique_ptr<PooledConnection> pooled_conn;
    LDAP* ld = nullptr;
    bool  owns_connection = false;

    if (pool_) {
        pooled_conn = pool_->checkout();
        if (pooled_conn) {
            ld = pooled_conn->rawHandle();
        }
    }

    if (ld) {
        // Set search time limit (seconds)
        ULONG timelimit = static_cast<ULONG>(config_.search_timeout_seconds);
        const ULONG timelimit_result = ldap_set_option(ld, LDAP_OPT_TIMELIMIT, static_cast<void*>(&timelimit));
        if (timelimit_result != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: failed to set LDAP search time limit: {}", timelimit_result);
        }

        // Disable referral chasing — following attacker-controlled referrals can
        // redirect authentication to a rogue LDAP server.
        const ULONG referrals_result =
            ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
        if (referrals_result != LDAP_SUCCESS) {
            spdlog::error("LDAPAuthenticator: failed to disable LDAP referrals: {}",
                          referrals_result);
            pooled_conn->markStale();
            audit.logLDAPFailure(username, "disable_referrals_failed");
            return LDAPAuthResult::Failed("LDAP configuration error (referrals still enabled)");
        }
    }

    if (!ld) {
        // ldap_init is deprecated in newer SDKs but still universally available
        ld = ldap_init(
            const_cast<PCHAR>(config_.server_url.c_str()),
            config_.port > 0 ? static_cast<ULONG>(config_.port) : DEFAULT_LDAP_PORT
        );
        if (!ld) {
            const std::string msg = "ldap_init failed";
            spdlog::error("LDAPAuthenticator: {}", msg);
            audit.logLDAPFailure(username, msg);
            return LDAPAuthResult::Failed("LDAP connection failed");
        }
        owns_connection = true;

        // Set search time limit (seconds)
        ULONG timelimit = static_cast<ULONG>(config_.search_timeout_seconds);
        const ULONG timelimit2_result = ldap_set_option(ld, LDAP_OPT_TIMELIMIT, static_cast<void*>(&timelimit));
        if (timelimit2_result != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: failed to set LDAP search time limit: {}", timelimit2_result);
        }

        // Disable referral chasing
        const ULONG referrals_result =
            ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
        if (referrals_result != LDAP_SUCCESS) {
            spdlog::error("LDAPAuthenticator: failed to disable LDAP referrals: {}",
                          referrals_result);
            ldap_unbind(ld);
            audit.logLDAPFailure(username, "disable_referrals_failed");
            return LDAPAuthResult::Failed("LDAP configuration error (referrals still enabled)");
        }

        if (config_.use_tls) {
            const ULONG tls_result = ldap_start_tls_s(ld, nullptr, nullptr, nullptr, nullptr);
            if (tls_result != LDAP_SUCCESS) {
                spdlog::error("LDAPAuthenticator: StartTLS failed: {}", tls_result);
                ldap_unbind(ld);
                audit.logLDAPFailure(username, "tls_failed");
                return LDAPAuthResult::Failed("LDAP TLS negotiation failed");
            }
        }
    }

    // Simple bind with user DN + password
    const ULONG bind_result = ldap_simple_bind_s(
        ld,
        const_cast<PCHAR>(dn.c_str()),
        const_cast<PCHAR>(password.c_str())
    );

    if (bind_result != LDAP_SUCCESS) {
        spdlog::warn("LDAPAuthenticator: bind failed for DN '{}': {}",
                     dn, bind_result);
        if (pooled_conn) {
            pooled_conn->markStale();
        } else if (owns_connection) {
            ldap_unbind(ld);
        }
        audit.logLDAPFailure(username, "bind_failed");
        return LDAPAuthResult::Failed("LDAP bind failed: invalid credentials");
    }

    // Optional group search
    std::vector<std::string> groups;
    if (config_.enable_group_search && !config_.group_search_filter.empty()) {
        // Build group filter with placeholders substituted and RFC 4515-escaped.
        const std::string filter = buildGroupSearchFilter(dn, username);

        const std::string search_base =
            config_.group_search_base.empty()
                ? config_.base_dn
                : config_.group_search_base;

        PCHAR attrs[] = {
            const_cast<PCHAR>(config_.group_attribute.c_str()),
            nullptr
        };
        LDAPMessage* result = nullptr;
        const ULONG search_result = ldap_search_s(
            ld,
            const_cast<PCHAR>(search_base.c_str()),
            LDAP_SCOPE_SUBTREE,
            const_cast<PCHAR>(filter.c_str()),
            attrs,
            0,
            &result
        );

        if (search_result == LDAP_SUCCESS && result) {
            for (LDAPMessage* entry = ldap_first_entry(ld, result);
                 entry != nullptr;
                 entry = ldap_next_entry(ld, entry))
            {
                PCHAR* vals = ldap_get_values(
                    ld, entry,
                    const_cast<PCHAR>(config_.group_attribute.c_str()));
                if (vals) {
                    for (int i = 0; vals[i] != nullptr; ++i) {
                        groups.emplace_back(vals[i]);
                    }
                    ldap_value_free(vals);
                }
            }
            ldap_msgfree(result);
        }
    }

    if (owns_connection) {
        ldap_unbind(ld);
    }
    // pooled_conn destructor returns the connection to the pool.

    const auto roles = mapGroupsToRoles(groups);
    audit.logLDAPSuccess(username, dn);
    spdlog::info("LDAPAuthenticator: user '{}' authenticated successfully",
                 themis::security::PIIRedactionPolicy::get().redactForLog(username));
    return LDAPAuthResult::Success(username, dn, roles, groups);
}

#elif defined(THEMIS_HAS_LDAP) && THEMIS_HAS_LDAP
// ---------------------------------------------------------------------------
// Unix / OpenLDAP implementation
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& dn,
                                              const std::string& password)
{
    if (auto bind_fn = getLdapBindFn(); bind_fn) {
        return bind_fn(username, dn, password);
    }

    AuthAuditLogger audit(audit_logger_);

    // -----------------------------------------------------------------------
    // Obtain an LDAP connection — from the pool if available, otherwise open
    // a new per-call connection (pool-disabled or pool exhausted path).
    // -----------------------------------------------------------------------

    std::unique_ptr<PooledConnection> pooled_conn;
    LDAP* ld = nullptr;
    bool  owns_connection = false;  // true when we must unbind on exit

    if (pool_) {
        pooled_conn = pool_->checkout();
        if (pooled_conn) {
            ld = pooled_conn->rawHandle();
        }
    }

    if (!ld) {
        // Fall back to a fresh per-call connection (pool disabled, exhausted,
        // or LDAP not compiled in path will hit the #else stub instead).
        int rc2 = ldap_initialize(&ld, config_.server_url.c_str());
        if (rc2 != LDAP_SUCCESS || !ld) {
            spdlog::error("LDAPAuthenticator: ldap_initialize failed: {}",
                          ldap_err2string(rc2));
            audit.logLDAPFailure(username, "connection_failed");
            return LDAPAuthResult::Failed("LDAP connection failed");
        }
        owns_connection = true;

        int version = LDAP_VERSION3;
        rc2 = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
        if (rc2 != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: failed to set LDAP protocol version: {}",
                          ldap_err2string(rc2));
        }

        struct timeval conn_tv{};
        conn_tv.tv_sec  = config_.connection_timeout_seconds;
        conn_tv.tv_usec = 0;
        rc2 = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &conn_tv);
        if (rc2 != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: failed to set LDAP network timeout: {}",
                          ldap_err2string(rc2));
        }

        struct timeval srch_tv{};
        srch_tv.tv_sec  = config_.search_timeout_seconds;
        srch_tv.tv_usec = 0;
        rc2 = ldap_set_option(ld, LDAP_OPT_TIMEOUT, &srch_tv);
        if (rc2 != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: failed to set LDAP search timeout: {}",
                          ldap_err2string(rc2));
        }

        rc2 = ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
        if (rc2 != LDAP_SUCCESS) {
            spdlog::error("LDAPAuthenticator: failed to disable LDAP referrals: {}",
                          ldap_err2string(rc2));
            ldap_unbind_ext_s(ld, nullptr, nullptr);
            audit.logLDAPFailure(username, "disable_referrals_failed");
            return LDAPAuthResult::Failed("LDAP configuration error (referrals still enabled)");
        }

        if (config_.use_tls) {
            rc2 = ldap_start_tls_s(ld, nullptr, nullptr);
            if (rc2 != LDAP_SUCCESS) {
                spdlog::error("LDAPAuthenticator: StartTLS failed: {}",
                              ldap_err2string(rc2));
                ldap_unbind_ext_s(ld, nullptr, nullptr);
                audit.logLDAPFailure(username, "tls_failed");
                return LDAPAuthResult::Failed("LDAP TLS negotiation failed");
            }
        }
    }

    // -----------------------------------------------------------------------
    // Bind with the user's DN + password
    // -----------------------------------------------------------------------

    struct berval cred{};
    cred.bv_val = const_cast<char*>(password.c_str());
    cred.bv_len = static_cast<ber_len_t>(password.size());

    int rc = ldap_sasl_bind_s(ld, dn.c_str(), LDAP_SASL_SIMPLE,
                              &cred, nullptr, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        spdlog::warn("LDAPAuthenticator: bind failed for DN '{}': {}",
                     dn, ldap_err2string(rc));
        if (pooled_conn) {
            pooled_conn->markStale();
        } else if (owns_connection) {
            ldap_unbind_ext_s(ld, nullptr, nullptr);
        }
        audit.logLDAPFailure(username, "bind_failed");
        return LDAPAuthResult::Failed("LDAP bind failed: invalid credentials");
    }

    // -----------------------------------------------------------------------
    // Optional group search
    // -----------------------------------------------------------------------

    std::vector<std::string> groups;
    if (config_.enable_group_search && !config_.group_search_filter.empty()) {
        const std::string filter = buildGroupSearchFilter(dn, username);

        const std::string search_base =
            config_.group_search_base.empty()
                ? config_.base_dn
                : config_.group_search_base;

        const char* attrs[] = {config_.group_attribute.c_str(), nullptr};
        struct timeval tv{};
        tv.tv_sec  = config_.search_timeout_seconds;
        tv.tv_usec = 0;

        // -----------------------------------------------------------------------
        // Paginated group search — RFC 2696 / LDAP_CONTROL_PAGEDRESULTS
        //
        // We issue repeated ldap_search_ext_s calls with a server-side page
        // control and advance the cursor via the returned cookie.  This bounds
        // memory usage and avoids hitting server-side result-set size limits.
        //
        // Defaults: page_size=500, max_results=5000.
        // -----------------------------------------------------------------------
        constexpr ber_int_t kPageSize   = 500;
        constexpr int       kMaxResults = 5000;

        struct berval* page_cookie = nullptr;
        int total_collected = 0;
        bool pagination_done = false;

        do {
            // Build the page control.  Pass the current cookie (nullptr on
            // the first page, non-nullptr on subsequent pages).
            LDAPControl* page_ctrl = nullptr;
            struct berval b_cookie{0, nullptr};
            if (page_cookie) {
                b_cookie = *page_cookie;
            }
            const int ctrl_rc = ldap_create_page_control(
                ld, static_cast<ber_int_t>(kPageSize), &b_cookie, 0, &page_ctrl);
            if (ctrl_rc != LDAP_SUCCESS || !page_ctrl) {
                spdlog::warn("[LDAP] Pagination error: {}", ldap_err2string(ctrl_rc));
                break;
            }

            LDAPControl* server_ctrls[] = {page_ctrl, nullptr};
            LDAPMessage* result = nullptr;
            rc = ldap_search_ext_s(
                ld,
                search_base.c_str(),
                LDAP_SCOPE_SUBTREE,
                filter.c_str(),
                const_cast<char**>(attrs),
                0,
                server_ctrls, nullptr,
                &tv,
                LDAP_NO_LIMIT,
                &result
            );
            ldap_control_free(page_ctrl);

            if (rc != LDAP_SUCCESS) {
                spdlog::warn("[LDAP] Pagination error: {}", ldap_err2string(rc));
                if (result) { ldap_msgfree(result); }
                if (page_cookie) { ber_bvfree(page_cookie); page_cookie = nullptr; }
                break;  // Return partial results (non-fatal)
            }

            // Collect entries from this page.
            for (LDAPMessage* entry = ldap_first_entry(ld, result);
                 entry && total_collected < kMaxResults;
                 entry = ldap_next_entry(ld, entry))
            {
                struct berval** vals = ldap_get_values_len(
                    ld, entry, config_.group_attribute.c_str());
                if (vals) {
                    for (int i = 0; vals[i] && total_collected < kMaxResults; ++i) {
                        groups.emplace_back(vals[i]->bv_val,
                                            static_cast<size_t>(vals[i]->bv_len));
                        ++total_collected;
                    }
                    ldap_value_free_len(vals);
                }
            }

            // Parse the response controls to get the next-page cookie BEFORE
            // freeing the result chain — ldap_parse_result needs the chain.
            LDAPControl** resp_ctrls = nullptr;
            ldap_parse_result(ld, result, nullptr, nullptr, nullptr, nullptr,
                              &resp_ctrls, 0 /* do not free result */);

            // Advance or terminate the cookie.
            if (page_cookie) { ber_bvfree(page_cookie); page_cookie = nullptr; }
            if (resp_ctrls) {
                ber_int_t total_count = 0;
                ldap_parse_page_control(ld, resp_ctrls, &total_count, &page_cookie);
                ldap_controls_free(resp_ctrls);
            }
            ldap_msgfree(result);

            // Stop when the server signals no more pages or we hit the cap.
            if (!page_cookie || page_cookie->bv_len == 0) {
                pagination_done = true;
            }
            if (total_collected >= kMaxResults) {
                spdlog::info("[LDAP] Group search capped at max_results={} for user '{}'",
                             kMaxResults, username);
                pagination_done = true;
            }

        } while (!pagination_done);

        if (page_cookie) { ber_bvfree(page_cookie); }
    }

    // -----------------------------------------------------------------------
    // Release connection — pool handles it via RAII; per-call connections are
    // explicitly unbound here.
    // -----------------------------------------------------------------------
    if (owns_connection) {
        ldap_unbind_ext_s(ld, nullptr, nullptr);
    }
    // pooled_conn destructor returns the connection to the pool.

    const auto roles = mapGroupsToRoles(groups);
    audit.logLDAPSuccess(username, dn);
    spdlog::info("LDAPAuthenticator: user '{}' authenticated successfully",
                 themis::security::PIIRedactionPolicy::get().redactForLog(username));
    return LDAPAuthResult::Success(username, dn, roles, groups);
}

#else
// ---------------------------------------------------------------------------
// PERMANENT FALLBACK NOTE:
// Purpose: Link-compatible LDAP fallback for builds without libldap.  Returns a
//   hard-failure from performBind() so any LDAP-gated authentication request
//   is explicitly rejected rather than accidentally allowed.  This fallback is
//   permanent for builds that intentionally omit libldap.
// Activation: Compiled when THEMIS_HAS_LDAP is NOT defined.  Set via
//   -DTHEMIS_ENABLE_LDAP=ON in CMake to enable the real implementation.
// Production Delta: All LDAP-based logins will fail with an explicit error
//   message.  No silent pass-through; the rejection is logged and audited.
// Real implementation: Install libldap and build with -DTHEMIS_ENABLE_LDAP=ON.
//   The #if THEMIS_HAS_LDAP branch above handles TLS, paging, group membership,
//   and attribute mapping.
// Roadmap ref: src/auth/FUTURE_ENHANCEMENTS.md § "LDAP Group Membership (v1.6.0)"
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& dn,
                                              const std::string& password)
{
    auto fn = getLdapBindFn();
    if (fn) {
        try {
            return fn(username, dn, password);
        } catch (...) {
            return LDAPAuthResult::Failed("LdapBindFn threw an exception");
        }
    }

    AuthAuditLogger audit(audit_logger_);
    const std::string msg =
        "LDAP support is not available: rebuild ThemisDB with THEMIS_ENABLE_LDAP=ON";
    spdlog::warn("LDAPAuthenticator: {}", msg);
    audit.logLDAPFailure(username, "ldap_not_compiled_in");
    return LDAPAuthResult::Failed(msg);
}

#endif  // defined(THEMIS_HAS_LDAP) && THEMIS_HAS_LDAP && defined(_WIN32)

} // namespace auth
} // namespace themis
