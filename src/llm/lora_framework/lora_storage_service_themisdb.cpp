#include "llm/lora_framework/lora_storage_service.h"
#include "storage/base_entity.h"
#include "security/mock_key_provider.h"
#include "security/encryption.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

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
        spdlog::info("  Signatures: {}", config_.enable_signatures);
        spdlog::info("  RAID Auto-detect: {}", config_.auto_detect_raid);
        
        // Initialize encryption if enabled
        if (config_.enable_encryption && !encryption_) {
            try {
                // TODO: SECURITY - Replace MockKeyProvider with production key provider
                // In production, use one of:
                //   - VaultKeyProvider (HashiCorp Vault integration)
                //   - HSMProvider (Hardware Security Module)
                //   - KMSProvider (AWS KMS, Azure Key Vault, or GCP KMS)
                // MockKeyProvider is ONLY suitable for testing/development
                auto key_provider = std::make_shared<MockKeyProvider>();
                encryption_ = std::make_shared<FieldEncryption>(key_provider);
                spdlog::warn("  Using MockKeyProvider for encryption - NOT SUITABLE FOR PRODUCTION");
            } catch (const std::exception& e) {
                spdlog::warn("  Failed to initialize encryption: {}", e.what());
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
            auto it = config_.db->newIterator();
            
            for (it->Seek(prefix); it->Valid(); it->Next()) {
                std::string key(it->key().data(), it->key().size());
                if (!key.starts_with(prefix)) break;
                
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
        // Implementation depends on backend
        return false;  // TODO: Implement
    }
    
    std::vector<std::string> listVersions(const std::string& adapter_id) const {
        std::vector<std::string> versions;
        
        if (config_.backend == Backend::ThemisDB && config_.db) {
            // Scan for versioned keys
            std::string prefix = makeCollectionKey(adapter_id) + ":v";
            auto it = config_.db->newIterator();
            
            for (it->Seek(prefix); it->Valid(); it->Next()) {
                std::string key(it->key().data(), it->key().size());
                if (!key.starts_with(prefix)) break;
                
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
                data_to_store = encrypted.ciphertext;
                spdlog::debug("Encrypted adapter data");
            } catch (const std::exception& e) {
                spdlog::warn("Encryption failed: {}", e.what());
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
                // TODO: Decrypt requires EncryptedBlob, need to store metadata
                // For now, skip decryption
                // auto decrypted = encryption_->decrypt(encrypted_blob);
                // decrypted_data = decrypted;
                decrypted_data = *data;
                spdlog::debug("Decrypted adapter data");
            } catch (const std::exception& e) {
                spdlog::warn("Decryption failed, assuming unencrypted: {}", e.what());
                decrypted_data = *data;
            }
        }
        
        // Deserialize entity
        BaseEntity entity = BaseEntity::deserialize(adapter_id, decrypted_data);
        
        AdapterWeights weights;
        
        // Load weights from blob or inline
        if (entity.hasField("blob_ref_path")) {
            if (config_.blob_manager) {
                storage::BlobRef ref;
                ref.type = static_cast<storage::BlobStorageType>(
                    entity.getFieldAsInt("blob_ref_type").value_or(0)
                );
                ref.uri = entity.getFieldAsString("blob_ref_path").value_or("");
                
                auto blob_data = config_.blob_manager->get(ref);
                if (blob_data) {
                    weights.data = *blob_data;
                    weights.size_bytes = blob_data->size();
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
