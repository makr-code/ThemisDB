/**
 * @file llm_deployment_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/model_downloader.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_model_storage.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

enum class DeploymentMode {
    OFFLINE,    // Only use locally cached models
    ONLINE,     // Download from remote sources
    AUTO        // Try local first, download if missing
};

struct ModelSource {
    /**
     * @brief Model Source.
     * @return Return value.
     */
    virtual ~ModelSource() = default;
    std::string type;              // "local", "ollama", "http", "https"
    std::string location;          // Path or URL
    std::string checksum_type;     // "sha256", "md5", etc.
    std::string checksum_value;    // Expected checksum
    int priority = 0;              // Higher priority sources are tried first
    
    json metadata;                 // Additional source-specific config
};

struct DeploymentConfig {
    DeploymentMode mode = DeploymentMode::AUTO;
    
    // Cache and storage
    std::string cache_directory = "./models";
    bool enable_cache = true;
    size_t max_cache_size_gb = 100;  // Maximum cache size in GB
    
    // BaseEntity storage (RocksDB integration)
    bool use_base_entity_storage = true;  // Store models in RocksDB as BaseEntity
    std::shared_ptr<RocksDBWrapper> db;   // RocksDB instance
    // Key prefix for RocksDB entries. Keys are constructed as: key_prefix + model_id
    // (e.g. default "llm_model::" + "my-model" → "llm_model::my-model").
    std::string key_prefix = "llm_model::";
    // Set to true only when a BlobStorageManager is configured; otherwise only
    // metadata is persisted and model weights remain on the local filesystem.
    bool store_weights_in_rocksdb = false;
    
    // Model sources (checked in priority order)
    std::vector<ModelSource> sources;
    
    // Ollama configuration
    std::string ollama_url = "http://localhost:11434";
    int ollama_timeout_seconds = 600;
    
    // Network configuration
    std::string proxy_url;
    std::string proxy_username;
    std::string proxy_password;
    
    // Authentication
    std::string auth_token;        // Bearer token for authenticated sources
    
    // Security
    bool verify_checksums = true;
    bool allow_insecure = false;   // Allow HTTP for testing
    
    // Logging and audit
    bool enable_audit_log = true;
    std::string audit_log_path = "./logs/model_deployment.log";
    
    // Cleanup policy
    bool auto_cleanup = false;
    int max_model_age_days = 90;   // Remove models older than this
    int keep_versions = 3;         // Keep N most recent versions
};

struct ModelStatus {
    /**
     * @brief Model Status.
     * @return Return value.
     */
    virtual ~ModelStatus() = default;
    std::string model_id;
    std::string model_path;
    std::string version;
    std::string format;            // "gguf", "safetensors", etc.
    
    bool is_loaded = false;
    bool is_cached = true;
    
    size_t size_bytes = 0;
    std::string checksum;
    std::string checksum_type;
    
    std::chrono::system_clock::time_point downloaded_at;
    std::chrono::system_clock::time_point last_used_at;
    std::chrono::system_clock::time_point last_verified_at;
    
    json metadata;
};

struct AuditEntry {
    std::string operation;         // "deploy", "fetch", "remove", "update", "verify"
    std::string model_id;
    std::string user;              // User/service that initiated operation
    std::chrono::system_clock::time_point timestamp;
    bool success = false;
    std::string error_message;
    json details;
};

class LLMDeploymentPlugin {
public:
    /**
     * @brief LLMDeployment Plugin.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LLMDeploymentPlugin(const DeploymentConfig& config);
    
    ~LLMDeploymentPlugin() = default;

    // ═══════════════════════════════════════════════════════════
    // Thread-local request context (JWT user propagation)
    // ═══════════════════════════════════════════════════════════

    struct RequestContext {
        std::string user_id;    ///< Authenticated user / service account
        std::string client_ip;  ///< Originating client IP address (may be empty)
    };

    /**
     * @brief Set Request Context.
     * @param[in] ctx Input parameter.
     * @note Exception safety: noexcept.
     */
    static void setRequestContext(const RequestContext& ctx) noexcept;

    /**
     * @brief Clear Request Context.
     * @note Exception safety: noexcept.
     */
    static void clearRequestContext() noexcept;

    static std::string currentUserId(const char* fallback = "system") noexcept;

    
    std::optional<ModelStatus> deployModel(const std::string& model_id, 
                                           bool force_download = false);
    
    ModelDownloadResult downloadModel(const std::string& model_id,
                                      DownloadProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Load Model.
     * @param[in] model_id Identifier of the model.
     * @param[in,out] llm_plugin Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool loadModel(const std::string& model_id, ILLMPlugin* llm_plugin);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Model Management ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    std::vector<std::string> listAvailableModels();
    
    /**
     * @brief List Cached Models.
     * @return Return value.
     */
    std::vector<ModelStatus> listCachedModels();
    
    /**
     * @brief Get Model Status.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelStatus> getModelStatus(const std::string& model_id);
    
    /**
     * @brief Verify Model.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool verifyModel(const std::string& model_id);
    
    /**
     * @brief Update Model.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelStatus> updateModel(const std::string& model_id);
    
    bool removeModel(const std::string& model_id, bool force = false);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Cleanup and Maintenance ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    int cleanupOldModels();
    
    /**
     * @brief Get Cache Size.
     * @return Return value.
     */
    size_t getCacheSize() const;
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    json getCacheStats() const;
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Configuration and Audit ═══════════════════════════════════════════════════════════
     * @param[in] config New access control configuration.
     */
    
    void updateConfig(const DeploymentConfig& config);
    
    const DeploymentConfig& getConfig() const { return config_; }
    
    std::vector<AuditEntry> getAuditLog(size_t limit = 100) const;
    
    /**
     * @brief Load Config From YAML.
     * @param[in] config_path Path to the retention policy configuration file.
     * @return Return value.
     */
    static std::optional<DeploymentConfig> loadConfigFromYAML(const std::string& config_path);
    
private:
    DeploymentConfig config_;
    std::shared_ptr<LLMModelStorage> model_storage_;  // BaseEntity storage for models
    std::unique_ptr<ModelDownloader> downloader_;
    std::vector<ModelStatus> model_registry_;
    std::vector<AuditEntry> audit_log_;
    
    // Helper methods
    /**
     * @brief Log Audit.
     * @param[in] entry Input parameter.
     */
    void logAudit(const AuditEntry& entry);
    /**
     * @brief Save Model Registry.
     */
    void saveModelRegistry();
    /**
     * @brief Load Model Registry.
     */
    void loadModelRegistry();
    /**
     * @brief Find Best Source.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelSource> findBestSource(const std::string& model_id);
    /**
     * @brief Get Model Path.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::string getModelPath(const std::string& model_id) const;
    /**
     * @brief Model Id To Filename.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    static std::string modelIdToFilename(const std::string& model_id);
    /**
     * @brief Verify Checksum.
     * @param[in] file_path Path to the file.
     * @param[in] expected_checksum Input parameter.
     * @param[in] checksum_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyChecksum(const std::string& file_path, 
                        const std::string& expected_checksum,
                        const std::string& checksum_type);
    
    // BaseEntity storage helpers
    /**
     * @brief Save Model To Storage.
     * @param[in] status Input parameter.
     * @param[in] file_path Path to the file.
     * @return True when the operation succeeds.
     */
    bool saveModelToStorage(const ModelStatus& status, const std::string& file_path);
    /**
     * @brief Load Model From Storage.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<LLMModelMetadata> loadModelFromStorage(const std::string& model_id);
    /**
     * @brief Update Model In Storage.
     * @param[in] model_id Identifier of the model.
     * @param[in] status Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateModelInStorage(const std::string& model_id, const ModelStatus& status);
    /**
     * @brief Delete Model From Storage.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool deleteModelFromStorage(const std::string& model_id);
};

} // namespace llm
} // namespace themis
