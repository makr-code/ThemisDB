/**
 * @file lora_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_provenance.h"
#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/multi_lora_manager.h"
#include "llm/decision_record_yaml_processor.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Unified LoRA Orchestrator - Complete CRUD Management
 * 
 * Central coordinator for ALL LoRA operations in ThemisDB.
 * Integrates:
 * - New simplified framework (adapter_manager, storage, training)
 * - Existing advanced MultiLoRAManager
 * - Workflow management and job scheduling
 * 
 * Provides complete CRUD interface:
 * - CREATE: Train new adapters from data
 * - READ: Query, list, get info about adapters
 * - UPDATE: Retrain, version, update metadata
 * - DELETE: Remove adapters and versions
 * 
 * Plus orchestration features:
 * - Job scheduling and queuing
 * - Workflow management
 * - Event notifications
 * - Health monitoring
 */
class LoRAOrchestrator {
public:
    /**
     * @brief Job status for async operations
     */
    enum class JobStatus {
        Pending,
        Running,
        Completed,
        Failed,
        Cancelled
    };
    
    /**
     * @brief Job type
     */
    enum class JobType {
        Training,
        Loading,
        Unloading,
        Versioning,
        Deployment
    };
    
    /**
     * @brief Job information
     */
    struct JobInfo {
        std::string job_id;
        JobType type;
        JobStatus status;
        std::string adapter_id;
        float progress = 0.0f;  // 0.0 to 1.0
        std::chrono::system_clock::time_point started_at;
        std::chrono::system_clock::time_point updated_at;
        std::string error_message;
        json metadata;
        
        json toJSON() const;
    };
    
    /**
     * @brief Event type for notifications
     */
    enum class EventType {
        AdapterLoaded,
        AdapterUnloaded,
        TrainingStarted,
        TrainingCompleted,
        TrainingFailed,
        VersionCreated,
        AdapterDeleted,
        JobQueued,
        JobStarted,
        JobCompleted,
        JobFailed
    };
    
    /**
     * @brief Event callback
     */
    using EventCallback = std::function<void(EventType, const std::string&, const json&)>;
    
    /**
     * @brief Configuration for orchestrator
     */
    struct Config {
        // Adapter manager config
        MultiLoRAManager::Config adapter_config;
        
        // Storage config
        LoRAStorageService::Config storage_config;
        
        // Training config
        LoRATrainingService::Config training_config;
        
        // Orchestration settings
        int max_concurrent_jobs = 3;
        bool enable_job_queue = true;
        bool enable_auto_versioning = true;
        bool enable_health_monitoring = true;
        std::chrono::seconds health_check_interval{60};
        
        // Integration with existing managers
        bool use_multi_lora_manager = true;  // Use advanced features when available
    };
    
    explicit LoRAOrchestrator(const Config& config);
    explicit LoRAOrchestrator();
    ~LoRAOrchestrator();
    
    // Disable copy
    LoRAOrchestrator(const LoRAOrchestrator&) = delete;
    LoRAOrchestrator& operator=(const LoRAOrchestrator&) = delete;
    
    // ═══════════════════════════════════════════════════════════
    // CREATE Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Create new LoRA adapter through training
     * @param adapter_id Unique identifier
     * @param training_data Training dataset
     * @param hyperparameters Training hyperparameters (optional)
     * @param async Run training asynchronously
     * @return Job ID if async, or immediate result
     */
    std::string createAdapter(
        const std::string& adapter_id,
        const TrainingData& training_data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );
    
    /**
     * @brief Create adapter from multiple datasets (batch training)
     * @param adapter_id Unique identifier
     * @param datasets Vector of training datasets
     * @param hyperparameters Training hyperparameters (optional)
     * @param async Run training asynchronously
     * @return Job ID if async, or immediate result
     */
    std::string createAdapterBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& datasets,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );
    
    /**
     * @brief Import existing adapter from external source
     * @param adapter_id Unique identifier
     * @param source_path Path to adapter weights
     * @param metadata Adapter metadata
     * @return true if imported successfully
     */
    bool importAdapter(
        const std::string& adapter_id,
        const std::string& source_path,
        const AdapterMetadata& metadata
    );
    
    // ═══════════════════════════════════════════════════════════
    // READ Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get adapter information
     * @param adapter_id Adapter identifier
     * @return Optional adapter info
     */
    std::optional<AdapterInfo> getAdapter(const std::string& adapter_id) const;
    
    /**
     * @brief List all adapters
     * @param filter Optional filter criteria
     * @return Vector of adapter infos
     */
    std::vector<AdapterInfo> listAdapters(const std::optional<std::string>& filter = std::nullopt) const;
    
    /**
     * @brief Check if adapter exists
     * @param adapter_id Adapter identifier
     * @return true if exists
     */
    bool exists(const std::string& adapter_id) const;
    
    /**
     * @brief Check if adapter is loaded in memory
     * @param adapter_id Adapter identifier
     * @return true if loaded
     */
    bool isLoaded(const std::string& adapter_id) const;
    
    /**
     * @brief Get adapter versions
     * @param adapter_id Adapter identifier
     * @return Vector of version identifiers
     */
    std::vector<std::string> getVersions(const std::string& adapter_id) const;
    
    /**
     * @brief Get current version of adapter
     * @param adapter_id Adapter identifier
     * @return Version identifier
     */
    std::string getCurrentVersion(const std::string& adapter_id) const;
    
    /**
     * @brief Search adapters by criteria
     * @param criteria Search criteria (JSON)
     * @return Vector of matching adapter infos
     */
    std::vector<AdapterInfo> searchAdapters(const json& criteria) const;
    
    // ═══════════════════════════════════════════════════════════
    // UPDATE Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update adapter through retraining
     * @param adapter_id Adapter identifier
     * @param training_data New training data
     * @param incremental Use incremental training (fine-tune existing)
     * @param async Run training asynchronously
     * @return Job ID if async, or immediate result
     */
    std::string updateAdapter(
        const std::string& adapter_id,
        const TrainingData& training_data,
        bool incremental = true,
        bool async = false
    );
    
    /**
     * @brief Update adapter metadata
     * @param adapter_id Adapter identifier
     * @param metadata New metadata
     * @return true if updated successfully
     */
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata);
    
    /**
     * @brief Create new version of adapter
     * @param adapter_id Adapter identifier
     * @param description Version description (optional)
     * @return Version identifier
     */
    std::string createVersion(const std::string& adapter_id, const std::string& description = "");
    
    /**
     * @brief Switch to specific version
     * @param adapter_id Adapter identifier
     * @param version Version identifier
     * @return true if switched successfully
     */
    bool switchVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief Rollback to previous version
     * @param adapter_id Adapter identifier
     * @return true if rolled back successfully
     */
    bool rollback(const std::string& adapter_id);
    
    // ═══════════════════════════════════════════════════════════
    // DELETE Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Delete adapter completely
     * @param adapter_id Adapter identifier
     * @param delete_all_versions Delete all versions (default: false)
     * @return true if deleted successfully
     */
    bool deleteAdapter(const std::string& adapter_id, bool delete_all_versions = false);
    
    /**
     * @brief Delete specific version
     * @param adapter_id Adapter identifier
     * @param version Version identifier
     * @return true if deleted successfully
     */
    bool deleteVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief Unload adapter from memory (keep in storage)
     * @param adapter_id Adapter identifier
     * @param force Force unload even if pinned
     * @return true if unloaded successfully
     */
    bool unloadAdapter(const std::string& adapter_id, bool force = false);
    
    // ═══════════════════════════════════════════════════════════
    // Orchestration & Job Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Load adapter into memory
     * @param adapter_id Adapter identifier
     * @param async Load asynchronously
     * @return Job ID if async, or immediate status
     */
    std::string loadAdapter(const std::string& adapter_id, bool async = false);
    
    /**
     * @brief Get job information
     * @param job_id Job identifier
     * @return Optional job info
     */
    std::optional<JobInfo> getJob(const std::string& job_id) const;
    
    /**
     * @brief List all jobs
     * @param status Filter by status (optional)
     * @return Vector of job infos
     */
    std::vector<JobInfo> listJobs(const std::optional<JobStatus>& status = std::nullopt) const;
    
    /**
     * @brief Cancel running job
     * @param job_id Job identifier
     * @return true if cancelled successfully
     */
    bool cancelJob(const std::string& job_id);
    
    /**
     * @brief Wait for job completion
     * @param job_id Job identifier
     * @param timeout_seconds Timeout in seconds (0 = no timeout)
     * @return Job result
     */
    JobInfo waitForJob(const std::string& job_id, int timeout_seconds = 0);
    
    /**
     * @brief Register event callback
     * @param callback Callback function
     */
    void registerEventCallback(EventCallback callback);
    
    // ═══════════════════════════════════════════════════════════
    // Health & Statistics
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get orchestrator statistics
     * @return Statistics as JSON
     */
    json getStats() const;
    
    /**
     * @brief Get health status
     * @return Health status as JSON
     */
    json getHealth() const;
    
    /**
     * @brief Get memory usage
     * @return Memory usage statistics
     */
    json getMemoryUsage() const;
    
    /**
     * @brief Perform health check
     * @return true if healthy
     */
    bool healthCheck() const;
    
    /**
     * @brief Clear cache
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Advanced Integration (with existing managers)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get MultiLoRAManager instance (if enabled)
     * @return Pointer to MultiLoRAManager or nullptr
     */
    MultiLoRAManager* getMultiLoRAManager();
    
    /**
     * @brief Enable/disable advanced features
     * @param enable Enable advanced features
     */
    void enableAdvancedFeatures(bool enable);
    
    // ═══════════════════════════════════════════════════════════
    // Component Access (for cross-shard sync)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get storage service instance
     * @return Shared pointer to storage service
     */
    std::shared_ptr<LoRAStorageService> getStorageService() const;
    
    /**
     * @brief Get consistency checker instance
     * @return Shared pointer to consistency checker
     */
    std::shared_ptr<AdapterConsistencyChecker> getConsistencyChecker() const;

    // ═══════════════════════════════════════════════════════════
    // Provenance, Snapshots, and Audit Log
    // ═══════════════════════════════════════════════════════════

    /// Attach a cryptographic provenance record to an adapter.
    /// Returns false if the adapter is not registered.
    bool attachProvenance(const std::string& adapter_id,
                          const LoRAProvenanceRecord& record);

    /// Retrieve the provenance record for an adapter.
    std::optional<LoRAProvenanceRecord> getProvenanceRecord(
        const std::string& adapter_id) const;

    /// Create an MVCC snapshot of the adapter's current state.
    AdapterSnapshot createAdapterSnapshot(const std::string& adapter_id,
                                          const std::string& version,
                                          const std::string& weights_hash);

    /// List all snapshots for an adapter (oldest first).
    std::vector<AdapterSnapshot> listAdapterSnapshots(
        const std::string& adapter_id) const;

    /// Append an inference audit entry to the adapter's Merkle chain.
    InferenceAuditEntry recordInferenceAudit(const std::string& adapter_id,
                                              InferenceAuditEntry entry);

    /// Retrieve the full Merkle-chained inference audit log.
    std::vector<InferenceAuditEntry> getInferenceAuditLog(
        const std::string& adapter_id) const;

    /// Verify the Merkle audit chain integrity.
    /// Returns true when the chain is intact; false when tampered or corrupt.
    bool verifyAuditChain(const std::string& adapter_id) const;

    /**
     * @brief Inject a `DecisionRecordYamlProcessor` for async YAML traceability.
     *
     * When set, every `loadAdapter()` call emits a `LOOP_TRIGGER` decision
     * record written asynchronously to
     * `logs/decisions/YYYY-MM-DD/<ts>_LOOP_TRIGGER_<id>.yaml`.
     *
     * @param processor  Shared processor instance (may be nullptr to disable).
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis

