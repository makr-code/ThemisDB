/**
 * @file lora_storage_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_config.h"
#include "lora_graph.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/blob_storage_manager.h"
#include "storage/security_signature_manager.h"
#include "security/encryption.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

struct AdapterWeights {
    /**
     * @brief Adapter Weights.
     * @return Return value.
     */
    virtual ~AdapterWeights() = default;
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

class LoRAStorageService {
public:
    enum class Backend {
        ThemisDB,      // Store in ThemisDB collection
        FileSystem,    // Store in file system
        S3             // Store in S3/object storage (future)
    };
    
    struct Config {
        Backend backend = Backend::ThemisDB;  // Use ThemisDB as primary backend
        std::string collection_name = "lora_adapters";
        std::string filesystem_path = "data/lora_adapters";
        bool enable_versioning = true;
        int max_versions = 5;
        bool enable_compression = true;
        
        // ThemisDB integration
        std::shared_ptr<RocksDBWrapper> db;  // RocksDB instance
        std::shared_ptr<storage::BlobStorageManager> blob_manager;  // Blob storage
        std::shared_ptr<storage::SecuritySignatureManager> signature_manager;  // Signatures
        
        // Security features
        bool enable_encryption = false;  // Encrypt adapter weights
        std::string encryption_key_id = "lora_adapters";  // Key ID for encryption
        bool enable_signatures = true;  // Digital signatures for integrity
        
        // HSM configuration (Hardware Security Module)
        bool use_hsm_for_encryption = false;           // Enable HSM-backed encryption
        std::string hsm_library_path;                   // PKCS#11 library path (e.g., "/usr/lib/softhsm/libsofthsm2.so")
        uint32_t hsm_slot_id = 0;                       // HSM slot ID (default: 0)
        std::string hsm_pin;                            // HSM user PIN (keep secure!)
        std::string hsm_key_label = "lora-adapter-kek"; // HSM key label for KEK
        uint32_t hsm_session_pool_size = 4;             // Parallel sessions for performance
      
        // PKI configuration for certificate-based encryption
        bool use_pki_for_encryption = false;        // Enable PKI-based encryption
        std::string pki_cert_path;                  // Certificate file path (PEM format)
        std::string pki_private_key_path;           // Private key file path (PEM format)
        std::string pki_ca_bundle_path;             // CA bundle for verification (optional)
        bool pki_verify_certificate = true;         // Verify certificate validity (default: true)
      
        // Vault Key Provider configuration
        bool use_vault_for_encryption = false;  // Enable Vault encryption (default: false)
        std::string vault_addr;          // Vault server address
        std::string vault_token;         // Vault authentication token
        std::string vault_kv_mount = "themis";      // KV mount path (default: "themis")
        
        // RAID/Redundancy (automatically detected from environment)
        bool auto_detect_raid = true;  // Auto-detect RAID configuration
        
        // Quorum-based consistency (for distributed LoRA adapters)
        bool enable_quorum_writes = false;      // Enable quorum enforcement for writes (default: OFF)
        bool enable_partition_detection = false; // Enable network partition detection
        uint32_t write_quorum_size = 2;         // Number of replicas for write quorum
        uint32_t read_quorum_size = 1;          // Number of replicas for read quorum
    };
    
    /**
     * @brief Lo RAStorage Service.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRAStorageService(const Config& config);
    /**
     * @brief Lo RAStorage Service.
     * @return Return value.
     */
    explicit LoRAStorageService();
    ~LoRAStorageService();
    
    // Disable copy
    LoRAStorageService(const LoRAStorageService&) = delete;
    LoRAStorageService& operator=(const LoRAStorageService&) = delete;
    
    /**
     * @brief Save Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] weights Input parameter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveAdapter(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    );
    
    /**
     * @brief Load Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterWeights> loadAdapter(const std::string& adapter_id);
    
    /**
     * @brief Load Metadata.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> loadMetadata(const std::string& adapter_id);
    
    /**
     * @brief Delete Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool deleteAdapter(const std::string& adapter_id);
    
    /**
     * @brief Exists.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool exists(const std::string& adapter_id) const;
    
    /**
     * @brief List Adapters.
     * @return Return value.
     */
    std::vector<std::string> listAdapters() const;
    
    /**
     * @brief Create Version.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::string createVersion(const std::string& adapter_id);
    
    /**
     * @brief Rollback To Version.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackToVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief List Versions.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<std::string> listVersions(const std::string& adapter_id) const;
    
    /**
     * @brief Update Metadata.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata);
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    json getStats() const;
    
    // ═══════════════════════════════════════════════════════════
    // Graph & Vector Extensions
    // ═══════════════════════════════════════════════════════════
    
    bool addGraphEdge(
        const std::string& from_id,
        const std::string& to_id,
        LoRAEdgeType edge_type,
        float weight = 1.0f
    );
    
    std::vector<LoRAGraphEdge> getGraphEdges(
        const std::string& adapter_id,
        const std::string& direction = "both"
    ) const;
    
    /**
     * @brief Get Lineage Path.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    LoRAGraphPath getLineagePath(const std::string& adapter_id) const;
    
    /**
     * @brief Store Embedding.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] embedding Input parameter.
     * @return True when the operation succeeds.
     */
    bool storeEmbedding(
        const std::string& adapter_id,
        const LoRAVectorEmbedding& embedding
    );
    
    /**
     * @brief Get Embeddings.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<LoRAVectorEmbedding> getEmbeddings(const std::string& adapter_id) const;
    
    std::vector<std::pair<std::string, float>> findSimilarAdapters(
        const std::string& adapter_id,
        int k = 10,
        float threshold = 0.7f
    ) const;
    
    /**
     * @brief Get Enhanced Info.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterInfoEnhanced> getEnhancedInfo(const std::string& adapter_id) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
