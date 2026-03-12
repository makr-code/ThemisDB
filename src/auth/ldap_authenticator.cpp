/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ldap_authenticator.cpp                             ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     443                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 879ea3571  2026-02-26  fix(auth): redact PII in log statements (LDAP, SAML, API ... ║
    • 1ce77d0ea  2026-02-24  fix(auth): code audit fixes for LDAP authenticator Window... ║
    • 79129146f  2026-02-24  feat(auth): implement LDAP/Active Directory direct bind a... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/ldap_authenticator.h"
#include "auth/auth_audit_logger.h"
#include "utils/audit_logger.h"
#include "security/pii_redaction_policy.h"

#include <spdlog/spdlog.h>

#include <algorithm>

// ---------------------------------------------------------------------------
// Platform-specific LDAP includes
// ---------------------------------------------------------------------------

#ifdef _WIN32
#  include <windows.h>
#  include <winldap.h>
#  define THEMIS_HAS_LDAP 1
#elif defined(THEMIS_HAS_LDAP)
#  include <ldap.h>
#  include <lber.h>
#endif

namespace themis {
namespace auth {

// ===========================================================================
// LDAP injection-prevention helpers (file-internal)
// ===========================================================================

namespace {

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

} // anonymous namespace

// ===========================================================================
// Construction / destruction
// ===========================================================================

LDAPAuthenticator::LDAPAuthenticator() = default;

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

    spdlog::info("LDAPAuthenticator: initialized for server {}", config_.server_url);
    return true;
}

// ===========================================================================
// Public helpers
// ===========================================================================

std::string LDAPAuthenticator::buildUserDN(const std::string& username) const
{
    std::string dn = config_.bind_dn_template;
    const std::string placeholder = "{username}";
    const auto pos = dn.find(placeholder);
    if (pos != std::string::npos) {
        dn.replace(pos, placeholder.size(), escapeLDAPDNComponent(username));
    }
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

std::string LDAPAuthenticator::buildGroupSearchFilter(const std::string& dn) const
{
    std::string filter = config_.group_search_filter;
    const std::string ph = "{dn}";
    const auto pos = filter.find(ph);
    if (pos != std::string::npos) {
        filter.replace(pos, ph.size(), escapeLDAPFilterValue(dn));
    }
    return filter;
}

// ===========================================================================
// Authentication
// ===========================================================================

LDAPAuthResult LDAPAuthenticator::authenticate(const std::string& username,
                                               const std::string& password)
{
    // --- Input validation ---------------------------------------------------
    if (username.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: username must not be empty"
        ));
    }
    if (username.size() > MAX_LDAP_USERNAME_LENGTH) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: username exceeds maximum length"
        ));
    }
    if (password.empty()) {
        // Reject anonymous/unauthenticated binds for direct-bind auth
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: password must not be empty (anonymous bind not permitted)"
        ));
    }
    if (password.size() > MAX_LDAP_PASSWORD_LENGTH) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "LDAP: password exceeds maximum length"
        ));
    }

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

// ===========================================================================
// Platform-specific bind implementation
// ===========================================================================

#if defined(THEMIS_HAS_LDAP) && defined(_WIN32)
// ---------------------------------------------------------------------------
// Windows WinLDAP implementation
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& dn,
                                              const std::string& password)
{
    AuthAuditLogger audit(audit_logger_);

    // ldap_init is deprecated in newer SDKs but still universally available
    LDAP* ld = ldap_init(
        const_cast<PCHAR>(config_.server_url.c_str()),
        config_.port > 0 ? static_cast<ULONG>(config_.port) : DEFAULT_LDAP_PORT
    );
    if (!ld) {
        const std::string msg = "ldap_init failed";
        spdlog::error("LDAPAuthenticator: {}", msg);
        audit.logLDAPFailure(username, msg);
        return LDAPAuthResult::Failed("LDAP connection failed");
    }

    // Set search time limit (seconds)
    ULONG timelimit = static_cast<ULONG>(config_.search_timeout_seconds);
    ldap_set_option(ld, LDAP_OPT_TIMELIMIT, static_cast<void*>(&timelimit));

    // Disable referral chasing — following attacker-controlled referrals can
    // redirect authentication to a rogue LDAP server.
    ULONG referrals_off = LDAP_OPT_OFF;
    const ULONG referrals_result =
        ldap_set_option(ld, LDAP_OPT_REFERRALS, static_cast<void*>(&referrals_off));
    if (referrals_result != LDAP_SUCCESS) {
        spdlog::error("LDAPAuthenticator: failed to disable LDAP referrals: {}",
                      referrals_result);
        ldap_unbind(ld);
        audit.logLDAPFailure(username, "disable_referrals_failed");
        return LDAPAuthResult::Failed("LDAP configuration error (referrals still enabled)");
    }

    // StartTLS if requested
    if (config_.use_tls) {
        const ULONG tls_result = ldap_start_tls_s(ld, nullptr, nullptr, nullptr, nullptr);
        if (tls_result != LDAP_SUCCESS) {
            spdlog::error("LDAPAuthenticator: StartTLS failed: {}", tls_result);
            ldap_unbind(ld);
            audit.logLDAPFailure(username, "tls_failed");
            return LDAPAuthResult::Failed("LDAP TLS negotiation failed");
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
        ldap_unbind(ld);
        audit.logLDAPFailure(username, "bind_failed");
        return LDAPAuthResult::Failed("LDAP bind failed: invalid credentials");
    }

    // Optional group search
    std::vector<std::string> groups;
    if (config_.enable_group_search && !config_.group_search_filter.empty()) {
        // Build group filter with {dn} substituted and filter-escaped (RFC 4515).
        const std::string filter = buildGroupSearchFilter(dn);

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

    ldap_unbind(ld);

    const auto roles = mapGroupsToRoles(groups);
    audit.logLDAPSuccess(username, dn);
    spdlog::info("LDAPAuthenticator: user '{}' authenticated successfully",
                 themis::security::PIIRedactionPolicy::get().redactForLog(username));
    return LDAPAuthResult::Success(username, dn, roles, groups);
}

#elif defined(THEMIS_HAS_LDAP)
// ---------------------------------------------------------------------------
// Unix / OpenLDAP implementation
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& dn,
                                              const std::string& password)
{
    AuthAuditLogger audit(audit_logger_);

    LDAP* ld = nullptr;
    int rc = ldap_initialize(&ld, config_.server_url.c_str());
    if (rc != LDAP_SUCCESS || !ld) {
        spdlog::error("LDAPAuthenticator: ldap_initialize failed: {}",
                      ldap_err2string(rc));
        audit.logLDAPFailure(username, "connection_failed");
        return LDAPAuthResult::Failed("LDAP connection failed");
    }

    // Protocol version
    int version = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    // Connection timeout
    struct timeval conn_tv{};
    conn_tv.tv_sec  = config_.connection_timeout_seconds;
    conn_tv.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &conn_tv);

    // Search timeout
    struct timeval srch_tv{};
    srch_tv.tv_sec  = config_.search_timeout_seconds;
    srch_tv.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_TIMEOUT, &srch_tv);

    // Disable referral chasing — following attacker-controlled referrals can
    // redirect authentication to a rogue LDAP server.
    int referrals_off = LDAP_OPT_OFF;
    rc = ldap_set_option(ld, LDAP_OPT_REFERRALS, &referrals_off);
    if (rc != LDAP_SUCCESS) {
        spdlog::error("LDAPAuthenticator: failed to disable LDAP referrals: {}",
                      ldap_err2string(rc));
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        audit.logLDAPFailure(username, "disable_referrals_failed");
        return LDAPAuthResult::Failed("LDAP configuration error (referrals still enabled)");
    }

    // StartTLS if requested
    if (config_.use_tls) {
        rc = ldap_start_tls_s(ld, nullptr, nullptr);
        if (rc != LDAP_SUCCESS) {
            spdlog::error("LDAPAuthenticator: StartTLS failed: {}",
                          ldap_err2string(rc));
            ldap_unbind_ext_s(ld, nullptr, nullptr);
            audit.logLDAPFailure(username, "tls_failed");
            return LDAPAuthResult::Failed("LDAP TLS negotiation failed");
        }
    }

    // Simple bind with user DN + password
    struct berval cred{};
    cred.bv_val = const_cast<char*>(password.c_str());
    cred.bv_len = static_cast<ber_len_t>(password.size());

    rc = ldap_sasl_bind_s(ld, dn.c_str(), LDAP_SASL_SIMPLE,
                          &cred, nullptr, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        spdlog::warn("LDAPAuthenticator: bind failed for DN '{}': {}",
                     dn, ldap_err2string(rc));
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        audit.logLDAPFailure(username, "bind_failed");
        return LDAPAuthResult::Failed("LDAP bind failed: invalid credentials");
    }

    // Optional group search
    std::vector<std::string> groups;
    if (config_.enable_group_search && !config_.group_search_filter.empty()) {
        // Build group filter with {dn} substituted and filter-escaped (RFC 4515).
        const std::string filter = buildGroupSearchFilter(dn);

        const std::string search_base =
            config_.group_search_base.empty()
                ? config_.base_dn
                : config_.group_search_base;

        const char* attrs[] = {config_.group_attribute.c_str(), nullptr};
        LDAPMessage* result = nullptr;
        struct timeval tv{};
        tv.tv_sec  = config_.search_timeout_seconds;
        tv.tv_usec = 0;

        rc = ldap_search_ext_s(
            ld,
            search_base.c_str(),
            LDAP_SCOPE_SUBTREE,
            filter.c_str(),
            const_cast<char**>(attrs),
            0,
            nullptr, nullptr,
            &tv,
            LDAP_NO_LIMIT,
            &result
        );

        if (rc == LDAP_SUCCESS && result) {
            for (LDAPMessage* entry = ldap_first_entry(ld, result);
                 entry != nullptr;
                 entry = ldap_next_entry(ld, entry))
            {
                struct berval** vals = ldap_get_values_len(
                    ld, entry, config_.group_attribute.c_str());
                if (vals) {
                    for (int i = 0; vals[i] != nullptr; ++i) {
                        groups.emplace_back(vals[i]->bv_val,
                                            static_cast<size_t>(vals[i]->bv_len));
                    }
                    ldap_value_free_len(vals);
                }
            }
            ldap_msgfree(result);
        } else if (rc != LDAP_SUCCESS) {
            spdlog::warn("LDAPAuthenticator: group search failed for '{}': {}",
                         username, ldap_err2string(rc));
            // Non-fatal: continue without groups
        }
    }

    ldap_unbind_ext_s(ld, nullptr, nullptr);

    const auto roles = mapGroupsToRoles(groups);
    audit.logLDAPSuccess(username, dn);
    spdlog::info("LDAPAuthenticator: user '{}' authenticated successfully",
                 themis::security::PIIRedactionPolicy::get().redactForLog(username));
    return LDAPAuthResult::Success(username, dn, roles, groups);
}

#else
// ---------------------------------------------------------------------------
// Stub: LDAP library not compiled in
// ---------------------------------------------------------------------------

LDAPAuthResult LDAPAuthenticator::performBind(const std::string& username,
                                              const std::string& /*dn*/,
                                              const std::string& /*password*/)
{
    AuthAuditLogger audit(audit_logger_);
    const std::string msg =
        "LDAP support is not available: rebuild ThemisDB with THEMIS_ENABLE_LDAP=ON";
    spdlog::warn("LDAPAuthenticator: {}", msg);
    audit.logLDAPFailure(username, "ldap_not_compiled_in");
    return LDAPAuthResult::Failed(msg);
}

#endif  // THEMIS_HAS_LDAP

} // namespace auth
} // namespace themis
