/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            diff_engine.cpp                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     571                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • c1d4f1e36  2026-02-06  Enhance Diff-Engine: Fix change detection, optimize times... ║
    • 2c691dfc7  2026-01-12  Phase 1 & 2: Implement Named Snapshots and Structured Dif... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "analytics/diff_engine.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace analytics {

// Change serialization
json DiffEngine::Change::toJson() const {
    json j;
    j["type"] = (type == ChangeType::ADDED ? "added" :
                 type == ChangeType::MODIFIED ? "modified" : "deleted");
    j["key"] = key;
    
    if (old_value.has_value()) {
        j["old_value"] = *old_value;
    }
    if (new_value.has_value()) {
        j["new_value"] = *new_value;
    }
    
    j["sequence"] = sequence;
    j["timestamp_ms"] = timestamp_ms;
    j["metadata"] = metadata;
    
    return j;
}

DiffEngine::Change DiffEngine::Change::fromJson(const json& j) {
    Change c;
    std::string type_str = j["type"];
    if (type_str == "added") c.type = ChangeType::ADDED;
    else if (type_str == "modified") c.type = ChangeType::MODIFIED;
    else c.type = ChangeType::DELETED;
    
    c.key = j["key"];
    if (j.contains("old_value")) {
        c.old_value = j["old_value"].get<std::string>();
    }
    if (j.contains("new_value")) {
        c.new_value = j["new_value"].get<std::string>();
    }
    c.sequence = j["sequence"];
    c.timestamp_ms = j["timestamp_ms"];
    c.metadata = j["metadata"];
    
    return c;
}

// DiffStats serialization
json DiffEngine::DiffStats::toJson() const {
    json j;
    j["added_count"] = added_count;
    j["modified_count"] = modified_count;
    j["deleted_count"] = deleted_count;
    j["total_changes"] = total_changes;
    return j;
}

// DiffResult serialization
json DiffEngine::DiffResult::toJson() const {
    json j;
    
    json added_arr = json::array();
    for (const auto& change : added) {
        added_arr.push_back(change.toJson());
    }
    j["added"] = added_arr;
    
    json modified_arr = json::array();
    for (const auto& change : modified) {
        modified_arr.push_back(change.toJson());
    }
    j["modified"] = modified_arr;
    
    json deleted_arr = json::array();
    for (const auto& change : deleted) {
        deleted_arr.push_back(change.toJson());
    }
    j["deleted"] = deleted_arr;
    
    j["stats"] = stats.toJson();
    j["from_sequence"] = from_sequence;
    j["to_sequence"] = to_sequence;
    
    if (from_timestamp_ms.has_value()) {
        j["from_timestamp_ms"] = *from_timestamp_ms;
    }
    if (to_timestamp_ms.has_value()) {
        j["to_timestamp_ms"] = *to_timestamp_ms;
    }
    
    return j;
}

DiffEngine::DiffResult DiffEngine::DiffResult::fromJson(const json& j) {
    DiffResult result;
    
    for (const auto& item : j["added"]) {
        result.added.push_back(Change::fromJson(item));
    }
    for (const auto& item : j["modified"]) {
        result.modified.push_back(Change::fromJson(item));
    }
    for (const auto& item : j["deleted"]) {
        result.deleted.push_back(Change::fromJson(item));
    }
    
    const auto& stats = j["stats"];
    result.stats.added_count = stats["added_count"];
    result.stats.modified_count = stats["modified_count"];
    result.stats.deleted_count = stats["deleted_count"];
    result.stats.total_changes = stats["total_changes"];
    
    result.from_sequence = j["from_sequence"];
    result.to_sequence = j["to_sequence"];
    
    if (j.contains("from_timestamp_ms")) {
        result.from_timestamp_ms = j["from_timestamp_ms"];
    }
    if (j.contains("to_timestamp_ms")) {
        result.to_timestamp_ms = j["to_timestamp_ms"];
    }
    
    return result;
}

// Constructor
DiffEngine::DiffEngine(Changefeed& changefeed, transaction::SnapshotManager* snapshot_manager)
    : changefeed_(changefeed), snapshot_manager_(snapshot_manager) {
}

// Compute diff between sequences
DiffEngine::DiffResult DiffEngine::computeDiff(
    uint64_t from_sequence,
    uint64_t to_sequence,
    const DiffOptions& options) {
    
    // Validate sequence range
    if (from_sequence >= to_sequence) {
        throw std::invalid_argument(
            fmt::format("Invalid sequence range: from={} >= to={}", from_sequence, to_sequence)
        );
    }
    
    // Validate options
    if (options.limit > MAX_DIFF_LIMIT) {
        throw std::invalid_argument(
            fmt::format("Limit too large: {} (max: {})", options.limit, MAX_DIFF_LIMIT)
        );
    }
    
    spdlog::debug("Computing diff: from_seq={} to_seq={}", from_sequence, to_sequence);
    
    // Check cache first
    if (options.enable_caching) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto cache_key = std::make_pair(from_sequence, to_sequence);
        auto it = diff_cache_.find(cache_key);
        if (it != diff_cache_.end() && isCacheValid(it->second)) {
            spdlog::debug("Cache hit for diff range [{}, {}]", from_sequence, to_sequence);
            return it->second.result;
        }
    }
    
    // Fetch events from changefeed
    Changefeed::ListOptions list_opts;
    list_opts.from_sequence = from_sequence;
    list_opts.limit = 0; // No limit, get all events
    
    auto all_events = changefeed_.listEvents(list_opts);
    
    // Filter to the sequence range
    std::vector<Changefeed::ChangeEvent> events;
    for (const auto& event : all_events) {
        if (event.sequence > from_sequence && event.sequence <= to_sequence) {
            events.push_back(event);
        }
        if (event.sequence > to_sequence) {
            break;
        }
    }
    
    spdlog::debug("Found {} events in range [{}, {}]", events.size(), from_sequence, to_sequence);
    
    // Process events
    DiffResult result = processEvents(events, options);
    result.from_sequence = from_sequence;
    result.to_sequence = to_sequence;
    
    // Cache result
    if (options.enable_caching) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (diff_cache_.size() >= MAX_CACHE_SIZE) {
            evictOldCacheEntries();
        }
        auto cache_key = std::make_pair(from_sequence, to_sequence);
        diff_cache_[cache_key] = {result, std::chrono::system_clock::now()};
    }
    
    return result;
}

// Compute diff by timestamp
DiffEngine::DiffResult DiffEngine::computeDiffByTimestamp(
    int64_t from_timestamp,
    int64_t to_timestamp,
    const DiffOptions& options) {
    
    if (from_timestamp >= to_timestamp) {
        throw std::invalid_argument(
            fmt::format("Invalid timestamp range: from={} >= to={}", from_timestamp, to_timestamp)
        );
    }
    
    spdlog::debug("Computing diff by timestamp: from={} to={}", from_timestamp, to_timestamp);
    
    // Find corresponding sequence numbers
    auto [from_seq, to_seq] = findSequenceRange(from_timestamp, to_timestamp);
    
    spdlog::debug("Timestamp range maps to sequence range [{}, {}]", from_seq, to_seq);
    
    // Compute diff using sequence range
    DiffResult result = computeDiff(from_seq, to_seq, options);
    result.from_timestamp_ms = from_timestamp;
    result.to_timestamp_ms = to_timestamp;
    
    return result;
}

// Compute diff by tag (now implemented with Phase 1)
DiffEngine::DiffResult DiffEngine::computeDiffByTag(
    const std::string& from_tag,
    const std::string& to_tag,
    const DiffOptions& options) {
    
    // Check if SnapshotManager is available
    if (!snapshot_manager_) {
        throw std::runtime_error(
            "Tag-based diff requires SnapshotManager. "
            "Pass SnapshotManager to DiffEngine constructor to enable this feature."
        );
    }
    
    spdlog::debug("Computing diff by tags: from='{}' to='{}'", from_tag, to_tag);
    
    // Get sequence numbers for tags
    auto from_seq_opt = snapshot_manager_->getSequenceForTag(from_tag);
    if (!from_seq_opt.has_value()) {
        throw std::runtime_error(fmt::format("Tag '{}' not found", from_tag));
    }
    
    auto to_seq_opt = snapshot_manager_->getSequenceForTag(to_tag);
    if (!to_seq_opt.has_value()) {
        throw std::runtime_error(fmt::format("Tag '{}' not found", to_tag));
    }
    
    uint64_t from_seq = *from_seq_opt;
    uint64_t to_seq = *to_seq_opt;
    
    spdlog::debug("Tag '{}' maps to sequence {}, tag '{}' maps to sequence {}", 
                  from_tag, from_seq, to_tag, to_seq);
    
    // Compute diff using sequence range
    DiffResult result = computeDiff(from_seq, to_seq, options);
    
    // Add tag information to result metadata
    result.from_timestamp_ms = snapshot_manager_->getTimestampForTag(from_tag);
    result.to_timestamp_ms = snapshot_manager_->getTimestampForTag(to_tag);
    
    return result;
}

// Clear cache
void DiffEngine::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    diff_cache_.clear();
    spdlog::info("Diff cache cleared");
}

// Get cache stats
json DiffEngine::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    json j;
    j["cache_size"] = diff_cache_.size();
    j["max_cache_size"] = MAX_CACHE_SIZE;
    j["cache_ttl_seconds"] = CACHE_TTL.count();
    return j;
}

// Process events and categorize them
DiffEngine::DiffResult DiffEngine::processEvents(
    const std::vector<Changefeed::ChangeEvent>& events,
    const DiffOptions& options) {
    
    DiffResult result;
    
    // Build key history to track state changes
    std::map<std::string, std::vector<Changefeed::ChangeEvent>> key_events;
    
    for (const auto& event : events) {
        // Apply filters
        if (!shouldIncludeEvent(event, options)) {
            continue;
        }
        
        key_events[event.key].push_back(event);
    }
    
    // Categorize changes for each key
    for (const auto& [key, key_event_list] : key_events) {
        if (key_event_list.empty()) continue;
        
        // Determine the net effect of all events for this key
        auto first_event = key_event_list.front();
        auto last_event = key_event_list.back();
        
        Change change;
        change.key = key;
        change.sequence = last_event.sequence;
        change.timestamp_ms = last_event.timestamp_ms;
        change.metadata = last_event.metadata;
        
        if (options.include_values) {
            // Check if key existed before the first event in range
            // For simplicity, we assume if first event is PUT, it's either ADDED or MODIFIED
            // If first event is DELETE, the key is being DELETED
            
            if (last_event.type == Changefeed::ChangeEventType::EVENT_DELETE) {
                // Key was deleted
                change.type = ChangeType::DELETED;
                if (first_event.value.has_value()) {
                    change.old_value = *first_event.value;
                }
                result.deleted.push_back(change);
                result.stats.deleted_count++;
            } else if (last_event.type == Changefeed::ChangeEventType::EVENT_PUT) {
                // Key was added or modified
                change.new_value = last_event.value;
                
                if (key_event_list.size() == 1) {
                    // Single PUT event in range
                    // Check if this is the first event for this key by querying before from_sequence
                    // For now, we check if from_sequence is 0 to determine if it's truly ADDED
                    // Otherwise, conservatively mark as MODIFIED
                    
                    // If from_sequence is 0, this is definitely ADDED (new key)
                    if (result.from_sequence == 0) {
                        change.type = ChangeType::ADDED;
                        result.added.push_back(change);
                        result.stats.added_count++;
                    } else {
                        // Could be ADDED or MODIFIED - conservatively assume MODIFIED
                        // Old value unknown (would need to query history before from_sequence)
                        change.type = ChangeType::MODIFIED;
                        result.modified.push_back(change);
                        result.stats.modified_count++;
                    }
                } else {
                    // Multiple events for same key - definitely MODIFIED
                    change.type = ChangeType::MODIFIED;
                    
                    // Get old value from first event in range
                    if (first_event.value.has_value()) {
                        change.old_value = first_event.value;
                    }
                    
                    result.modified.push_back(change);
                    result.stats.modified_count++;
                }
            }
        } else {
            // Without values, just report the change type based on last event
            if (last_event.type == Changefeed::ChangeEventType::EVENT_DELETE) {
                change.type = ChangeType::DELETED;
                result.deleted.push_back(change);
                result.stats.deleted_count++;
            } else {
                change.type = ChangeType::MODIFIED;
                result.modified.push_back(change);
                result.stats.modified_count++;
            }
        }
    }
    
    // Apply pagination
    if (options.limit > 0 || options.offset > 0) {
        // Combine all changes
        std::vector<Change> all_changes;
        all_changes.insert(all_changes.end(), result.added.begin(), result.added.end());
        all_changes.insert(all_changes.end(), result.modified.begin(), result.modified.end());
        all_changes.insert(all_changes.end(), result.deleted.begin(), result.deleted.end());
        
        // Sort by sequence
        std::sort(all_changes.begin(), all_changes.end(),
                  [](const Change& a, const Change& b) { return a.sequence < b.sequence; });
        
        // Apply offset and limit
        size_t start = std::min(options.offset, all_changes.size());
        size_t end = all_changes.size();
        if (options.limit > 0) {
            end = std::min(start + options.limit, all_changes.size());
        }
        
        // Rebuild categorized results
        result.added.clear();
        result.modified.clear();
        result.deleted.clear();
        
        for (size_t i = start; i < end; ++i) {
            const auto& change = all_changes[i];
            switch (change.type) {
                case ChangeType::ADDED:
                    result.added.push_back(change);
                    break;
                case ChangeType::MODIFIED:
                    result.modified.push_back(change);
                    break;
                case ChangeType::DELETED:
                    result.deleted.push_back(change);
                    break;
            }
        }
        
        // Update stats to reflect pagination
        result.stats.added_count = result.added.size();
        result.stats.modified_count = result.modified.size();
        result.stats.deleted_count = result.deleted.size();
    }
    
    result.stats.total_changes = result.stats.added_count + 
                                  result.stats.modified_count + 
                                  result.stats.deleted_count;
    
    spdlog::debug("Diff result: {} added, {} modified, {} deleted",
                  result.stats.added_count, result.stats.modified_count, result.stats.deleted_count);
    
    return result;
}

// Check if event should be included based on filters
bool DiffEngine::shouldIncludeEvent(
    const Changefeed::ChangeEvent& event,
    const DiffOptions& options) const {
    
    // Filter by table (assuming key format: "table:pk" or "entity:table:pk")
    if (options.table_filter.has_value()) {
        const auto& table = *options.table_filter;
        if (event.key.find(table) == std::string::npos) {
            return false;
        }
    }
    
    // Filter by key prefix
    if (options.key_prefix.has_value()) {
        const auto& prefix = *options.key_prefix;
        if (event.key.substr(0, prefix.length()) != prefix) {
            return false;
        }
    }
    
    return true;
}

// Find sequence range for timestamp range
std::pair<uint64_t, uint64_t> DiffEngine::findSequenceRange(
    int64_t from_timestamp,
    int64_t to_timestamp) const {
    
    // Fetch all events (Note: This could be optimized with index on timestamps)
    Changefeed::ListOptions opts;
    opts.from_sequence = 0;
    opts.limit = 0; // No limit
    
    auto all_events = changefeed_.listEvents(opts);
    
    if (all_events.empty()) {
        spdlog::warn("No events available for timestamp range [{}, {}]", from_timestamp, to_timestamp);
        return {0, 0};
    }
    
    // Binary search for from_timestamp
    auto from_it = std::lower_bound(all_events.begin(), all_events.end(), from_timestamp,
        [](const Changefeed::ChangeEvent& event, int64_t ts) {
            return event.timestamp_ms < ts;
        });
    
    // Binary search for to_timestamp  
    auto to_it = std::upper_bound(all_events.begin(), all_events.end(), to_timestamp,
        [](int64_t ts, const Changefeed::ChangeEvent& event) {
            return ts < event.timestamp_ms;
        });
    
    uint64_t from_seq = 0;
    uint64_t to_seq = 0;
    
    // Set from_sequence (just before the first event in range)
    if (from_it != all_events.end()) {
        from_seq = from_it->sequence > 0 ? from_it->sequence - 1 : 0;
    }
    
    // Set to_sequence (last event within range)
    if (to_it != all_events.begin()) {
        --to_it; // Move back to last event within range
        to_seq = to_it->sequence;
    }
    
    // Validate range
    if (from_seq >= to_seq && to_seq > 0) {
        spdlog::warn("No events found in timestamp range [{}, {}]", from_timestamp, to_timestamp);
        return {0, 0};
    }
    
    if (to_seq == 0) {
        spdlog::warn("No events found in timestamp range [{}, {}]", from_timestamp, to_timestamp);
        return {0, 0};
    }
    
    spdlog::debug("Timestamp range [{}, {}] maps to sequence range [{}, {}]",
                  from_timestamp, to_timestamp, from_seq, to_seq);
    
    return {from_seq, to_seq};
}

// Check if cached result is still valid
bool DiffEngine::isCacheValid(const CachedDiff& cached) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - cached.cached_at);
    return age < CACHE_TTL;
}

// Evict oldest cache entries
void DiffEngine::evictOldCacheEntries() {
    if (diff_cache_.empty()) return;
    
    // Find oldest entry
    auto oldest = diff_cache_.begin();
    for (auto it = diff_cache_.begin(); it != diff_cache_.end(); ++it) {
        if (it->second.cached_at < oldest->second.cached_at) {
            oldest = it;
        }
    }
    
    spdlog::debug("Evicting oldest cache entry: range [{}, {}]",
                  oldest->first.first, oldest->first.second);
    diff_cache_.erase(oldest);
}

} // namespace analytics
} // namespace themis
