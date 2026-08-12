/**
 * @file llm_deployment_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Deployment mode for LLM models
 */
enum class DeploymentMode {
    OFFLINE,    // Only use locally cached models
    ONLINE,     // Download from remote sources
    AUTO        // Try local first, download if missing
};

/**
 * @brief Source configuration for model deployment
 */
struct ModelSource {
    virtual ~ModelSource() = default;
    std::string type;              // "local", "ollama", "http", "https"
    std::string location;          // Path or URL
    std::string checksum_type;     // "sha256", "md5", etc.
    std::string checksum_value;    // Expected checksum
    int priority = 0;              // Higher priority sources are tried first
    
    json metadata;                 // Additional source-specific config
};

/**
 * @brief Configuration for LLM deployment plugin
 */
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

/**
 * @brief Status of a deployed model
 */
struct ModelStatus {
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

/**
 * @brief Audit log entry for deployment operations
 */
struct AuditEntry {
    std::string operation;         // "deploy", "fetch", "remove", "update", "verify"
    std::string model_id;
    std::string user;              // User/service that initiated operation
    std::chrono::system_clock::time_point timestamp;
    bool success = false;
    std::string error_message;
    json details;
};

/**
 * @brief Production-ready LLM deployment plugin
 * 
 * This plugin provides enterprise-grade model deployment capabilities:
 * - Offline/online/auto deployment modes
 * - Multiple source support (local, Ollama, HTTP/S)
 * - Integrity verification with checksums
 * - Proxy and authentication support
 * - Version tracking and management
 * - Audit logging for compliance
 * - Automatic cleanup policies
 * 
 * Inspired by Ollama's deployment model with enhanced enterprise features.
 */
class LLMDeploymentPlugin {
public:
    /**
     * @brief Construct deployment plugin with configuration
     */
    explicit LLMDeploymentPlugin(const DeploymentConfig& config);
    
    ~LLMDeploymentPlugin() = default;

    // ═══════════════════════════════════════════════════════════
    // Thread-local request context (JWT user propagation)
    // ═══════════════════════════════════════════════════════════

    /// Authentication context set by HTTP handlers before invoking deployment operations.
    struct RequestContext {
        std::string user_id;    ///< Authenticated user / service account
        std::string client_ip;  ///< Originating client IP address (may be empty)
    };

    /// Set the authentication context for the calling thread.
    /// Must be called before any method that performs audit logging.
    static void setRequestContext(const RequestContext& ctx) noexcept;

    /// Clear the authentication context for the calling thread.
    static void clearRequestContext() noexcept;

    /// Return the user_id from the thread-local request context, or @p fallback.
    static std::string currentUserId(const char* fallback = "system") noexcept;

    
    /**
     * @brief Deploy a model (download if needed, verify, make available)
     * 
     * This is the main entry point for model deployment. It:
     * 1. Checks if model exists locally in cache directory
     * 2. Downloads from configured sources if needed (respecting deployment mode)
     * 3. Verifies integrity (checksum) if verify_checksums is enabled
     * 4. Makes model available for loading
     * 5. Updates status tracking
     * 6. Logs to audit log
     * 
     * @param model_id Model identifier (e.g., "llama2:7b")
     * @param force_download Force re-download even if cached
     * @return Model status or nullopt on failure
     */
    std::optional<ModelStatus> deployModel(const std::string& model_id, 
                                           bool force_download = false);
    
    /**
     * @brief Download a model from configured sources
     * 
     * @param model_id Model identifier
     * @param progress_callback Optional progress tracking
     * @return Download result
     */
    ModelDownloadResult downloadModel(const std::string& model_id,
                                      DownloadProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Load a model into memory using the LLM plugin
     * 
     * @param model_id Model identifier
     * @param llm_plugin LLM plugin instance to load into
     * @return true if loaded successfully
     */
    bool loadModel(const std::string& model_id, ILLMPlugin* llm_plugin);
    
    // ═══════════════════════════════════════════════════════════
    // Model Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief List all available models from configured sources
     * 
     * @return List of model identifiers
     */
    std::vector<std::string> listAvailableModels();
    
    /**
     * @brief List locally cached models
     * 
     * @return List of model statuses
     */
    std::vector<ModelStatus> listCachedModels();
    
    /**
     * @brief Get status of a specific model
     * 
     * @param model_id Model identifier
     * @return Model status or nullopt if not found
     */
    std::optional<ModelStatus> getModelStatus(const std::string& model_id);
    
    /**
     * @brief Verify model integrity (checksum)
     * 
     * @param model_id Model identifier
     * @return true if verification passed
     */
    bool verifyModel(const std::string& model_id);
    
    /**
     * @brief Update a model to the latest version
     * 
     * @param model_id Model identifier
     * @return Updated model status or nullopt on failure
     */
    std::optional<ModelStatus> updateModel(const std::string& model_id);
    
    /**
     * @brief Remove a model from cache
     * 
     * @param model_id Model identifier
     * @param force Force removal even if currently loaded
     * @return true if removed successfully
     */
    bool removeModel(const std::string& model_id, bool force = false);
    
    // ═══════════════════════════════════════════════════════════
    // Cleanup and Maintenance
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Clean up old models based on policy
     * 
     * Removes models that:
     * - Haven't been used for max_model_age_days
     * - Would reduce cache size below max_cache_size_gb
     * 
     * @return Number of models removed
     */
    int cleanupOldModels();
    
    /**
     * @brief Get current cache size
     * 
     * @return Cache size in bytes
     */
    size_t getCacheSize() const;
    
    /**
     * @brief Get cache usage statistics
     * 
     * @return JSON with cache stats
     */
    json getCacheStats() const;
    
    // ═══════════════════════════════════════════════════════════
    // Configuration and Audit
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update deployment configuration
     * 
     * @param config New configuration
     */
    void updateConfig(const DeploymentConfig& config);
    
    /**
     * @brief Get current configuration
     * 
     * @return Current configuration
     */
    const DeploymentConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get audit log entries
     * 
     * @param limit Maximum number of entries (0 = all)
     * @return Audit log entries
     */
    std::vector<AuditEntry> getAuditLog(size_t limit = 100) const;
    
    /**
     * @brief Load configuration from YAML file
     * 
     * @param config_path Path to YAML configuration
     * @return Loaded configuration or nullopt on failure
     */
    static std::optional<DeploymentConfig> loadConfigFromYAML(const std::string& config_path);
    
private:
    DeploymentConfig config_;
    std::shared_ptr<LLMModelStorage> model_storage_;  // BaseEntity storage for models
    std::unique_ptr<ModelDownloader> downloader_;
    std::vector<ModelStatus> model_registry_;
    std::vector<AuditEntry> audit_log_;
    
    // Helper methods
    void logAudit(const AuditEntry& entry);
    void saveModelRegistry();
    void loadModelRegistry();
    std::optional<ModelSource> findBestSource(const std::string& model_id);
    std::string getModelPath(const std::string& model_id) const;
    /// Converts a model_id into a sanitised filename (colons/slashes → '_', '.gguf' appended
    /// when no recognised extension is present). Shared by getModelPath() and findBestSource().
    static std::string modelIdToFilename(const std::string& model_id);
    bool verifyChecksum(const std::string& file_path, 
                        const std::string& expected_checksum,
                        const std::string& checksum_type);
    
    // BaseEntity storage helpers
    bool saveModelToStorage(const ModelStatus& status, const std::string& file_path);
    std::optional<LLMModelMetadata> loadModelFromStorage(const std::string& model_id);
    bool updateModelInStorage(const std::string& model_id, const ModelStatus& status);
    bool deleteModelFromStorage(const std::string& model_id);
};

} // namespace llm
} // namespace themis
