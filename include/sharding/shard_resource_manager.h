/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_resource_manager.h                           ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     174                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "sharding/gossip_config_manager.h"
#include <atomic>
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
