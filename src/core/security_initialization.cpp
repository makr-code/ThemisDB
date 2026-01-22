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
    
    // Create field encryption
    auto key_provider_impl = std::dynamic_pointer_cast<KeyProvider>(key_provider_);
    if (!key_provider_impl) {
        // If it's not a KeyProvider, create a default mock
        key_provider_impl = std::make_shared<MockKeyProvider>();
    }
    auto field_enc = std::make_shared<FieldEncryption>(key_provider_impl);
    if (!encryption_config_.empty()) {
        field_enc->setEncryptionConfig(encryption_config_);
    }
    layer.field_encryption = field_enc;
    
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

IKeyProviderPtr SecurityLayerBuilder::createKeyProvider(
    KeyProviderType type,
    const std::string& config_json)
{
    nlohmann::json config;
    
    try {
        if (!config_json.empty() && config_json != "{}") {
            config = nlohmann::json::parse(config_json);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse key provider config JSON: " + std::string(e.what()));
    }
    
    switch (type) {
        case KeyProviderType::LOCAL: {
            return std::make_shared<MockKeyProvider>();
        }
        
        case KeyProviderType::VAULT: {
            VaultKeyProvider::Config vault_config;
            
            if (config.contains("vault_addr")) {
                vault_config.vault_addr = config["vault_addr"].get<std::string>();
            }
            if (config.contains("vault_token")) {
                vault_config.vault_token = config["vault_token"].get<std::string>();
            }
            if (config.contains("kv_mount_path")) {
                vault_config.kv_mount_path = config["kv_mount_path"].get<std::string>();
            }
            
            if (vault_config.vault_addr.empty() || vault_config.vault_token.empty()) {
                throw std::runtime_error("VAULT key provider requires vault_addr and vault_token in config");
            }
            
            return std::make_shared<VaultKeyProvider>(vault_config);
        }
        
        // HSM case commented out due to missing dependencies
        // case KeyProviderType::HSM: {
        //     // HSM configuration
        //     std::string library_path;
        //     std::string slot_id;
        //     std::string pin;
        //     
        //     if (config.contains("library_path")) {
        //         library_path = config["library_path"].get<std::string>();
        //     }
        //     if (config.contains("slot_id")) {
        //         slot_id = config["slot_id"].get<std::string>();
        //     }
        //     if (config.contains("pin")) {
        //         pin = config["pin"].get<std::string>();
        //     }
        //     
        //     if (library_path.empty()) {
        //         throw std::runtime_error("HSM key provider requires library_path in config");
        //     }
        //     
        //     return std::make_shared<HSMKeyProviderAdapter>(library_path, slot_id, pin);
        // }
        
        default:
            throw std::runtime_error("Unknown key provider type");
    }
}

} // namespace themis
