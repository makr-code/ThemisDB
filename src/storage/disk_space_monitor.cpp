/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            disk_space_monitor.cpp                             ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:36:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     565                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9f9d86ceb0  2026-03-15  feat(storage): implement proper size calculation in Rocks... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/disk_space_monitor.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

// Platform-specific includes for disk space
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace themis {
namespace storage {

// ============================================================================
// DiskSpaceMonitor Implementation
// ============================================================================

DiskSpaceMonitor::DiskSpaceMonitor(const std::string& path)
    : path_(path)
    , config_{} {
    spdlog::info("Disk Space Monitor initialized for: {}", path_);
    spdlog::info("  Warning threshold: {:.1f}%", config_.warning_threshold * 100);
    spdlog::info("  Critical threshold: {:.1f}%", config_.critical_threshold * 100);
    spdlog::info("  Emergency threshold: {:.1f}%", config_.emergency_threshold * 100);
    spdlog::info("  Reserved space: {} GB", config_.reserved_bytes / (1024.0 * 1024 * 1024));
    
    // Initial check
    checkSpace();
    
    if (config_.enable_auto_monitoring) {
        startMonitoring();
    }
}

DiskSpaceMonitor::DiskSpaceMonitor(const std::string& path, const Config& config)
    : path_(path)
    , config_(config) {
    spdlog::info("Disk Space Monitor initialized for: {}", path_);
    spdlog::info("  Warning threshold: {:.1f}%", config_.warning_threshold * 100);
    spdlog::info("  Critical threshold: {:.1f}%", config_.critical_threshold * 100);
    spdlog::info("  Emergency threshold: {:.1f}%", config_.emergency_threshold * 100);
    spdlog::info("  Reserved space: {} GB", config_.reserved_bytes / (1024.0 * 1024 * 1024));
    
    // Initial check
    checkSpace();
    
    if (config_.enable_auto_monitoring) {
        startMonitoring();
    }
}

DiskSpaceMonitor::~DiskSpaceMonitor() {
    stopMonitoring();
}

void DiskSpaceMonitor::startMonitoring() {
    if (monitoring_active_.load()) {
        return;
    }
    
    monitoring_active_ = true;
    should_stop_ = false;
    
    monitor_thread_ = std::thread(&DiskSpaceMonitor::monitoringLoop, this);
    
    spdlog::info("Disk space monitoring started (interval: {}s)", 
                 config_.check_interval.count());
}

void DiskSpaceMonitor::stopMonitoring() {
    if (!monitoring_active_.load()) {
        return;
    }
    
    should_stop_ = true;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    monitoring_active_ = false;
    
    spdlog::info("Disk space monitoring stopped");
}

DiskSpaceMonitor::SpaceInfo DiskSpaceMonitor::checkSpace() {
    auto info = queryDiskSpace();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    SpaceLevel old_level = current_level_.load();
    // Preserve the RocksDB size that was set via setRocksDBSize() — it is
    // managed independently from OS disk-space queries.
    info.rocksdb_size_bytes = current_info_.rocksdb_size_bytes;
    current_info_ = info;
    
    updateSpaceLevel(info);
    recordUsage(info);
    
    stats_.total_checks++;
    stats_.last_check = std::chrono::system_clock::now();
    
    SpaceLevel new_level = current_level_.load();
    if (old_level != new_level) {
        handleSpaceLevelChange(old_level, new_level);
    }
    
    return info;
}

bool DiskSpaceMonitor::canWrite(size_t bytes_to_write) {
    // Check if writes are globally blocked
    if (writes_blocked_.load()) {
        spdlog::warn("Write blocked: disk space critical");
        stats_.writes_blocked++;
        return false;
    }
    
    // Check if in read-only mode
    if (isReadOnly()) {
        spdlog::warn("Write blocked: system in read-only mode");
        return false;
    }
    
    // Pre-flight check
    auto info = getSpaceInfo();
    
    size_t required_space = bytes_to_write + config_.reserved_bytes;
    if (info.available_bytes < required_space) {
        spdlog::error("Insufficient disk space: need {} MB, have {} MB available",
                     required_space / (1024.0 * 1024),
                     info.available_bytes / (1024.0 * 1024));
        stats_.writes_blocked++;
        return false;
    }
    
    // Check if write would push us into emergency level
    size_t would_be_free = info.free_bytes - bytes_to_write;
    float would_be_percent = static_cast<float>(would_be_free) / 
                             static_cast<float>(info.total_bytes);
    
    if (would_be_percent < config_.emergency_threshold) {
        spdlog::warn("Write would reduce free space to {:.1f}% - blocked",
                    would_be_percent * 100);
        stats_.writes_blocked++;
        return false;
    }
    
    return true;
}

bool DiskSpaceMonitor::isReadOnly() const {
    if (read_only_override_.load()) {
        return true;
    }
    
    return current_info_.read_only;
}

DiskSpaceMonitor::SpaceLevel DiskSpaceMonitor::getSpaceLevel() const {
    return current_level_.load();
}

DiskSpaceMonitor::SpaceInfo DiskSpaceMonitor::getSpaceInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_info_;
}

DiskSpaceMonitor::MonitorStats DiskSpaceMonitor::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void DiskSpaceMonitor::setAlertCallback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_callback_ = std::move(callback);
}

void DiskSpaceMonitor::setGCCallback(GCCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    gc_callback_ = std::move(callback);
}

void DiskSpaceMonitor::triggerGC() {
    GCCallback gc_cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        gc_cb = gc_callback_;
        stats_.gc_triggers++;
    }
    
    if (gc_cb) {
        spdlog::info("Triggering garbage collection");
        try {
            gc_cb();
        } catch (const std::exception& e) {
            spdlog::error("GC callback threw exception: {}", e.what());
        }
    }
}

void DiskSpaceMonitor::setReadOnlyOverride(bool read_only) {
    read_only_override_ = read_only;
    spdlog::warn("Read-only override set to: {}", read_only);
}

void DiskSpaceMonitor::setRocksDBSize(uint64_t size_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_info_.rocksdb_size_bytes = size_bytes;
}

std::string DiskSpaceMonitor::getRecommendedAction() const {
    SpaceLevel level = current_level_.load();
    
    switch (level) {
        case SpaceLevel::NORMAL:
            return "No action needed";
        
        case SpaceLevel::WARNING:
            return "Monitor disk usage. Consider running cleanup or archiving old data.";
        
        case SpaceLevel::CRITICAL:
            return "URGENT: Free disk space immediately. Remove unnecessary files, "
                   "archive old data, or add more storage.";
        
        case SpaceLevel::EMERGENCY:
            return "CRITICAL: Disk almost full! Database may be read-only. "
                   "Free space immediately or risk data loss.";
        
        default:
            return "Unknown";
    }
}

std::chrono::seconds DiskSpaceMonitor::estimateTimeUntilFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (usage_history_.size() < 2) {
        return std::chrono::seconds(0);  // Not enough data
    }
    
    // Calculate usage rate (bytes per second)
    auto oldest = usage_history_.front();
    auto newest = usage_history_.back();
    
    auto time_diff = newest.timestamp - oldest.timestamp;
    auto time_diff_seconds = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
    
    if (time_diff_seconds.count() == 0) {
        return std::chrono::seconds(0);
    }
    
    int64_t bytes_diff = static_cast<int64_t>(newest.used_bytes) - 
                         static_cast<int64_t>(oldest.used_bytes);
    
    if (bytes_diff <= 0) {
        // Usage is decreasing or stable
        return std::chrono::seconds(0);
    }
    
    float bytes_per_second = static_cast<float>(bytes_diff) / time_diff_seconds.count();
    
    // Calculate time until current free space is exhausted
    size_t bytes_until_full = current_info_.free_bytes;
    if (bytes_until_full > config_.reserved_bytes) {
        bytes_until_full -= config_.reserved_bytes;
    }
    
    float seconds_until_full = bytes_until_full / bytes_per_second;
    
    return std::chrono::seconds(static_cast<long long>(seconds_until_full));
}

void DiskSpaceMonitor::monitoringLoop() {
    while (!should_stop_.load()) {
        try {
            checkSpace();
        } catch (const std::exception& e) {
            spdlog::error("Disk space check failed: {}", e.what());
        }
        
        // Sleep in small intervals to allow quick shutdown
        auto remaining = config_.check_interval;
        while (remaining > std::chrono::seconds(0) && !should_stop_.load()) {
            auto sleep_time = std::min(remaining, std::chrono::seconds(1));
            std::this_thread::sleep_for(sleep_time);
            remaining -= sleep_time;
        }
    }
}

DiskSpaceMonitor::SpaceInfo DiskSpaceMonitor::queryDiskSpace() {
    SpaceInfo info;
    info.path = path_;
    
    size_t total, free, available;
    if (!disk_utils::getDiskSpace(path_, total, free, available)) {
        spdlog::error("Failed to query disk space for: {}", path_);
        return info;
    }
    
    info.total_bytes = total;
    info.free_bytes = free;
    info.used_bytes = total - free;
    
    // Calculate available (free - reserved)
    if (free > config_.reserved_bytes) {
        info.available_bytes = free - config_.reserved_bytes;
    } else {
        info.available_bytes = 0;
    }
    
    info.usage_percent = static_cast<float>(info.used_bytes) / 
                        static_cast<float>(info.total_bytes);
    info.free_percent = static_cast<float>(info.free_bytes) / 
                       static_cast<float>(info.total_bytes);
    
    // Determine space level
    if (info.free_percent < config_.emergency_threshold) {
        info.level = SpaceLevel::EMERGENCY;
        info.writes_blocked = config_.enable_write_blocking;
        info.read_only = config_.enable_read_only_mode;
    } else if (info.free_percent < config_.critical_threshold) {
        info.level = SpaceLevel::CRITICAL;
        info.writes_blocked = config_.enable_write_blocking;
    } else if (info.free_percent < config_.warning_threshold) {
        info.level = SpaceLevel::WARNING;
    } else {
        info.level = SpaceLevel::NORMAL;
    }
    
    return info;
}

void DiskSpaceMonitor::updateSpaceLevel(const SpaceInfo& info) {
    current_level_ = info.level;
    writes_blocked_ = info.writes_blocked;
}

void DiskSpaceMonitor::handleSpaceLevelChange(SpaceLevel old_level, SpaceLevel new_level) {
    spdlog::warn("Disk space level changed from {} to {}", 
                static_cast<int>(old_level), static_cast<int>(new_level));
    
    // Update statistics
    switch (new_level) {
        case SpaceLevel::WARNING:
            stats_.warning_triggers++;
            break;
        case SpaceLevel::CRITICAL:
            stats_.critical_triggers++;
            break;
        case SpaceLevel::EMERGENCY:
            stats_.emergency_triggers++;
            break;
        default:
            break;
    }
    
    // Send alert
    if (config_.enable_alerts && shouldSendAlert()) {
        auto info = current_info_;
        std::ostringstream msg;
        msg << "Disk space " << static_cast<int>(new_level) << ": "
            << disk_utils::formatBytes(info.free_bytes) << " free ("
            << std::fixed << std::setprecision(1) << (info.free_percent * 100) << "%)";
        
        sendAlert(info, msg.str());
    }
    
    // Trigger garbage collection if enabled
    if (config_.enable_auto_gc && 
        (new_level == SpaceLevel::CRITICAL || new_level == SpaceLevel::EMERGENCY)) {
        triggerGC();
    }
}

void DiskSpaceMonitor::sendAlert(const SpaceInfo& info, const std::string& message) {
    AlertCallback alert_cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        alert_cb = alert_callback_;
        stats_.alerts_sent++;
        stats_.last_alert = std::chrono::system_clock::now();
    }
    
    spdlog::warn("DISK SPACE ALERT: {}", message);
    
    if (alert_cb) {
        try {
            alert_cb(info, message);
        } catch (const std::exception& e) {
            spdlog::error("Alert callback threw exception: {}", e.what());
        }
    }
}

bool DiskSpaceMonitor::shouldSendAlert() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - stats_.last_alert;
    auto elapsed_minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
    
    return elapsed_minutes.count() >= config_.alert_cooldown_minutes;
}

void DiskSpaceMonitor::recordUsage(const SpaceInfo& info) {
    UsageSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.used_bytes = info.used_bytes;
    
    usage_history_.push_back(snapshot);
    
    // Keep only recent history
    if (usage_history_.size() > max_history_size_) {
        usage_history_.erase(usage_history_.begin());
    }
}

// ============================================================================
// DiskSpaceGuard Implementation
// ============================================================================

DiskSpaceGuard::DiskSpaceGuard(
    DiskSpaceMonitor& monitor,
    size_t required_bytes,
    const std::string& operation_name
)
    : monitor_(monitor)
    , required_bytes_(required_bytes)
    , operation_name_(operation_name)
    , valid_(false) {
    
    valid_ = monitor_.canWrite(required_bytes_);
    
    if (!valid_) {
        error_ = "Insufficient disk space for operation '" + operation_name_ + "'";
        spdlog::error("{}", error_);
    }
}

DiskSpaceGuard::~DiskSpaceGuard() {
    // Nothing to clean up
}

bool DiskSpaceGuard::isValid() const {
    return valid_;
}

std::string DiskSpaceGuard::getError() const {
    return error_;
}

// ============================================================================
// Disk Utils Implementation
// ============================================================================

namespace disk_utils {

bool getDiskSpace(
    const std::string& path,
    size_t& total_bytes,
    size_t& free_bytes,
    size_t& available_bytes
) {
#ifdef _WIN32
    // Windows implementation
    ULARGE_INTEGER free_bytes_available;
    ULARGE_INTEGER total_number_of_bytes;
    ULARGE_INTEGER total_number_of_free_bytes;
    
    if (!GetDiskFreeSpaceExA(
        path.c_str(),
        &free_bytes_available,
        &total_number_of_bytes,
        &total_number_of_free_bytes
    )) {
        return false;
    }
    
    total_bytes = total_number_of_bytes.QuadPart;
    free_bytes = total_number_of_free_bytes.QuadPart;
    available_bytes = free_bytes_available.QuadPart;
    
#else
    // Unix/Linux implementation
    struct statvfs stat;
    
    if (statvfs(path.c_str(), &stat) != 0) {
        return false;
    }
    
    total_bytes = stat.f_blocks * stat.f_frsize;
    free_bytes = stat.f_bfree * stat.f_frsize;
    available_bytes = stat.f_bavail * stat.f_frsize;
    
#endif
    
    return true;
}

std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

bool pathExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
#endif
}

std::string getDirectory(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    return path.substr(0, pos);
}

} // namespace disk_utils

} // namespace storage
} // namespace themis
