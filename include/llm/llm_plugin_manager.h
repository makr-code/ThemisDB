#pragma once

/**
 * @file llm_plugin_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief LLM Plugin Manager
 * 
 * Central manager for all LLM plugins in ThemisDB.
 * Provides a simplified interface for LLM operations while
 * managing multiple backend plugins underneath.
 */
class LLMPluginManager {
public:
    LLMPluginManager();
    ~LLMPluginManager() noexcept;
    
    // Prevent copying
    LLMPluginManager(const LLMPluginManager&) = delete;
    LLMPluginManager& operator=(const LLMPluginManager&) = delete;
    
    /**
     * @brief Register an LLM plugin
     * @param name Plugin identifier
     * @param plugin Plugin instance
     */
    void registerPlugin(
        const std::string& name,
        std::unique_ptr<ILLMPlugin> plugin
    );
    
    /**
     * @brief Unregister a plugin
     * @param name Plugin identifier
     */
    void unregisterPlugin(const std::string& name);
    
    /**
     * @brief Get a specific plugin
     * @param name Plugin identifier
     * @return Plugin instance or nullptr if not found
     */
    ILLMPlugin* getPlugin(const std::string& name) const;
    
    /**
     * @brief Get the default/primary plugin
     * @return Primary plugin or nullptr if none registered
     */
    ILLMPlugin* getDefaultPlugin() const;
    
    /**
     * @brief Set the default plugin
     * @param name Plugin identifier
     */
    void setDefaultPlugin(const std::string& name);
    
    /**
     * @brief List all registered plugins
     * @return Vector of plugin names
     */
    std::vector<std::string> listPlugins() const;
    
    /**
     * @brief Check if a plugin is registered
     * @param name Plugin identifier
     */
    bool hasPlugin(const std::string& name) const;
    
    /**
     * @brief Get aggregated capabilities from all plugins
     */
    json getAggregatedCapabilities() const;
    
    /**
     * @brief Get aggregated statistics from all plugins
     */
    json getAggregatedStats() const;
    
    /**
     * @brief Singleton instance
     */
    static LLMPluginManager& instance();
    
    // ═══════════════════════════════════════════════════════════
    // Convenience methods (delegate to default plugin)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate text using default plugin
     */
    InferenceResponse generate(const InferenceRequest& request);
    
    /**
     * @brief RAG generation using default plugin
     */
    InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    /**
     * @brief Embed text using default plugin
     */
    std::vector<float> embed(const std::string& text);

    // Convenience wrappers for model management
    /**
     * @brief Load an LLM model via the default plugin
     * @param model_id Unique model identifier
     * @param path Model file path
     * @return true if loaded successfully, false if model_id/path empty or load failed
     */
    bool loadModel(const std::string& model_id, const std::string& path);
    void unloadModel(const std::string& model_id);
    std::vector<std::string> listModels() const;

    // Convenience wrappers for LoRA management
    /**
     * @brief Load a LoRA adapter via the default plugin
     * @param lora_id Unique LoRA adapter identifier
     * @param path LoRA file path
     * @param base_model Base model context identifier
     * @return true if loaded successfully, false if lora_id/path empty or load failed
     */
    bool loadLoRA(const std::string& lora_id, const std::string& path, const std::string& base_model);
    bool unloadLoRA(const std::string& lora_id);
    std::vector<LoRAInfo> listLoRAs() const;

    // Streaming and ingestion helpers
    std::vector<std::string> generateStream(const InferenceRequest& request);
    bool ingestModel(const std::string& model_id, const std::string& data);
    std::optional<ModelInfo> getModelInfo(const std::string& model_id) const;

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

    /**
     * @brief Configuration for SSM state store initialization (P2-D04).
     *
     * Controls whether and how the LLMPluginManager manages persistent SSM state
     * snapshots via RocksDB. Used by initializeStateStore() during setup.
     */
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

    PluginStatistics getStatistics() const;
    CacheStatistics getCacheStatistics() const;
    HealthStatus getHealthStatus() const;
    void clearAllCaches();

    /**
     * @brief Return a snapshot of tracked VRAM statistics.
     *
     * Includes total/free/used bytes, peak usage, live allocation count,
     * OOM event and recovery counters, and the OOM-threshold flag.
     * In CPU-simulation builds (no CUDA) the numbers reflect
     * the simulation budget configured in ActiveVRAMAllocator::Config.
     */
    ActiveVRAMAllocator::Stats getVRAMStats() const;

    // ── MSW: MetricsServer Admin Callback Wiring ─────────────────────────────

    /**
     * @brief Callable type for session-cancellation callbacks (MSW).
     *
     * Receives a `session_id` string and returns `true` if the session was
     * found and cancelled, `false` if the session was not found.
     *
     * Wire this via `setCancelSessionCallback()` from wherever the
     * `ContinuousBatchScheduler` is owned (e.g. the `LlamaWrapper` or
     * `InferenceEngineEnhanced` that holds `batch_scheduler_`).
     */
    using CancelSessionCallback = std::function<bool(const std::string& session_id)>;

    /**
     * @brief Inject a session-cancellation callback for the MetricsServer
     *        DELETE /admin/sessions/{id} endpoint (MSW).
     *
     * When set, `wireMetricsServerCallbacks()` connects this callback to the
     * `MetricsServer::setSessionDeleteCallback()` slot so that DELETEs are
     * forwarded to the `ContinuousBatchScheduler::cancelRequest()` of the
     * underlying inference runtime.
     *
     * @param cb  `bool(const std::string& session_id)`.  May be `nullptr`
     *            to clear an existing registration.
     */
    void setCancelSessionCallback(CancelSessionCallback cb);

    /**
     * @brief Wire the three MetricsServer admin callbacks to this manager.
     *
     * After this call, the following HTTP endpoints on @p server become
     * operational instead of returning `{"status":"not_implemented"}`:
     *
     * | Endpoint                           | Wired to                          |
     * |------------------------------------|-----------------------------------|
     * | POST /admin/models/reload          | `loadModel(model_id, path)`       |
     * | POST /admin/prompt/simulate        | `estimateTokens(prompt)` heuristic|
     * | DELETE /admin/sessions/{id}        | `cancel_session_cb_` (if set)     |
     *
     * **Thread safety**: the lambdas capture `this` by raw pointer.  Ensure
     * that `server` lifetime does not exceed that of this `LLMPluginManager`.
     *
     * **Session-delete**: only operational if `setCancelSessionCallback()` was
     * called before `wireMetricsServerCallbacks()`.  Otherwise the DELETE
     * endpoint returns a clear `{"status":"not_configured"}` JSON body.
     *
     * @param server  `MetricsServer` instance to wire.  Must outlive the
     *                callbacks (i.e. must not be destroyed before this manager).
     */
    void wireMetricsServerCallbacks(monitoring::MetricsServer& server);
    /**
     * @brief Wire a @c GossipAdapterPublisher into the manager.
     *
     * When set, successful @c loadLoRA() calls broadcast an
     * @c AdapterCapabilityAnnouncement to the gossip network, and
     * @c unloadLoRA() broadcasts a zeroed-out withdrawal announcement.
     *
     * The pointer is non-owning: the caller must keep the publisher alive
     * for the lifetime of this @c LLMPluginManager instance.
     *
     * Pass @c nullptr to disconnect.
     *
     * @param publisher  Pointer to an initialised @c GossipAdapterPublisher,
     *                   or @c nullptr to disable gossip announcements.
     * @param local_shard_id  Shard ID embedded in outgoing announcements.
     */
    void setAdapterPublisher(
        distributed_knowledge::GossipAdapterPublisher* publisher,
        std::string local_shard_id = "");

    // ═══════════════════════════════════════════════════════════
    // SSM State Store Management (P2-D04 / P2-D05 Runtime Integration)
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Initialize and configure SSM state persistence via RocksDB.
     *
     * Creates an SSMStateRocksDBStore instance for durable session state snapshots.
     * Must be called before checkpoint() or recovery operations. If called multiple
     * times, the previous state store is replaced.
     *
     * @param config SSMStateStoreConfig specifying database path, retention policy, etc.
     * @return true if initialization succeeded, false if disabled or failed
     * @throws std::runtime_error if RocksDB initialization fails (when enabled)
     *
     * **Thread Safety:** Safe to call concurrently with plugin registration.
     * Acquires internal mutex briefly to swap state store instance.
     *
     * **Typical Usage:**
     * @code
     * LLMPluginManager mgr;
     * LLMPluginManager::SSMStateStoreConfig cfg;
     * cfg.enabled = true;
     * cfg.rocksdb_path = "/var/lib/themis/ssm_state";
     * mgr.initializeStateStore(cfg);
     *
     * // Later, checkpoint LLM operation results:
     * SSMStateSnapshot snap = {...};
     * mgr.checkpointState("session_123", snap);
     *
     * // And recover on restart:
     * auto restored = mgr.recoverState("session_123");
     * @endcode
     */
    bool initializeStateStore(const SSMStateStoreConfig& config);

    /**
     * @brief Persist an SSM state snapshot to durable storage.
     *
     * Checkpoints the given SSMStateSnapshot for later recovery. Requires a prior
     * call to initializeStateStore(). Does nothing if state store is not initialized.
     *
     * @param session_id Unique session identifier
     * @param snapshot SSMStateSnapshot to persist (includes LLM response, drift signal, etc.)
     * @return true if checkpoint succeeded or state store is disabled, false on error
     *
     * **Thread Safety:** Safe to call concurrently from multiple threads/sessions.
     * Each session_id is independently keyed.
     *
     * **Performance:** O(log N) RocksDB write with HLC-ordered key (typically <1ms).
     */
    bool checkpointState(const std::string& session_id, const SSMStateSnapshot& snapshot);

    /**
     * @brief Recover the most recent SSM state snapshot for a session.
     *
     * Retrieves the latest persisted SSM state snapshot for the given session_id.
     * Returns std::nullopt if no snapshots exist or state store is not initialized.
     *
     * @param session_id Unique session identifier
     * @return Most recent SSMStateSnapshot if available, nullopt otherwise
     *
     * **Thread Safety:** Safe to call concurrently.
     *
     * **Performance:** O(log N) RocksDB range scan (typically <5ms for 1000+ snapshots).
     *
     * **Use Case:**
     * - Load LLM session state on recovery/reconnect
     * - Validate that a session's episodic memory compressions are consistent
     * - Support point-in-time recovery via overload with explicit HLC timestamp
     */
    std::optional<SSMStateSnapshot> recoverState(const std::string& session_id);

    /**
     * @brief Clear all persisted snapshots for a session.
     *
     * Invalidates all stored state for the session_id (useful for session cleanup
     * or explicit state reset).
     *
     * @param session_id Unique session identifier
     * @return true if invalidation succeeded, false if state store not initialized or error
     *
     * **Thread Safety:** Safe to call concurrently.
     */
    bool invalidateState(const std::string& session_id);

    /**
     * @brief Run compaction to remove expired SSM state snapshots.
     *
     * Removes snapshots older than the configured retention window (default 24h).
     * Typically called during maintenance or low-traffic periods.
     *
     * @return Number of snapshots removed (0 if disabled or no expired snapshots)
     *
     * **Thread Safety:** Safe to call concurrently. Acquires mutex briefly.
     *
     * **Performance:** O(N) full-table scan (can take 100s of milliseconds for large databases).
     */
    uint64_t compactStateStore();

    /**
     * @brief Get statistics about persisted SSM state snapshots.
     *
     * Returns a JSON string with session count, total snapshot count, storage size, etc.
     * Useful for monitoring and capacity planning.
     *
     * @return JSON object as string, or "{}" if state store not initialized
     */
    std::string getStateStoreStatistics() const;

    /**
     * @brief Safely create plugin with null checks and exception handling (CRITICAL-4-1)
     * @param plugin_name Factory identifier
     * @param config_json Configuration JSON
     * @return Unique pointer to created plugin
     * @throws std::invalid_argument if plugin_name is empty
     * @throws std::runtime_error if factory not found or creation fails
     */
    std::unique_ptr<ILLMPlugin> CreatePluginSafe(
        const std::string& plugin_name,
        const std::string& config_json = ""
    );

    /**
     * @brief Safely initialize plugin with exception handling (CRITICAL-4-2)
     * @param name Plugin name
     * @param plugin Plugin instance reference
     * @return true if initialization succeeded
     */
    bool InitializePluginSafe(
        const std::string& name,
        std::unique_ptr<ILLMPlugin>& plugin
    );

    /**
     * @brief Validate model state and resources (CRITICAL-1-6)
     * @param model_id Model identifier
     * @return true if model is valid and ready
     */
    bool ValidateModelState(const std::string& model_id);

    /**
     * @brief Process tokens safely with bounds checking (CRITICAL-2-5)
     * @param tokens Token strings to process
     * @param max_tokens Maximum tokens to process
     * @return Vector of token IDs
     * @throws std::invalid_argument if inputs invalid
     * @throws std::overflow_error if token limit exceeded
     */
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

    /// VRAM budget tracker — registers externally-managed GPU memory (loaded models)
    /// for system-wide VRAM pressure monitoring and OOM-threshold alerting.
    ActiveVRAMAllocator vram_allocator_;
    
    // THREAD-SAFETY: Protects vram_allocator_ and vram_handles_ to ensure
    // atomic registration/deregistration and prevent data races during
    // loadModel() / unloadModel() / getHealthStatus() sequences.
    mutable std::mutex vram_mutex_;

    /// Maps model_id → VRAM handle so we can deregister on unload.
    std::unordered_map<std::string, ActiveVRAMAllocator::AllocationHandle> vram_handles_;

    /// MSW: injectable session-cancellation callback wired to the DELETE
    /// /admin/sessions/{id} endpoint of the MetricsServer.
    CancelSessionCallback cancel_session_cb_;
    /// Optional gossip publisher wired via setAdapterPublisher().
    /// Non-owning; may be nullptr when gossip is not configured.
    distributed_knowledge::GossipAdapterPublisher* adapter_publisher_ = nullptr;

    /// Shard ID used in outgoing adapter capability announcements.
    std::string local_shard_id_;

    /// SSM state store for persistent snapshot storage (P2-D04 / P2-D05)
    std::unique_ptr<SSMStateRocksDBStore> state_store_;
    
    /// RocksDB TransactionDB instance opened by initializeStateStore() (owned).
    /// When set, state_db_ below points to the same object.
    std::unique_ptr<rocksdb::TransactionDB> owned_state_db_;

    /// RocksDB instance for state storage.
    /// May point to owned_state_db_.get() (opened internally) or to an
    /// externally-injected instance.  Lifetime is always >= state_store_.
    rocksdb::TransactionDB* state_db_ = nullptr;
    
    /// Column family handle for SSM state (not owned by this class)
    rocksdb::ColumnFamilyHandle* state_cf_ = nullptr;
    
    ILLMPlugin* getDefaultPluginLocked() const;
};

/**
 * @brief Helper function to create and register a llama.cpp plugin
 * 
 * This is a convenience function for the most common use case.
 * 
 * @param name Plugin name (default: "llamacpp")
 * @param model_path Path to GGUF model file
 * @param config Plugin configuration
 * @return true if created and registered successfully
 */
bool createLlamaWrapper(
    const std::string& name = "llamacpp",
    const std::string& model_path = "",
    const json& config = {}
);

} // namespace llm
} // namespace themis
