/**
 * @file gssapi_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/gssapi_authenticator.h"
#include "utils/logger.h"
#include "utils/audit_logger.h"
#include <algorithm>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#ifndef MICROSOFT_KERBEROS_NAME_A
#define MICROSOFT_KERBEROS_NAME_A "Kerberos"
#endif
#endif

#ifndef _WIN32
#include <krb5.h>
#endif

namespace themis {
namespace auth {

GSSAPIAuthenticator::GSSAPIAuthenticator()
    : initialized_(false)
#ifndef _WIN32
    , context_(GSS_C_NO_CONTEXT)
    , server_creds_(GSS_C_NO_CREDENTIAL)
    , server_name_(GSS_C_NO_NAME)
#endif
{
}

GSSAPIAuthenticator::~GSSAPIAuthenticator() {
    cleanup();
}

bool GSSAPIAuthenticator::initialize(const KerberosConfig& config) {
    if (initialized_) {
        THEMIS_WARN("GSSAPIAuthenticator already initialized");
        return true;
    }
    
    config_ = config;
    
    if (config_.service_principal.empty()) {
        THEMIS_ERROR("Service principal is required for Kerberos authentication");
        return false;
    }
    
    // Set KRB5_CONFIG environment variable if specified
    if (!config_.krb5_config.empty()) {
#ifdef _WIN32
        _putenv_s("KRB5_CONFIG", config_.krb5_config.c_str());
#else
        setenv("KRB5_CONFIG", config_.krb5_config.c_str(), 1);
#endif
        THEMIS_INFO("Set KRB5_CONFIG to: {}", config_.krb5_config);
    }
    
    // Set KRB5_KTNAME environment variable if keytab specified
    if (!config_.keytab_file.empty()) {
#ifdef _WIN32
        _putenv_s("KRB5_KTNAME", config_.keytab_file.c_str());
#else
        setenv("KRB5_KTNAME", config_.keytab_file.c_str(), 1);
#endif
        THEMIS_INFO("Set KRB5_KTNAME to: {}", config_.keytab_file);
    }
    
    // Initialize server credentials
    if (!initializeServerCredentials()) {
        THEMIS_ERROR("Failed to initialize server credentials");
        return false;
    }
    
    initialized_ = true;
    THEMIS_INFO("GSSAPI authenticator initialized with service principal: {}", 
                config_.service_principal);
    
    return true;
}

bool GSSAPIAuthenticator::initializeServerCredentials() {
#ifdef _WIN32
    // Windows SSPI implementation
    SECURITY_STATUS status;
    TimeStamp lifetime;
    
    // Acquire credentials handle for Kerberos
    status = AcquireCredentialsHandle(
        const_cast<SEC_CHAR*>(config_.service_principal.c_str()),
        const_cast<SEC_CHAR*>(MICROSOFT_KERBEROS_NAME_A),
        SECPKG_CRED_INBOUND,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &server_creds_,
        &lifetime
    );
    
    if (status != SEC_E_OK) {
        THEMIS_ERROR("AcquireCredentialsHandle failed: 0x{:X}", status);
        return false;
    }
    
    creds_expiry_ = lifetime;
    return true;
    
#else
    // Unix GSSAPI implementation
    OM_uint32 major_status, minor_status;
    gss_buffer_desc name_buf;
    
    // Convert service principal string to gss_name_t
    name_buf.value = const_cast<char*>(config_.service_principal.c_str());
    name_buf.length = config_.service_principal.length();
    
    major_status = gss_import_name(
        &minor_status,
        &name_buf,
        GSS_C_NT_HOSTBASED_SERVICE,
        &server_name_
    );
    
    if (GSS_ERROR(major_status)) {
        THEMIS_ERROR("gss_import_name failed: {}", 
                    getGSSAPIError(major_status, minor_status));
        return false;
    }
    
    // Acquire credentials for the service principal
    major_status = gss_acquire_cred(
        &minor_status,
        server_name_,
        GSS_C_INDEFINITE,
        GSS_C_NO_OID_SET,
        GSS_C_ACCEPT,
        &server_creds_,
        nullptr,
        nullptr
    );
    
    if (GSS_ERROR(major_status)) {
        THEMIS_ERROR("gss_acquire_cred failed: {}", 
                    getGSSAPIError(major_status, minor_status));
        gss_release_name(&minor_status, &server_name_);
        server_name_ = GSS_C_NO_NAME;
        return false;
    }
    
    return true;
#endif
}

GSSAPIAuthResult GSSAPIAuthenticator::authenticateToken(const std::string& token) {
    if (!initialized_) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                "", "kerberos/principal", {{"reason", "not_initialized"}});
        }
        return GSSAPIAuthResult::Failed("GSSAPI authenticator not initialized");
    }
    
    if (token.empty()) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                "", "kerberos/principal", {{"reason", "empty_token"}});
        }
        return GSSAPIAuthResult::Failed("Empty authentication token");
    }
    
    // Decode base64 token (simplified - in production use proper base64 decoder)
    // For now, assume token is already in binary format or use existing base64 utilities
    std::vector<uint8_t> token_bytes(token.begin(), token.end());
    
    std::string principal_name;
    if (!acceptSecurityContext(token_bytes, principal_name)) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                "", "kerberos/principal", {{"reason", "context_rejected"}});
        }
        return GSSAPIAuthResult::Failed("Failed to accept security context");
    }
    
    // Map principal to roles
    auto roles = mapPrincipalToRoles(principal_name);

    std::string roles_str;
    for (size_t i = 0; i < roles.size(); ++i) {
        if (i > 0) {
            roles_str += ", ";
        }
        roles_str += roles[i];
    }

    THEMIS_INFO("Authenticated Kerberos principal: {} with roles: [{}]",
               principal_name, roles_str);
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_SUCCESS,
            principal_name, "kerberos/principal", {});
    }
    
    return GSSAPIAuthResult::Success(principal_name, roles);
}

bool GSSAPIAuthenticator::acceptSecurityContext(
    const std::vector<uint8_t>& input_token,
    std::string& principal_name) {
    
#ifdef _WIN32
    // Windows SSPI implementation
    SecBufferDesc input_desc;
    SecBuffer input_buf;
    SecBufferDesc output_desc;
    SecBuffer output_buf;
    CtxtHandle context;
    TimeStamp lifetime;
    ULONG context_attr;
    
    input_buf.BufferType = SECBUFFER_TOKEN;
    input_buf.cbBuffer = static_cast<ULONG>(input_token.size());
    input_buf.pvBuffer = const_cast<uint8_t*>(input_token.data());
    input_desc.ulVersion = SECBUFFER_VERSION;
    input_desc.cBuffers = 1;
    input_desc.pBuffers = &input_buf;
    
    std::vector<uint8_t> output_buffer(8192);
    output_buf.BufferType = SECBUFFER_TOKEN;
    output_buf.cbBuffer = static_cast<ULONG>(output_buffer.size());
    output_buf.pvBuffer = output_buffer.data();
    output_desc.ulVersion = SECBUFFER_VERSION;
    output_desc.cBuffers = 1;
    output_desc.pBuffers = &output_buf;
    
    SECURITY_STATUS status = AcceptSecurityContext(
        &server_creds_,
        nullptr,
        &input_desc,
        ASC_REQ_MUTUAL_AUTH,
        SECURITY_NATIVE_DREP,
        &context,
        &output_desc,
        &context_attr,
        &lifetime
    );
    
    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED) {
        THEMIS_ERROR("AcceptSecurityContext failed: 0x{:X}", status);
        return false;
    }
    
    // Query context for client name
    SecPkgContext_Names names;
    status = QueryContextAttributes(&context, SECPKG_ATTR_NAMES, &names);
    if (status == SEC_E_OK) {
        principal_name = names.sUserName;
        FreeContextBuffer(names.sUserName);
    }
    
    DeleteSecurityContext(&context);
    return !principal_name.empty();
    
#else
    // Unix GSSAPI implementation
    OM_uint32 major_status, minor_status;
    gss_buffer_desc input_buffer;
    gss_buffer_desc output_buffer = GSS_C_EMPTY_BUFFER;
    gss_name_t client_name = GSS_C_NO_NAME;
    OM_uint32 ret_flags;
    
    input_buffer.length = input_token.size();
    input_buffer.value = const_cast<uint8_t*>(input_token.data());
    
    // Create a fresh context for this authentication
    gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
    
    major_status = gss_accept_sec_context(
        &minor_status,
        &ctx,
        server_creds_,
        &input_buffer,
        GSS_C_NO_CHANNEL_BINDINGS,
        &client_name,
        nullptr,
        &output_buffer,
        &ret_flags,
        nullptr,
        nullptr
    );
    
    // Release output buffer if allocated
    if (output_buffer.length > 0) {
        gss_release_buffer(&minor_status, &output_buffer);
    }
    
    if (GSS_ERROR(major_status)) {
        THEMIS_ERROR("gss_accept_sec_context failed: {}", 
                    getGSSAPIError(major_status, minor_status));
        if (ctx != GSS_C_NO_CONTEXT) {
            gss_delete_sec_context(&minor_status, &ctx, GSS_C_NO_BUFFER);
        }
        return false;
    }
    
    // Extract principal name
    if (client_name != GSS_C_NO_NAME) {
        gss_buffer_desc name_buffer;
        major_status = gss_display_name(&minor_status, client_name, &name_buffer, nullptr);
        
        if (!GSS_ERROR(major_status)) {
            principal_name = std::string(
                static_cast<char*>(name_buffer.value),
                name_buffer.length
            );
            gss_release_buffer(&minor_status, &name_buffer);
        }
        
        gss_release_name(&minor_status, &client_name);
    }
    
    // Clean up context
    if (ctx != GSS_C_NO_CONTEXT) {
        gss_delete_sec_context(&minor_status, &ctx, GSS_C_NO_BUFFER);
    }
    
    return !principal_name.empty();
#endif
}

std::vector<std::string> GSSAPIAuthenticator::mapPrincipalToRoles(
    const std::string& principal) const {
    
    std::vector<std::string> roles;
    
    // Check each mapping pattern
    for (const auto& mapping : config_.principal_mappings) {
        if (principalMatchesPattern(principal, mapping.principal_pattern)) {
            roles.push_back(mapping.role);
            THEMIS_DEBUG("Principal '{}' matched pattern '{}' -> role '{}'",
                        principal, mapping.principal_pattern, mapping.role);
        }
    }
    
    // If no roles matched, assign default readonly role
    if (roles.empty()) {
        THEMIS_WARN("No role mapping found for principal '{}', assigning 'readonly'", principal);
        roles.push_back("readonly");
    }
    
    return roles;
}

bool GSSAPIAuthenticator::principalMatchesPattern(
    const std::string& principal,
    const std::string& pattern) const {
    
    // Exact match
    if (principal == pattern) {
        return true;
    }
    
    // Wildcard matching
    // Simple implementation: "*@REALM.COM" matches any user in REALM.COM
    if (pattern.find('*') != std::string::npos) {
        // Find the realm part
        auto at_pos = pattern.find('@');
        if (at_pos != std::string::npos) {
            std::string pattern_realm = pattern.substr(at_pos);
            auto principal_at_pos = principal.find('@');
            if (principal_at_pos != std::string::npos) {
                std::string principal_realm = principal.substr(principal_at_pos);
                return principal_realm == pattern_realm;
            }
        }
    }
    
    return false;
}

void GSSAPIAuthenticator::cleanup() {
    if (!initialized_) {
        return;
    }
    
#ifdef _WIN32
    FreeCredentialsHandle(&server_creds_);
#else
    OM_uint32 minor_status;
    
    if (context_ != GSS_C_NO_CONTEXT) {
        gss_delete_sec_context(&minor_status, &context_, GSS_C_NO_BUFFER);
        context_ = GSS_C_NO_CONTEXT;
    }
    
    if (server_creds_ != GSS_C_NO_CREDENTIAL) {
        gss_release_cred(&minor_status, &server_creds_);
        server_creds_ = GSS_C_NO_CREDENTIAL;
    }
    
    if (server_name_ != GSS_C_NO_NAME) {
        gss_release_name(&minor_status, &server_name_);
        server_name_ = GSS_C_NO_NAME;
    }
#endif
    
    initialized_ = false;
}

#ifndef _WIN32
std::string GSSAPIAuthenticator::getGSSAPIError(
    uint32_t major_status,
    uint32_t minor_status) const {
    
    OM_uint32 message_context = 0;
    OM_uint32 min_stat;
    gss_buffer_desc status_string;
    std::ostringstream oss;
    
    // Get major status message
    do {
        gss_display_status(
            &min_stat,
            major_status,
            GSS_C_GSS_CODE,
            GSS_C_NO_OID,
            &message_context,
            &status_string
        );
        
        oss << std::string(static_cast<char*>(status_string.value), status_string.length);
        gss_release_buffer(&min_stat, &status_string);
        
        if (message_context != 0) {
            oss << " ";
        }
    } while (message_context != 0);
    
    // Get minor status message
    if (minor_status != 0) {
        message_context = 0;
        oss << " (";
        
        do {
            gss_display_status(
                &min_stat,
                minor_status,
                GSS_C_MECH_CODE,
                GSS_C_NO_OID,
                &message_context,
                &status_string
            );
            
            oss << std::string(static_cast<char*>(status_string.value), status_string.length);
            gss_release_buffer(&min_stat, &status_string);
            
            if (message_context != 0) {
                oss << " ";
            }
        } while (message_context != 0);
        
        oss << ")";
    }
    
    return oss.str();
}
#else
std::string GSSAPIAuthenticator::getGSSAPIError(
    uint32_t major_status,
    uint32_t minor_status) const {
    std::ostringstream oss;
    oss << "SSPI Error: 0x" << std::hex << major_status;
    if (minor_status != 0) {
        oss << " (0x" << minor_status << ")";
    }
    return oss.str();
}
#endif

} // namespace auth
} // namespace themis
