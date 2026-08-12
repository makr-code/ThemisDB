/**
 * @file lora_storage_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Adapter weights representation
 */
struct AdapterWeights {
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
    
    explicit LoRAStorageService(const Config& config);
    explicit LoRAStorageService();
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
    
    // ═══════════════════════════════════════════════════════════
    // Graph & Vector Extensions
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Add graph edge between adapters
     * @param from_id Source adapter
     * @param to_id Target adapter
     * @param edge_type Edge type
     * @param weight Edge weight (optional)
     * @return true if added successfully
     */
    bool addGraphEdge(
        const std::string& from_id,
        const std::string& to_id,
        LoRAEdgeType edge_type,
        float weight = 1.0f
    );
    
    /**
     * @brief Get graph edges for adapter
     * @param adapter_id Adapter identifier
     * @param direction "incoming", "outgoing", or "both"
     * @return Vector of edges
     */
    std::vector<LoRAGraphEdge> getGraphEdges(
        const std::string& adapter_id,
        const std::string& direction = "both"
    ) const;
    
    /**
     * @brief Get lineage path (from base model to adapter)
     * @param adapter_id Adapter identifier
     * @return Graph path
     */
    LoRAGraphPath getLineagePath(const std::string& adapter_id) const;
    
    /**
     * @brief Store vector embedding for adapter
     * @param adapter_id Adapter identifier
     * @param embedding Vector embedding
     * @return true if stored successfully
     */
    bool storeEmbedding(
        const std::string& adapter_id,
        const LoRAVectorEmbedding& embedding
    );
    
    /**
     * @brief Get vector embeddings for adapter
     * @param adapter_id Adapter identifier
     * @return Vector of embeddings
     */
    std::vector<LoRAVectorEmbedding> getEmbeddings(const std::string& adapter_id) const;
    
    /**
     * @brief Find similar adapters using vector similarity
     * @param adapter_id Reference adapter
     * @param k Number of similar adapters to find
     * @param threshold Minimum similarity threshold (0-1)
     * @return Vector of similar adapter IDs with similarity scores
     */
    std::vector<std::pair<std::string, float>> findSimilarAdapters(
        const std::string& adapter_id,
        int k = 10,
        float threshold = 0.7f
    ) const;
    
    /**
     * @brief Get enhanced adapter info with graph and vector data
     * @param adapter_id Adapter identifier
     * @return Optional enhanced adapter info
     */
    std::optional<AdapterInfoEnhanced> getEnhancedInfo(const std::string& adapter_id) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
