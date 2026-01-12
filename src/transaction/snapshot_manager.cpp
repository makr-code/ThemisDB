#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <regex>
#include <algorithm>

namespace themis {

// Snapshot serialization
nlohmann::json SnapshotManager::Snapshot::toJson() const {
    return {
        {"tag_name", tag_name},
        {"sequence_number", sequence_number},
        {"timestamp_ms", timestamp_ms},
        {"description", description},
        {"created_by", created_by}
    };
}

SnapshotManager::Snapshot SnapshotManager::Snapshot::fromJson(const nlohmann::json& j) {
    Snapshot snapshot;
    snapshot.tag_name = j.at("tag_name").get<std::string>();
    snapshot.sequence_number = j.at("sequence_number").get<uint64_t>();
    snapshot.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
    snapshot.description = j.value("description", "");
    snapshot.created_by = j.value("created_by", "system");
    return snapshot;
}

// Constructor
SnapshotManager::SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed)
    : db_(db), changefeed_(changefeed) {
    THEMIS_INFO("SnapshotManager initialized");
}

// Create a named snapshot
SnapshotManager::Status SnapshotManager::createTag(
    const std::string& tag_name,
    const std::string& description,
    const std::string& created_by
) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate tag name
    auto validation = validateTagName(tag_name);
    if (!validation.ok) {
        return validation;
    }

    // Validate description length
    if (description.length() > MAX_DESCRIPTION_LENGTH) {
        return Status::Error("Description too long (max " + 
                           std::to_string(MAX_DESCRIPTION_LENGTH) + " characters)");
    }

    // Check if tag already exists
    if (tagExists(tag_name)) {
        return Status::Error("Tag '" + tag_name + "' already exists");
    }

    // Get current sequence from changefeed
    uint64_t current_sequence = changefeed_.getLatestSequence();
    
    // Create snapshot
    Snapshot snapshot;
    snapshot.tag_name = tag_name;
    snapshot.sequence_number = current_sequence;
    snapshot.timestamp_ms = getCurrentTimestampMs();
    snapshot.description = description;
    snapshot.created_by = created_by;

    // Serialize to JSON
    std::string json_value = snapshot.toJson().dump();

    // Store in RocksDB
    std::string key = makeKey(tag_name);
    auto status = db_.put(key, json_value);
    
    if (!status.ok()) {
        THEMIS_ERROR("Failed to create snapshot tag '{}': {}", tag_name, status.ToString());
        return Status::Error("Failed to store snapshot: " + status.ToString());
    }

    THEMIS_INFO("Created snapshot tag '{}' at sequence {} by {}", 
                tag_name, current_sequence, created_by);
    
    return Status::OK();
}

// Get a specific snapshot
std::optional<SnapshotManager::Snapshot> SnapshotManager::getTag(const std::string& tag_name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = makeKey(tag_name);
    std::string value;
    
    auto status = db_.get(key, value);
    if (!status.ok()) {
        return std::nullopt;
    }

    try {
        auto json = nlohmann::json::parse(value);
        return Snapshot::fromJson(json);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse snapshot JSON for tag '{}': {}", tag_name, e.what());
        return std::nullopt;
    }
}

// List all snapshots
std::vector<SnapshotManager::Snapshot> SnapshotManager::listTags() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Snapshot> snapshots;
    
    // Scan all keys with prefix "tags:"
    auto iter = db_.newIterator();
    if (!iter) {
        THEMIS_ERROR("Failed to create iterator for listing snapshots");
        return snapshots;
    }

    std::string prefix = KEY_PREFIX;
    iter->Seek(prefix);
    
    while (iter->Valid()) {
        std::string key = iter->key().ToString();
        
        // Check if key starts with our prefix
        if (key.substr(0, prefix.length()) != prefix) {
            break;
        }

        try {
            std::string value = iter->value().ToString();
            auto json = nlohmann::json::parse(value);
            snapshots.push_back(Snapshot::fromJson(json));
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse snapshot JSON for key '{}': {}", key, e.what());
        }

        iter->Next();
    }

    // Sort by timestamp (newest first)
    std::sort(snapshots.begin(), snapshots.end(), 
              [](const Snapshot& a, const Snapshot& b) {
                  return a.timestamp_ms > b.timestamp_ms;
              });

    return snapshots;
}

// Delete a snapshot
SnapshotManager::Status SnapshotManager::deleteTag(const std::string& tag_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if tag exists
    if (!tagExists(tag_name)) {
        return Status::Error("Tag '" + tag_name + "' does not exist");
    }

    std::string key = makeKey(tag_name);
    auto status = db_.erase(key);
    
    if (!status.ok()) {
        THEMIS_ERROR("Failed to delete snapshot tag '{}': {}", tag_name, status.ToString());
        return Status::Error("Failed to delete snapshot: " + status.ToString());
    }

    THEMIS_INFO("Deleted snapshot tag '{}'", tag_name);
    return Status::OK();
}

// Check if tag exists
bool SnapshotManager::tagExists(const std::string& tag_name) const {
    std::string key = makeKey(tag_name);
    std::string value;
    return db_.get(key, value).ok();
}

// Get statistics
SnapshotManager::SnapshotStats SnapshotManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    SnapshotStats stats{};
    stats.total_tags = 0;
    stats.oldest_sequence = UINT64_MAX;
    stats.newest_sequence = 0;
    stats.oldest_timestamp_ms = INT64_MAX;
    stats.newest_timestamp_ms = 0;

    auto snapshots = listTags();
    stats.total_tags = snapshots.size();

    if (!snapshots.empty()) {
        for (const auto& snapshot : snapshots) {
            if (snapshot.sequence_number < stats.oldest_sequence) {
                stats.oldest_sequence = snapshot.sequence_number;
            }
            if (snapshot.sequence_number > stats.newest_sequence) {
                stats.newest_sequence = snapshot.sequence_number;
            }
            if (snapshot.timestamp_ms < stats.oldest_timestamp_ms) {
                stats.oldest_timestamp_ms = snapshot.timestamp_ms;
            }
            if (snapshot.timestamp_ms > stats.newest_timestamp_ms) {
                stats.newest_timestamp_ms = snapshot.timestamp_ms;
            }
        }
    } else {
        // No snapshots, reset to 0
        stats.oldest_sequence = 0;
        stats.newest_sequence = 0;
        stats.oldest_timestamp_ms = 0;
        stats.newest_timestamp_ms = 0;
    }

    return stats;
}

// Validate tag name
SnapshotManager::Status SnapshotManager::validateTagName(const std::string& tag_name) {
    // Check length
    if (tag_name.empty()) {
        return Status::Error("Tag name cannot be empty");
    }
    if (tag_name.length() > MAX_TAG_NAME_LENGTH) {
        return Status::Error("Tag name too long (max " + 
                           std::to_string(MAX_TAG_NAME_LENGTH) + " characters)");
    }

    // Check format: alphanumeric, hyphens, underscores only
    // Pattern: ^[a-zA-Z0-9_-]+$
    static const std::regex valid_pattern("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(tag_name, valid_pattern)) {
        return Status::Error("Tag name contains invalid characters (use only alphanumeric, hyphens, underscores)");
    }

    return Status::OK();
}

// Get sequence for tag
std::optional<uint64_t> SnapshotManager::getSequenceForTag(const std::string& tag_name) const {
    auto snapshot = getTag(tag_name);
    if (snapshot) {
        return snapshot->sequence_number;
    }
    return std::nullopt;
}

// Helper methods
std::string SnapshotManager::makeKey(const std::string& tag_name) const {
    return std::string(KEY_PREFIX) + tag_name;
}

std::string SnapshotManager::extractTagName(const std::string& key) const {
    std::string prefix = KEY_PREFIX;
    if (key.substr(0, prefix.length()) == prefix) {
        return key.substr(prefix.length());
    }
    return key;
}

int64_t SnapshotManager::getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace themis
