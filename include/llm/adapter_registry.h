/**
 * @file adapter_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AdapterVersion {
    /**
     * @brief Adapter Version.
     * @return Return value.
     */
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
    
    /**
     * @brief From String.
     * @param[in] version_str Input parameter.
     * @return Return value.
     */
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

struct AdapterSignature {
    std::string content_hash;       // SHA-256 hash of adapter weights
    std::string signature;          // Ed25519 digital signature
    std::string signer_identity;    // Identity of the signer
    std::string signing_timestamp;  // ISO 8601 timestamp
    std::string parent_adapter_signature;  // Chain of trust for incremental training
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AdapterSignature fromJson(const nlohmann::json& j);
};

struct AdapterProvenance {
    std::string dataset_name;
    std::string data_source_uri;    // ThemisDB connection string or query
    std::string training_query;      // AQL query used for training
    std::string created_by;
    std::string created_at;          // ISO 8601 timestamp
    std::string parent_adapter_id;   // For incremental/continual training
    std::map<std::string, std::string> custom_metadata;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AdapterProvenance fromJson(const nlohmann::json& j);
};

struct TrainingConfig {
    /**
     * @brief Training Config.
     * @return Return value.
     */
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
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TrainingConfig fromJson(const nlohmann::json& j);
};

struct QualityMetrics {
    /**
     * @brief Quality Metrics.
     * @return Return value.
     */
    virtual ~QualityMetrics() = default;
    double final_loss = 0.0;
    double perplexity = 0.0;
    double accuracy = 0.0;
    std::map<std::string, double> eval_metrics;  // Custom evaluation metrics
    size_t training_samples = 0;
    size_t validation_samples = 0;
    std::string metrics_json;  // Full metrics as JSON
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static QualityMetrics fromJson(const nlohmann::json& j);
};

enum class AdapterRole {
    GENERAL,  ///< Default: task/domain LoRA adapter.
    DRAFT,    ///< Speculative-decoding draft model adapter.
};

struct AdapterMetadata {
    /**
     * @brief Adapter Metadata.
     * @return Return value.
     */
    virtual ~AdapterMetadata() = default;
    // Identification
    std::string adapter_id;          // Unique identifier (includes base_model)
    AdapterVersion version;
    std::string task_type;           // e.g., "question-answering", "summarization"
    std::string domain;              // e.g., "legal", "medical", "general"
    std::string language = "en";

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
    /**
     * @brief Is Compatible With.
     * @param[in] base_model Input parameter.
     * @param[in] model_version Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCompatibleWith(const std::string& base_model, const std::string& model_version) const;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AdapterMetadata fromJson(const nlohmann::json& j);
};

class AdapterRegistry {
public:
    /**
     * @brief Adapter Registry.
     * @param[in] sig_manager Input parameter.
     * @return Return value.
     */
    explicit AdapterRegistry(std::shared_ptr<storage::SecuritySignatureManager> sig_manager);
    ~AdapterRegistry();
    
    // CRUD Operations
    
    /**
     * @brief Register Adapter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool registerAdapter(const AdapterMetadata& metadata);
    
    /**
     * @brief Get Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> getAdapter(const std::string& adapter_id);
    
    /**
     * @brief Update Adapter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateAdapter(const AdapterMetadata& metadata);
    
    /**
     * @brief Delete Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool deleteAdapter(const std::string& adapter_id);
    
    /**
     * @brief List Adapters.
     * @return Return value.
     */
    std::vector<AdapterMetadata> listAdapters();
    
    /**
     * @brief List Adapters By Base Model.
     * @param[in] base_model Input parameter.
     * @return Return value.
     */
    std::vector<AdapterMetadata> listAdaptersByBaseModel(const std::string& base_model);
    
    /**
     * @brief List Adapters By Domain.
     * @param[in] domain Input parameter.
     * @return Return value.
     */
    std::vector<AdapterMetadata> listAdaptersByDomain(const std::string& domain);

    /**
     * @brief List Adapters By Role.
     * @param[in] role Input parameter.
     * @return Return value.
     */
    std::vector<AdapterMetadata> listAdaptersByRole(AdapterRole role);

    /**
     * @brief Find Draft Adapter For Family.
     * @param[in] model_family Input parameter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> findDraftAdapterForFamily(
        const std::string& model_family);
    
    // Compatibility Validation
    
    struct ValidationResult {
        bool compatible = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        /**
         * @brief To String.
         * @return Return value.
         */
        std::string toString() const;
    };
    
    /**
     * @brief Validate Compatibility.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] base_model Input parameter.
     * @param[in] model_version Input parameter.
     * @return Return value.
     */
    ValidationResult validateCompatibility(
        const std::string& adapter_id,
        const std::string& base_model,
        const std::string& model_version
    );
    
    // Signature Operations
    
    /**
     * @brief Sign Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] private_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool signAdapter(const std::string& adapter_id, const std::string& private_key);
    
    /**
     * @brief Verify Signature.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool verifySignature(const std::string& adapter_id);
    
    /**
     * @brief Get Signature.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterSignature> getSignature(const std::string& adapter_id);
    
    // Version Management
    
    /**
     * @brief Get Latest Version.
     * @param[in] adapter_base_id Identifier of the adapter base.
     * @return Return value.
     */
    std::optional<AdapterMetadata> getLatestVersion(const std::string& adapter_base_id);
    
    /**
     * @brief Get Version.
     * @param[in] adapter_base_id Identifier of the adapter base.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> getVersion(const std::string& adapter_base_id, const AdapterVersion& version);
    
    /**
     * @brief List Versions.
     * @param[in] adapter_base_id Identifier of the adapter base.
     * @return Return value.
     */
    std::vector<AdapterMetadata> listVersions(const std::string& adapter_base_id);
    
    // Search and Discovery
    
    struct SearchCriteria {
        std::optional<std::string> base_model;
        std::optional<std::string> domain;
        std::optional<std::string> task_type;
        std::optional<std::string> language;
        std::optional<AdapterMetadata::Status> status;
    };
    
    /**
     * @brief Search Adapters.
     * @param[in] criteria Input parameter.
     * @return Return value.
     */
    std::vector<AdapterMetadata> searchAdapters(const SearchCriteria& criteria);
    
    // Statistics
    
    struct RegistryStats {
        size_t total_adapters = 0;
        size_t total_base_models = 0;
        std::map<std::string, size_t> adapters_per_base_model;
        std::map<std::string, size_t> adapters_per_domain;
        size_t signed_adapters = 0;
        size_t deployed_adapters = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    RegistryStats getStats() const;

    // Hot-Loading Interface

    using HotLoadCallback = std::function<void(const std::string& adapter_id,
                                               const std::string& weights_path,
                                               float scale)>;

    bool hotLoad(const std::string& adapter_id,
                 const std::string& weights_path,
                 const AdapterMetadata& metadata,
                 float scale = 1.0f);

    /**
     * @brief Add Hot Load Observer.
     * @param[in] callback Input parameter.
     */
    void addHotLoadObserver(HotLoadCallback callback);

    // Provenance Integration
    
    /**
     * @brief Attach Provenance.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool attachProvenance(const std::string& adapter_id,
                          const lora::LoRAProvenanceRecord& record);

    /**
     * @brief Get Provenance Record.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<lora::LoRAProvenanceRecord> getProvenanceRecord(
        const std::string& adapter_id) const;

    /**
     * @brief Record Inference Audit.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    lora::InferenceAuditEntry recordInferenceAudit(
        const std::string& adapter_id,
        lora::InferenceAuditEntry entry);

    /**
     * @brief Get Inference Audit Log.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<lora::InferenceAuditEntry> getInferenceAuditLog(
        const std::string& adapter_id) const;

    /**
     * @brief Verify Audit Chain.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
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

    /**
     * @brief Make Adapter Key.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::string makeAdapterKey(const std::string& adapter_id) const;
    /**
     * @brief Make Base Model Index Key.
     * @param[in] base_model Input parameter.
     * @return Return value.
     */
    std::string makeBaseModelIndexKey(const std::string& base_model) const;
    /**
     * @brief Make Domain Index Key.
     * @param[in] domain Input parameter.
     * @return Return value.
     */
    std::string makeDomainIndexKey(const std::string& domain) const;

    // Helper: Update indices when adapter is registered/updated/deleted
    void updateIndices(const AdapterMetadata& metadata, bool remove = false);
};

} // namespace llm
} // namespace themis
