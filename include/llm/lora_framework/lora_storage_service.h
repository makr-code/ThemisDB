#pragma once

#include "lora_config.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Adapter weights representation
 */
struct AdapterWeights {
    std::vector<uint8_t> data;        // Binary weight data
    LoRAHyperparameters hyperparameters;
    size_t size_bytes = 0;
    std::string format = "safetensors"; // "safetensors", "pickle", "gguf"
    
    json toJSON() const {
        return json{
            {"size_bytes", size_bytes},
            {"format", format},
            {"hyperparameters", hyperparameters.toJSON()}
        };
    }
};

/**
 * @brief Manages LoRA adapter storage and versioning
 * 
 * Features:
 * - ThemisDB collection integration
 * - File system backup
 * - Versioning system
 * - Metadata management
 */
class LoRAStorageService {
public:
    /**
     * @brief Storage backend type
     */
    enum class Backend {
        ThemisDB,      // Store in ThemisDB collection
        FileSystem,    // Store in file system
        S3             // Store in S3/object storage (future)
    };
    
    /**
     * @brief Configuration for storage service
     */
    struct Config {
        Backend backend = Backend::FileSystem;  // Start with filesystem for simplicity
        std::string collection_name = "lora_adapters";
        std::string filesystem_path = "data/lora_adapters";
        bool enable_versioning = true;
        int max_versions = 5;
        bool enable_compression = true;
    };
    
    explicit LoRAStorageService(const Config& config = Config{});
    ~LoRAStorageService();
    
    // Disable copy
    LoRAStorageService(const LoRAStorageService&) = delete;
    LoRAStorageService& operator=(const LoRAStorageService&) = delete;
    
    /**
     * @brief Save adapter to storage
     * @param adapter_id Adapter identifier
     * @param weights Adapter weights
     * @param metadata Adapter metadata
     * @return true if saved successfully
     */
    bool saveAdapter(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    );
    
    /**
     * @brief Load adapter from storage
     * @param adapter_id Adapter identifier
     * @return Optional adapter weights
     */
    std::optional<AdapterWeights> loadAdapter(const std::string& adapter_id);
    
    /**
     * @brief Load adapter metadata without weights
     * @param adapter_id Adapter identifier
     * @return Optional adapter metadata
     */
    std::optional<AdapterMetadata> loadMetadata(const std::string& adapter_id);
    
    /**
     * @brief Delete adapter from storage
     * @param adapter_id Adapter identifier
     * @return true if deleted successfully
     */
    bool deleteAdapter(const std::string& adapter_id);
    
    /**
     * @brief Check if adapter exists in storage
     * @param adapter_id Adapter identifier
     * @return true if exists
     */
    bool exists(const std::string& adapter_id) const;
    
    /**
     * @brief List all stored adapters
     * @return Vector of adapter IDs
     */
    std::vector<std::string> listAdapters() const;
    
    /**
     * @brief Create new version of adapter
     * @param adapter_id Adapter identifier
     * @return Version identifier (e.g., "v1", "v2")
     */
    std::string createVersion(const std::string& adapter_id);
    
    /**
     * @brief Rollback to specific version
     * @param adapter_id Adapter identifier
     * @param version Version identifier
     * @return true if rolled back successfully
     */
    bool rollbackToVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief List all versions of adapter
     * @param adapter_id Adapter identifier
     * @return Vector of version identifiers
     */
    std::vector<std::string> listVersions(const std::string& adapter_id) const;
    
    /**
     * @brief Update adapter metadata
     * @param adapter_id Adapter identifier
     * @param metadata New metadata
     * @return true if updated successfully
     */
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata);
    
    /**
     * @brief Get storage statistics
     * @return JSON with statistics
     */
    json getStats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
