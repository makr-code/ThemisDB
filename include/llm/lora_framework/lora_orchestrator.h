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
#include "llm/lora_framework/lora_training_backend.h"
#include "llm/lora_framework/lora_provenance.h"
#include "llm/lora_framework/adapter_provenance.h"
#include "llm/lora_framework/training_snapshot.h"
#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/multi_lora_manager.h"
#include "llm/decision_record_yaml_processor.h"
#include <chrono>
#include <cstdint>
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
 * @brief CRUD + lifecycle orchestrator for LoRA/AdaLoRA adapters.
 *
 * Training: createAdapter()/updateAdapter() submit real fine-tuning jobs to an
 * injected ILoRATrainingBackend. A job is reported Completed only after the
 * backend produced a trained artifact that was validated and published to the
 * versioned artifact store:
 *
 *   <training.artifact_root>/<adapter_id>/<version>/
 *       adapter_model.safetensors   distribution format (safetensors, no pickle)
 *       adapter_config.json         PEFT plain-LoRA config
 *       adapter.gguf                serving format (llama.cpp LoRA adapter)
 *       adapter.manifest.json       convenience copy of the embedded metadata
 *
 * Source of truth for adapter metadata is the `themis.*` key set embedded in
 * the safetensors `__metadata__` header and in the GGUF KV store; the manifest
 * is informational only. Adapters are never merged into base-model weights;
 * they are registered for dynamic (hot) loading via the adapter registrar
 * (default: MultiLoRAManager::loadLoRA by path).
 *
 * Thread-safety: all public methods are thread-safe. Async jobs run on worker
 * threads owned by the orchestrator (at most Config::max_concurrent_jobs);
 * the destructor cancels running jobs and joins the workers.
 */
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

    /**
     * @brief Registers a published GGUF adapter for dynamic loading.
     * @param adapter_id Adapter identifier.
     * @param gguf_path Path of the published GGUF adapter in the artifact store.
     * @param adapter_metadata Embedded `themis.*` metadata plus artifact digests.
     * @param[out] error Reason on failure.
     * @return True if the adapter is now loadable/loaded at runtime.
     */
    using AdapterRegistrar = std::function<bool(const std::string& adapter_id,
                                                const std::string& gguf_path,
                                                const json& adapter_metadata,
                                                std::string& error)>;

    /**
     * @brief Converts a safetensors PEFT package directory into a GGUF adapter
     *        and embeds the given `themis.*` metadata in the GGUF KV store.
     *
     * Server wiring binds this to
     * PeftSubprocessTrainingBackend::convertSafetensorsToGguf. On success the
     * result has success=true and gguf_adapter_path set.
     */
    using AdapterConverter = std::function<TrainingJobResult(const std::string& peft_dir,
                                                             const json& themis_metadata,
                                                             const std::string& output_gguf)>;

    /// @brief Defaults applied to every TrainingJobRequest built by the orchestrator.
    struct TrainingDefaults {
        std::string base_model;              ///< HF model id or local HF dir used for training.
        std::string base_model_gguf;         ///< GGUF base model the adapters must match when served.
        std::string base_model_gguf_sha256;  ///< Optional SHA-256 of the base GGUF (compatibility check).
        std::string work_root = "data/lora_training";     ///< Job-private scratch directories.
        std::string artifact_root = "data/lora_artifacts"; ///< Versioned artifact store root.
        AdapterTrainingMethod method = AdapterTrainingMethod::AdaLoRA;
        AdaLoRAScheduleParams adalora;       ///< AdaLoRA budget schedule.
        double holdout_fraction = 0.1;       ///< (0, 0.5]
        bool require_improvement = true;     ///< Fail if adapter eval loss >= baseline.
        std::uint64_t seed = 42;
    };

    /// @brief Outcome of importAdapterPackage().
    struct ImportResult {
        bool success = false;      ///< Package validated, converted, stored.
        std::string error;         ///< Non-empty iff !success.
        std::string version;       ///< Assigned local version.
        bool registered = false;   ///< Registered for dynamic loading.
        std::string registration_error;
    };

    /// @brief Target of regenerateAdapter(): same training data, different base LLM.
    struct RegenerationTarget {
        std::string adapter_id;              ///< New adapter id ([A-Za-z0-9_.-]+).
        std::string base_model;              ///< HF model id / local HF dir of the new base.
        std::string base_model_gguf;         ///< GGUF base model the new adapter is served on.
        std::string base_model_gguf_sha256;  ///< Optional SHA-256 of that GGUF.
    };

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

        /// Defaults for real training jobs (see TrainingDefaults).
        TrainingDefaults training;
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

    /// @brief Construction-time configuration (immutable after construction).
    [[nodiscard]] const Config& getConfig() const noexcept;
    
    // ═══════════════════════════════════════════════════════════
    // Training backend & dynamic-loading integration
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Inject the differentiable training backend (e.g. PEFT subprocess).
     * @param backend Backend; nullptr detaches it (new training jobs then fail
     *        with "no training backend configured").
     */
    void setTrainingBackend(std::shared_ptr<ILoRATrainingBackend> backend);

    /**
     * @brief Inject the registrar used to make published adapters loadable.
     * @note Overrides the registrar installed by setMultiLoRAManager().
     */
    void setAdapterRegistrar(AdapterRegistrar registrar);

    /**
     * @brief Attach a MultiLoRAManager; installs a registrar that (re)loads the
     *        published GGUF by path (unloadLoRA + loadLoRA) and makes
     *        unloadAdapter()/deleteAdapter() release the runtime slot.
     */
    void setMultiLoRAManager(std::shared_ptr<MultiLoRAManager> manager);

    /// @brief Inject the safetensors→GGUF converter used by importAdapterPackage().
    void setAdapterConverter(AdapterConverter converter);

    // ═══════════════════════════════════════════════════════════
    // Training-data snapshot provenance & adapter signing (WP H)
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Inject the MVCC snapshot reader used by createAdapterFromSnapshot()
     *        and regenerateAdapter().  nullptr disables both (jobs fail).
     */
    void setTrainingSnapshotReader(std::shared_ptr<TrainingSnapshotReader> reader);

    /**
     * @brief Inject the signer for newly trained adapters.
     *
     * When set, every trained adapter's safetensors `__metadata__` is signed
     * (`themis.provenance.*`) after training and the GGUF is regenerated from
     * the signed package via the AdapterConverter (required then).
     */
    void setAdapterSigner(std::shared_ptr<AdapterProvenanceSigner> signer);

    /**
     * @brief Inject the provenance verifier used on import, regeneration and
     *        (via MultiLoRAManager::setAdapterMetadataVerifier) GGUF hot-load.
     */
    void setAdapterProvenanceVerifier(std::shared_ptr<AdapterProvenanceVerifier> verifier);

    /**
     * @brief Set the signature policy.
     *
     * Defaults (fail closed): require_signed_imports, require_signed_hot_load
     * and require_signed_training are all true, so a deployment must wire an
     * AdapterSignaturePolicy-conformant signer (training) and verifier
     * (import / hot-load) or explicitly relax the policy.  A present but
     * invalid signature is always rejected regardless of the flags.
     */
    void setAdapterSignaturePolicy(const AdapterSignaturePolicy& policy);

    /// @brief Current signature policy.
    [[nodiscard]] AdapterSignaturePolicy getAdapterSignaturePolicy() const;

    /**
     * @brief Train a new adapter on an MVCC snapshot of `lora_ds:<collection>:*`.
     *
     * Captures one HLC snapshot (with a GC retention pin if the store has a
     * pin registry), reads all records at that timestamp, and trains on them.
     * The resulting adapter carries `themis.training.snapshot.*` metadata;
     * before signing, the snapshot is re-read and the job fails if the Merkle
     * root changed.  The pin lease id is recorded in the job / adapter
     * metadata as `snapshot_lease_id` (not signed).
     *
     * @param adapter_id Adapter id.
     * @param collection Dataset collection (identifier syntax).
     * @param limit      Maximum records (1..LoRADatasetLoader::kMaxSamples).
     * @return Job id (Failed immediately on capture/validation errors).
     */
    std::string createAdapterFromSnapshot(
        const std::string& adapter_id,
        const std::string& collection,
        std::size_t limit,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false);

    /**
     * @brief Retrain an existing adapter's exact training data on another base LLM.
     *
     * Reads the source adapter's stored safetensors package, verifies its
     * provenance signature (policy require_signed_imports), parses
     * `themis.training.snapshot.*`, pins and re-reads the data at `hlc_ts`
     * and recomputes the Merkle root.  On mismatch or if the snapshot is no
     * longer available (pruned) the job is Failed BEFORE the training backend
     * is invoked.  Split seed and holdout fraction are taken from the source.
     *
     * @param source_adapter_id Adapter whose snapshot is reused.
     * @param target            New adapter id and base model.
     * @return Job id.
     */
    std::string regenerateAdapter(
        const std::string& source_adapter_id,
        const RegenerationTarget& target,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false);

    /**
     * @brief Release a training-snapshot retention pin (ends reproducibility
     *        guarantees for adapters referencing it once GC runs).
     * @return true if the lease existed and was released.
     */
    bool releaseSnapshotPin(const std::string& lease_id);

    // ═══════════════════════════════════════════════════════════
    // CREATE Operations
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Submit a real fine-tuning job for a new adapter version.
     * @param adapter_id Adapter id ([A-Za-z0-9_.-]+, not "." / "..").
     * @param training_data At least two input/output samples.
     *        training_data.dataset_name is recorded as themis.training.dataset_id.
     * @param hyperparameters Optional overrides (default LoRAHyperparameters).
     * @param async true: enqueue and return immediately (job Pending/Running);
     *        false: run in the calling thread and return after completion.
     * @return Job id. Without a training backend the job is Failed with error
     *         "no training backend configured" and no adapter is registered.
     *         A job is Completed only after a trained artifact was published.
     */
    std::string createAdapter(
        const std::string& adapter_id,
        const TrainingData& training_data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );

    /// @brief Concatenate @p datasets and call createAdapter().
    std::string createAdapterBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& datasets,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt,
        bool async = false
    );

    /**
     * @brief Import an externally trained adapter distributed as safetensors package.
     *
     * Delegates to importAdapterPackage(); @p metadata.description is recorded.
     * @return True on successful validation, conversion and storage.
     */
    bool importAdapter(
        const std::string& adapter_id,
        const std::string& source_path,
        const AdapterMetadata& metadata
    );

    /**
     * @brief Validate, convert, store and register a distributable adapter package.
     *
     * Steps: reject pickle checkpoints (.bin/.pt/.pth/.pkl/.pickle/.ckpt);
     * natively validate the safetensors header; require `themis.adapter.id`
     * == @p adapter_id, `themis.adapter.sha256_tensors` == SHA-256 of the
     * tensor data region, and base-model identity compatible with
     * Config::training (base_model / base_model_gguf file name / sha256);
     * verify the `themis.provenance.*` signature over the canonical metadata
     * (AdapterSignaturePolicy::require_signed_imports; a present but invalid
     * signature is always rejected) and, when present, the structural
     * validity of `themis.training.snapshot.*`;
     * convert to GGUF via the injected converter; store safetensors + GGUF in
     * a new version directory; register for dynamic loading.
     * @param adapter_id Target adapter id.
     * @param source_path Package directory or its adapter_model.safetensors.
     */
    [[nodiscard]] ImportResult importAdapterPackage(const std::string& adapter_id,
                                                    const std::string& source_path);
    
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

    /**
     * @brief Train a new version of @p adapter_id on @p training_data.
     * @note The backend contract has no warm-start input, so each version is
     *       trained from the base model; @p incremental is recorded in the job
     *       metadata only.
     * @return Job id (same semantics as createAdapter()).
     */
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
     * @brief Cancel a Pending or Running job.
     * @param[in] job_id Identifier of the job.
     * @return True if a cancellation was requested (Pending jobs become
     *         Cancelled immediately; Running training jobs are signalled and
     *         become Cancelled once the backend stops).
     */
    bool cancelJob(const std::string& job_id);

    /**
     * @brief Block until the job reaches Completed/Failed/Cancelled.
     * @param timeout_seconds 0 = wait without timeout.
     * @return Latest job state (may still be non-terminal after a timeout);
     *         status Failed with "Job not found" for unknown ids.
     */
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

