/**
 * @file gssapi_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <cstdint>

#ifndef THEMIS_HAVE_GSSAPI
#define THEMIS_HAVE_GSSAPI 0
#endif

#ifdef _WIN32
#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>
#else
#if defined(THEMIS_HAS_KERBEROS) && THEMIS_HAS_KERBEROS
#if defined(__has_include)
#if __has_include(<gssapi/gssapi.h>) && __has_include(<gssapi/gssapi_krb5.h>)
#undef THEMIS_HAVE_GSSAPI
#define THEMIS_HAVE_GSSAPI 1
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#endif
#else
#undef THEMIS_HAVE_GSSAPI
#define THEMIS_HAVE_GSSAPI 1
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#endif
#endif
#endif

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

// Input validation limits for GSSAPI
constexpr size_t MAX_GSSAPI_TOKEN_SIZE = 64 * 1024;  // 64KB max for GSSAPI tokens
constexpr size_t MAX_KERBEROS_PRINCIPAL_LENGTH = 256; // 256 chars max for Kerberos principals
constexpr int DEFAULT_GSSAPI_CONTEXT_TIMEOUT = 30;    // 30 second timeout for GSSAPI context

struct KerberosConfig {
    bool enabled = false;
    std::string service_principal;      // e.g., "themisdb/hostname@REALM.COM"
    std::string keytab_file;            // Path to keytab file
    std::string krb5_config;            // Path to krb5.conf (optional)
    bool fallback_to_basic = true;      // Allow fallback to basic auth if Kerberos fails
    int context_timeout_seconds{DEFAULT_GSSAPI_CONTEXT_TIMEOUT}; // GSSAPI context timeout
    
    // Principal to role mapping
    struct PrincipalMapping {
        std::string principal_pattern;   // Can contain wildcards like "*@REALM.COM"
        std::string role;
    };
    std::vector<PrincipalMapping> principal_mappings;
};

struct GSSAPIAuthResult {
    bool success = false;
    std::string principal_name;         // Authenticated Kerberos principal
    std::string error_message;
    std::vector<std::string> roles;     // Mapped roles from principal
    
    /**
     * @brief Success.
     * @param[in] principal Input parameter.
     * @param[in] roles Input parameter.
     * @return Return value.
     * @details Implements Success without additional internal calls.
     */
    static GSSAPIAuthResult Success(const std::string& principal, const std::vector<std::string>& roles) {
        return {true, principal, "", roles};
    }
    
    /**
     * @brief Failed.
     * @param[in] error Input parameter.
     * @return Return value.
     * @details Implements Failed without additional internal calls.
     */
    static GSSAPIAuthResult Failed(const std::string& error) {
        return {false, "", error, {}};
    }
};

class GSSAPIAuthenticator {
public:
    GSSAPIAuthenticator();
    
    ~GSSAPIAuthenticator();
    
    // Disable copy and move
    GSSAPIAuthenticator(const GSSAPIAuthenticator&) = delete;
    GSSAPIAuthenticator& operator=(const GSSAPIAuthenticator&) = delete;
    GSSAPIAuthenticator(GSSAPIAuthenticator&&) = delete;
    GSSAPIAuthenticator& operator=(GSSAPIAuthenticator&&) = delete;
    
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
    bool initialize(const KerberosConfig& config);
    
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Authenticate Token.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    GSSAPIAuthResult authenticateToken(const std::string& token);
    
    std::string getServicePrincipal() const { return config_.service_principal; }
    
    /**
     * @brief Map Principal To Roles.
     * @param[in] principal Input parameter.
     * @return Return value.
     */
    std::vector<std::string> mapPrincipalToRoles(const std::string& principal) const;
    
    const KerberosConfig& getConfig() const { return config_; }

private:
    bool initialized_ = false;
    KerberosConfig config_;
    utils::AuditLogger* audit_logger_ = nullptr;  ///< Non-owning, optional.
    
#ifdef _WIN32
    // Windows SSPI handles
    CredHandle server_creds_;
    TimeStamp creds_expiry_;
#elif THEMIS_HAVE_GSSAPI
    // Unix GSSAPI handles
    gss_ctx_id_t context_;
    gss_cred_id_t server_creds_;
    gss_name_t server_name_;
#else
    // GSSAPI headers unavailable; no platform-specific handle state.
#endif
    
    /**
     * @brief Initialize Server Credentials.
     * @return True when the operation succeeds.
     */
    bool initializeServerCredentials();
    
    /**
     * @brief Accept Security Context.
     * @param[in] input_token Input parameter.
     * @param[in,out] principal_name Name of the principal.
     * @return True when the operation succeeds.
     */
    bool acceptSecurityContext(const std::vector<uint8_t>& input_token,
                              std::string& principal_name);
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
    
    /**
     * @brief Principal Matches Pattern.
     * @param[in] principal Input parameter.
     * @param[in] pattern Input parameter.
     * @return True when the operation succeeds.
     */
    bool principalMatchesPattern(const std::string& principal,
                                 const std::string& pattern) const;
    
    /**
     * @brief Get GSSAPIError.
     * @param[in] major_status Input parameter.
     * @param[in] minor_status Input parameter.
     * @return Return value.
     */
    std::string getGSSAPIError(uint32_t major_status, uint32_t minor_status) const;
};

} // namespace auth
} // namespace themis
