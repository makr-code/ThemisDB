#include "llm/llm_model_storage.h"
#include "storage/base_entity.h"
#include "security/mock_key_provider.h"
#include <spdlog/spdlog.h"
>
#include <algorithm>

namespace themis {
namespace llm {

/**
 * @brief Implementation of LLMModelStorage using ThemisDB Base Infrastructure
 * 
 * Integrates with:
 * - BaseEntity for structured storage
 * - RocksDBWrapper for CRUD operations
 * - BlobStorageManager for large model files
 * - SecuritySignatureManager for integrity verification
 * - Encryption for data at rest
 */
class LLMModelStorage::Impl {
public:
    explicit Impl(const Config& config) : config_(config) {
        spdlog::info("LLMModelStorage initialized:");
        spdlog::info("  Collection: {}", config_.collection_name);
        spdlog::info("  Encryption: {}", config_.enable_encryption);
        spdlog::info("  Signatures: {}", config_.enable_signatures);
        spdlog::info("  Blob Storage: {}", config_.use_blob_storage);
        
        // Initialize encryption if enabled
        if (config_.enable_encryption && !encryption_) {
            try {
                // Use mock key provider for now (in production, use Vault/HSM)
                auto key_provider = std::make_shared<MockKeyProvider>();
                encryption_ = std::make_shared<FieldEncryption>(key_provider);
                spdlog::info("  Encryption service initialized");
            } catch (const std::exception& e) {
                spdlog::warn("  Failed to initialize encryption: {}", e.what());
            }
        }
        
        // Validate configuration
        if (!config_.db) {
            spdlog::error("RocksDBWrapper not configured");
        }
    }
    
    bool storeModel(
        const LLMModelMetadata& metadata,
        const std::optional<std::vector<uint8_t>>& model_data
    ) {
        try {
            if (!config_.db) {
                spdlog::error("Database not configured");
                return false;
            }
            
            // Create BaseEntity for model
            BaseEntity entity;
            entity.setId(metadata.model_id);
            entity.setType("llm_model");
            
            // Set metadata fields
            entity.setField("model_id", Value(metadata.model_id));
            entity.setField("model_name", Value(metadata.model_name));
            entity.setField("version", Value(metadata.version));
            entity.setField("architecture", Value(metadata.architecture));
            entity.setField("format", Value(metadata.format));
            entity.setField("quantization", Value(metadata.quantization));
            entity.setField("size_bytes", Value(static_cast<int64_t>(metadata.size_bytes)));
            entity.setField("checksum", Value(metadata.checksum));
            entity.setField("parameter_count", Value(metadata.parameter_count));
            entity.setField("context_length", Value(static_cast<int64_t>(metadata.context_length)));
            entity.setField("vocabulary_size", Value(static_cast<int64_t>(metadata.vocabulary_size)));
            entity.setField("num_layers", Value(static_cast<int64_t>(metadata.num_layers)));
            entity.setField("hidden_size", Value(static_cast<int64_t>(metadata.hidden_size)));
            
            // Store capabilities, languages, and tags as JSON strings
            if (!metadata.capabilities.empty()) {
                json cap_json = metadata.capabilities;
                entity.setField("capabilities", Value(cap_json.dump()));
            }
            if (!metadata.languages.empty()) {
                json lang_json = metadata.languages;
                entity.setField("languages", Value(lang_json.dump()));
            }
            if (!metadata.tags.empty()) {
                json tags_json = metadata.tags;
                entity.setField("tags", Value(tags_json.dump()));
            }
            
            // Performance metrics
            entity.setField("tokens_per_second", Value(static_cast<double>(metadata.tokens_per_second)));
            entity.setField("vram_required_mb", Value(static_cast<int64_t>(metadata.vram_required_mb)));
            entity.setField("ram_required_mb", Value(static_cast<int64_t>(metadata.ram_required_mb)));
            
            // Usage statistics
            entity.setField("total_inferences", Value(metadata.total_inferences));
            entity.setField("total_tokens_generated", Value(metadata.total_tokens_generated));
            
            // Provenance
            entity.setField("source", Value(metadata.source));
            entity.setField("source_url", Value(metadata.source_url));
            entity.setField("license", Value(metadata.license));
            entity.setField("created_by", Value(metadata.created_by));
            
            // Store custom metadata as JSON
            if (!metadata.custom_metadata.empty()) {
                entity.setField("custom_metadata", Value(metadata.custom_metadata.dump()));
            }
            
            // Handle model data storage
            if (model_data && config_.use_blob_storage && config_.blob_manager) {
                // Store in blob storage if large enough
                size_t threshold_bytes = config_.inline_threshold_mb * 1024 * 1024;
                
                if (model_data->size() > threshold_bytes) {
                    spdlog::info("Storing model {} in blob storage ({} bytes)", 
                                 metadata.model_id, model_data->size());
                    
                    // Store model file in blob storage
                    auto blob_ref = config_.blob_manager->put(metadata.model_id, *model_data);
                    
                    // Store blob reference in metadata
                    entity.setField("blob_ref_type", Value(static_cast<int>(blob_ref.type)));
                    entity.setField("blob_ref_id", Value(blob_ref.id));
                    entity.setField("blob_ref_uri", Value(blob_ref.uri));
                    entity.setField("blob_ref_hash", Value(blob_ref.hash_sha256));
                    entity.setField("blob_ref_size", Value(blob_ref.size_bytes));
                    entity.setField("blob_ref_compressed", Value(blob_ref.compressed));
                    if (blob_ref.compressed) {
                        entity.setField("blob_ref_compression", Value(blob_ref.compression_type));
                    }
                    
                    spdlog::info("Model {} stored in blob storage: type={}, uri={}", 
                                 metadata.model_id, static_cast<int>(blob_ref.type), blob_ref.uri);
                } else if (model_data->size() > 0) {
                    // Store inline if small enough
                    spdlog::info("Storing model {} inline ({} bytes)", 
                                 metadata.model_id, model_data->size());
                    std::string data_str(model_data->begin(), model_data->end());
                    entity.setField("model_data_inline", Value(data_str));
                }
            } else if (model_data && model_data->size() > 0) {
                // Store inline if blob storage not available
                spdlog::info("Storing model {} inline (no blob storage, {} bytes)", 
                             metadata.model_id, model_data->size());
                std::string data_str(model_data->begin(), model_data->end());
                entity.setField("model_data_inline", Value(data_str));
            }
            
            // Serialize entity
            auto serialized = entity.serialize();
            
            // Encrypt if needed
            std::vector<uint8_t> data_to_store;
            if (config_.enable_encryption && encryption_) {
                auto encrypted = encryption_->encryptBytes(serialized, config_.encryption_key_id);
                std::string encrypted_str = encrypted.toBase64();
                data_to_store.assign(encrypted_str.begin(), encrypted_str.end());
            } else {
                data_to_store = serialized;
            }
            
            // Store in RocksDB
            std::string key = config_.collection_name + ":" + metadata.model_id;
            bool success = config_.db->put(key, data_to_store);
            
            if (success) {
                spdlog::info("Model {} stored successfully", metadata.model_id);
            } else {
                spdlog::error("Failed to store model {}", metadata.model_id);
            }
            
            return success;
        } catch (const std::exception& e) {
            spdlog::error("Failed to store model {}: {}", metadata.model_id, e.what());
            return false;
        }
    }
    
    std::optional<LLMModelMetadata> loadModel(const std::string& model_id) {
        try {
            if (!config_.db) {
                spdlog::error("Database not configured");
                return std::nullopt;
            }
            
            // Retrieve from RocksDB
            std::string key = config_.collection_name + ":" + model_id;
            auto data = config_.db->get(key);
            
            if (!data) {
                spdlog::warn("Model {} not found", model_id);
                return std::nullopt;
            }
            
            // Decrypt if needed
            std::vector<uint8_t> decrypted_data;
            if (config_.enable_encryption && encryption_) {
                try {
                    std::string data_str(data->begin(), data->end());
                    auto encrypted_blob = EncryptedBlob::fromBase64(data_str);
                    decrypted_data = encryption_->decryptToBytes(encrypted_blob);
                } catch (const std::exception& e) {
                    spdlog::error("Failed to decrypt model {}: {}", model_id, e.what());
                    return std::nullopt;
                }
            } else {
                decrypted_data = *data;
            }
            
            // Deserialize BaseEntity
            BaseEntity entity = BaseEntity::deserialize(model_id, decrypted_data);
            
            // Parse metadata
            LLMModelMetadata metadata;
            metadata.model_id = entity.getField("model_id").asString();
            metadata.model_name = entity.getField("model_name").asString();
            metadata.version = entity.getField("version").asString();
            metadata.architecture = entity.getField("architecture").asString();
            metadata.format = entity.getField("format").asString();
            metadata.quantization = entity.getField("quantization").asString();
            metadata.size_bytes = entity.getField("size_bytes").asInt64();
            metadata.checksum = entity.getField("checksum").asString();
            metadata.parameter_count = entity.getField("parameter_count").asInt64();
            metadata.context_length = entity.getField("context_length").asInt();
            metadata.vocabulary_size = entity.getField("vocabulary_size").asInt();
            metadata.num_layers = entity.getField("num_layers").asInt();
            metadata.hidden_size = entity.getField("hidden_size").asInt();
            
            // Parse JSON fields
            if (entity.hasField("capabilities")) {
                try {
                    auto cap_json = json::parse(entity.getField("capabilities").asString());
                    metadata.capabilities = cap_json.get<std::vector<std::string>>();
                } catch (...) {
                    // Ignore parse errors
                }
            }
            
            if (entity.hasField("languages")) {
                try {
                    auto lang_json = json::parse(entity.getField("languages").asString());
                    metadata.languages = lang_json.get<std::vector<std::string>>();
                } catch (...) {
                    // Ignore parse errors
                }
            }
            
            if (entity.hasField("tags")) {
                try {
                    auto tags_json = json::parse(entity.getField("tags").asString());
                    metadata.tags = tags_json.get<std::vector<std::string>>();
                } catch (...) {
                    // Ignore parse errors
                }
            }
            
            // Performance metrics
            if (entity.hasField("tokens_per_second")) {
                metadata.tokens_per_second = entity.getField("tokens_per_second").asDouble();
            }
            if (entity.hasField("vram_required_mb")) {
                metadata.vram_required_mb = entity.getField("vram_required_mb").asInt64();
            }
            if (entity.hasField("ram_required_mb")) {
                metadata.ram_required_mb = entity.getField("ram_required_mb").asInt64();
            }
            
            // Usage statistics
            if (entity.hasField("total_inferences")) {
                metadata.total_inferences = entity.getField("total_inferences").asInt64();
            }
            if (entity.hasField("total_tokens_generated")) {
                metadata.total_tokens_generated = entity.getField("total_tokens_generated").asInt64();
            }
            
            // Provenance
            if (entity.hasField("source")) {
                metadata.source = entity.getField("source").asString();
            }
            if (entity.hasField("source_url")) {
                metadata.source_url = entity.getField("source_url").asString();
            }
            if (entity.hasField("license")) {
                metadata.license = entity.getField("license").asString();
            }
            if (entity.hasField("created_by")) {
                metadata.created_by = entity.getField("created_by").asString();
            }
            
            // Custom metadata
            if (entity.hasField("custom_metadata")) {
                try {
                    metadata.custom_metadata = json::parse(entity.getField("custom_metadata").asString());
                } catch (...) {
                    // Ignore parse errors
                }
            }
            
            // File path (for local models)
            if (entity.hasField("file_path")) {
                metadata.file_path = entity.getField("file_path").asString();
            }
            
            spdlog::info("Model {} loaded successfully", model_id);
            return metadata;
        } catch (const std::exception& e) {
            spdlog::error("Failed to load model {}: {}", model_id, e.what());
            return std::nullopt;
        }
    }
    
    bool updateModel(const std::string& model_id, const LLMModelMetadata& metadata) {
        // For simplicity, just store again
        return storeModel(metadata, std::nullopt);
    }
    
    bool deleteModel(const std::string& model_id) {
        try {
            if (!config_.db) {
                spdlog::error("Database not configured");
                return false;
            }
            
            // Load metadata to get blob reference
            auto metadata_opt = loadModel(model_id);
            if (metadata_opt && config_.blob_manager) {
                // Try to delete blob if it exists
                std::string key = config_.collection_name + ":" + model_id;
                auto data = config_.db->get(key);
                if (data) {
                    try {
                        // Decrypt if needed
                        std::vector<uint8_t> decrypted_data;
                        if (config_.enable_encryption && encryption_) {
                            std::string data_str(data->begin(), data->end());
                            auto encrypted_blob = EncryptedBlob::fromBase64(data_str);
                            decrypted_data = encryption_->decryptToBytes(encrypted_blob);
                        } else {
                            decrypted_data = *data;
                        }
                        
                        BaseEntity entity = BaseEntity::deserialize(model_id, decrypted_data);
                        
                        if (entity.hasField("blob_ref_uri")) {
                            storage::BlobRef ref;
                            ref.id = entity.getField("blob_ref_id").asString();
                            ref.uri = entity.getField("blob_ref_uri").asString();
                            ref.type = static_cast<storage::BlobStorageType>(
                                entity.getField("blob_ref_type").asInt()
                            );
                            ref.hash_sha256 = entity.getField("blob_ref_hash").asString();
                            ref.size_bytes = entity.getField("blob_ref_size").asInt64();
                            
                            config_.blob_manager->remove(ref);
                            spdlog::info("Deleted blob for model {}", model_id);
                        }
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to delete blob for model {}: {}", model_id, e.what());
                    }
                }
            }
            
            // Delete metadata from RocksDB
            std::string key = config_.collection_name + ":" + model_id;
            bool success = config_.db->remove(key);
            
            if (success) {
                spdlog::info("Model {} deleted successfully", model_id);
            } else {
                spdlog::error("Failed to delete model {}", model_id);
            }
            
            return success;
        } catch (const std::exception& e) {
            spdlog::error("Failed to delete model {}: {}", model_id, e.what());
            return false;
        }
    }
    
    bool exists(const std::string& model_id) const {
        if (!config_.db) {
            return false;
        }
        
        std::string key = config_.collection_name + ":" + model_id;
        auto data = config_.db->get(key);
        return data.has_value();
    }
    
    std::vector<std::string> listModels(const std::optional<std::string>& filter) const {
        std::vector<std::string> model_ids;
        
        if (!config_.db) {
            return model_ids;
        }
        
        // List all keys with collection prefix
        std::string prefix = config_.collection_name + ":";
        auto keys = config_.db->listKeysWithPrefix(prefix);
        
        for (const auto& key : keys) {
            // Extract model ID from key
            std::string model_id = key.substr(prefix.length());
            
            // Apply filter if provided
            if (filter) {
                // Simple substring filter
                if (model_id.find(*filter) != std::string::npos) {
                    model_ids.push_back(model_id);
                }
            } else {
                model_ids.push_back(model_id);
            }
        }
        
        return model_ids;
    }
    
    bool updateUsageStats(const std::string& model_id, int64_t tokens_generated) {
        auto metadata_opt = loadModel(model_id);
        if (!metadata_opt) {
            return false;
        }
        
        auto& metadata = *metadata_opt;
        metadata.total_inferences++;
        metadata.total_tokens_generated += tokens_generated;
        metadata.last_used = std::chrono::system_clock::now();
        
        return updateModel(model_id, metadata);
    }
    
    json getStats() const {
        json stats;
        stats["collection"] = config_.collection_name;
        stats["encryption_enabled"] = config_.enable_encryption;
        stats["blob_storage_enabled"] = config_.use_blob_storage;
        
        // Count models
        auto models = listModels(std::nullopt);
        stats["total_models"] = models.size();
        
        return stats;
    }
    
    // Helper: Get blob reference from metadata
    std::optional<storage::BlobRef> getBlobReference(const std::string& model_id) {
        if (!config_.db) {
            return std::nullopt;
        }
        
        std::string key = config_.collection_name + ":" + model_id;
        auto data = config_.db->get(key);
        
        if (!data) {
            return std::nullopt;
        }
        
        try {
            // Decrypt if needed
            std::vector<uint8_t> decrypted_data;
            if (config_.enable_encryption && encryption_) {
                std::string data_str(data->begin(), data->end());
                auto encrypted_blob = EncryptedBlob::fromBase64(data_str);
                decrypted_data = encryption_->decryptToBytes(encrypted_blob);
            } else {
                decrypted_data = *data;
            }
            
            BaseEntity entity = BaseEntity::deserialize(model_id, decrypted_data);
            
            if (entity.hasField("blob_ref_uri")) {
                storage::BlobRef ref;
                ref.id = entity.getField("blob_ref_id").asString();
                ref.uri = entity.getField("blob_ref_uri").asString();
                ref.type = static_cast<storage::BlobStorageType>(
                    entity.getField("blob_ref_type").asInt()
                );
                ref.hash_sha256 = entity.getField("blob_ref_hash").asString();
                ref.size_bytes = entity.getField("blob_ref_size").asInt64();
                if (entity.hasField("blob_ref_compressed")) {
                    ref.compressed = entity.getField("blob_ref_compressed").asBool();
                }
                if (entity.hasField("blob_ref_compression")) {
                    ref.compression_type = entity.getField("blob_ref_compression").asString();
                }
                
                return ref;
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to get blob reference for model {}: {}", model_id, e.what());
        }
        
        return std::nullopt;
    }
    
private:
    Config config_;
    std::shared_ptr<FieldEncryption> encryption_;
};

// LLMModelStorage implementation
LLMModelStorage::LLMModelStorage(const Config& config)
    : impl_(std::make_unique<Impl>(config)), config_(config) {}

LLMModelStorage::~LLMModelStorage() = default;

bool LLMModelStorage::storeModel(
    const LLMModelMetadata& metadata,
    const std::optional<std::vector<uint8_t>>& model_data
) {
    return impl_->storeModel(metadata, model_data);
}

std::optional<LLMModelMetadata> LLMModelStorage::loadModel(const std::string& model_id) {
    return impl_->loadModel(model_id);
}

bool LLMModelStorage::updateModel(const std::string& model_id, const LLMModelMetadata& metadata) {
    return impl_->updateModel(model_id, metadata);
}

bool LLMModelStorage::deleteModel(const std::string& model_id) {
    return impl_->deleteModel(model_id);
}

bool LLMModelStorage::exists(const std::string& model_id) const {
    return impl_->exists(model_id);
}

std::vector<std::string> LLMModelStorage::listModels(const std::optional<std::string>>& filter) const {
    return impl_->listModels(filter);
}

bool LLMModelStorage::addEdge(
    const std::string& from_id,
    const std::string& to_id,
    LLMEdgeType edge_type,
    float weight
) {
    // TODO: Implement graph edges
    spdlog::warn("Graph edges not yet implemented");
    return false;
}

std::vector<json> LLMModelStorage::getEdges(
    const std::string& model_id,
    const std::string& direction
) const {
    // TODO: Implement graph edges
    return {};
}

bool LLMModelStorage::storeEmbedding(
    const std::string& model_id,
    const std::vector<float>& embedding
) {
    // TODO: Implement vector embeddings
    spdlog::warn("Vector embeddings not yet implemented");
    return false;
}

std::vector<std::pair<std::string, float>> LLMModelStorage::findSimilarModels(
    const std::string& model_id,
    int k,
    float threshold
) const {
    // TODO: Implement vector similarity search
    return {};
}

bool LLMModelStorage::updateUsageStats(const std::string& model_id, int64_t tokens_generated) {
    return impl_->updateUsageStats(model_id, tokens_generated);
}

json LLMModelStorage::getStats() const {
    return impl_->getStats();
}

const LLMModelStorage::Config& LLMModelStorage::getConfig() const {
    return config_;
}

} // namespace llm
} // namespace themis
