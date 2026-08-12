/**
 * @file disk_space_monitor.h
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
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

namespace themis {
namespace storage {

/**
 * @brief Disk Space Monitor
 * 
 * Monitors disk space usage and implements safe-fail mechanisms:
 * - Pre-flight disk space checks before writes
 * - Write prevention when disk space critical
 * - Administrator alerting for low disk space
 * - Automatic garbage collection triggering
 * - Graceful degradation when disk full
 * 
 * Best Practices:
 * - Proactive monitoring: Check space before operations
 * - Fail-safe thresholds: Stop writes before completely full
 * - Alert administrators: Early warning system
 * - Graceful degradation: Read-only mode when critical
 * - Space reclamation: Trigger cleanup automatically
 */
class DiskSpaceMonitor {
public:
    enum class SpaceLevel {
        NORMAL,      // > 20% free space
        WARNING,     // 10-20% free space
        CRITICAL,    // 5-10% free space
        EMERGENCY    // < 5% free space - stop writes
    };
    
    struct Config {
        // Thresholds (as percentage of total space)
        float warning_threshold = 0.20f;    // 20% free
        float critical_threshold = 0.10f;   // 10% free
        float emergency_threshold = 0.05f;  // 5% free - stop writes
        
        // Reserve space for critical operations
        size_t reserved_bytes = 1024 * 1024 * 1024;  // 1 GB reserved
        
        // Monitoring settings
        std::chrono::seconds check_interval{60};  // Check every minute
        bool enable_auto_monitoring = true;
        
        // Alert settings
        bool enable_alerts = true;
        size_t alert_cooldown_minutes = 15;  // Don't spam alerts
        
        // Action settings
        bool enable_auto_gc = true;          // Trigger garbage collection
        bool enable_write_blocking = true;   // Block writes when critical
        bool enable_read_only_mode = false;  // Switch to read-only when emergency
    };
    
    struct SpaceInfo {
        std::string path;
        size_t total_bytes = 0;
        size_t used_bytes = 0;
        size_t free_bytes = 0;
        size_t available_bytes = 0;  // Free - reserved
        float usage_percent = 0.0f;
        float free_percent = 0.0f;
        SpaceLevel level = SpaceLevel::NORMAL;
        bool writes_blocked = false;
        bool read_only = false;
        uint64_t rocksdb_size_bytes = 0;  // RocksDB on-disk SST files size (set via setRocksDBSize)
    };
    
    struct MonitorStats {
        size_t total_checks = 0;
        size_t warning_triggers = 0;
        size_t critical_triggers = 0;
        size_t emergency_triggers = 0;
        size_t writes_blocked = 0;
        size_t alerts_sent = 0;
        size_t gc_triggers = 0;
        std::chrono::system_clock::time_point last_check;
        std::chrono::system_clock::time_point last_alert;
    };
    
    using AlertCallback = std::function<void(const SpaceInfo&, const std::string& message)>;
    using GCCallback = std::function<void()>;
    
    DiskSpaceMonitor(const std::string& path);
    explicit DiskSpaceMonitor(const std::string& path, const Config& config);
    ~DiskSpaceMonitor();
    
    /**
     * @brief Start automatic monitoring
     */
    void startMonitoring();
    
    /**
     * @brief Stop automatic monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check disk space immediately
     * 
     * @return Current space information
     */
    SpaceInfo checkSpace();
    
    /**
     * @brief Check if write operation can proceed
     * 
     * Performs pre-flight check before allowing write operations.
     * 
     * @param bytes_to_write Size of planned write operation
        * @note Zero-byte writes are always allowed because they do not consume
        *       disk capacity.
     * @return true if write should proceed, false if blocked
     */
    bool canWrite(size_t bytes_to_write = 0);
    
    /**
     * @brief Check if database is in read-only mode
     */
    bool isReadOnly() const;
    
    /**
     * @brief Get current space level
     */
    SpaceLevel getSpaceLevel() const;
    
    /**
     * @brief Get current space info
     */
    SpaceInfo getSpaceInfo() const;
    
    /**
     * @brief Get monitoring statistics
     */
    MonitorStats getStats() const;
    
    /**
     * @brief Register callback for space alerts
     * 
     * Callback is invoked when space level changes or threshold crossed
     */
    void setAlertCallback(AlertCallback callback);
    
    /**
     * @brief Register callback for garbage collection
     * 
     * Callback is invoked when automatic GC should run
     */
    void setGCCallback(GCCallback callback);
    
    /**
     * @brief Force a garbage collection
     * 
     * Manually trigger the registered GC callback
     */
    void triggerGC();
    
    /**
     * @brief Override read-only mode
     * 
     * Use with caution - allows writes even in critical state
     */
    void setReadOnlyOverride(bool read_only);
    
    /**
     * @brief Update the tracked RocksDB on-disk size
     * 
     * Called by storage components after computing the SST files size via
     * RocksDBWrapper::getApproximateSize(). The value is propagated into
     * SpaceInfo::rocksdb_size_bytes and is returned by getSpaceInfo().
     *
     * @param size_bytes Total RocksDB SST on-disk size in bytes
     */
    void setRocksDBSize(uint64_t size_bytes);
    
    /**
     * @brief Get recommended action based on space level
     */
    std::string getRecommendedAction() const;
    
    /**
     * @brief Calculate time until disk full
     * 
     * Estimates time until disk full based on recent usage patterns
     * 
     * @return Estimated seconds until full, or 0 if cannot estimate
     */
    std::chrono::seconds estimateTimeUntilFull() const;
    
private:
    std::string path_;
    Config config_;
    
    mutable std::mutex mutex_;
    std::atomic<bool> monitoring_active_{false};
    std::atomic<bool> should_stop_{false};
    std::thread monitor_thread_;
    
    // Current state
    SpaceInfo current_info_;
    std::atomic<SpaceLevel> current_level_{SpaceLevel::NORMAL};
    std::atomic<bool> writes_blocked_{false};
    std::atomic<bool> read_only_override_{false};
    
    // Statistics
    MonitorStats stats_;
    
    // Callbacks
    AlertCallback alert_callback_;
    GCCallback gc_callback_;
    
    // Usage tracking for trend analysis
    struct UsageSnapshot {
        std::chrono::system_clock::time_point timestamp;
        size_t used_bytes;
    };
    std::vector<UsageSnapshot> usage_history_;
    static constexpr size_t max_history_size_ = 100;
    
    // Helper methods
    void monitoringLoop();
    SpaceInfo queryDiskSpace();
    void updateSpaceLevel(const SpaceInfo& info);
    void handleSpaceLevelChange(SpaceLevel old_level, SpaceLevel new_level);
    void sendAlert(const SpaceInfo& info, const std::string& message);
    bool shouldSendAlert() const;
    void recordUsage(const SpaceInfo& info);
    float calculateUsageTrend() const;  // Bytes per second
};

/**
 * @brief Disk Space Guard
 * 
 * RAII-style guard that ensures enough disk space exists before operation
 */
class DiskSpaceGuard {
public:
    DiskSpaceGuard(
        DiskSpaceMonitor& monitor,
        size_t required_bytes,
        const std::string& operation_name
    );
    
    ~DiskSpaceGuard();
    
    /**
     * @brief Check if guard acquired space successfully
     */
    bool isValid() const;
    
    /**
     * @brief Get error message if guard is invalid
     */
    std::string getError() const;
    
private:
    DiskSpaceMonitor& monitor_;
    size_t required_bytes_;
    std::string operation_name_;
    bool valid_;
    std::string error_;
};

/**
 * @brief Utility functions for disk space operations
 */
namespace disk_utils {

/**
 * @brief Get disk space info for a path
 * 
 * Platform-independent disk space query
 * 
 * @param path Path to query (file or directory)
 * @param[out] total_bytes Total disk space
 * @param[out] free_bytes Free disk space
 * @param[out] available_bytes Available space (may differ from free on some systems)
 * @return true if query successful
 */
bool getDiskSpace(
    const std::string& path,
    size_t& total_bytes,
    size_t& free_bytes,
    size_t& available_bytes
);

/**
 * @brief Format bytes as human-readable string
 * 
 * @param bytes Byte count
 * @return Formatted string (e.g., "1.5 GB")
 */
std::string formatBytes(size_t bytes);

/**
 * @brief Check if path exists
 */
bool pathExists(const std::string& path);

/**
 * @brief Get directory of a file path
 */
std::string getDirectory(const std::string& path);

} // namespace disk_utils

} // namespace storage
} // namespace themis
