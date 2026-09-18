/**
 * @file lora_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class LoRAOrchestrator {
public:
    enum class JobStatus {
        Pending,
        Running,
        Completed,
        Failed,
        Cancelled
    };
    
    enum class JobType {
        Training,
        Loading,
        Unloading,
        Versioning,
        Deployment
    };
    
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
        
        /**
         * @brief To JSON.
         * @return Return value.
         */
        json toJSON() const;
    };
    
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
    
    using EventCallback = std::function<void(EventType, const std::string&, const json&)>;
    
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
    
    /**
     * @brief Lo RAOrchestrator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRAOrchestrator(const Config& config);
    /**
     * @brief Lo RAOrchestrator.
     * @return Return value.
     */
    explicit LoRAOrchestrator();
    ~LoRAOrchestrator();
    
    // Disable copy
    LoRAOrchestrator(const LoRAOrchestrator&) = delete;
    LoRAOrchestrator& operator=(const LoRAOrchestrator&) = delete;
    
    // ═══════════════════════════════════════════════════════════
    // CREATE Operations
    // ═══════════════════════════════════════════════════════════
    
    std::string createAdapter(
        const std::string& adapter_id,
        const TrainingData& training_data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );
    
    std::string createAdapterBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& datasets,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );
    
    /**
     * @brief Import Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] source_path Path to the source.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool importAdapter(
        const std::string& adapter_id,
        const std::string& source_path,
        const AdapterMetadata& metadata
    );
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ READ Operations ═══════════════════════════════════════════════════════════
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    
    std::optional<AdapterInfo> getAdapter(const std::string& adapter_id) const;
    
    std::vector<AdapterInfo> listAdapters(const std::optional<std::string>& filter = std::nullopt) const;
    
    /**
     * @brief Exists.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool exists(const std::string& adapter_id) const;
    
    /**
     * @brief Is Loaded.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool isLoaded(const std::string& adapter_id) const;
    
    /**
     * @brief Get Versions.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<std::string> getVersions(const std::string& adapter_id) const;
    
    /**
     * @brief Get Current Version.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::string getCurrentVersion(const std::string& adapter_id) const;
    
    /**
     * @brief Search Adapters.
     * @param[in] criteria Input parameter.
     * @return Return value.
     */
    std::vector<AdapterInfo> searchAdapters(const json& criteria) const;
    
    // ═══════════════════════════════════════════════════════════
    // UPDATE Operations
    // ═══════════════════════════════════════════════════════════
    
    std::string updateAdapter(
        const std::string& adapter_id,
        const TrainingData& training_data,
        bool incremental = true,
        bool async = false
    );
    
    /**
     * @brief Update Metadata.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata);
    
    std::string createVersion(const std::string& adapter_id, const std::string& description = "");
    
    /**
     * @brief Switch Version.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool switchVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief Rollback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool rollback(const std::string& adapter_id);
    
    // ═══════════════════════════════════════════════════════════
    // DELETE Operations
    // ═══════════════════════════════════════════════════════════
    
    bool deleteAdapter(const std::string& adapter_id, bool delete_all_versions = false);
    
    /**
     * @brief Delete Version.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteVersion(const std::string& adapter_id, const std::string& version);
    
    bool unloadAdapter(const std::string& adapter_id, bool force = false);
    
    // ═══════════════════════════════════════════════════════════
    // Orchestration & Job Management
    // ═══════════════════════════════════════════════════════════
    
    std::string loadAdapter(const std::string& adapter_id, bool async = false);
    
    /**
     * @brief Get Job.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    std::optional<JobInfo> getJob(const std::string& job_id) const;
    
    std::vector<JobInfo> listJobs(const std::optional<JobStatus>& status = std::nullopt) const;
    
    /**
     * @brief Cancel Job.
     * @param[in] job_id Identifier of the job.
     * @return True when the operation succeeds.
     */
    bool cancelJob(const std::string& job_id);
    
    JobInfo waitForJob(const std::string& job_id, int timeout_seconds = 0);
    
    /**
     * @brief Register Event Callback.
     * @param[in] callback Input parameter.
     */
    void registerEventCallback(EventCallback callback);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Health & Statistics ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    json getStats() const;
    
    /**
     * @brief Get Health.
     * @return Return value.
     */
    json getHealth() const;
    
    /**
     * @brief Get Memory Usage.
     * @return Return value.
     */
    json getMemoryUsage() const;
    
    /**
     * @brief Health Check.
     * @return True when the operation succeeds.
     */
    bool healthCheck() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Advanced Integration (with existing managers) ═══════════════════════════════════════════════════════════
     * @return Pointer to the result.
     */
    
    MultiLoRAManager* getMultiLoRAManager();
    
    /**
     * @brief Enable Advanced Features.
     * @param[in] enable Input parameter.
     */
    void enableAdvancedFeatures(bool enable);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Component Access (for cross-shard sync) ═══════════════════════════════════════════════════════════
     * @return Return value.
     */
    
    std::shared_ptr<LoRAStorageService> getStorageService() const;
    
    /**
     * @brief Get Consistency Checker.
     * @return Return value.
     */
    std::shared_ptr<AdapterConsistencyChecker> getConsistencyChecker() const;

    /**
     * @brief ═══════════════════════════════════════════════════════════ Provenance, Snapshots, and Audit Log ═══════════════════════════════════════════════════════════
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */

    bool attachProvenance(const std::string& adapter_id,
                          const LoRAProvenanceRecord& record);

    /**
     * @brief Get Provenance Record.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<LoRAProvenanceRecord> getProvenanceRecord(
        const std::string& adapter_id) const;

    /**
     * @brief Create Adapter Snapshot.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] version Input parameter.
     * @param[in] weights_hash Input parameter.
     * @return Return value.
     */
    AdapterSnapshot createAdapterSnapshot(const std::string& adapter_id,
                                          const std::string& version,
                                          const std::string& weights_hash);

    /**
     * @brief List Adapter Snapshots.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<AdapterSnapshot> listAdapterSnapshots(
        const std::string& adapter_id) const;

    /**
     * @brief Record Inference Audit.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    InferenceAuditEntry recordInferenceAudit(const std::string& adapter_id,
                                              InferenceAuditEntry entry);

    /**
     * @brief Get Inference Audit Log.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<InferenceAuditEntry> getInferenceAuditLog(
        const std::string& adapter_id) const;

    /**
     * @brief Verify Audit Chain.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool verifyAuditChain(const std::string& adapter_id) const;

    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
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

