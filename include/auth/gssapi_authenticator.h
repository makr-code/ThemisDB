/**
 * @file gssapi_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>

#ifdef _WIN32
#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>
#else
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#endif

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

// Input validation limits for GSSAPI
constexpr size_t MAX_GSSAPI_TOKEN_SIZE = 64 * 1024;  // 64KB max for GSSAPI tokens
constexpr size_t MAX_KERBEROS_PRINCIPAL_LENGTH = 256; // 256 chars max for Kerberos principals
constexpr int DEFAULT_GSSAPI_CONTEXT_TIMEOUT = 30;    // 30 second timeout for GSSAPI context

/**
 * @brief Configuration for Kerberos/GSSAPI authentication
 */
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

/**
 * @brief Result of GSSAPI authentication
 */
struct GSSAPIAuthResult {
    bool success = false;
    std::string principal_name;         // Authenticated Kerberos principal
    std::string error_message;
    std::vector<std::string> roles;     // Mapped roles from principal
    
    static GSSAPIAuthResult Success(const std::string& principal, const std::vector<std::string>& roles) {
        return {true, principal, "", roles};
    }
    
    static GSSAPIAuthResult Failed(const std::string& error) {
        return {false, "", error, {}};
    }
};

/**
 * @brief GSSAPI/Kerberos authenticator for enterprise SSO integration
 * 
 * This class provides Kerberos v5 authentication using GSSAPI (Generic Security
 * Services API). It supports:
 * - MIT Kerberos 5
 * - Active Directory
 * - Heimdal Kerberos
 * 
 * Authentication flow:
 * 1. Server initializes with service principal and keytab
 * 2. Client sends Kerberos ticket in authentication token
 * 3. Server validates ticket using GSSAPI
 * 4. Principal is extracted and mapped to ThemisDB roles
 * 5. User is authenticated with assigned roles
 */
class GSSAPIAuthenticator {
public:
    /**
     * @brief Constructor
     */
    GSSAPIAuthenticator();
    
    /**
     * @brief Destructor - cleans up GSSAPI resources
     */
    ~GSSAPIAuthenticator();
    
    // Disable copy and move
    GSSAPIAuthenticator(const GSSAPIAuthenticator&) = delete;
    GSSAPIAuthenticator& operator=(const GSSAPIAuthenticator&) = delete;
    GSSAPIAuthenticator(GSSAPIAuthenticator&&) = delete;
    GSSAPIAuthenticator& operator=(GSSAPIAuthenticator&&) = delete;
    
    /**
     * @brief Attach an AuditLogger to receive LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }
    
    /**
     * @brief Initialize GSSAPI with service principal
     * 
     * @param config Kerberos configuration
     * @return true if initialization successful
     */
    bool initialize(const KerberosConfig& config);
    
    /**
     * @brief Check if authenticator is initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Authenticate a Kerberos token
     * 
     * @param token Base64-encoded GSSAPI token from client
     * @return Authentication result with principal and roles
     */
    GSSAPIAuthResult authenticateToken(const std::string& token);
    
    /**
     * @brief Get the service principal name
     */
    std::string getServicePrincipal() const { return config_.service_principal; }
    
    /**
     * @brief Map Kerberos principal to ThemisDB roles
     * 
     * @param principal Kerberos principal (e.g., "user@REALM.COM")
     * @return List of roles based on configuration
     */
    std::vector<std::string> mapPrincipalToRoles(const std::string& principal) const;
    
    /**
     * @brief Get configuration
     */
    const KerberosConfig& getConfig() const { return config_; }

private:
    bool initialized_ = false;
    KerberosConfig config_;
    utils::AuditLogger* audit_logger_ = nullptr;  ///< Non-owning, optional.
    
#ifdef _WIN32
    // Windows SSPI handles
    CredHandle server_creds_;
    TimeStamp creds_expiry_;
#else
    // Unix GSSAPI handles
    gss_ctx_id_t context_;
    gss_cred_id_t server_creds_;
    gss_name_t server_name_;
#endif
    
    /**
     * @brief Initialize server credentials from keytab
     */
    bool initializeServerCredentials();
    
    /**
     * @brief Accept security context from client token
     */
    bool acceptSecurityContext(const std::vector<uint8_t>& input_token,
                              std::string& principal_name);
    
    /**
     * @brief Cleanup GSSAPI resources
     */
    void cleanup();
    
    /**
     * @brief Check if principal matches pattern (supports wildcards)
     */
    bool principalMatchesPattern(const std::string& principal,
                                 const std::string& pattern) const;
    
    /**
     * @brief Get GSSAPI error string
     */
    std::string getGSSAPIError(uint32_t major_status, uint32_t minor_status) const;
};

} // namespace auth
} // namespace themis
