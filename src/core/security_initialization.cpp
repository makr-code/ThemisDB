/**
 * @file security_initialization.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "core/security_initialization.h"
#include "core/production_mode.h"
#include "core/config_validator.h"
#include "security/mock_key_provider.h"
#include "security/vault_key_provider.h"
#include "security/hsm_provider.h"
#include "security/hsm_key_provider_adapter.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis {

/**
 * @brief Construct a security-layer builder with empty/default state.
 */
SecurityLayerBuilder::SecurityLayerBuilder() = default;

/**
 * @brief Configure key-provider type and raw JSON config payload.
 * @param type Key provider selection.
 * @param config_json Provider configuration JSON.
 * @return Reference to this builder for fluent chaining.
 * @throws std::runtime_error If JSON parsing fails or provider-specific validation fails.
 */
SecurityLayerBuilder& SecurityLayerBuilder::withKeyProvider(
    KeyProviderType type,
    const std::string& config_json)
{
    // Parse and validate configuration
    nlohmann::json config;
    
    try {
        if (!config_json.empty() && config_json != "{}") {
            config = nlohmann::json::parse(config_json);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse key provider config JSON: " + std::string(e.what()));
    }
    
    // Validate configuration based on type
    if (type == KeyProviderType::VAULT) {
        auto validation = core::ConfigValidator::validateVaultConfig(config);
        if (!validation.valid) {
            throw std::runtime_error("Invalid Vault configuration:\n" + validation.formatErrors());
        }
    }
    
    // Store provider type and config for later initialization in build()
    key_provider_type_ = type;
    key_provider_config_ = config_json;
    
    return *this;
}

/**
 * @brief Configure field-encryption rules.
 * @param config Encryption policy object.
 * @return Reference to this builder.
 */
SecurityLayerBuilder& SecurityLayerBuilder::withFieldEncryption(
    const EncryptionConfig& config)
{
    encryption_config_ = config;
    return *this;
}

/**
 * @brief Configure RBAC policy file path.
 * @param policy_file Filesystem path to RBAC policy config.
 * @return Reference to this builder.
 */
SecurityLayerBuilder& SecurityLayerBuilder::withRBACPolicy(
    const std::string& policy_file)
{
    rbac_policy_file_ = policy_file;
    return *this;
}

/**
 * @brief Configure JWT validator via certificate path and issuer allow-list.
 * @param cert_file Certificate/JWKS file path.
 * @param allowed_issuers Allowed issuer list. Empty disables issuer validation.
 * @return Reference to this builder.
 */
SecurityLayerBuilder& SecurityLayerBuilder::withJWT(
    const std::string& cert_file,
    const std::vector<std::string>& allowed_issuers)
{
    // For now, store the cert file path
    // In a real implementation, we'd load the certificate
    jwt_config_.jwks_url = cert_file;  // Using jwks_url to store cert path
    if (!allowed_issuers.empty()) {
        jwt_config_.expected_issuer = allowed_issuers[0];
    } else {
        jwt_config_.require_issuer_validation = false;
    }
    jwt_configured_ = true;
    return *this;
}

/**
 * @brief Configure JWT validator via full configuration struct.
 * @param config JWT validator configuration.
 * @return Reference to this builder.
 */
SecurityLayerBuilder& SecurityLayerBuilder::withJWT(
    const auth::JWTValidatorConfig& config)
{
    jwt_config_ = config;
    jwt_configured_ = true;
    return *this;
}

/**
 * @brief Build fully initialized security components.
 * @return SecurityLayer with encryption, RBAC, and JWT components.
 * @throws std::runtime_error On invalid configuration, failed policy loading,
 *         failed provider initialization, or production-mode policy violations.
 * @note Production mode is fail-closed: mock/local key providers and missing JWT
 *       configuration are rejected.
 */
SecurityLayerBuilder::SecurityLayer SecurityLayerBuilder::build() {
    SecurityLayer layer;
    
    bool production_mode = core::ProductionMode::isEnabled();
    
    // Create or validate key provider
    IKeyProviderPtr key_provider_impl = {};
    
    if (key_provider_type_.has_value()) {
        // User explicitly configured a key provider
        key_provider_impl = createKeyProvider(key_provider_type_.value(), key_provider_config_);
        
        // In production, reject mock/local providers
        if (production_mode && key_provider_type_.value() == KeyProviderType::LOCAL) {
            throw std::runtime_error(
                "Production mode violation: LOCAL (mock) key provider is not allowed in production. "
                "Use VAULT or HSM key provider instead. "
                "Set THEMIS_PRODUCTION_MODE=0 or THEMIS_ENVIRONMENT=development for testing."
            );
        }
    } else {
        // No key provider configured
        if (production_mode) {
            throw std::runtime_error(
                "Production mode violation: No key provider configured. "
                "Call withKeyProvider() with VAULT or HSM configuration before build(). "
                "Mock/default key providers are not allowed in production."
            );
        } else {
            // In development, allow default mock provider
            key_provider_impl = std::make_shared<MockKeyProvider>();
        }
    }
    
    // Cast to KeyProvider interface
    auto key_provider_concrete = std::dynamic_pointer_cast<KeyProvider>(key_provider_impl);
    if (!key_provider_concrete) {
        throw std::runtime_error("Key provider does not implement KeyProvider interface");
    }
    
    // Create field encryption
    auto field_enc = std::make_shared<FieldEncryption>(key_provider_concrete);
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
        // Validate JWT configuration
        auto validation = core::ConfigValidator::validateJWTConfig(jwt_config_, production_mode);
        if (!validation.valid) {
            throw std::runtime_error("Invalid JWT configuration:\n" + validation.formatErrors());
        }
        
        layer.jwt = std::make_shared<auth::JWTValidator>(jwt_config_);
    } else {
        // No JWT configured
        if (production_mode) {
            throw std::runtime_error(
                "Production mode violation: No JWT validation configured. "
                "Call withJWT() with proper configuration before build(). "
                "JWT validation is required in production mode."
            );
        } else {
            // Create a default validator with empty config (for development)
            layer.jwt = std::make_shared<auth::JWTValidator>("");
        }
    }
    
    return layer;
}

/**
 * @brief Create a builder with development-friendly defaults.
 * @return Builder preconfigured with LOCAL key provider.
 */
SecurityLayerBuilder SecurityLayerBuilder::standard() {
    return SecurityLayerBuilder()
        .withKeyProvider(KeyProviderType::LOCAL, "{}");
}

/**
 * @brief Load full text content from a file path.
 * @param path File path to read.
 * @return File content as a string.
 * @throws std::runtime_error If the file cannot be opened.
 */
std::string SecurityLayerBuilder::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    std::stringstream buffer = {};
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Instantiate concrete key provider based on type and JSON config.
 * @param type Selected key-provider backend.
 * @param config_json JSON payload for backend-specific options.
 * @return Key provider instance suitable for field encryption.
 * @throws std::runtime_error If config parsing/validation fails, backend is not
 *         enabled, or provider initialization fails.
 */
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
            if (config.contains("tls_skip_verify")) {
                vault_config.verify_ssl = !config["tls_skip_verify"].get<bool>();
            }
            // Note: namespace and role are Vault Enterprise features not yet in the config struct
            // They are validated but not used until VaultKeyProvider::Config is extended
            
            if (vault_config.vault_addr.empty() || vault_config.vault_token.empty()) {
                throw std::runtime_error("VAULT key provider requires vault_addr and vault_token in config");
            }
            
            return std::make_shared<VaultKeyProvider>(vault_config);
        }
        
        case KeyProviderType::HSM: {
            // Check if HSM support is enabled
            const char* hsm_enabled = std::getenv("THEMIS_HSM_ENABLED");
            if (!hsm_enabled || std::string(hsm_enabled) != "1") {
                throw std::runtime_error(
                    "HSM key provider is not enabled. "
                    "Set THEMIS_HSM_ENABLED=1 to enable HSM support. "
                    "Note: HSM support requires PKCS#11 libraries to be installed."
                );
            }
            
            // HSM configuration
            std::string library_path = {};
            std::string slot_id = {};
            std::string pin = {};
            
            if (config.contains("library_path")) {
                library_path = config["library_path"].get<std::string>();
            }
            if (config.contains("slot_id")) {
                slot_id = config["slot_id"].get<std::string>();
            }
            if (config.contains("pin")) {
                pin = config["pin"].get<std::string>();
            }
            
            if (library_path.empty()) {
                throw std::runtime_error("HSM key provider requires library_path in config");
            }

            std::error_code library_ec = {};
            const std::filesystem::path library_fs_path(library_path);
            if (!std::filesystem::exists(library_fs_path, library_ec) ||
                !std::filesystem::is_regular_file(library_fs_path, library_ec)) {
                throw std::runtime_error("HSM key provider library_path does not point to an existing file: " +
                                         library_path);
            }
            
            try {
                security::HSMConfig hsm_config;
                hsm_config.library_path = library_path;
                if (!slot_id.empty()) {
                    const bool valid_slot_id = std::all_of(
                        slot_id.begin(), slot_id.end(),
                        [](unsigned char ch) { return std::isdigit(ch) != 0; });
                    if (!valid_slot_id) {
                        throw std::runtime_error("HSM key provider slot_id must be an unsigned integer");
                    }

                    const unsigned long long parsed_slot_id = std::stoull(slot_id);
                    if (parsed_slot_id > std::numeric_limits<uint32_t>::max()) {
                        throw std::runtime_error("HSM key provider slot_id exceeds uint32 range");
                    }

                    hsm_config.slot_id = static_cast<uint32_t>(parsed_slot_id);
                }
                hsm_config.pin = pin;

                auto hsm = std::make_shared<security::HSMProvider>(hsm_config);
                if (!hsm->initialize()) {
                    throw std::runtime_error("HSM provider initialization failed");
                }

                return std::make_shared<security::HSMKeyProviderAdapter>(hsm);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to initialize HSM key provider: " + std::string(e.what()));
            }
        }
        
        default:
            throw std::runtime_error("Unknown key provider type");
    }
}

} // namespace themis
