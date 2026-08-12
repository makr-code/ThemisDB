/**
 * @file security_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * The builder is fail-closed in production mode:
 * - LOCAL/mock key providers are rejected.
 * - Missing key-provider configuration is rejected.
 * - Missing/invalid JWT configuration is rejected.
 *
 * Production mode is determined via core::ProductionMode.
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
        * For VAULT and HSM providers, configuration is parsed and validated
        * immediately. Invalid JSON or invalid provider settings raise an error.
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
        * Stores certificate path and issuer allow-list in JWT validator config.
        * When allowed_issuers is empty, issuer validation is disabled.
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
        * In development mode, missing key provider falls back to LOCAL/mock,
        * and missing JWT config falls back to a permissive default validator.
        * In production mode, both are hard failures.
        *
     * @return SecurityLayer structure with initialized components
        * @throws std::runtime_error if configuration is invalid or violates
        *         production-mode security requirements
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
        * This preset is intended for development/testing and is not valid for
        * production-mode builds unless additional secure settings are applied
        * before calling build().
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
    
   /**
    * @brief Load complete file contents as text.
    * @param path Input file path.
    * @return File contents.
    * @throws std::runtime_error if the file cannot be opened.
    */
    std::string loadFile(const std::string& path);
    
   /**
    * @brief Create a concrete key provider implementation.
    * @param type Backend type to instantiate.
    * @param config_json Backend configuration JSON.
    * @return Initialized key-provider instance.
    * @throws std::runtime_error on invalid configuration or backend init errors.
    */
    IKeyProviderPtr createKeyProvider(
        KeyProviderType type,
        const std::string& config_json);
};

} // namespace themis
