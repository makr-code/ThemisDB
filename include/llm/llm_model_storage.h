/**
 * @file llm_model_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/blob_storage_manager.h"
#include "storage/security_signature_manager.h"
#include "security/encryption.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief LLM Model Metadata (BaseEntity-compliant)
 * 
 * Stores LLM model information as BaseEntity documents in ThemisDB.
 * This ensures that LLM models follow the same pattern as LoRA adapters
 * and all other ThemisDB data: BaseEntity → Document/Graph/Vector.
 */
struct LLMModelMetadata {
    virtual ~LLMModelMetadata() = default;
    // Identity
    std::string model_id;              // Unique identifier (e.g., "llama-2-7b")
    std::string model_name;            // Display name
    std::string version;               // Model version (e.g., "2.0")
    std::string architecture;          // "llama", "mistral", "falcon", etc.
    
    // File information
    std::string file_path;             // Path to model file
    std::string format;                // "gguf", "safetensors", "pytorch"
    std::string quantization;          // "Q4_K_M", "Q8_0", "FP16", etc.
    size_t size_bytes = 0;            // Model file size
    std::string checksum;              // SHA256 hash
    std::string checksum_type = "sha256"; // Checksum algorithm type
    
    // Model parameters
    int64_t parameter_count = 0;       // Number of parameters (e.g., 7B)
    int context_length = 4096;         // Maximum context length
    int vocabulary_size = 32000;       // Vocabulary size
    int num_layers = 32;               // Number of layers
    int hidden_size = 4096;            // Hidden dimension size
    
    // Capabilities
    std::vector<std::string> capabilities;  // ["text-generation", "embeddings", "chat"]
    std::vector<std::string> languages;     // ["en", "de", "fr", etc.]
    std::vector<std::string> tags;          // ["instruction-tuned", "code", etc.]
    
    // Performance metrics
    float tokens_per_second = 0.0f;    // Avg tokens/sec
    size_t vram_required_mb = 0;       // VRAM requirement
    size_t ram_required_mb = 0;        // RAM requirement
    
    // Usage statistics
    int64_t total_inferences = 0;      // Total inference count
    int64_t total_tokens_generated = 0; // Total tokens generated
    std::chrono::system_clock::time_point first_used;
    std::chrono::system_clock::time_point last_used;
    
    // Provenance
    std::string source;                // "huggingface", "ollama", "custom"
    std::string source_url;            // Download URL
    std::string license;               // "Apache-2.0", "MIT", etc.
    std::string created_by;            // Organization/author
    std::chrono::system_clock::time_point created_at;
    
    // Metadata
    json custom_metadata;              // Additional metadata
    
    json toJSON() const {
        auto first_used_ts = std::chrono::system_clock::to_time_t(first_used);
        auto last_used_ts = std::chrono::system_clock::to_time_t(last_used);
        auto created_at_ts = std::chrono::system_clock::to_time_t(created_at);
        
        return json{
            {"model_id", model_id},
            {"model_name", model_name},
            {"version", version},
            {"architecture", architecture},
            {"file_path", file_path},
            {"format", format},
            {"quantization", quantization},
            {"size_bytes", size_bytes},
            {"checksum", checksum},
            {"checksum_type", checksum_type},
            {"parameter_count", parameter_count},
            {"context_length", context_length},
            {"vocabulary_size", vocabulary_size},
            {"num_layers", num_layers},
            {"hidden_size", hidden_size},
            {"capabilities", capabilities},
            {"languages", languages},
            {"tags", tags},
            {"tokens_per_second", tokens_per_second},
            {"vram_required_mb", vram_required_mb},
            {"ram_required_mb", ram_required_mb},
            {"total_inferences", total_inferences},
            {"total_tokens_generated", total_tokens_generated},
            {"first_used", first_used_ts},
            {"last_used", last_used_ts},
            {"source", source},
            {"source_url", source_url},
            {"license", license},
            {"created_by", created_by},
            {"created_at", created_at_ts},
            {"custom_metadata", custom_metadata}
        };
    }
    
    static LLMModelMetadata fromJSON(const json& j) {
        LLMModelMetadata metadata;
        
        if (j.contains("model_id")) {
          metadata.model_id = j["model_id"];
        }
        if (j.contains("model_name")) {
          metadata.model_name = j["model_name"];
        }
        if (j.contains("version")) {
          metadata.version = j["version"];
        }
        if (j.contains("architecture")) {
          metadata.architecture = j["architecture"];
        }
        if (j.contains("file_path")) {
          metadata.file_path = j["file_path"];
        }
        if (j.contains("format")) {
          metadata.format = j["format"];
        }
        if (j.contains("quantization")) {
          metadata.quantization = j["quantization"];
        }
        if (j.contains("size_bytes")) {
          metadata.size_bytes = j["size_bytes"];
        }
        if (j.contains("checksum")) {
          metadata.checksum = j["checksum"];
        }
        if (j.contains("parameter_count")) {
          metadata.parameter_count = j["parameter_count"];
        }
        if (j.contains("context_length")) {
          metadata.context_length = j["context_length"];
        }
        if (j.contains("capabilities")) {
          metadata.capabilities = j["capabilities"].get<std::vector<std::string>>();
        }
        if (j.contains("tags")) {
          metadata.tags = j["tags"].get<std::vector<std::string>>();
        }
        
        return metadata;
    }
};

/**
 * @brief Graph edge types for LLM model relationships
 */
enum class LLMEdgeType {
    DERIVED_FROM,       // Model derived from another (fine-tuned)
    QUANTIZED_FROM,     // Quantized version of another model
    MERGED_FROM,        // Merged from multiple models
    ADAPTED_WITH,       // Uses LoRA adapter
    SIMILAR_TO,         // Semantically similar models
    RECOMMENDED_FOR,    // Recommended for specific task
    DEPLOYED_ON,        // Deployed on specific infrastructure
    TRAINED_BY,         // Training provenance
    EVALUATED_ON        // Evaluation dataset
};

/**
 * @brief LLM Model Storage Service (BaseEntity-compliant)
 * 
 * Stores LLM models as BaseEntity documents with:
 * - Document model: Metadata as BaseEntity
 * - Graph model: Relationships between models
 * - Vector model: Embeddings for similarity search
 * - Blob storage: Actual model weights (smart tiering)
 * - Audit logging: Complete traceability
 */
class LLMModelStorage {
public:
    struct Config {
        // ThemisDB integration
        std::shared_ptr<RocksDBWrapper> db;
        std::shared_ptr<storage::BlobStorageManager> blob_manager;
        std::shared_ptr<storage::SecuritySignatureManager> signature_manager;
        std::shared_ptr<KeyProvider> key_provider;  // Configurable key provider (Vault/HSM/Mock)
        
        // Storage key prefix
        // Keys are constructed as: key_prefix + model_id (e.g. "llm_model::my-model")
        std::string key_prefix = "llm_model::";  // Full RocksDB key prefix
        
        // Security
        bool enable_encryption = false;
        std::string encryption_key_id = "llm_models";
        bool enable_signatures = true;
        
        // Storage
        bool use_blob_storage = true;  // Store model files in blob storage
        size_t inline_threshold_mb = 100;  // Files > 100MB go to blob storage
    };
    
    LLMModelStorage();
    explicit LLMModelStorage(const Config& config);
    ~LLMModelStorage();
    
    // ═══════════════════════════════════════════════════════════
    // CRUD Operations (BaseEntity-compliant)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Store LLM model as BaseEntity
     * @param metadata Model metadata
     * @param model_data Optional model file data (for blob storage)
     * @return true if stored successfully
     */
    bool storeModel(
        const LLMModelMetadata& metadata,
        const std::optional<std::vector<uint8_t>>& model_data = std::nullopt
    );
    
    /**
     * @brief Load LLM model metadata
     * @param model_id Model identifier
     * @return Optional model metadata
     */
    std::optional<LLMModelMetadata> loadModel(const std::string& model_id);
    
    /**
     * @brief Load model blob data from storage
     * 
     * Retrieves the actual model file data from blob storage or inline storage.
     * Handles both inline storage (for small models) and blob storage (for large models).
     * 
     * @param model_id Model identifier
     * @return Optional vector containing model file data, or nullopt if not found/error
     */
    std::optional<std::vector<uint8_t>> loadModelBlob(const std::string& model_id);
    
    /**
     * @brief Update model metadata
     * @param model_id Model identifier
     * @param metadata Updated metadata
     * @return true if updated successfully
     */
    bool updateModel(const std::string& model_id, const LLMModelMetadata& metadata);
    
    /**
     * @brief Delete model
     * @param model_id Model identifier
     * @return true if deleted successfully
     */
    bool deleteModel(const std::string& model_id);
    
    /**
     * @brief Check if model exists
     * @param model_id Model identifier
     * @return true if exists
     */
    bool exists(const std::string& model_id) const;
    
    /**
     * @brief List all models
     * @param filter Optional filter (e.g., by architecture, quantization)
     * @return Vector of model IDs
     */
    std::vector<std::string> listModels(const std::optional<std::string>& filter = std::nullopt) const;
    
    // ═══════════════════════════════════════════════════════════
    // Graph Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Add edge between models
     * @param from_id Source model
     * @param to_id Target model
     * @param edge_type Edge type
     * @param weight Edge weight (optional)
     * @return true if added successfully
     */
    bool addEdge(
        const std::string& from_id,
        const std::string& to_id,
        LLMEdgeType edge_type,
        float weight = 1.0f
    );
    
    /**
     * @brief Get edges for model
     * @param model_id Model identifier
     * @param direction "incoming", "outgoing", or "both"
     * @return Vector of edges
     */
    std::vector<json> getEdges(
        const std::string& model_id,
        const std::string& direction = "both"
    ) const;
    
    // ═══════════════════════════════════════════════════════════
    // Vector Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Store vector embedding for model
     * @param model_id Model identifier
     * @param embedding Vector embedding
     * @return true if stored successfully
     */
    bool storeEmbedding(
        const std::string& model_id,
        const std::vector<float>& embedding
    );
    
    /**
     * @brief Find similar models using vector similarity
     * @param model_id Reference model
     * @param k Number of similar models to find
     * @param threshold Minimum similarity threshold (0-1)
     * @return Vector of similar model IDs with similarity scores
     */
    std::vector<std::pair<std::string, float>> findSimilarModels(
        const std::string& model_id,
        int k = 10,
        float threshold = 0.7f
    ) const;
    
    // ═══════════════════════════════════════════════════════════
    // Statistics & Monitoring
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update usage statistics
     * @param model_id Model identifier
     * @param tokens_generated Number of tokens generated
     * @return true if updated successfully
     */
    bool updateUsageStats(const std::string& model_id, int64_t tokens_generated);
    
    /**
     * @brief Get storage statistics
     * @return Statistics as JSON
     */
    json getStats() const;
    
    /**
     * @brief Get configuration (for accessing blob manager, etc.)
     * @return Configuration object
     */
    const Config& getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    Config config_;  // Store config for external access
};

} // namespace llm
} // namespace themis
