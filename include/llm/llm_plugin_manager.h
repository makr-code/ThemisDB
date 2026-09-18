#pragma once

/**
 * @file llm_plugin_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "distributed_knowledge/adapter_capability_announcement.h"
#include "llm/active_vram_allocator.h"
#include "llm/grafana_metrics.h"
#include "llm/llm_plugin_interface.h"
#include "llm/ssm_state_store.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace llm {

// Forward declaration
class SSMStateRocksDBStore;

class LLMPluginManager {
public:
    LLMPluginManager();
    ~LLMPluginManager() noexcept;
    
    // Prevent copying
    LLMPluginManager(const LLMPluginManager&) = delete;
    LLMPluginManager& operator=(const LLMPluginManager&) = delete;
    
    /**
     * @brief Register Plugin.
     * @param[in] name Input parameter.
     * @param[in] plugin Input parameter.
     */
    void registerPlugin(
        const std::string& name,
        std::unique_ptr<ILLMPlugin> plugin
    );
    
    /**
     * @brief Unregister Plugin.
     * @param[in] name Input parameter.
     */
    void unregisterPlugin(const std::string& name);
    
    /**
     * @brief Get Plugin.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     */
    ILLMPlugin* getPlugin(const std::string& name) const;
    
    /**
     * @brief Get Default Plugin.
     * @return Pointer to the result.
     */
    ILLMPlugin* getDefaultPlugin() const;
    
    /**
     * @brief Set Default Plugin.
     * @param[in] name Input parameter.
     */
    void setDefaultPlugin(const std::string& name);
    
    /**
     * @brief List Plugins.
     * @return Return value.
     */
    std::vector<std::string> listPlugins() const;
    
    /**
     * @brief Has Plugin.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasPlugin(const std::string& name) const;
    
    /**
     * @brief Get Aggregated Capabilities.
     * @return Return value.
     */
    json getAggregatedCapabilities() const;
    
    /**
     * @brief Get Aggregated Stats.
     * @return Return value.
     */
    json getAggregatedStats() const;
    
    /**
     * @brief Instance.
     * @return Return value.
     */
    static LLMPluginManager& instance();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Convenience methods (delegate to default plugin) ═══════════════════════════════════════════════════════════
     * @param[in] request Input parameter.
     * @return Return value.
     */
    
    InferenceResponse generate(const InferenceRequest& request);
    
    /**
     * @brief Generate RAG.
     * @param[in] rag_context Input parameter.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    /**
     * @brief Embed.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<float> embed(const std::string& text);

    /**
     * @brief Convenience wrappers for model management
     * @param[in] model_id Identifier of the model.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadModel(const std::string& model_id, const std::string& path);
    /**
     * @brief Unload Model.
     * @param[in] model_id Identifier of the model.
     */
    void unloadModel(const std::string& model_id);
    /**
     * @brief List Models.
     * @return Return value.
     */
    std::vector<std::string> listModels() const;

    /**
     * @brief Convenience wrappers for LoRA management
     * @param[in] lora_id Identifier of the lora.
     * @param[in] path Input parameter.
     * @param[in] base_model Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadLoRA(const std::string& lora_id, const std::string& path, const std::string& base_model);
    /**
     * @brief Unload Lo RA.
     * @param[in] lora_id Identifier of the lora.
     * @return True when the operation succeeds.
     */
    bool unloadLoRA(const std::string& lora_id);
    /**
     * @brief List Lo RAs.
     * @return Return value.
     */
    std::vector<LoRAInfo> listLoRAs() const;

    /**
     * @brief Streaming and ingestion helpers
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::vector<std::string> generateStream(const InferenceRequest& request);
    /**
     * @brief Ingest Model.
     * @param[in] model_id Identifier of the model.
     * @param[in] data Input parameter.
     * @return True when the operation succeeds.
     */
    bool ingestModel(const std::string& model_id, const std::string& data);
    /**
     * @brief Get Model Info.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelInfo> getModelInfo(const std::string& model_id) const;

    uint64_t getPluginOperationCount() const {
        return plugin_operation_count_.load(std::memory_order_acquire);
    }

    struct PluginStatistics {
        int models_loaded = 0;
        int loras_loaded = 0;
        uint64_t total_requests = 0;
        double throughput = 0.0;
        double average_latency_ms = 0.0;
        double cache_hit_rate = 0.0;
        int active_workers = 0;
        int queue_depth = 0;
    };

    struct CacheStatistics {
        size_t response_cache_hits = 0;
        size_t response_cache_misses = 0;
        size_t response_cache_entries = 0;
        double response_cache_hit_rate = 0.0;
        size_t prefix_cache_hits = 0;
        size_t prefix_cache_misses = 0;
        size_t prefix_cache_entries = 0;
        double prefix_cache_hit_rate = 0.0;
    };

    struct HealthStatus {
        bool is_healthy = true;
        std::string plugin_manager_status = "ok";
        std::string async_engine_status = "ok";
        int models_loaded = 0;
        int loras_loaded = 0;
        // VRAM pressure summary (populated from ActiveVRAMAllocator)
        size_t vram_total_bytes = 0;
        size_t vram_used_bytes  = 0;
        size_t vram_free_bytes  = 0;
        bool   vram_oom_threshold_exceeded = false;
    };

    struct SSMStateStoreConfig {
        // Enable SSM state persistence
        bool enabled = false;

        // Path to RocksDB database directory for state storage
        std::string rocksdb_path = "";

        // Retention window for snapshots (milliseconds)
        int64_t retention_window_ms = 24 * 60 * 60 * 1000;  // 24 hours

        // Maximum snapshots per session
        int32_t max_snapshots_per_session = 100;

        // Enable compression for stored snapshots
        bool enable_compression = true;

        // Sync writes to disk (safety vs. performance tradeoff)
        bool sync_on_checkpoint = false;
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    PluginStatistics getStatistics() const;
    /**
     * @brief Get Cache Statistics.
     * @return Return value.
     */
    CacheStatistics getCacheStatistics() const;
    /**
     * @brief Get Health Status.
     * @return Return value.
     */
    HealthStatus getHealthStatus() const;
    /**
     * @brief Clear All Caches.
     */
    void clearAllCaches();

    /**
     * @brief Get VRAMStats.
     * @return Return value.
     */
    ActiveVRAMAllocator::Stats getVRAMStats() const;

    // ── MSW: MetricsServer Admin Callback Wiring ─────────────────────────────

    using CancelSessionCallback = std::function<bool(const std::string& session_id)>;

    /**
     * @brief Set Cancel Session Callback.
     * @param[in] cb Input parameter.
     */
    void setCancelSessionCallback(CancelSessionCallback cb);

    /**
     * @brief Wire Metrics Server Callbacks.
     * @param[in,out] server Input/output parameter.
     */
    void wireMetricsServerCallbacks(monitoring::MetricsServer& server);
    void setAdapterPublisher(
        distributed_knowledge::GossipAdapterPublisher* publisher,
        std::string local_shard_id = "");

    /**
     * @brief ═══════════════════════════════════════════════════════════ SSM State Store Management (P2-D04 / P2-D05 Runtime Integration) ═══════════════════════════════════════════════════════════
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */

    bool initializeStateStore(const SSMStateStoreConfig& config);

    /**
     * @brief Checkpoint State.
     * @param[in] session_id Identifier of the session.
     * @param[in] snapshot Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkpointState(const std::string& session_id, const SSMStateSnapshot& snapshot);

    /**
     * @brief Recover State.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    std::optional<SSMStateSnapshot> recoverState(const std::string& session_id);

    /**
     * @brief Invalidate State.
     * @param[in] session_id Identifier of the session.
     * @return True when the operation succeeds.
     */
    bool invalidateState(const std::string& session_id);

    /**
     * @brief Compact State Store.
     * @return Return value.
     */
    uint64_t compactStateStore();

    /**
     * @brief Get State Store Statistics.
     * @return Return value.
     */
    std::string getStateStoreStatistics() const;

    std::unique_ptr<ILLMPlugin> CreatePluginSafe(
        const std::string& plugin_name,
        const std::string& config_json = ""
    );

    /**
     * @brief Initialize Plugin Safe.
     * @param[in] name Input parameter.
     * @param[in,out] plugin Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool InitializePluginSafe(
        const std::string& name,
        std::unique_ptr<ILLMPlugin>& plugin
    );

    /**
     * @brief Validate Model State.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool ValidateModelState(const std::string& model_id);

    std::vector<int32_t> ProcessTokensSafe(
        const std::vector<std::string>& tokens,
        size_t max_tokens = 8192
    );

private:
    struct PluginEntry {
        std::string name;
        std::unique_ptr<ILLMPlugin> plugin;
    };
    
    std::unordered_map<std::string, PluginEntry> plugins_;
    std::string default_plugin_name_;
    mutable std::mutex mutex_;

    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    // plugin_operation_count_ tracks total registerPlugin() calls atomically so that
    // concurrent registrations from multiple threads yield an exact final count
    // (verified by test L7-TS-04 via 8-thread stress).
    std::atomic<uint64_t> plugin_operation_count_{0};

    ActiveVRAMAllocator vram_allocator_;
    
    // THREAD-SAFETY: Protects vram_allocator_ and vram_handles_ to ensure
    // atomic registration/deregistration and prevent data races during
    // loadModel() / unloadModel() / getHealthStatus() sequences.
    mutable std::mutex vram_mutex_;

    std::unordered_map<std::string, ActiveVRAMAllocator::AllocationHandle> vram_handles_;

    CancelSessionCallback cancel_session_cb_;
    distributed_knowledge::GossipAdapterPublisher* adapter_publisher_ = nullptr;

    std::string local_shard_id_;

    std::unique_ptr<SSMStateRocksDBStore> state_store_;
    
    std::unique_ptr<rocksdb::TransactionDB> owned_state_db_;

    rocksdb::TransactionDB* state_db_ = nullptr;
    
    rocksdb::ColumnFamilyHandle* state_cf_ = nullptr;
    
    /**
     * @brief Get Default Plugin Locked.
     * @return Pointer to the result.
     */
    ILLMPlugin* getDefaultPluginLocked() const;
};

bool createLlamaWrapper(
    const std::string& name = "llamacpp",
    const std::string& model_path = "",
    const json& config = {}
);

} // namespace llm
} // namespace themis
