/*
 * ThemisDB | File: security_initialization.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 165
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "themis/base/interfaces/security_interface.h"
#include "security/encryption.h"
#include "security/rbac.h"
#include "auth/jwt_validator.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {

/**
 * @brief Security Layer Builder
 * 
 * Provides a fluent API for constructing and configuring the security layer
 * components (FieldEncryption, RBAC, JWT validation).
 * 
 * Usage:
 * @code
 * auto security = SecurityLayerBuilder()
 *     .withKeyProvider(SecurityLayerBuilder::KeyProviderType::VAULT, config_json)
 *     .withFieldEncryption(encryption_config)
 *     .withRBACPolicy(policy_file)
 *     .withJWT(cert_file, {"https://auth.example.com"})
 *     .build();
 * 
 * // Use components
 * security.field_encryption->encrypt_field("ssn", data);
 * security.rbac->checkPermission({"admin"}, "data", "write");
 * security.jwt->parseAndValidate(token);
 * @endcode
 */
class SecurityLayerBuilder {
public:
    /**
     * @brief Key provider types
     */
    enum class KeyProviderType {
        LOCAL,   ///< In-memory provider for testing/development
        VAULT,   ///< HashiCorp Vault integration
        HSM      ///< Hardware Security Module (PKCS#11)
    };
    
    /**
     * @brief Security layer components
     */
    struct SecurityLayer {
        /// Field-level encryption component
        std::shared_ptr<IFieldEncryption> field_encryption;
        
        /// Role-Based Access Control component
        std::shared_ptr<security::RBAC> rbac;
        
        /// JWT validator component
        std::shared_ptr<auth::JWTValidator> jwt;
    };
    
    /**
     * @brief Construct a new SecurityLayerBuilder
     */
    SecurityLayerBuilder();
    
    /**
     * @brief Set key provider type and configuration
     * 
     * @param type Type of key provider (LOCAL, VAULT, HSM)
     * @param config_json JSON configuration for the provider
     * @return Reference to this builder for chaining
     */
    SecurityLayerBuilder& withKeyProvider(
        KeyProviderType type,
        const std::string& config_json = "{}");
    
    /**
     * @brief Set field encryption configuration
     * 
     * @param config Encryption configuration defining which fields to encrypt
     * @return Reference to this builder for chaining
     */
    SecurityLayerBuilder& withFieldEncryption(
        const EncryptionConfig& config);
    
    /**
     * @brief Set RBAC policy file
     * 
     * @param policy_file Path to RBAC policy configuration file (JSON/YAML)
     * @return Reference to this builder for chaining
     */
    SecurityLayerBuilder& withRBACPolicy(
        const std::string& policy_file);
    
    /**
     * @brief Set JWT validation configuration
     * 
     * @param cert_file Path to JWT signing certificate (PEM format)
     * @param allowed_issuers List of allowed token issuers
     * @return Reference to this builder for chaining
     */
    SecurityLayerBuilder& withJWT(
        const std::string& cert_file,
        const std::vector<std::string>& allowed_issuers);
    
    /**
     * @brief Set JWT validation configuration with full config
     * 
     * @param config JWT validator configuration
     * @return Reference to this builder for chaining
     */
    SecurityLayerBuilder& withJWT(
        const auth::JWTValidatorConfig& config);
    
    /**
     * @brief Build the complete security layer
     * 
     * Creates and initializes all security components based on the
     * builder configuration.
     * 
     * @return SecurityLayer structure with initialized components
     * @throws std::runtime_error if configuration is invalid
     */
    SecurityLayer build();
    
    /**
     * @brief Create a standard security layer with defaults
     * 
     * Uses:
     * - Local key provider (for development/testing)
     * - No field encryption configuration (encrypt all fields)
     * - No RBAC policy (allow all)
     * - No JWT validation
     * 
     * @return SecurityLayerBuilder with default configuration
     */
    static SecurityLayerBuilder standard();
    
private:
    IKeyProviderPtr key_provider_;
    std::optional<KeyProviderType> key_provider_type_;
    std::string key_provider_config_;
    EncryptionConfig encryption_config_;
    std::string rbac_policy_file_;
    auth::JWTValidatorConfig jwt_config_;
    bool jwt_configured_ = false;
    
    /// Helper: Load file contents
    std::string loadFile(const std::string& path);
    
    /// Helper: Create key provider from type
    IKeyProviderPtr createKeyProvider(
        KeyProviderType type,
        const std::string& config_json);
};

} // namespace themis
