// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_WAL_RETENTION_MANAGER_H
#define THEMISDB_SHARDING_WAL_RETENTION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <mutex>

namespace themisdb {
namespace sharding {

/**
 * @brief WAL segment metadata
 */
struct WALSegmentInfo {
    std::string segment_id;
    size_t size_bytes;
    std::chrono::system_clock::time_point created_at;
    bool is_active;
};

/**
 * @brief WAL Retention Manager
 * 
 * Manages WAL segment lifecycle with:
 * - Size-based segment rotation
 * - Total size limits
 * - Time-based retention
 * - Automatic cleanup of old segments
 */
class WALRetentionManager {
public:
    struct Config {
        size_t max_segment_size{1073741824};  // 1 GB
        size_t max_total_size{10737418240};   // 10 GB
        std::chrono::milliseconds retention_time{86400000};  // 24 hours
        std::function<void(const std::string& segment_id)> on_segment_ready_for_deletion;
    };
    
    struct Stats {
        size_t active_segments;
        size_t total_size;
        std::vector<std::pair<std::string, size_t>> segment_sizes;
    };
    
    explicit WALRetentionManager(const Config& config);
    
    /**
     * @brief Register new segment
     * @param segment_id Segment identifier
     * @return true if registered successfully
     */
    bool registerSegment(const std::string& segment_id);
    
    /**
     * @brief Update segment size
     * @param segment_id Segment identifier
     * @param size_bytes Current size in bytes
     */
    void updateSegmentSize(const std::string& segment_id, size_t size_bytes);
    
    /**
     * @brief Mark segment as inactive (ready for rotation)
     * @param segment_id Segment identifier
     */
    void markSegmentInactive(const std::string& segment_id);
    
    /**
     * @brief Check if active segment needs rotation
     * @return true if rotation needed
     */
    bool needsRotation() const;
    
    /**
     * @brief Clean up old segments
     * @return Number of segments marked for deletion
     */
    size_t cleanupOldSegments();
    
    /**
     * @brief Get statistics
     * @return Current stats
     */
    Stats getStats() const;

private:
    Config config_;
    mutable std::mutex mutex_;
    std::map<std::string, WALSegmentInfo> segments_;
    std::string active_segment_id_;
    
    /**
     * @brief Get total size of all segments
     */
    size_t getTotalSize() const;
    
    /**
     * @brief Check if segment should be deleted
     */
    bool shouldDeleteSegment(const WALSegmentInfo& info) const;
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_WAL_RETENTION_MANAGER_H
