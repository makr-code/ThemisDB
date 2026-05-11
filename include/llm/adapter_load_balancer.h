/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_load_balancer.h                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 2873683f74  2026-03-18  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/decision_record_yaml_processor.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>

namespace themis {
namespace llm {

// Forward declarations
class GPUMemoryManager;

/**
 * @brief Adapter Load Balancer for Multi-GPU LoRA Distribution
 * 
 * Manages dynamic placement, migration, and eviction of LoRA adapters
 * across multiple GPUs based on utilization, health, and priorities.
 */
class AdapterLoadBalancer {
public:
    /**
     * @brief Adapter placement information
     */
    struct AdapterPlacement {
        std::string adapter_id;
        int gpu_device_id;
        size_t vram_bytes;
        int priority;  // Higher = more important
        bool is_pinned;  // Cannot be evicted
        int64_t last_access_time_ms;
        size_t access_count;
    };
    
    /**
     * @brief Load balancing statistics
     */
    struct LoadBalanceStats {
        int num_adapters;
        int num_gpus;
        float average_gpu_load;
        float max_gpu_load;
        float min_gpu_load;
        int num_migrations;
        int num_evictions;
        int64_t last_balance_time_ms;
    };
    
    /**
     * @brief Configuration for load balancer
     */
    struct Config {
        bool enable_dynamic_balancing = true;
        float rebalance_threshold = 0.8f;  // Trigger at 80% avg utilization
        int rebalance_interval_ms = 5000;  // Check every 5 seconds
        
        bool enable_jit_eviction = true;
        size_t max_adapters_per_gpu = 10;
        float eviction_threshold = 0.9f;  // Evict when VRAM > 90%
        
        bool enable_migration = true;
        float migration_threshold = 0.3f;  // Migrate if load diff > 30%
        
        bool respect_pinning = true;
    };
    
    explicit AdapterLoadBalancer(
        std::shared_ptr<GPUMemoryManager> memory_manager,
        const Config& config);
    ~AdapterLoadBalancer();
    
    // Adapter placement
    int selectGPUForAdapter(const std::string& adapter_id, size_t vram_bytes, int priority);
    bool placeAdapter(const std::string& adapter_id, int gpu_device_id, 
                      size_t vram_bytes, int priority, bool pinned = false);
    bool removeAdapter(const std::string& adapter_id);
    
    // Adapter queries
    int getAdapterGPU(const std::string& adapter_id) const;
    std::vector<std::string> getGPUAdapters(int gpu_device_id) const;
    AdapterPlacement getAdapterPlacement(const std::string& adapter_id) const;
    bool isAdapterLoaded(const std::string& adapter_id) const;
    
    // Pinning management
    bool pinAdapter(const std::string& adapter_id);
    bool unpinAdapter(const std::string& adapter_id);
    bool isAdapterPinned(const std::string& adapter_id) const;
    
    // Load balancing operations
    bool rebalance();  // Perform load balancing across GPUs
    bool migrateAdapter(const std::string& adapter_id, int target_gpu_id);
    std::vector<std::string> evictLRUAdapters(int gpu_device_id, size_t required_bytes);
    
    // Access tracking (for LRU)
    void recordAccess(const std::string& adapter_id);
    
    // Statistics
    LoadBalanceStats getStats() const;
    float getGPULoad(int gpu_device_id) const;
    
    // Health-aware operations
    void markGPUUnhealthy(int gpu_device_id);
    void markGPUHealthy(int gpu_device_id);
    bool shouldMigrateFromGPU(int gpu_device_id) const;

    /**
     * @brief Inject a `DecisionRecordYamlProcessor` for async YAML traceability.
     *
     * When set, every successful `rebalance()` call that performs at least one
     * adapter migration emits a `LORA_RANK_ADJUSTMENT` decision record written
     * asynchronously to
     * `logs/decisions/YYYY-MM-DD/&lt;ts&gt;_LORA_RANK_ADJUSTMENT_&lt;id&gt;.yaml`.
     *
     * @param processor  Shared processor instance (may be nullptr to disable).
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<DecisionRecordYamlProcessor> processor);

    // Hot-load in-progress tracking
    /// Mark @p adapter_id as currently being hot-loaded.
    /// Requests for this adapter will be routed to @p fallback_id until
    /// endHotLoad() is called.  If @p fallback_id is empty the caller is
    /// responsible for routing (e.g. base model).
    void beginHotLoad(const std::string& adapter_id,
                      const std::string& fallback_id = "");

    /// Mark hot-load for @p adapter_id as finished (or failed).
    void endHotLoad(const std::string& adapter_id);

    /// Returns true while a hot-load is in progress for @p adapter_id.
    bool isHotLoadInProgress(const std::string& adapter_id) const;

    /// Resolve the adapter to serve for @p adapter_id.
    /// If a hot-load is in progress returns the registered fallback_id
    /// (or empty string when no fallback was given); otherwise returns
    /// @p adapter_id unchanged.
    std::string resolveAdapter(const std::string& adapter_id) const;
    
private:
    std::shared_ptr<GPUMemoryManager> memory_manager_;
    Config config_;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, AdapterPlacement> placements_;
    std::unordered_map<int, std::vector<std::string>> gpu_to_adapters_;
    
    // Statistics
    int total_migrations_ = 0;
    int total_evictions_ = 0;
    int64_t last_rebalance_time_ = 0;

    // Hot-load in-progress tracking: adapter_id → fallback_id
    std::unordered_map<std::string, std::string> hot_loading_adapters_;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<DecisionRecordYamlProcessor> dr_processor_;
    
    // Helper methods
    bool canPlaceOnGPU(int gpu_device_id, size_t vram_bytes) const;
    std::vector<std::string> selectAdaptersForEviction(
        int gpu_device_id, size_t required_bytes) const;
    bool shouldRebalance() const;
    float calculateGPULoad(int gpu_device_id) const;
    int findLeastLoadedHealthyGPU() const;
    
    // Migration helpers
    bool performMigration(const std::string& adapter_id, int source_gpu, int target_gpu);
    bool performEviction(const std::string& adapter_id);
    
    int64_t getCurrentTimeMs() const;

    /// Emit a LORA_RANK_ADJUSTMENT DecisionRecord (non-blocking, caller holds mutex_).
    void emitRebalanceRecord(int migrations, int num_gpus, float avg_load) const;
};

} // namespace llm
} // namespace themis
