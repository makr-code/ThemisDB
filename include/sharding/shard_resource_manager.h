/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_resource_manager.h                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:20:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 096960f501  2026-03-13  feat(sharding): implement Reed-Solomon repair engine para... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

class ShardResourceManager {
public: 
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
        
        // Serialization
        nlohmann::json toJson() const;
        static ResourceSnapshot fromJson(const nlohmann::json& j);
    };
    
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
    
    struct QuerySpec {
        std::string query_id;
        size_t estimated_memory_bytes = 0;
        uint32_t estimated_cpu_percent = 0;
        std::chrono::milliseconds estimated_duration{0};
    };
    
    explicit ShardResourceManager(
        const std::string& local_shard_id,
        std::shared_ptr<GossipConfigManager> gossip_manager,
        const Config& config
    );

    explicit ShardResourceManager(
        const std::string& local_shard_id,
        std::shared_ptr<GossipConfigManager> gossip_manager
    );
    
    ~ShardResourceManager();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Local resource management
    ResourceSnapshot getCurrentSnapshot() const;
    bool canAcceptQuery(const QuerySpec& spec) const;
    void updateQueryMetrics(uint32_t active, uint32_t pending, float avg_latency_ms);
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
    bool acquireRepairIOToken(double io_ops = 1.0);

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
    void broadcastResourceUpdate();
    void receiveResourceUpdate(const std::string& shard_id, 
                                const ResourceSnapshot& snapshot);
    
    // Peer awareness (YARN-inspired)
    std::map<std::string, ResourceSnapshot> getPeerResources() const;
    std::optional<ResourceSnapshot> getPeerResource(const std::string& shard_id) const;
    std::vector<std::string> getHealthyPeers() const;
    std::vector<std::string> getOverloadedPeers(float threshold = 0.85f) const;
    
    // Health scoring
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
    void monitoringLoop();
    void collectSystemMetrics();
    void cleanupStaleSnapshots();
    
    // Platform-specific helpers
    float getCpuUsage() const;
    std::pair<uint64_t, uint64_t> getRamUsage() const;
    std::pair<uint64_t, uint64_t> getVramUsage() const;
    std::pair<uint64_t, uint64_t> getDiskUsage() const;
    std::pair<uint64_t, uint64_t> getNetworkUsage() const;
    
    // Internal helper for health score calculation (no lock acquisition)
    float calculateHealthScoreInternal(const ResourceSnapshot& snapshot) const;
};

} // namespace themis::sharding
