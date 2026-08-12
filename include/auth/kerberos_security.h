/**
 * @file kerberos_security.h
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
#include <memory>
#include <optional>
#include <cstdint>

namespace themis {
namespace auth {

/**
 * @brief Kerberos Security Hardening
 * 
 * Security Feature: Enhances GSSAPI/Kerberos authentication with additional security checks.
 * Prevents replay attacks, MITM attacks, and validates token structure.
 * 
 * Features:
 * - Channel bindings (RFC 5056, RFC 5929) to prevent MITM
 * - Strict ASN.1 validation for GSSAPI tokens
 * - Service ticket target principal verification
 * - Token structure validation
 * - Replay detection integration
 * 
 * Channel Bindings:
 * - tls-unique: TLS Finished message binding
 * - tls-server-end-point: Server certificate binding
 * - tls-exporter: TLS exporter binding (RFC 5705)
 * 
 * P1 (High Priority) security hardening feature.
 */
class KerberosSecurityValidator {
public:
    enum class ChannelBindingType {
        NONE,                   // No channel binding
        TLS_UNIQUE,            // TLS Finished message (RFC 5929)
        TLS_SERVER_ENDPOINT,   // Server certificate hash (RFC 5929)
        TLS_EXPORTER           // TLS exporter (RFC 5705)
    };
    
    struct Config {
        // Channel bindings
        bool enable_channel_bindings = false;
        ChannelBindingType binding_type = ChannelBindingType::TLS_SERVER_ENDPOINT;
        
        // ASN.1 validation
        bool strict_asn1_validation = true;
        size_t max_token_depth = 10;           // Max ASN.1 nesting depth
        size_t max_sequence_length = 10000;    // Max sequence length
        
        // Service ticket verification
        bool verify_service_target = true;
        std::string expected_service_principal;  // e.g., "HTTP/server.example.com@REALM"
        
        // Token structure validation
        bool validate_token_structure = true;
        bool require_mutual_auth = false;      // Require mutual authentication
        bool require_integrity = true;         // Require integrity protection
        bool require_confidentiality = false;  // Require confidentiality
        
        // Timing constraints
        int max_clock_skew_seconds = 300;      // 5 minutes
        bool reject_expired_tickets = true;
    };
    
    explicit KerberosSecurityValidator(const Config& config);
    
    /**
     * @brief Get the configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Validate GSSAPI token with security checks
     * 
     * Performs comprehensive validation:
     * - ASN.1 structure validation
     * - Service principal verification
     * - Channel binding verification
     * - Expiration checking
     * 
     * @param token_data GSSAPI token bytes
     * @param channel_binding Channel binding data (optional)
     * @return true if token is valid and secure
     * @throws std::runtime_error on validation failure with details
     */
    bool validateToken(
        const std::vector<uint8_t>& token_data,
        const std::vector<uint8_t>& channel_binding = {}
    );
    
    /**
     * @brief Validate ASN.1 structure
     * 
     * Checks for:
     * - Valid ASN.1 encoding
     * - Proper tag/length encoding
     * - No buffer overruns
     * - Depth limits
     * - Length limits
     * 
     * @param data ASN.1 encoded data
     * @return true if structure is valid
     */
    bool validateASN1Structure(const std::vector<uint8_t>& data);
    
    /**
     * @brief Verify service ticket target principal
     * 
     * Ensures ticket was issued for the expected service.
     * 
     * @param token_data GSSAPI token
     * @param expected_principal Expected service principal
     * @return true if principal matches
     */
    bool verifyServicePrincipal(
        const std::vector<uint8_t>& token_data,
        const std::string& expected_principal
    );
    
    /**
     * @brief Verify channel bindings
     * 
     * Binds Kerberos authentication to TLS channel, preventing MITM.
     * 
     * @param token_data GSSAPI token
     * @param channel_binding Channel binding data from TLS
     * @return true if channel binding is valid
     */
    bool verifyChannelBinding(
        const std::vector<uint8_t>& token_data,
        const std::vector<uint8_t>& channel_binding
    );
    
    /**
     * @brief Extract service principal from token
     * 
     * @param token_data GSSAPI token
     * @return std::string Service principal name
     */
    std::string extractServicePrincipal(const std::vector<uint8_t>& token_data);
    
    /**
     * @brief Check if ticket is expired
     * 
     * @param token_data GSSAPI token
     * @return true if ticket is expired
     */
    bool isTicketExpired(const std::vector<uint8_t>& token_data);
    
    /**
     * @brief Get token info for debugging/auditing
     */
    struct TokenInfo {
        std::string service_principal;
        std::string client_principal;
        std::string realm;
        int64_t auth_time;
        int64_t start_time;
        int64_t end_time;
        int64_t renew_till;
        bool has_mutual_auth;
        bool has_integrity;
        bool has_confidentiality;
        bool has_channel_binding;
    };
    
    TokenInfo getTokenInfo(const std::vector<uint8_t>& token_data);
    
    /**
     * @brief Create config with channel bindings
     * 
     * @param type Channel binding type
     * @return Config Configuration with channel bindings enabled
     */
    static Config withChannelBindings(ChannelBindingType type);
    
    /**
     * @brief Create config with strict validation
     * 
     * @return Config Maximally strict security configuration
     */
    static Config strictValidation();
    
    /**
     * @brief Create config for specific service
     * 
     * @param service_principal Expected service principal
     * @return Config Configuration with service verification
     */
    static Config forService(const std::string& service_principal);

private:
    Config config_;
    
    // ASN.1 parsing helpers
    struct ASN1Tag {
        uint8_t tag_class;      // Universal, Application, Context, Private
        bool is_constructed;
        uint32_t tag_number;
        size_t length;
        const uint8_t* value;
    };
    
    bool parseASN1Tag(const uint8_t* data, size_t size, ASN1Tag& tag);
    bool validateASN1Depth(const uint8_t* data, size_t size, size_t current_depth);
    
    // Channel binding helpers
    std::vector<uint8_t> computeTLSServerEndpoint(const std::vector<uint8_t>& cert_data);
};

/**
 * @brief Channel Binding Generator
 * 
 * Generates channel binding data from TLS connection info.
 * 
 * Usage:
 * ```cpp
 * ChannelBindingGenerator generator;
 * auto binding = generator.generateFromTLSCertificate(cert_data);
 * validator.verifyChannelBinding(token, binding);
 * ```
 */
class ChannelBindingGenerator {
public:
    /**
     * @brief Generate TLS server endpoint binding
     * 
     * Hash of server certificate (RFC 5929 tls-server-end-point).
     * 
     * @param server_cert Server certificate (DER or PEM)
     * @return std::vector<uint8_t> Channel binding data
     */
    static std::vector<uint8_t> generateFromTLSCertificate(
        const std::vector<uint8_t>& server_cert
    );
    
    /**
     * @brief Generate TLS unique binding
     * 
     * TLS Finished message (RFC 5929 tls-unique).
     * 
     * @param finished_message TLS Finished message
     * @return std::vector<uint8_t> Channel binding data
     */
    static std::vector<uint8_t> generateFromTLSFinished(
        const std::vector<uint8_t>& finished_message
    );
    
    /**
     * @brief Generate TLS exporter binding
     * 
     * TLS exporter (RFC 5705).
     * 
     * @param exporter_value TLS exporter value
     * @return std::vector<uint8_t> Channel binding data
     */
    static std::vector<uint8_t> generateFromTLSExporter(
        const std::vector<uint8_t>& exporter_value
    );
    
    /**
     * @brief Format channel binding for GSSAPI
     * 
     * Formats channel binding data per RFC 5056.
     * 
     * @param initiator_address Initiator address (optional)
     * @param acceptor_address Acceptor address (optional)
     * @param application_data Application-specific data
     * @return std::vector<uint8_t> Formatted channel binding
     */
    static std::vector<uint8_t> formatChannelBinding(
        const std::vector<uint8_t>& initiator_address,
        const std::vector<uint8_t>& acceptor_address,
        const std::vector<uint8_t>& application_data
    );
};

} // namespace auth
} // namespace themis
