/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_plugin_manager.h                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/active_vram_allocator.h"
#include <functional>
#include "distributed_knowledge/adapter_capability_announcement.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include &lt;optional&gt;
#include <mutex>

// Forward-declare MetricsServer to avoid pulling httplib into every TU that
// includes llm_plugin_manager.h.
namespace themis { namespace llm { namespace monitoring { class MetricsServer; } } }

/**
 * @file llm_plugin_manager.h
 * @brief Manager for LLM plugins in ThemisDB
 * 
 * Coordinates multiple LLM backends and provides a unified interface
 * for the rest of ThemisDB to interact with LLM functionality.
 * 
 * Features:
 * - Multiple LLM backend support (llama.cpp, vLLM, etc.)
 * - Plugin discovery and loading
 * - Fallback and load balancing
 * - Integration with PluginManager
 */

namespace themis {
namespace llm {

/**
 * @brief LLM Plugin Manager
 * 
 * Central manager for all LLM plugins in ThemisDB.
 * Provides a simplified interface for LLM operations while
 * managing multiple backend plugins underneath.
 */
class LLMPluginManager {
public:
    LLMPluginManager() = default;
    ~LLMPluginManager() = default;
    
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
    bool loadModel(const std::string& model_id, const std::string& path);
    void unloadModel(const std::string& model_id);
    std::vector<std::string> listModels() const;

    // Convenience wrappers for LoRA management
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
