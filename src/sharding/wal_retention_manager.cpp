// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/wal_retention_manager.h"
#include <algorithm>

namespace themisdb {
namespace sharding {

WALRetentionManager::WALRetentionManager(const Config& config)
    : config_(config) {}

bool WALRetentionManager::registerSegment(const std::string& segment_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (segments_.find(segment_id) != segments_.end()) {
        return false;
    }
    
    WALSegmentInfo info;
    info.segment_id = segment_id;
    info.size_bytes = 0;
    info.created_at = std::chrono::system_clock::now();
    info.is_active = true;
    
    segments_[segment_id] = info;
    active_segment_id_ = segment_id;
    
    return true;
}

void WALRetentionManager::updateSegmentSize(
    const std::string& segment_id,
    size_t size_bytes
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = segments_.find(segment_id);
    if (it != segments_.end()) {
        it->second.size_bytes = size_bytes;
    }
}

void WALRetentionManager::markSegmentInactive(const std::string& segment_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = segments_.find(segment_id);
    if (it != segments_.end()) {
        it->second.is_active = false;
        if (active_segment_id_ == segment_id) {
            active_segment_id_.clear();
        }
    }
}

bool WALRetentionManager::needsRotation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (active_segment_id_.empty()) {
        return false;
    }
    
    auto it = segments_.find(active_segment_id_);
    if (it == segments_.end()) {
        return false;
    }
    
    // Check if active segment exceeds max size
    return it->second.size_bytes >= config_.max_segment_size;
}

size_t WALRetentionManager::cleanupOldSegments() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total_size = getTotalSize();
    size_t cleaned = 0;
    
    // Sort segments by creation time (oldest first)
    std::vector<std::pair<std::string, WALSegmentInfo>> sorted_segments;
    for (const auto& [id, info] : segments_) {
        if (!info.is_active) {
            sorted_segments.push_back({id, info});
        }
    }
    
    std::sort(sorted_segments.begin(), sorted_segments.end(),
        [](const auto& a, const auto& b) {
            return a.second.created_at < b.second.created_at;
        });
    
    // Delete segments based on retention criteria
    for (const auto& [segment_id, info] : sorted_segments) {
        bool should_delete = false;
        
        // Check time-based retention
        if (shouldDeleteSegment(info)) {
            should_delete = true;
        }
        
        // Check size-based retention
        if (total_size > config_.max_total_size) {
            should_delete = true;
        }
        
        if (should_delete) {
            if (config_.on_segment_ready_for_deletion) {
                config_.on_segment_ready_for_deletion(segment_id);
            }
            
            total_size -= info.size_bytes;
            segments_.erase(segment_id);
            ++cleaned;
        }
    }
    
    return cleaned;
}

WALRetentionManager::Stats WALRetentionManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.active_segments = 0;
    stats.total_size = 0;
    
    for (const auto& [segment_id, info] : segments_) {
        if (info.is_active) {
            ++stats.active_segments;
        }
        stats.total_size += info.size_bytes;
        stats.segment_sizes.push_back({segment_id, info.size_bytes});
    }
    
    return stats;
}

size_t WALRetentionManager::getTotalSize() const {
    size_t total = 0;
    for (const auto& [id, info] : segments_) {
        total += info.size_bytes;
    }
    return total;
}

bool WALRetentionManager::shouldDeleteSegment(const WALSegmentInfo& info) const {
    auto now = std::chrono::system_clock::now();
    auto age = now - info.created_at;
    return age > config_.retention_time;
}

} // namespace sharding
} // namespace themisdb
