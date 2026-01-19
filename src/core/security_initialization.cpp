#include "core/security_initialization.h"
#include "security/mock_key_provider.h"
#include "security/vault_key_provider.h"
#include "security/hsm_key_provider_adapter.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis {

SecurityLayerBuilder::SecurityLayerBuilder() = default;

SecurityLayerBuilder& SecurityLayerBuilder::withKeyProvider(
    KeyProviderType type,
    const std::string& config_json)
{
    // Key provider initialization deferred - to be implemented with proper interface design
    // For now, this is a placeholder to maintain API compatibility
    return *this;
}

SecurityLayerBuilder& SecurityLayerBuilder::withFieldEncryption(
    const EncryptionConfig& config)
{
    encryption_config_ = config;
    return *this;
}

SecurityLayerBuilder& SecurityLayerBuilder::withRBACPolicy(
    const std::string& policy_file)
{
    rbac_policy_file_ = policy_file;
    return *this;
}

SecurityLayerBuilder& SecurityLayerBuilder::withJWT(
    const std::string& cert_file,
    const std::vector<std::string>& allowed_issuers)
{
    // For now, store the cert file path
    // In a real implementation, we'd load the certificate
    jwt_config_.jwks_url = cert_file;  // Using jwks_url to store cert path
    jwt_config_.expected_issuer = allowed_issuers.empty() ? "" : allowed_issuers[0];
    jwt_configured_ = true;
    return *this;
}

SecurityLayerBuilder& SecurityLayerBuilder::withJWT(
    const auth::JWTValidatorConfig& config)
{
    jwt_config_ = config;
    jwt_configured_ = true;
    return *this;
}

SecurityLayerBuilder::SecurityLayer SecurityLayerBuilder::build() {
    SecurityLayer layer;
    
    // Note: Key provider initialization deferred - to be properly implemented in future
    // For now, security layer can be used with external key management
    
    // Create field encryption with stub (to be properly integrated)
    // auto field_enc = std::make_shared<FieldEncryption>(key_provider_);
    // if (!encryption_config_.empty()) {
    //     field_enc->setEncryptionConfig(encryption_config_);
    // }
    // layer.field_encryption = field_enc;
    
    // Create RBAC
    security::RBACConfig rbac_config;
    if (!rbac_policy_file_.empty()) {
        rbac_config.config_path = rbac_policy_file_;
        rbac_config.use_builtin_roles = false;
    }
    layer.rbac = std::make_shared<security::RBAC>(rbac_config);
    
    // Load RBAC policy if file is specified
    if (!rbac_policy_file_.empty()) {
        if (!layer.rbac->loadConfig(rbac_policy_file_)) {
            throw std::runtime_error("Failed to load RBAC policy from: " + rbac_policy_file_);
        }
    }
    
    // Create JWT validator
    if (jwt_configured_) {
        layer.jwt = std::make_shared<auth::JWTValidator>(jwt_config_);
    } else {
        // Create a default validator with empty config
        layer.jwt = std::make_shared<auth::JWTValidator>("");
    }
    
    return layer;
}

SecurityLayerBuilder SecurityLayerBuilder::standard() {
    return SecurityLayerBuilder()
        .withKeyProvider(KeyProviderType::LOCAL, "{}");
}

std::string SecurityLayerBuilder::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// NOTE: createKeyProvider() implementation removed - key provider types not properly integrated yet
// This function is deprecated and not used in the current architecture
// To be re-implemented with proper interface design

} // namespace themis
