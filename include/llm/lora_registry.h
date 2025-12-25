#ifndef THEMIS_LORA_REGISTRY_H
#define THEMIS_LORA_REGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace llm {

/**
 * @brief LoRARegistry - Manages LoRA adapters for agent specialization
 * 
 * Features:
 * - Adapter registration and metadata storage
 * - Dynamic loading/unloading
 * - Hot-swapping support
 * - Memory-efficient caching
 * 
 * LoRA (Low-Rank Adaptation) adapters enable efficient fine-tuning
 * for domain-specific tasks without modifying base models.
 */
class LoRARegistry {
public:
    struct LoRAAdapter {
        std::string adapter_id;
        std::string name;
        std::string base_model;         // Compatible base model
        std::string adapter_path;       // Path to adapter weights
        std::string domain;             // "legal", "medical", "finance", etc.
        std::vector<std::string> capabilities;
        int rank;                       // LoRA rank (typically 8-64)
        float alpha;                    // LoRA alpha parameter
        size_t size_bytes;              // Adapter size
        nlohmann::json metadata;
        
        nlohmann::json toJson() const;
        static LoRAAdapter fromJson(const nlohmann::json& j);
    };

    struct LoadStats {
        size_t total_adapters;
        size_t loaded_adapters;
        size_t total_memory_bytes;
        std::vector<std::string> loaded_adapter_ids;
        
        nlohmann::json toJson() const;
    };

    /**
     * @brief Construct LoRARegistry
     * @param db RocksDB TransactionDB instance
     * @param cf Optional column family handle
     */
    explicit LoRARegistry(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr
    );

    ~LoRARegistry() = default;

    /**
     * @brief Register new LoRA adapter
     * @param adapter Adapter definition
     * @return True if successfully registered
     */
    bool registerAdapter(const LoRAAdapter& adapter);

    /**
     * @brief Get adapter by ID
     * @param adapter_id Adapter identifier
     * @return Adapter if found
     */
    std::optional<LoRAAdapter> getAdapter(const std::string& adapter_id) const;

    /**
     * @brief List adapters by domain
     * @param domain Domain filter (empty = all)
     * @return Vector of adapters
     */
    std::vector<LoRAAdapter> listAdapters(const std::string& domain = "") const;

    /**
     * @brief Load adapter into memory
     * @param adapter_id Adapter identifier
     * @return True if successfully loaded
     */
    bool loadAdapter(const std::string& adapter_id);

    /**
     * @brief Unload adapter from memory
     * @param adapter_id Adapter identifier
     * @return True if successfully unloaded
     */
    bool unloadAdapter(const std::string& adapter_id);

    /**
     * @brief Get loaded adapter IDs
     * @return Vector of loaded adapter IDs
     */
    std::vector<std::string> getLoadedAdapters() const;

    /**
     * @brief Check if adapter is loaded
     * @param adapter_id Adapter identifier
     * @return True if loaded
     */
    bool isLoaded(const std::string& adapter_id) const;

    /**
     * @brief Get load statistics
     * @return Load statistics
     */
    LoadStats getStats() const;

    /**
     * @brief Preload multiple adapters
     * @param adapter_ids Vector of adapter IDs
     * @return Number of successfully loaded adapters
     */
    size_t preloadAdapters(const std::vector<std::string>& adapter_ids);

    /**
     * @brief Garbage collect unused adapters
     * @param ttl_seconds Time-to-live in seconds
     * @return Number of unloaded adapters
     */
    size_t gcUnusedAdapters(int ttl_seconds = 300);

    /**
     * @brief Update adapter metadata
     * @param adapter_id Adapter identifier
     * @param adapter Updated adapter definition
     * @return True if successfully updated
     */
    bool updateAdapter(const std::string& adapter_id, const LoRAAdapter& adapter);

    /**
     * @brief Delete adapter
     * @param adapter_id Adapter identifier
     * @return True if successfully deleted
     */
    bool deleteAdapter(const std::string& adapter_id);

    /**
     * @brief Find adapters by capability
     * @param capability Capability identifier
     * @return Vector of adapters with matching capability
     */
    std::vector<LoRAAdapter> findAdaptersByCapability(const std::string& capability) const;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    
    // Thread safety mutex
    mutable std::mutex mutex_;
    
    // Loaded adapters in memory
    std::set<std::string> loaded_adapters_;
    
    // Adapter cache: adapter_id -> LoRAAdapter
    std::map<std::string, LoRAAdapter> adapter_cache_;
    
    // Last access time: adapter_id -> timestamp
    std::map<std::string, int64_t> last_access_time_;
    
    // Helper methods
    std::string makeKey(const std::string& adapter_id) const;
    bool persistAdapter(const LoRAAdapter& adapter);
    std::optional<LoRAAdapter> loadAdapterMetadata(const std::string& adapter_id) const;
    void rebuildCache();
    int64_t getCurrentTimestampMs() const;
};

} // namespace llm
} // namespace themis

#endif // THEMIS_LORA_REGISTRY_H
