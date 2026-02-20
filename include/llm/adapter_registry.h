#pragma once

#include "storage/security_signature_manager.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/// Semantic versioning for adapters
struct AdapterVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    std::string pre_release;  // e.g., "alpha", "beta", "rc.1"
    
    std::string toString() const {
        std::string version = std::to_string(major) + "." + 
                             std::to_string(minor) + "." + 
                             std::to_string(patch);
        if (!pre_release.empty()) {
            version += "-" + pre_release;
        }
        return version;
    }
    
    static AdapterVersion fromString(const std::string& version_str);
    
    bool operator<(const AdapterVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
    
    bool operator==(const AdapterVersion& other) const {
        return major == other.major && 
               minor == other.minor && 
               patch == other.patch &&
               pre_release == other.pre_release;
    }
};

/// Adapter signature for authenticity and integrity
struct AdapterSignature {
    std::string content_hash;       // SHA-256 hash of adapter weights
    std::string signature;          // Ed25519 digital signature
    std::string signer_identity;    // Identity of the signer
    std::string signing_timestamp;  // ISO 8601 timestamp
    std::string parent_adapter_signature;  // Chain of trust for incremental training
    
    nlohmann::json toJson() const;
    static AdapterSignature fromJson(const nlohmann::json& j);
};

/// Adapter provenance tracking
struct AdapterProvenance {
    std::string dataset_name;
    std::string data_source_uri;    // ThemisDB connection string or query
    std::string training_query;      // AQL query used for training
    std::string created_by;
    std::string created_at;          // ISO 8601 timestamp
    std::string parent_adapter_id;   // For incremental/continual training
    std::map<std::string, std::string> custom_metadata;
    
    nlohmann::json toJson() const;
    static AdapterProvenance fromJson(const nlohmann::json& j);
};

/// Training configuration
struct TrainingConfig {
    std::string dataset_name;
    size_t num_samples = 0;
    int epochs = 3;
    double learning_rate = 2e-4;
    int lora_rank = 8;
    double lora_alpha = 16.0;
    double lora_dropout = 0.1;
    std::vector<std::string> target_modules;  // e.g., ["q_proj", "v_proj", "k_proj", "o_proj"]
    std::string optimizer = "adamw";
    int batch_size = 4;
    int gradient_accumulation_steps = 4;
    int max_seq_length = 2048;
    double warmup_ratio = 0.03;
    std::string lr_scheduler = "cosine";
    
    nlohmann::json toJson() const;
    static TrainingConfig fromJson(const nlohmann::json& j);
};

/// Quality metrics from training
struct QualityMetrics {
    double final_loss = 0.0;
    double perplexity = 0.0;
    double accuracy = 0.0;
    std::map<std::string, double> eval_metrics;  // Custom evaluation metrics
    size_t training_samples = 0;
    size_t validation_samples = 0;
    std::string metrics_json;  // Full metrics as JSON
    
    nlohmann::json toJson() const;
    static QualityMetrics fromJson(const nlohmann::json& j);
};

/// Adapter metadata - Complete information about a LoRA adapter
struct AdapterMetadata {
    // Identification
    std::string adapter_id;          // Unique identifier (includes base_model)
    AdapterVersion version;
    std::string task_type;           // e.g., "question-answering", "summarization"
    std::string domain;              // e.g., "legal", "medical", "general"
    std::string language = "en";
    
    // Model compatibility
    std::string base_model_name;     // e.g., "mistral-7b", "llama-3-8b"
    std::string base_model_version;
    std::string architecture;        // e.g., "llama", "mistral", "gpt"
    int hidden_size = 0;            // Model hidden dimension
    int ffn_dimension = 0;          // FFN intermediate dimension
    std::string tokenizer_name;
    
    // Training information
    TrainingConfig training_config;
    AdapterProvenance provenance;
    QualityMetrics quality_metrics;
    
    // Security
    AdapterSignature signature;
    
    // Deployment
    std::string storage_path;        // Path in blob storage
    size_t file_size_bytes = 0;
    std::string format = "GGUF-ST";  // "GGUF-ST", "SafeTensors", "GGUF"
    std::string quantization = "Q4_K_M";  // Quantization type
    
    // Status
    enum class Status {
        TRAINING,
        TRAINED,
        DEPLOYED,
        DEPRECATED,
        FAILED
    };
    Status status = Status::TRAINED;
    
    std::string created_at;          // ISO 8601 timestamp
    std::string updated_at;          // ISO 8601 timestamp
    
    // Validation
    bool isCompatibleWith(const std::string& base_model, const std::string& model_version) const;
    
    nlohmann::json toJson() const;
    static AdapterMetadata fromJson(const nlohmann::json& j);
};

/// Adapter Registry - Manages adapter metadata with base-model grouping
/// Extends SecuritySignatureManager for cryptographic signing
class AdapterRegistry {
public:
    explicit AdapterRegistry(std::shared_ptr<storage::SecuritySignatureManager> sig_manager);
    
    // CRUD Operations
    
    /// Register a new adapter
    bool registerAdapter(const AdapterMetadata& metadata);
    
    /// Get adapter metadata by ID
    std::optional<AdapterMetadata> getAdapter(const std::string& adapter_id);
    
    /// Update adapter metadata
    bool updateAdapter(const AdapterMetadata& metadata);
    
    /// Delete adapter
    bool deleteAdapter(const std::string& adapter_id);
    
    /// List all adapters
    std::vector<AdapterMetadata> listAdapters();
    
    /// List adapters for a specific base model
    std::vector<AdapterMetadata> listAdaptersByBaseModel(const std::string& base_model);
    
    /// List adapters for a specific domain
    std::vector<AdapterMetadata> listAdaptersByDomain(const std::string& domain);
    
    // Compatibility Validation
    
    /// Validate adapter compatibility with base model
    struct ValidationResult {
        bool compatible = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        std::string toString() const;
    };
    
    ValidationResult validateCompatibility(
        const std::string& adapter_id,
        const std::string& base_model,
        const std::string& model_version
    );
    
    // Signature Operations
    
    /// Sign adapter with private key
    bool signAdapter(const std::string& adapter_id, const std::string& private_key);
    
    /// Verify adapter signature
    bool verifySignature(const std::string& adapter_id);
    
    /// Get adapter signature
    std::optional<AdapterSignature> getSignature(const std::string& adapter_id);
    
    // Version Management
    
    /// Get latest version of adapter
    std::optional<AdapterMetadata> getLatestVersion(const std::string& adapter_base_id);
    
    /// Get specific version of adapter
    std::optional<AdapterMetadata> getVersion(const std::string& adapter_base_id, const AdapterVersion& version);
    
    /// List all versions of adapter
    std::vector<AdapterMetadata> listVersions(const std::string& adapter_base_id);
    
    // Search and Discovery
    
    /// Search adapters by criteria
    struct SearchCriteria {
        std::optional<std::string> base_model;
        std::optional<std::string> domain;
        std::optional<std::string> task_type;
        std::optional<std::string> language;
        std::optional<AdapterMetadata::Status> status;
    };
    
    std::vector<AdapterMetadata> searchAdapters(const SearchCriteria& criteria);
    
    // Statistics
    
    struct RegistryStats {
        size_t total_adapters = 0;
        size_t total_base_models = 0;
        std::map<std::string, size_t> adapters_per_base_model;
        std::map<std::string, size_t> adapters_per_domain;
        size_t signed_adapters = 0;
        size_t deployed_adapters = 0;
        
        nlohmann::json toJson() const;
    };
    
    RegistryStats getStats() const;
    
private:
    std::shared_ptr<storage::SecuritySignatureManager> sig_manager_;
    static constexpr const char* ADAPTER_KEY_PREFIX = "adapter:";
    static constexpr const char* BASE_MODEL_INDEX_PREFIX = "adapter_by_base_model:";
    static constexpr const char* DOMAIN_INDEX_PREFIX = "adapter_by_domain:";

    // Pimpl for in-memory storage (thread-safe via mutex)
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string makeAdapterKey(const std::string& adapter_id) const;
    std::string makeBaseModelIndexKey(const std::string& base_model) const;
    std::string makeDomainIndexKey(const std::string& domain) const;

    // Helper: Update indices when adapter is registered/updated/deleted
    void updateIndices(const AdapterMetadata& metadata, bool remove = false);
};

} // namespace llm
} // namespace themis
