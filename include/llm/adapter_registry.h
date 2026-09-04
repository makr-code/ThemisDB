/**
 * @file adapter_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/security_signature_manager.h"
#include "llm/lora_framework/lora_provenance.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/// Semantic versioning for adapters
struct AdapterVersion {
    virtual ~AdapterVersion() = default;
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
        if (major != other.major) {
          return major < other.major;
        }
        if (minor != other.minor) {
          return minor < other.minor;
        }
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
    virtual ~TrainingConfig() = default;
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
    virtual ~QualityMetrics() = default;
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

/// Adapter role within the inference pipeline.
///
/// GENERAL  – standard task/domain fine-tuning adapter (default).
/// DRAFT    – small speculative-decoding draft model registered alongside a
///            larger target model.  Registered with a quantized (INT4) weight
///            set; @see AdaptiveVRAMAllocator::calculateDualModelAllocation().
enum class AdapterRole {
    GENERAL,  ///< Default: task/domain LoRA adapter.
    DRAFT,    ///< Speculative-decoding draft model adapter.
};

/// Adapter metadata - Complete information about a LoRA adapter
struct AdapterMetadata {
    virtual ~AdapterMetadata() = default;
    // Identification
    std::string adapter_id;          // Unique identifier (includes base_model)
    AdapterVersion version;
    std::string task_type;           // e.g., "question-answering", "summarization"
    std::string domain;              // e.g., "legal", "medical", "general"
    std::string language = "en";

    /// Role of this adapter in the inference pipeline.
    /// Set to AdapterRole::DRAFT when registering a speculative-decoding draft
    /// model so that InferenceEngineEnhanced can auto-discover it via
    /// AdapterRegistry::findDraftAdapterForFamily().
    AdapterRole role = AdapterRole::GENERAL;
    
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
    ~AdapterRegistry();
    
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

    /// List all adapters with a specific role.
    ///
    /// Useful for discovering registered DRAFT adapters:
    /// @code
    ///   auto drafts = registry.listAdaptersByRole(AdapterRole::DRAFT);
    /// @endcode
    std::vector<AdapterMetadata> listAdaptersByRole(AdapterRole role);

    /// Find the best DRAFT adapter for a given model family (architecture).
    ///
    /// Searches adapters whose role == DRAFT and whose `architecture` field
    /// contains @p model_family (case-insensitive substring match).  Among
    /// multiple candidates the adapter in DEPLOYED status is preferred; ties
    /// are broken by the highest version number.
    ///
    /// @param model_family  Model family string, e.g. "llama", "mistral".
    /// @return              Matching DRAFT adapter metadata, or std::nullopt
    ///                      when no DRAFT adapter for the family is registered.
    std::optional<AdapterMetadata> findDraftAdapterForFamily(
        const std::string& model_family);
    
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

    // Hot-Loading Interface

    /// Callback type invoked when an adapter is hot-loaded via hotLoad().
    /// @param adapter_id   The unique adapter identifier.
    /// @param weights_path Filesystem path to the adapter weights file.
    /// @param scale        LoRA scaling factor.
    using HotLoadCallback = std::function<void(const std::string& adapter_id,
                                               const std::string& weights_path,
                                               float scale)>;

    /// Register a LoRA adapter for hot-loading at inference time without
    /// engine restart.  Registers (or updates) the adapter metadata and then
    /// invokes all callbacks previously registered with addHotLoadObserver().
    ///
    /// Thread-safe: metadata update is protected by the registry mutex;
    /// callbacks are dispatched outside the lock to avoid inversion.
    ///
    /// @param adapter_id   Unique identifier for the adapter; must not be empty.
    /// @param weights_path Filesystem path to the adapter weights; must not be empty.
    /// @param metadata     Adapter metadata (base_model_name, version, etc.).
    /// @param scale        LoRA scaling factor (default 1.0).
    /// @return true if the adapter was registered/updated and callbacks fired
    ///         successfully, false on validation failure.
    bool hotLoad(const std::string& adapter_id,
                 const std::string& weights_path,
                 const AdapterMetadata& metadata,
                 float scale = 1.0f);

    /// Register an observer callback that is invoked whenever hotLoad() is
    /// called successfully.  Callbacks are dispatched in registration order.
    ///
    /// Thread-safe: protected by the registry mutex.
    ///
    /// @param callback Observer to register; must be non-null.
    void addHotLoadObserver(HotLoadCallback callback);

    // Provenance Integration
    
    /// Attach a cryptographic provenance record to a registered adapter.
    /// Returns false if the adapter does not exist.
    bool attachProvenance(const std::string& adapter_id,
                          const lora::LoRAProvenanceRecord& record);

    /// Retrieve the provenance record attached to an adapter.
    std::optional<lora::LoRAProvenanceRecord> getProvenanceRecord(
        const std::string& adapter_id) const;

    /// Record one inference event in the Merkle-chained audit log for an adapter.
    /// Populates entry_id, timestamp, previous_hash and entry_hash automatically.
    lora::InferenceAuditEntry recordInferenceAudit(
        const std::string& adapter_id,
        lora::InferenceAuditEntry entry);

    /// Retrieve the full Merkle-chained inference audit log for an adapter.
    std::vector<lora::InferenceAuditEntry> getInferenceAuditLog(
        const std::string& adapter_id) const;

    /// Verify the integrity of the Merkle audit chain for an adapter.
    bool verifyAuditChain(const std::string& adapter_id) const;

private:
    std::shared_ptr<storage::SecuritySignatureManager> sig_manager_;
    static constexpr const char* ADAPTER_KEY_PREFIX = "adapter:";
    static constexpr const char* BASE_MODEL_INDEX_PREFIX = "adapter_by_base_model:";
    static constexpr const char* DOMAIN_INDEX_PREFIX = "adapter_by_domain:";

    // Provenance manager for cryptographic audit and MVCC snapshots
    lora::LoRAProvenanceManager provenance_mgr_;

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
