/**
 * @file index_maintenance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "utils/expected.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;
class IndexManager;
class VectorIndexManager;

/**
 * @brief Index fragmentation levels
 */
enum class FragmentationLevel {
    LOW,      // 0-10%: No maintenance needed, optimal performance
    MEDIUM,   // 10-30%: Consider reorganization, acceptable with minor degradation
    HIGH      // >30%: Rebuild recommended, significant degradation possible
};

/**
 * @brief Maintenance operation types
 */
enum class MaintenanceOperation {
    INDEX_REBUILD,                // Full reconstruction for high fragmentation
    INDEX_REORGANIZATION,         // In-place defragmentation
    STATISTICS_UPDATE,            // Refresh cardinality information
    ORPHAN_ENTRY_CLEANUP,         // Remove dead entries
    CONSISTENCY_CHECK,            // Validation and repair
    VECTOR_INCREMENTAL_REINDEX    // Incremental HNSW re-index without full rebuild
};

/**
 * @brief Maintenance scheduling types
 */
enum class MaintenanceSchedule {
    DISABLED,       // No automatic maintenance
    TIME_BASED,     // Run at specific intervals
    EVENT_BASED,    // Run when thresholds are exceeded
    HYBRID          // Combination of time and event-based
};

/**
 * @brief Index fragmentation metrics
 */
struct FragmentationMetrics {
    std::string index_name;
    double fragmentation_percentage = 0.0;
    FragmentationLevel level = FragmentationLevel::LOW;
    uint64_t total_entries = 0;
    uint64_t orphan_entries = 0;
    uint64_t last_updated_ms = 0;
    uint64_t statistics_staleness_ms = 0;
    
    // Additional metrics
    uint64_t file_count = 0;
    uint64_t size_bytes = 0;
    uint64_t wasted_space_bytes = 0;
};

/**
 * @brief Maintenance job status
 */
struct MaintenanceJobStatus {
    std::string job_id;
    std::string index_name;
    MaintenanceOperation operation;
    bool is_running = false;
    bool is_completed = false;
    bool is_failed = false;
    double progress_percentage = 0.0;
    std::string error_message;
    std::string result_summary;  ///< Human-readable summary of the completed operation (non-error)
    uint64_t start_time_ms = 0;
    uint64_t end_time_ms = 0;
    uint64_t duration_ms = 0;
    
    // Before/After metrics
    FragmentationMetrics before_metrics;
    FragmentationMetrics after_metrics;
};

/**
 * @brief Maintenance policy configuration
 */
struct MaintenancePolicy {
    // Fragmentation thresholds
    double reorganize_threshold = 10.0;  // 10%: trigger reorganization
    double rebuild_threshold = 30.0;     // 30%: trigger rebuild
    
    // Statistics staleness thresholds
    uint64_t statistics_update_interval_ms = 3600000;  // 1 hour
    
    // Scheduling
    MaintenanceSchedule schedule_type = MaintenanceSchedule::HYBRID;
    uint64_t time_based_interval_ms = 86400000;  // 24 hours
    
    // Resource allocation
    int max_concurrent_jobs = 2;
    int priority_level = 5;  // 1-10, higher = more important
    
    // Maintenance windows (hours in 24h format)
    bool enable_maintenance_window = false;
    int window_start_hour = 2;   // 2 AM
    int window_end_hour = 6;     // 6 AM
    
    // Resource throttling
    bool enable_throttling = true;
    double max_cpu_usage_percent = 50.0;
    double max_memory_usage_mb = 1024.0;
    
    // Online maintenance (minimal locking)
    bool online_maintenance = true;
};

/**
 * @brief Index Maintenance Manager
 * 
 * Provides automated index maintenance operations including:
 * - Index fragmentation monitoring
 * - Automatic rebuild and reorganization
 * - Statistics updates
 * - Orphan entry cleanup
 * - Consistency checking
 * - Background scheduling
 * 
 * Thread-Safe: All operations are thread-safe for concurrent access
 */
class IndexMaintenanceManager {
public:
    /**
     * @brief Constructor
     * @param db_wrapper RocksDB wrapper for storage operations
     * @param index_manager Optional index manager for index-specific operations
     */
    explicit IndexMaintenanceManager(
        std::shared_ptr<RocksDBWrapper> db_wrapper,
        std::shared_ptr<IndexManager> index_manager = nullptr
    );
    
    /**
     * @brief Destructor - stops background maintenance thread
     */
    ~IndexMaintenanceManager();
    
    // Disable copy and move
    IndexMaintenanceManager(const IndexMaintenanceManager&) = delete;
    IndexMaintenanceManager& operator=(const IndexMaintenanceManager&) = delete;
    IndexMaintenanceManager(IndexMaintenanceManager&&) = delete;
    IndexMaintenanceManager& operator=(IndexMaintenanceManager&&) = delete;
    
    /**
     * @brief Start background maintenance thread
     * @return Result indicating success or error
     */
    Result<void> start();
    
    /**
     * @brief Stop background maintenance thread
     * @return Result indicating success or error
     */
    Result<void> stop();
    
    /**
     * @brief Check if maintenance is running
     * @return true if background thread is active
     */
    bool isRunning() const;
    
    /**
     * @brief Set maintenance policy
     * @param policy New maintenance policy configuration
     * @return Result indicating success or error
     */
    Result<void> setPolicy(const MaintenancePolicy& policy);
    
    /**
     * @brief Get current maintenance policy
     * @return Current policy configuration
     */
    MaintenancePolicy getPolicy() const;
    
    /**
     * @brief Monitor fragmentation for a specific index
     * @param index_name Name of the index to monitor
     * @return Result with fragmentation metrics or error
     */
    Result<FragmentationMetrics> monitorFragmentation(const std::string& index_name);
    
    /**
     * @brief Rebuild an index (full reconstruction)
     * @param index_name Name of the index to rebuild
     * @param async Run asynchronously in background
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> rebuildIndex(const std::string& index_name, bool async = true);
    
    /**
     * @brief Reorganize an index (in-place defragmentation)
     * @param index_name Name of the index to reorganize
     * @param async Run asynchronously in background
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> reorganizeIndex(const std::string& index_name, bool async = true);
    
    /**
     * @brief Update index statistics
     * @param index_name Name of the index to update statistics
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> updateStatistics(const std::string& index_name);
    
    /**
     * @brief Cleanup orphan entries
     * @param index_name Name of the index to cleanup
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> cleanupOrphanEntries(const std::string& index_name);
    
    /**
     * @brief Perform consistency check
     * @param index_name Name of the index to check
     * @param repair Attempt to repair issues if found
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> checkConsistency(const std::string& index_name, bool repair = false);
    
    /**
     * @brief Get status of a maintenance job
     * @param job_id Job identifier
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> getJobStatus(const std::string& job_id);
    
    /**
     * @brief List all active maintenance jobs
     * @return Vector of active job statuses
     */
    std::vector<MaintenanceJobStatus> listActiveJobs() const;
    
    /**
     * @brief Cancel a running maintenance job
     * @param job_id Job identifier
     * @return Result indicating success or error
     */
    Result<void> cancelJob(const std::string& job_id);
    
    /**
     * @brief Get fragmentation metrics for all indices
     * @return Map of index names to fragmentation metrics
     */
    std::map<std::string, FragmentationMetrics> getAllFragmentationMetrics();
    
    /**
     * @brief Trigger immediate maintenance check
     * Checks all indices and performs maintenance if needed
     * @return Result indicating success or error
     */
    Result<void> triggerMaintenanceCheck();

    // ===== Vector Index Integration =====

    /**
     * @brief Set the VectorIndexManager for HNSW maintenance operations
     * @param vector_index Shared pointer to the vector index manager
     */
    void setVectorIndexManager(std::shared_ptr<VectorIndexManager> vector_index);

    /**
     * @brief Run incremental HNSW re-index (sync in-memory index with storage)
     * 
     * Delegates to VectorIndexManager::incrementalReindex().  If no
     * VectorIndexManager has been set via setVectorIndexManager(), returns an
     * error without performing any operation.
     *
     * @param rebuild_threshold Deleted-label ratio that triggers a full rebuild (0–1).
     * @param vector_field      Name of the vector field in stored entities.
     * @return Result with job status or error
     */
    Result<MaintenanceJobStatus> vectorIncrementalReindex(
        float rebuild_threshold = 0.20f,
        std::string_view vector_field = "embedding");

private:
    // Background maintenance thread
    void maintenanceThreadFunc();
    
    // Helper methods
    Result<FragmentationMetrics> calculateFragmentation(const std::string& index_name);
    Result<void> performRebuild(const std::string& index_name, MaintenanceJobStatus& status);
    Result<void> performReorganize(const std::string& index_name, MaintenanceJobStatus& status);
    Result<void> performStatisticsUpdate(const std::string& index_name, MaintenanceJobStatus& status);
    Result<void> performOrphanCleanup(const std::string& index_name, MaintenanceJobStatus& status);
    Result<void> performConsistencyCheck(const std::string& index_name, bool repair, MaintenanceJobStatus& status);
    
    bool shouldRunMaintenance() const;
    bool isInMaintenanceWindow() const;
    std::string generateJobId();
    
    FragmentationLevel classifyFragmentation(double percentage) const;
    
    // Members
    std::shared_ptr<RocksDBWrapper> db_wrapper_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<VectorIndexManager> vector_index_manager_; ///< Optional; for HNSW incremental reindex
    
    MaintenancePolicy policy_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread maintenance_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Job tracking
    std::map<std::string, MaintenanceJobStatus> active_jobs_;
    std::map<std::string, MaintenanceJobStatus> completed_jobs_;
    std::atomic<uint64_t> job_counter_{0};
    
    // Metrics cache
    std::map<std::string, FragmentationMetrics> metrics_cache_;
    uint64_t last_metrics_update_ms_{0};
};

} // namespace themis
