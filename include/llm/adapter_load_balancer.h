/**
 * @file adapter_load_balancer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class AdapterLoadBalancer {
public:
    struct AdapterPlacement {
        std::string adapter_id;
        int gpu_device_id = 0;
        size_t vram_bytes = 0;
        int priority = 0;  // Higher = more important
        bool is_pinned = false;  // Cannot be evicted
        int64_t last_access_time_ms = 0;
        size_t access_count = 0;
    };
    
    struct LoadBalanceStats {
        int num_adapters = 0;
        int num_gpus = 0;
        float average_gpu_load = 0.0f;
        float max_gpu_load = 0.0f;
        float min_gpu_load = 0.0f;
        int num_migrations = 0;
        int num_evictions = 0;
        int64_t last_balance_time_ms = 0;
    };
    
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
    
    /**
     * @brief Adapter Load Balancer.
     * @param[in] memory_manager Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdapterLoadBalancer(
        std::shared_ptr<GPUMemoryManager> memory_manager,
        const Config& config);
    ~AdapterLoadBalancer();
    
    // Adapter placement
    /**
     * @brief Select GPUFor Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] vram_bytes Input parameter.
     * @param[in] priority Input parameter.
     * @return Return value.
     */
    int selectGPUForAdapter(const std::string& adapter_id, size_t vram_bytes, int priority);
    bool placeAdapter(const std::string& adapter_id, int gpu_device_id, 
                      size_t vram_bytes, int priority, bool pinned = false);
    /**
     * @brief Remove Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool removeAdapter(const std::string& adapter_id);
    
    // Adapter queries
    /**
     * @brief Get Adapter GPU.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    int getAdapterGPU(const std::string& adapter_id) const;
    /**
     * @brief Get GPUAdapters.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @return Return value.
     */
    std::vector<std::string> getGPUAdapters(int gpu_device_id) const;
    /**
     * @brief Get Adapter Placement.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    AdapterPlacement getAdapterPlacement(const std::string& adapter_id) const;
    /**
     * @brief Is Adapter Loaded.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool isAdapterLoaded(const std::string& adapter_id) const;
    
    // Pinning management
    /**
     * @brief Pin Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool pinAdapter(const std::string& adapter_id);
    /**
     * @brief Unpin Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool unpinAdapter(const std::string& adapter_id);
    /**
     * @brief Is Adapter Pinned.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool isAdapterPinned(const std::string& adapter_id) const;
    
    // Load balancing operations
    /**
     * @brief Rebalance.
     * @return True when the operation succeeds.
     */
    bool rebalance();  // Perform load balancing across GPUs
    /**
     * @brief Migrate Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] target_gpu_id Identifier of the target gpu.
     * @return True when the operation succeeds.
     */
    bool migrateAdapter(const std::string& adapter_id, int target_gpu_id);
    /**
     * @brief Evict LRUAdapters.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @param[in] required_bytes Input parameter.
     * @return Return value.
     */
    std::vector<std::string> evictLRUAdapters(int gpu_device_id, size_t required_bytes);
    
    /**
     * @brief Access tracking (for LRU)
     * @param[in] adapter_id Identifier of the adapter.
     */
    void recordAccess(const std::string& adapter_id);
    
    // Statistics
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    LoadBalanceStats getStats() const;
    /**
     * @brief Get GPULoad.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @return Return value.
     */
    float getGPULoad(int gpu_device_id) const;
    
    // Health-aware operations
    /**
     * @brief Mark GPUUnhealthy.
     * @param[in] gpu_device_id Identifier of the gpu device.
     */
    void markGPUUnhealthy(int gpu_device_id);
    /**
     * @brief Mark GPUHealthy.
     * @param[in] gpu_device_id Identifier of the gpu device.
     */
    void markGPUHealthy(int gpu_device_id);
    /**
     * @brief Should Migrate From GPU.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @return True when the operation succeeds.
     */
    bool shouldMigrateFromGPU(int gpu_device_id) const;

    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<DecisionRecordYamlProcessor> processor);

    // Hot-load in-progress tracking
    void beginHotLoad(const std::string& adapter_id,
                      const std::string& fallback_id = "");

    /**
     * @brief End Hot Load.
     * @param[in] adapter_id Identifier of the adapter.
     */
    void endHotLoad(const std::string& adapter_id);

    /**
     * @brief Is Hot Load In Progress.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool isHotLoadInProgress(const std::string& adapter_id) const;

    /**
     * @brief Resolve Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
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
    /**
     * @brief Can Place On GPU.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @param[in] vram_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool canPlaceOnGPU(int gpu_device_id, size_t vram_bytes) const;
    /**
     * @brief Select Adapters For Eviction.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @param[in] required_bytes Input parameter.
     * @return Return value.
     */
    std::vector<std::string> selectAdaptersForEviction(
        int gpu_device_id, size_t required_bytes) const;
    /**
     * @brief Should Rebalance.
     * @return True when the operation succeeds.
     */
    bool shouldRebalance() const;
    /**
     * @brief Calculate GPULoad.
     * @param[in] gpu_device_id Identifier of the gpu device.
     * @return Return value.
     */
    float calculateGPULoad(int gpu_device_id) const;
    /**
     * @brief Find Least Loaded Healthy GPU.
     * @return Return value.
     */
    int findLeastLoadedHealthyGPU() const;
    
    // Migration helpers
    /**
     * @brief Perform Migration.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] source_gpu Input parameter.
     * @param[in] target_gpu Input parameter.
     * @return True when the operation succeeds.
     */
    bool performMigration(const std::string& adapter_id, int source_gpu, int target_gpu);
    /**
     * @brief Perform Eviction.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool performEviction(const std::string& adapter_id);
    
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs() const;

    /**
     * @brief Emit Rebalance Record.
     * @param[in] migrations Input parameter.
     * @param[in] num_gpus Input parameter.
     * @param[in] avg_load Input parameter.
     */
    void emitRebalanceRecord(int migrations, int num_gpus, float avg_load) const;
};

} // namespace llm
} // namespace themis

