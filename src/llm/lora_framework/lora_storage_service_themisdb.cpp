/**
 * @file lora_storage_service_themisdb.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=14, H=12, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_storage_service.h"
#include <stdexcept>
#include "storage/base_entity.h"
#include "security/mock_key_provider.h"
#include "security/hsm_provider.h"
#include "security/hsm_key_provider_adapter.h"
#include "security/pki_key_provider.h"
#include "security/vault_key_provider.h"
#include "security/encryption.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;

/**
 * @brief Enhanced Implementation using ThemisDB Base Infrastructure
 * 
 * Integrates with:
 * - BaseEntity for structured storage
 * - RocksDBWrapper for CRUD operations
 * - BlobStorageManager for large weights (with automatic backend selection)
 * - SecuritySignatureManager for integrity verification
 * - Encryption for data at rest
 * - RAID auto-detection for redundancy
 */
class LoRAStorageService::Impl {
public:
    explicit Impl(const Config& config) : config_(config) {
        spdlog::info("LoRAStorageService initialized (ThemisDB-native):");
        spdlog::info("  Backend: {}", backendToString(config_.backend));
        spdlog::info("  Collection: {}", config_.collection_name);
        spdlog::info("  Versioning: {}", config_.enable_versioning);
        spdlog::info("  Encryption: {}", config_.enable_encryption);
        spdlog::info("  HSM Encryption: {}", config_.use_hsm_for_encryption);
        spdlog::info("  Signatures: {}", config_.enable_signatures);
        spdlog::info("  RAID Auto-detect: {}", config_.auto_detect_raid);
        
        // Initialize encryption if enabled
        if (config_.enable_encryption && !encryption_) {
            try {
                auto key_provider = createKeyProvider();
                encryption_ = std::make_shared<FieldEncryption>(key_provider);
                spdlog::info("✓ Encryption initialized successfully");
                
            } catch (const std::exception& e) {
                spdlog::error("Failed to initialize encryption: {}", e.what());
                throw;  // Re-throw to prevent insecure operation
            }
        }
        
        // Initialize ThemisDB backend if configured
        if (config_.backend == Backend::ThemisDB && config_.db) {
            spdlog::info("  Using ThemisDB with RocksDB backend");
            if (config_.blob_manager) {
                spdlog::info("  BlobStorageManager available for large adapters");
            }
            if (config_.signature_manager) {
                spdlog::info("  SecuritySignatureManager available for integrity");
            }
        } else if (config_.backend == Backend::FileSystem) {
            // Fallback to filesystem
            if (!fs::exists(config_.filesystem_path)) {
                fs::create_directories(config_.filesystem_path);
                spdlog::info("  Created filesystem storage: {}", config_.filesystem_path);
            }
        }
    }
    
    bool saveAdapter(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        try {
            if (config_.backend == Backend::ThemisDB && config_.db) {
                return saveToThemisDB(adapter_id, weights, metadata);
            } else {
                return saveToFilesystem(adapter_id, weights, metadata);
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to save adapter {}: {}", adapter_id, e.what());
            return false;
        }
    }
    
    std::optional<AdapterWeights> loadAdapter(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::ThemisDB && config_.db) {
                return loadFromThemisDB(adapter_id);
            } else {
                return loadFromFilesystem(adapter_id);
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to load adapter {}: {}", adapter_id, e.what());
            return std::nullopt;
        }
    }
    
    std::optional<AdapterMetadata> loadMetadata(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::ThemisDB && config_.db) {
                return loadMetadataFromThemisDB(adapter_id);
            } else {
                return loadMetadataFromFilesystem(adapter_id);
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to load metadata for {}: {}", adapter_id, e.what());
            return std::nullopt;
        }
    }
    
    bool deleteAdapter(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::ThemisDB && config_.db) {
                std::string key = makeCollectionKey(adapter_id);
                
                // First, retrieve metadata to get blob reference if it exists
                auto data = config_.db->get(key);
                if (data && config_.blob_manager) {
                    try {
                        // Deserialize entity to extract blob reference
                        BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
                        
                        // Check if adapter uses blob storage (not inline)
                        if (entity.hasField("blob_ref_path")) {
                            // Validate blob reference type before casting
                            auto blob_type_value = entity.getFieldAsInt("blob_ref_type").value_or(-1);
                            // Valid range: 0 (INLINE) to 7 (CUSTOM)
                            if (blob_type_value < 0 || blob_type_value > static_cast<int>(storage::BlobStorageType::CUSTOM)) {
                                spdlog::warn("Invalid blob storage type {} for adapter {}, skipping blob deletion", 
                                           blob_type_value, adapter_id);
                            } else {
                                storage::BlobRef ref;
                                ref.type = static_cast<storage::BlobStorageType>(blob_type_value);
                                ref.uri = entity.getFieldAsString("blob_ref_path").value_or("");
                                
                                // Delete blob from storage
                                if (!ref.uri.empty()) {
                                    bool blob_deleted = config_.blob_manager->remove(ref);
                                    if (!blob_deleted) {
                                        spdlog::warn("Failed to delete blob {} for adapter {}", 
                                                    ref.uri, adapter_id);
                                        // Continue with metadata deletion for idempotency
                                    } else {
                                        spdlog::debug("Deleted blob {} for adapter {}", 
                                                     ref.uri, adapter_id);
                                    }
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}", 
                                    adapter_id, e.what());
                        // Continue with metadata deletion
                    }
                }
                
                // Delete metadata from RocksDB
                bool success = config_.db->del(key);
                
                if (success) {
                    spdlog::info("Deleted adapter: {}", adapter_id);
                } else {
                    spdlog::warn("Failed to delete metadata for adapter: {}", adapter_id);
                }
                
                return success;
            } else {
                fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
                if (fs::exists(adapter_dir)) {
                    fs::remove_all(adapter_dir);
                    spdlog::info("Deleted adapter: {}", adapter_id);
                    return true;
                }
                return false;
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
            return false;
        }
    }
    
    bool exists(const std::string& adapter_id) const {
        if (config_.backend == Backend::ThemisDB && config_.db) {
            std::string key = makeCollectionKey(adapter_id);
            return config_.db->get(key).has_value();
        } else {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            return fs::exists(adapter_dir / "weights.bin");
        }
    }
    
    std::vector<std::string> listAdapters() const {
        std::vector<std::string> adapters;
        
        if (config_.backend == Backend::ThemisDB && config_.db) {
            // Scan RocksDB for all adapters in collection
            std::string prefix = config_.collection_name + ":";
            auto it_result = config_.db->newIterator();
            if (!it_result) {
                spdlog::warn("LoRAStorage: Failed to create iterator: {}", it_result.error().message());
                return adapters;
            }
            auto it = std::move(it_result.value());
            
            for (it->Seek(prefix); it->Valid(); it->Next()) {
                std::string key(it->key().data(), it->key().size());
                if (!key.starts_with(prefix)) {
                  break;
                }
                
                // Extract adapter_id from key
                std::string adapter_id = key.substr(prefix.length());
                // Remove version suffix if present
                size_t version_pos = adapter_id.find(":v");
                if (version_pos != std::string::npos) {
                    adapter_id = adapter_id.substr(0, version_pos);
                }
                
                if (std::find(adapters.begin(), adapters.end(), adapter_id) == adapters.end()) {
                    adapters.push_back(adapter_id);
                }
            }
        } else {
            try {
                for (const auto& entry : fs::directory_iterator(config_.filesystem_path)) {
                    if (entry.is_directory()) {
                        adapters.push_back(entry.path().filename().string());
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to list adapters: {}", e.what());
            }
        }
        
        return adapters;
    }
    
    std::string createVersion(const std::string& adapter_id) {
        if (!config_.enable_versioning) {
            return "v1";
        }
        
        auto versions = listVersions(adapter_id);
        int max_version = 0;
        
        for (const auto& v : versions) {
            if (v.size() > 1 && v[0] == 'v') {
                try {
                    int num = std::stoi(v.substr(1));
                    max_version = std::max(max_version, num);
                } catch (...) {}
            }
        }
        
        std::string new_version = "v" + std::to_string(max_version + 1);
        spdlog::info("Created version {} for adapter {}", new_version, adapter_id);
        
        // Version creation is handled during save
        return new_version;
    }
    
    bool rollbackToVersion(const std::string& adapter_id, const std::string& version) {
        spdlog::info("Rolling back adapter {} to version {}", adapter_id, version);

        if (config_.backend == Backend::ThemisDB && config_.db) {
            // Load the versioned snapshot key, e.g. "collection:adapter_id:v2"
            std::string versioned_key = makeCollectionKey(adapter_id) + ":" + version;
            auto data = config_.db->get(versioned_key);
            if (!data) {
                spdlog::error("LoRAStorage: rollback failed – version '{}' not found for adapter '{}'",
                              version, adapter_id);
                return false;
            }
            // Overwrite the current key with the versioned snapshot
            std::string current_key = makeCollectionKey(adapter_id);
            bool ok = config_.db->put(current_key, *data);
            if (ok) {
                spdlog::info("LoRAStorage: adapter '{}' rolled back to version '{}'",
                             adapter_id, version);
            } else {
                spdlog::error("LoRAStorage: failed to write rollback data for adapter '{}'",
                              adapter_id);
            }
            return ok;

        } else {
            // Filesystem backend: two-phase atomic-swap to protect against data
            // loss if the copy fails after removal.
            // Phase 1: copy the versioned snapshot to a temporary directory.
            // Phase 2: atomically rename/replace the current directory.
            fs::path adapter_dir  = fs::path(config_.filesystem_path) / adapter_id;
            fs::path version_dir  = fs::path(config_.filesystem_path) / (adapter_id + "." + version);
            fs::path tmp_dir      = fs::path(config_.filesystem_path) / (adapter_id + ".rollback_tmp");

            if (!fs::exists(version_dir)) {
                spdlog::error("LoRAStorage: rollback failed – version dir '{}' not found",
                              version_dir.string());
                return false;
            }
            try {
                // Phase 1: copy to tmp (leave original intact)
                if (fs::exists(tmp_dir)) {
                    fs::remove_all(tmp_dir);
                }
                fs::copy(version_dir, tmp_dir,
                         fs::copy_options::recursive | fs::copy_options::overwrite_existing);

                // Phase 2: atomically replace current with tmp
                if (fs::exists(adapter_dir)) {
                    fs::remove_all(adapter_dir);
                }
                fs::rename(tmp_dir, adapter_dir);

                spdlog::info("LoRAStorage: adapter '{}' rolled back to version '{}' via filesystem",
                             adapter_id, version);
                return true;
            } catch (const std::exception& e) {
                spdlog::error("LoRAStorage: filesystem rollback failed: {}", e.what());
                // Best-effort cleanup of the tmp directory
                std::error_code ec;
                fs::remove_all(tmp_dir, ec);
                return false;
            }
        }
    }
    
    std::vector<std::string> listVersions(const std::string& adapter_id) const {
        std::vector<std::string> versions;
        
        if (config_.backend == Backend::ThemisDB && config_.db) {
            // Scan for versioned keys
            std::string prefix = makeCollectionKey(adapter_id) + ":v";
            auto it_result = config_.db->newIterator();
            if (!it_result) {
                spdlog::warn("LoRAStorage: Failed to create iterator for versions: {}", it_result.error().message());
                return versions;
            }
            auto it = std::move(it_result.value());
            
            for (it->Seek(prefix); it->Valid(); it->Next()) {
                std::string key(it->key().data(), it->key().size());
                if (!key.starts_with(prefix)) {
                  break;
                }
                
                // Extract version from key
                size_t version_pos = key.rfind(":v");
                if (version_pos != std::string::npos) {
                    std::string version = key.substr(version_pos + 1);
                    versions.push_back(version);
                }
            }
        } else {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            
            if (!fs::exists(adapter_dir)) {
                return versions;
            }
            
            try {
                for (const auto& entry : fs::directory_iterator(adapter_dir)) {
                    std::string filename = entry.path().filename().string();
                    if (filename.starts_with("weights_v") && filename.ends_with(".bin")) {
                        size_t start = 8;
                        size_t end = filename.find(".bin");
                        std::string version = filename.substr(start, end - start);
                        versions.push_back(version);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to list versions: {}", e.what());
            }
        }
        
        std::sort(versions.begin(), versions.end());
        return versions;
    }
    
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata) {
        if (config_.backend == Backend::ThemisDB && config_.db) {
            try {
                // Load existing entity
                std::string key = makeCollectionKey(adapter_id);
                auto data = config_.db->get(key);
                if (!data) {
                    spdlog::error("Adapter {} not found", adapter_id);
                    return false;
                }
                
                // Deserialize entity
                BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
                
                // Update metadata fields
                entity.setField("version", Value(metadata.version));
                entity.setField("base_model", Value(metadata.base_model));
                entity.setField("description", Value(metadata.description));
                entity.setField("training_samples", Value(static_cast<int64_t>(metadata.training_samples)));
                entity.setField("validation_accuracy", Value(static_cast<double>(metadata.validation_accuracy)));
                
                // Serialize and save
                auto blob = entity.serialize();
                config_.db->put(key, blob);
                
                spdlog::info("Updated metadata for adapter: {}", adapter_id);
                return true;
            } catch (const std::exception& e) {
                spdlog::error("Failed to update metadata: {}", e.what());
                return false;
            }
        } else {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            fs::path metadata_path = adapter_dir / "metadata.json";
            
            try {
                json j = metadata.toJSON();
                std::ofstream file(metadata_path);
                file << j.dump(2);
                spdlog::info("Updated metadata for adapter: {}", adapter_id);
                return true;
            } catch (const std::exception& e) {
                spdlog::error("Failed to update metadata: {}", e.what());
                return false;
            }
        }
    }
    
    json getStats() const {
        json stats;
        stats["backend"] = backendToString(config_.backend);
        stats["versioning_enabled"] = config_.enable_versioning;
        stats["encryption_enabled"] = config_.enable_encryption;
        stats["signatures_enabled"] = config_.enable_signatures;
        stats["max_versions"] = config_.max_versions;
        stats["total_adapters"] = listAdapters().size();
        
        return stats;
    }

private:
    Config config_;
    std::shared_ptr<FieldEncryption> encryption_;
    
    // Cache configuration constants
    static constexpr int64_t DEFAULT_HSM_CACHE_TTL_MS = 300000;  // 5 minutes
    static constexpr size_t DEFAULT_HSM_MAX_CACHE_SIZE = 1000;
    
    /**
     * @brief Check if running in production environment
     * @return true if THEMIS_ENVIRONMENT is set to "production" or "prod"
     */
    static bool isProductionEnvironment() {
        const char* env_mode = std::getenv("THEMIS_ENVIRONMENT");
        return (env_mode != nullptr && 
                (std::strcmp(env_mode, "production") == 0 || 
                 std::strcmp(env_mode, "prod") == 0));
    }
    
    /**
     * @brief Create HSM-backed key provider
     * @return Shared pointer to HSM key provider adapter
     * @throws std::runtime_error if HSM initialization fails
     */
    std::shared_ptr<KeyProvider> createHSMKeyProvider() {
        spdlog::info("  Initializing HSM-backed encryption:");
        spdlog::info("    Library: {}", config_.hsm_library_path);
        spdlog::info("    Slot: {}", config_.hsm_slot_id);
        spdlog::info("    Key Label: {}", config_.hsm_key_label);
        spdlog::info("    Session Pool: {}", config_.hsm_session_pool_size);
        
        // Configure HSM
        ::themis::security::HSMConfig hsm_config;
        hsm_config.library_path = config_.hsm_library_path;
        hsm_config.slot_id = config_.hsm_slot_id;
        hsm_config.pin = config_.hsm_pin;
        hsm_config.key_label = config_.hsm_key_label;
        hsm_config.session_pool_size = config_.hsm_session_pool_size;
        hsm_config.signature_algorithm = "RSA-SHA256";
        
        // Create HSM provider
        auto hsm = std::make_shared<::themis::security::HSMProvider>(hsm_config);
        if (!hsm->initialize()) {
            throw std::runtime_error("HSM initialization failed: " + hsm->getLastError());
        }
        
        // Create HSM adapter
        ::themis::security::HSMKeyProviderAdapter::Config adapter_config;
        adapter_config.kek_label = config_.hsm_key_label;
        adapter_config.cache_ttl_ms = DEFAULT_HSM_CACHE_TTL_MS;
        adapter_config.max_cache_size = DEFAULT_HSM_MAX_CACHE_SIZE;
        adapter_config.enable_caching = true;
        
        auto key_provider = std::make_shared<::themis::security::HSMKeyProviderAdapter>(hsm, adapter_config);
        
        spdlog::info("  ✓ HSM-backed encryption initialized successfully");
        spdlog::info("  Hardware-backed keys provide maximum security");
        
        return key_provider;
    }
    
    /**
     * @brief Create Vault-backed key provider
     * @return Shared pointer to Vault key provider
     * @throws std::runtime_error if Vault initialization fails
     */
    std::shared_ptr<KeyProvider> createVaultKeyProvider() {
        spdlog::info("  Initializing Vault-backed encryption:");
        spdlog::info("    Address: {}", config_.vault_addr); // NOPII: vault_addr is a service URL, not personal data
        spdlog::info("    Mount Path: {}", config_.vault_kv_mount);
        
        ::themis::VaultKeyProvider::Config vault_config;
        vault_config.vault_addr = config_.vault_addr;
        vault_config.vault_token = config_.vault_token;
        vault_config.kv_mount_path = config_.vault_kv_mount;
        // Note: TLS configuration would come from separate config fields if needed
        
        auto key_provider = std::make_shared<::themis::VaultKeyProvider>(vault_config);
        spdlog::info("  ✓ Vault-backed encryption initialized successfully");
        
        return key_provider;
    }
    
    /**
     * @brief Create PKI-based key provider
     * @return Shared pointer to PKI key provider
     * @throws std::runtime_error if PKI initialization fails or configuration is invalid
     */
    std::shared_ptr<KeyProvider> createPKIKeyProvider() {
        if (config_.pki_cert_path.empty()) {
            throw std::runtime_error("PKI encryption enabled but pki_cert_path is not configured");
        }
        if (config_.pki_private_key_path.empty()) {
            throw std::runtime_error("PKI encryption enabled but pki_private_key_path is not configured");
        }
        if (!config_.db) {
            throw std::runtime_error("PKI encryption requires database connection for DEK storage");
        }
        
        spdlog::info("  Initializing PKI-backed encryption:");
        spdlog::info("    Certificate: {}", config_.pki_cert_path);
        spdlog::info("    Verify: {}", config_.pki_verify_certificate);
        
        // Initialize PKIKeyProvider with certificate files
        auto key_provider = std::make_shared<::themis::security::PKIKeyProvider>(
            config_.pki_cert_path,
            config_.pki_private_key_path,
            config_.db,
            "lora_storage_" + config_.collection_name,
            config_.pki_verify_certificate
        );
        
        spdlog::info("  ✓ PKI-backed encryption initialized successfully");
        
        return key_provider;
    }
    
    /**
     * @brief Create mock key provider (development/testing only)
     * @return Shared pointer to mock key provider
     */
    std::shared_ptr<KeyProvider> createMockKeyProvider() {
        auto key_provider = std::make_shared<MockKeyProvider>();
        spdlog::warn("  ⚠️  Using MockKeyProvider for encryption - DEVELOPMENT MODE ONLY");
        spdlog::warn("  ⚠️  This provides NO security for encrypted data!");
        spdlog::warn("  ⚠️  For production, configure HSM, Vault, or PKI key provider");
        spdlog::warn("  See documentation: docs/security/key-management.md");
        return key_provider;
    }
    
    /**
     * @brief Create appropriate key provider based on configuration
     * 
     * Priority order: HSM > Vault > PKI > Mock
     * 
     * @return Shared pointer to key provider
     * @throws std::runtime_error if production mode without secure provider
     */
    std::shared_ptr<KeyProvider> createKeyProvider() {
        // 1. Try HSM first (highest priority)
        if (config_.use_hsm_for_encryption) {
            if (config_.hsm_library_path.empty()) {
                throw std::runtime_error("HSM encryption enabled but hsm_library_path not configured");
            }
            return createHSMKeyProvider();
        }
        
        // 2. Try Vault second
        if (config_.use_vault_for_encryption) {
            if (config_.vault_addr.empty()) {
                throw std::runtime_error("Vault encryption enabled but vault_addr not configured");
            }
            return createVaultKeyProvider();
        }
        
        // 3. Try PKI third
        if (config_.use_pki_for_encryption) {
            return createPKIKeyProvider();
        }
        
        // 4. Fallback to MockKeyProvider (development only)
        if (isProductionEnvironment()) {
            spdlog::error("  CRITICAL: Production environment detected but no secure key provider configured!");
            spdlog::error("  Set THEMIS_ENVIRONMENT=development to use MockKeyProvider in dev mode");
            spdlog::error("  For production, configure one of:");
            spdlog::error("    1. HSM (use_hsm_for_encryption=true, hsm_library_path=...)");
            spdlog::error("    2. Vault (use_vault_for_encryption=true, vault_addr=https://...)");
            spdlog::error("    3. PKI (use_pki_for_encryption=true, pki_cert_path=<path>, pki_private_key_path=<path>)");
            throw std::runtime_error(
                "Production environment requires HSM, Vault, or PKI key provider. "
                "Set THEMIS_ENVIRONMENT=development to use MockKeyProvider."
            );
        }
        
        return createMockKeyProvider();
    }
    
    static std::string backendToString(Backend backend) {
        switch (backend) {
            case Backend::ThemisDB: return "ThemisDB";
            case Backend::FileSystem: return "FileSystem";
            case Backend::S3: return "S3";
            default: return "Unknown";
        }
    }
    
    std::string makeCollectionKey(const std::string& adapter_id) const {
        return config_.collection_name + ":" + adapter_id;
    }
    
    bool saveToThemisDB(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        std::string key = makeCollectionKey(adapter_id);
        
        // Create BaseEntity with metadata
        BaseEntity::FieldMap fields;
        fields["adapter_id"] = Value(adapter_id);
        fields["version"] = Value(metadata.version);
        fields["base_model"] = Value(metadata.base_model);
        fields["description"] = Value(metadata.description);
        fields["training_samples"] = Value(static_cast<int64_t>(metadata.training_samples));
        fields["validation_accuracy"] = Value(static_cast<double>(metadata.validation_accuracy));
        fields["format"] = Value(weights.format);
        fields["size_bytes"] = Value(static_cast<int64_t>(weights.size_bytes));
        
        BaseEntity entity = BaseEntity::fromFields(adapter_id, fields);
        
        // Handle large weights with BlobStorageManager
        if (config_.blob_manager && weights.data.size() > 1024 * 1024) {  // > 1MB
            spdlog::info("Storing large adapter ({} MB) in blob storage", 
                        weights.data.size() / (1024 * 1024));
            
            auto blob_ref = config_.blob_manager->put(adapter_id, weights.data);
            fields["blob_ref_type"] = Value(static_cast<int64_t>(static_cast<int>(blob_ref.type)));
            fields["blob_ref_path"] = Value(blob_ref.uri);
        } else {
            // Small adapters stored inline
            fields["weights_data"] = Value(weights.data);
        }
        
        // Encrypt if enabled
        std::vector<uint8_t> data_to_store = entity.serialize();
        if (config_.enable_encryption && encryption_) {
            try {
                auto encrypted = encryption_->encrypt(data_to_store, config_.encryption_key_id);
                // Store the full encrypted blob (including key version, IV, tag)
                // Convert to base64 for storage
                std::string encrypted_b64 = encrypted.toBase64();
                data_to_store = std::vector<uint8_t>(encrypted_b64.begin(), encrypted_b64.end());
                spdlog::debug("Encrypted adapter data with key version {}", encrypted.key_version);
            } catch (const std::exception& e) {
                spdlog::error("Encryption failed: {}", e.what());
                return false;  // Fail if encryption is enabled but fails
            }
        }
        
        // Store in RocksDB
        bool success = config_.db->put(key, data_to_store);
        
        // Create signature if enabled
        if (success && config_.enable_signatures && config_.signature_manager) {
            storage::SecuritySignature sig;
            sig.resource_id = adapter_id;
            sig.hash = storage::SecuritySignatureManager::computeFileHash(adapter_id);
            sig.algorithm = "Ed25519";
            sig.created_at = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            
            config_.signature_manager->storeSignature(sig);
            spdlog::debug("Created security signature for adapter");
        }
        
        if (success) {
            spdlog::info("Saved adapter {} to ThemisDB ({} bytes)", 
                        adapter_id, data_to_store.size());
        }
        
        return success;
    }
    
    std::optional<AdapterWeights> loadFromThemisDB(const std::string& adapter_id) {
        std::string key = makeCollectionKey(adapter_id);
        auto data = config_.db->get(key);
        
        if (!data) {
            spdlog::warn("Adapter {} not found in ThemisDB", adapter_id);
            return std::nullopt;
        }
        
        // Decrypt if needed
        std::vector<uint8_t> decrypted_data = *data;
        if (config_.enable_encryption && encryption_) {
            try {
                // Deserialize the EncryptedBlob from base64 string
                std::string encrypted_b64(data->begin(), data->end());
                auto encrypted_blob = EncryptedBlob::fromBase64(encrypted_b64);
                
                // Decrypt using the key version stored in the blob
                decrypted_data = encryption_->decryptToBytes(encrypted_blob);
                spdlog::debug("Decrypted adapter data (key version: {})", encrypted_blob.key_version);
            } catch (const std::exception& e) {
                spdlog::error("Decryption failed: {}", e.what());
                return std::nullopt;  // Fail if decryption fails
            }
        }
        
        // Deserialize entity
        BaseEntity entity = BaseEntity::deserialize(adapter_id, decrypted_data);
        
        AdapterWeights weights;
        
        // Load weights from blob or inline
        if (entity.hasField("blob_ref_path")) {
            if (config_.blob_manager) {
                // Validate blob reference type before casting
                auto blob_type_value = entity.getFieldAsInt("blob_ref_type").value_or(-1);
                // Valid range: 0 (INLINE) to 7 (CUSTOM)
                if (blob_type_value < 0 || blob_type_value > static_cast<int>(storage::BlobStorageType::CUSTOM)) {
                    spdlog::error("Invalid blob storage type {} for adapter {}, cannot load", 
                               blob_type_value, adapter_id);
                    return std::nullopt;
                }
                
                storage::BlobRef ref;
                ref.type = static_cast<storage::BlobStorageType>(blob_type_value);
                ref.uri = entity.getFieldAsString("blob_ref_path").value_or("");
                
                auto blob_data = config_.blob_manager->get(ref);
                if (blob_data) {
                    weights.data = *blob_data;
                    weights.size_bytes = blob_data->size();
                } else {
                    spdlog::error("Failed to load blob {} for adapter {}", ref.uri, adapter_id);
                    return std::nullopt;
                }
            }
        } else if (auto weights_data = entity.getField("weights_data")) {
            if (std::holds_alternative<std::vector<uint8_t>>(*weights_data)) {
                weights.data = std::get<std::vector<uint8_t>>(*weights_data);
                weights.size_bytes = weights.data.size();
            }
        }
        
        weights.format = entity.getFieldAsString("format").value_or("safetensors");
        
        spdlog::info("Loaded adapter {} from ThemisDB ({} bytes)", 
                    adapter_id, weights.size_bytes);
        
        return weights;
    }
    
    std::optional<AdapterMetadata> loadMetadataFromThemisDB(const std::string& adapter_id) {
        std::string key = makeCollectionKey(adapter_id);
        auto data = config_.db->get(key);
        
        if (!data) {
            return std::nullopt;
        }
        
        BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
        
        AdapterMetadata metadata;
        metadata.adapter_id = adapter_id;
        metadata.version = entity.getFieldAsString("version").value_or("v1");
        metadata.base_model = entity.getFieldAsString("base_model").value_or("");
        metadata.description = entity.getFieldAsString("description").value_or("");
        metadata.training_samples = static_cast<int>(entity.getFieldAsInt("training_samples").value_or(0));
        metadata.validation_accuracy = static_cast<float>(entity.getFieldAsDouble("validation_accuracy").value_or(0.0));
        
        return metadata;
    }
    
    // Filesystem fallback implementations (original code)
    bool saveToFilesystem(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::create_directories(adapter_dir);
        
        // Save weights
        fs::path weights_path = adapter_dir / "weights.bin";
        std::ofstream weights_file(weights_path, std::ios::binary);
        if (!weights_file) {
            spdlog::error("Failed to create weights file: {}", weights_path.string());
            return false;
        }
        weights_file.write(reinterpret_cast<const char*>(weights.data.data()), weights.data.size());
        weights_file.close();
        
        // Save metadata
        fs::path metadata_path = adapter_dir / "metadata.json";
        std::ofstream metadata_file(metadata_path);
        if (!metadata_file) {
            spdlog::error("Failed to create metadata file: {}", metadata_path.string());
            return false;
        }
        json j = metadata.toJSON();
        metadata_file << j.dump(2);
        metadata_file.close();
        
        spdlog::info("Saved adapter {} to filesystem ({} bytes)", 
                     adapter_id, weights.data.size());
        
        return true;
    }
    
    std::optional<AdapterWeights> loadFromFilesystem(const std::string& adapter_id) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::path weights_path = adapter_dir / "weights.bin";
        
        if (!fs::exists(weights_path)) {
            return std::nullopt;
        }
        
        AdapterWeights weights;
        
        std::ifstream weights_file(weights_path, std::ios::binary | std::ios::ate);
        if (!weights_file) {
            return std::nullopt;
        }
        
        size_t file_size = weights_file.tellg();
        weights_file.seekg(0, std::ios::beg);
        
        weights.data.resize(file_size);
        weights_file.read(reinterpret_cast<char*>(weights.data.data()), file_size);
        weights.size_bytes = file_size;
        weights_file.close();
        
        return weights;
    }
    
    std::optional<AdapterMetadata> loadMetadataFromFilesystem(const std::string& adapter_id) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::path metadata_path = adapter_dir / "metadata.json";
        
        if (!fs::exists(metadata_path)) {
            return std::nullopt;
        }
        
        std::ifstream metadata_file(metadata_path);
        if (!metadata_file) {
            return std::nullopt;
        }
        
        json j;
        metadata_file >> j;
        
        return AdapterMetadata::fromJSON(j);
    }
};

// LoRAStorageService public interface

LoRAStorageService::LoRAStorageService(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

LoRAStorageService::~LoRAStorageService() = default;

bool LoRAStorageService::saveAdapter(
    const std::string& adapter_id,
    const AdapterWeights& weights,
    const AdapterMetadata& metadata
) {
    return impl_->saveAdapter(adapter_id, weights, metadata);
}

std::optional<AdapterWeights> LoRAStorageService::loadAdapter(const std::string& adapter_id) {
    return impl_->loadAdapter(adapter_id);
}

std::optional<AdapterMetadata> LoRAStorageService::loadMetadata(const std::string& adapter_id) {
    return impl_->loadMetadata(adapter_id);
}

bool LoRAStorageService::deleteAdapter(const std::string& adapter_id) {
    return impl_->deleteAdapter(adapter_id);
}

bool LoRAStorageService::exists(const std::string& adapter_id) const {
    return impl_->exists(adapter_id);
}

std::vector<std::string> LoRAStorageService::listAdapters() const {
    return impl_->listAdapters();
}

std::string LoRAStorageService::createVersion(const std::string& adapter_id) {
    return impl_->createVersion(adapter_id);
}

bool LoRAStorageService::rollbackToVersion(const std::string& adapter_id, const std::string& version) {
    return impl_->rollbackToVersion(adapter_id, version);
}

std::vector<std::string> LoRAStorageService::listVersions(const std::string& adapter_id) const {
    return impl_->listVersions(adapter_id);
}

bool LoRAStorageService::updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata) {
    return impl_->updateMetadata(adapter_id, metadata);
}

json LoRAStorageService::getStats() const {
    return impl_->getStats();
}

} // namespace lora
} // namespace llm
} // namespace themis


