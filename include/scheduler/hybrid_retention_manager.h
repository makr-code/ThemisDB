/**
 * @file hybrid_retention_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include "timeseries/tsstore.h"
#include "timeseries/gorilla.h"
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;

/**
 * @brief Configuration for hybrid retention strategy
 */
struct HybridRetentionConfig {
    // Stage 1: Gorilla Compression (Hot Data)
    struct Stage1Config {
        bool enabled = true;
        std::chrono::hours duration{24 * 7};  // 7 days
        std::chrono::hours check_interval{24};  // Daily
        std::string metric_pattern = "*";  // All metrics
    } stage1;
    
    // Stage 2: Adaptive Retention (Warm Data)
    struct Stage2Config {
        bool enabled = true;
        std::chrono::hours min_age{24 * 7};      // 7 days
        std::chrono::hours max_age{24 * 365};    // 1 year
        std::chrono::hours check_interval{12};   // Every 12 hours
        
        // Variance thresholds (Coefficient of Variation)
        double low_cv_threshold = 5.0;       // CV < 5%
        double medium_cv_threshold = 20.0;   // CV 5-20%
        
        // Target resolutions based on variance
        std::string low_cv_resolution = "1h";
        std::string medium_cv_resolution = "15m";
        std::string high_cv_resolution = "1m";
        
        // Anomaly detection
        bool detect_anomalies = true;
        double anomaly_sigma_threshold = 3.0;  // 3-sigma rule
    } stage2;
    
    // Stage 3: Time-Based Retention (Cold Data)
    struct Stage3Config {
        bool enabled = true;
        std::chrono::hours min_age{24 * 365};  // 1 year
        std::chrono::hours check_interval{24};  // Daily
        std::string target_resolution = "1d";   // Daily aggregates
    } stage3;
    
    // Global settings
    bool auto_cleanup = true;  // Delete original data after aggregation
    bool verify_aggregates = true;  // Verify aggregates exist before deletion
    std::string source_table = "timeseries";
    std::string adaptive_table = "timeseries_adaptive";
    std::string longterm_table = "timeseries_longterm";
};

/**
 * @brief Statistics for hybrid retention system
 */
struct HybridRetentionStats {
    // Stage 1 stats
    struct {
        size_t compressions_total = 0;
        size_t compressions_failed = 0;
        double avg_compression_ratio = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage1;
    
    // Stage 2 stats
    struct {
        size_t aggregations_total = 0;
        size_t aggregations_failed = 0;
        size_t anomalies_preserved = 0;
        double avg_storage_reduction = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage2;
    
    // Stage 3 stats
    struct {
        size_t aggregations_total = 0;
        size_t aggregations_failed = 0;
        double avg_storage_reduction = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage3;
    
    // Overall stats
    size_t total_storage_bytes_saved = 0;
    double overall_storage_reduction_percent = 0.0;
};

/**
 * @brief Hybrid Retention Manager
 * 
 * Manages a three-stage retention strategy that balances storage efficiency
 * with analytical capability and anomaly preservation.
 * 
 * Usage:
 * @code
 *   HybridRetentionConfig config;
 *   config.stage2.low_cv_threshold = 3.0;  // More aggressive
 *   
 *   HybridRetentionManager manager(query_engine, tsstore, scheduler, config);
 *   manager.start();
 *   
 *   // ... runs automatically ...
 *   
 *   auto stats = manager.getStats();
 *   std::cout << "Storage saved: " << stats.total_storage_bytes_saved / 1024 / 1024 
 *             << " MB" << std::endl;
 *   
 *   manager.stop();
 * @endcode
 */
class HybridRetentionManager {
public:
    /**
     * @brief Construct hybrid retention manager
     * @param query_engine Query engine for AQL execution
     * @param tsstore Time-series store for data access
     * @param scheduler Task scheduler for periodic execution
     * @param config Retention configuration
     */
    HybridRetentionManager(
        QueryEngine* query_engine,
        TSStore* tsstore,
        TaskScheduler* scheduler,
        const HybridRetentionConfig& config = HybridRetentionConfig{}
    );
    
    ~HybridRetentionManager() noexcept;
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void updateConfig(const HybridRetentionConfig& config);
    HybridRetentionConfig getConfig() const;
    
    // Manual execution
    void executeStage1();  // Run Gorilla compression now
    void executeStage2();  // Run adaptive retention now
    void executeStage3();  // Run time-based retention now
    void executeAll();     // Run all stages now
    
    // Statistics
    HybridRetentionStats getStats() const;
    void resetStats();
    
    // Monitoring
    nlohmann::json getStatusReport() const;
    
private:
    QueryEngine* query_engine_;
    TSStore* tsstore_;
    TaskScheduler* scheduler_;
    HybridRetentionConfig config_;
    
    bool running_ = false;
    mutable std::shared_mutex mutex_;
    
    // Task IDs for cleanup
    std::string stage1_task_id_;
    std::string stage2_task_id_;
    std::string stage3_task_id_;
    std::string cleanup_task_id_;
    
    // Statistics
    HybridRetentionStats stats_;
    
    // Stage implementations
    void setupStage1Tasks();
    void setupStage2Tasks();
    void setupStage3Tasks();
    void setupCleanupTasks();
    
    // Helper methods
    nlohmann::json compressWithGorilla(const nlohmann::json& params);
    nlohmann::json applyAdaptiveRetention(const nlohmann::json& params);
    nlohmann::json applyTimeBasedRetention(const nlohmann::json& params);
    nlohmann::json cleanupOriginalData(const nlohmann::json& params);
    
    void updateStats(int stage, bool success, const nlohmann::json& result);
};

} // namespace themis
