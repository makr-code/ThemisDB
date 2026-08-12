/**
 * @file shard_resource_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/gossip_config_manager.h"
#include "utils/rate_limiter.h"
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <map>
#include <chrono>
#include <optional>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/** @brief Shard resource manager component. */
class ShardResourceManager {
public: 
    /** @brief Point-in-time resource and workload telemetry snapshot for one shard. */
    struct ResourceSnapshot {
        // Compute resources
        float cpu_usage_percent = 0.0f;
        uint64_t ram_usage_bytes = 0;
        uint64_t ram_total_bytes = 0;
        uint64_t vram_usage_bytes = 0;
        uint64_t vram_total_bytes = 0;
        
        // Storage resources
        uint64_t disk_used_bytes = 0;
        uint64_t disk_available_bytes = 0;
        
        // Network resources (rolling average)
        uint64_t network_in_bps = 0;
        uint64_t network_out_bps = 0;
        
        // Workload metrics
        uint32_t active_queries = 0;
        uint32_t pending_queries = 0;
        uint32_t active_transactions = 0;
        float avg_query_latency_ms = 0.0f;
        float p99_query_latency_ms = 0.0f;
        
        // Health score (0-100, 100 = perfect health)
        float health_score = 100.0f;
        
        std::chrono::system_clock::time_point timestamp;
        
        /** @brief Serialize snapshot into JSON for gossip transport/storage. */
        nlohmann::json toJson() const;
        /** @brief Deserialize snapshot from JSON payload. */
        static ResourceSnapshot fromJson(const nlohmann::json& j);
    };
    
    /** @brief Runtime configuration for sampling, throttling and gossip behavior. */
    struct Config {
        uint32_t snapshot_interval_ms = 5000;      // 5s
        bool enable_auto_throttling = true;
        float throttle_threshold = 0.85f;          // 85% capacity
        float critical_threshold = 0.95f;          // 95% capacity
        bool enable_gossip_broadcast = true;
        uint32_t peer_cache_ttl_ms = 30000;        // 30s
        bool enable_adaptive_health_score = true;

        // ── Repair I/O throttle (token-bucket) ─────────────────────────────
        /// Enable the IOPS token-bucket rate limiter for repair I/O.
        bool enable_repair_iops_throttle = true;
        /// Maximum percentage of node peak IOPS that repair may consume (0–100).
        float repair_iops_budget_percent = 10.0f;
        /// Estimated peak IOPS of the local node (used to derive the token rate).
        uint64_t peak_node_iops = 100'000;

        // ── GPU erasure coding feature flag ────────────────────────────────
        /// Enable GPU-accelerated erasure coding for bulk repair.
        /// When false (or when no CUDA device is detected at runtime) the
        /// engine falls back to the CPU/OpenCL path.
        bool enable_gpu_erasure_coding = false;
    };
    
    /** @brief Query admission estimate used by canAcceptQuery(). */
    struct QuerySpec {
        std::string query_id;
        size_t estimated_memory_bytes = 0;
        uint32_t estimated_cpu_percent = 0;
        std::chrono::milliseconds estimated_duration{0};
    };
    
    /** @brief Construct manager with explicit runtime configuration. */
    explicit ShardResourceManager(
        const std::string& local_shard_id,
        std::shared_ptr<GossipConfigManager> gossip_manager,
        const Config& config
    );

    /** @brief Construct manager with default configuration. */
    explicit ShardResourceManager(
        const std::string& local_shard_id,
        std::shared_ptr<GossipConfigManager> gossip_manager
    );
    
    /** @brief Stop background sampling thread and release monitoring resources. */
    ~ShardResourceManager();
    
    // Lifecycle
    /** @brief Start periodic resource sampling and optional gossip publication. */
    void start();
    /** @brief Stop periodic sampling loop and join background worker thread. */
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Local resource management
    /** @brief Return latest locally sampled resource snapshot. */
    ResourceSnapshot getCurrentSnapshot() const;
    /** @brief Evaluate whether query can be admitted under current local load. */
    bool canAcceptQuery(const QuerySpec& spec) const;
    /** @brief Update local query queue/latency metrics used for health scoring. */
    void updateQueryMetrics(uint32_t active, uint32_t pending, float avg_latency_ms);
    /** @brief Apply emergency throttling side effects when critical load is reached. */
    void throttleIfNeeded();

    // Repair I/O throttle
    /**
     * @brief Attempt to acquire @p io_ops repair I/O tokens without blocking.
     *
     * Returns true when the token-bucket had sufficient capacity (tokens
     * consumed).  Returns false when the repair IOPS budget is exhausted;
     * the caller should back-off before retrying.
     *
     * When the throttle is disabled (`config_.enable_repair_iops_throttle ==
     * false`) this always returns true.
     */
    bool acquireRepairIOToken(double io_ops = 1.0,
                              std::chrono::milliseconds wait_timeout = std::chrono::milliseconds{0});

    // GPU erasure coding feature flag
    /**
     * @brief Returns true when GPU erasure coding is both enabled in the
     *        config AND a CUDA device is available at runtime.
     *
     * When this returns false the caller should use the CPU/OpenCL path
     * (`gpu_erasure_coder_opencl.cpp`).
     */
    bool isGPUErasureCodingEnabled() const;

    // Gossip integration
    /** @brief Publish local resource snapshot through gossip manager. */
    void broadcastResourceUpdate();
    /** @brief Ingest peer snapshot update into local peer cache. */
    void receiveResourceUpdate(const std::string& shard_id, 
                                const ResourceSnapshot& snapshot);
    
    // Peer awareness (YARN-inspired)
    /** @brief Return complete peer snapshot cache keyed by shard id. */
    std::map<std::string, ResourceSnapshot> getPeerResources() const;
    /** @brief Return one peer snapshot when available in cache. */
    std::optional<ResourceSnapshot> getPeerResource(const std::string& shard_id) const;
    /** @brief Return peer ids whose health score is above healthy threshold. */
    std::vector<std::string> getHealthyPeers() const;
    /** @brief Return peer ids whose max(cpu,ram) load exceeds threshold. */
    std::vector<std::string> getOverloadedPeers(float threshold = 0.85f) const;
    
    // Health scoring
    /** @brief Compute current local shard health score in range [0,100]. */
    float calculateHealthScore() const;
    
private: 
    std::string local_shard_id_;
    std::shared_ptr<GossipConfigManager> gossip_manager_;
    Config config_;
    
    std::atomic<bool> running_{false};
    std::thread monitoring_thread_;
    
    // Repair I/O token-bucket rate limiter
    std::unique_ptr<themis::utils::RateLimiter> repair_io_limiter_;

    // Local resource cache
    mutable ResourceSnapshot local_snapshot_;
    mutable std::shared_mutex local_mutex_;
    
    // Peer resource cache
    std::map<std::string, ResourceSnapshot> peer_resources_;
    mutable std::shared_mutex peer_mutex_;
    
    // Platform-specific state for resource monitoring
#ifdef _WIN32
    mutable void* pdh_query_ = nullptr;
    mutable void* pdh_counter_ = nullptr;
    mutable bool pdh_initialized_ = false;
    mutable std::mutex pdh_mutex_;
#else
    mutable uint64_t prev_cpu_total_ = 0;
    mutable uint64_t prev_cpu_idle_ = 0;
    mutable std::mutex cpu_mutex_;
#endif
    
    // Monitoring loop
    /** @brief Background worker loop collecting local telemetry and peer-cache upkeep. */
    void monitoringLoop();
    /** @brief Collect platform-specific system metrics into local snapshot. */
    void collectSystemMetrics();
    /** @brief Remove peer snapshots older than configured TTL. */
    void cleanupStaleSnapshots();
    
    // Platform-specific helpers
    /** @brief Sample host CPU utilization percentage. */
    float getCpuUsage() const;
    /** @brief Sample RAM usage as {used,total} bytes. */
    std::pair<uint64_t, uint64_t> getRamUsage() const;
    /** @brief Sample VRAM usage as {used,total} bytes (or zeros when unavailable). */
    std::pair<uint64_t, uint64_t> getVramUsage() const;
    /** @brief Sample disk usage as {used,available} bytes. */
    std::pair<uint64_t, uint64_t> getDiskUsage() const;
    /** @brief Sample network usage counters/rates as {in,out}. */
    std::pair<uint64_t, uint64_t> getNetworkUsage() const;
    
    // Internal helper for health score calculation (no lock acquisition)
    float calculateHealthScoreInternal(const ResourceSnapshot& snapshot) const;
};

} // namespace themis::sharding
