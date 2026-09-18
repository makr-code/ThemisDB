/**
 * @file security_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class SecurityLayerBuilder {
public:
    enum class KeyProviderType {
        LOCAL,   ///< In-memory provider for testing/development
        VAULT,   ///< HashiCorp Vault integration
        HSM      ///< Hardware Security Module (PKCS#11)
    };
    
    struct SecurityLayer {
        std::shared_ptr<IFieldEncryption> field_encryption;
        
        std::shared_ptr<security::RBAC> rbac;
        
        std::shared_ptr<auth::JWTValidator> jwt;
    };
    
    SecurityLayerBuilder();
    
    SecurityLayerBuilder& withKeyProvider(
        KeyProviderType type,
        const std::string& config_json = "{}");
    
    /**
     * @brief With Field Encryption.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    SecurityLayerBuilder& withFieldEncryption(
        const EncryptionConfig& config);
    
    /**
     * @brief With RBACPolicy.
     * @param[in] policy_file Input parameter.
     * @return Return value.
     */
    SecurityLayerBuilder& withRBACPolicy(
        const std::string& policy_file);
    
    /**
     * @brief With JWT.
     * @param[in] cert_file Input parameter.
     * @param[in] allowed_issuers Input parameter.
     * @return Return value.
     */
    SecurityLayerBuilder& withJWT(
        const std::string& cert_file,
        const std::vector<std::string>& allowed_issuers);
    
    /**
     * @brief With JWT.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    SecurityLayerBuilder& withJWT(
        const auth::JWTValidatorConfig& config);
    
    /**
     * @brief Build.
     * @return Return value.
     */
    SecurityLayer build();
    
    /**
     * @brief Standard.
     * @return Return value.
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
     * @brief Load File.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::string loadFile(const std::string& path);
    
    /**
     * @brief Create Key Provider.
     * @param[in] type Input parameter.
     * @param[in] config_json Input parameter.
     * @return Return value.
     */
    IKeyProviderPtr createKeyProvider(
        KeyProviderType type,
        const std::string& config_json);
};

} // namespace themis
