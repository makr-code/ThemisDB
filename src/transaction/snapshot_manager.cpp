#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <regex>
#include <algorithm>
#include <chrono>

namespace themis {

// Serialization for Snapshot
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
    Snapshot s;
    s.tag_name = j.at("tag_name").get<std::string>();
    s.sequence_number = j.at("sequence_number").get<uint64_t>();
    s.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
    s.description = j.at("description").get<std::string>();
    s.created_by = j.at("created_by").get<std::string>();
    return s;
}

SnapshotManager::SnapshotManager(rocksdb::TransactionDB* db,
                                rocksdb::ColumnFamilyHandle* cf,
                                Changefeed* changefeed)
    : db_(db), cf_(cf), changefeed_(changefeed) {
    if (!db_) {
        throw std::invalid_argument("SnapshotManager: db cannot be null");
    }
    if (!cf_) {
        throw std::invalid_argument("SnapshotManager: cf cannot be null");
    }
    if (!changefeed_) {
        throw std::invalid_argument("SnapshotManager: changefeed cannot be null");
    }
}

SnapshotManager::Status SnapshotManager::createTag(const std::string& tag_name,
                                                   const std::string& description,
                                                   const std::string& created_by) {
    // Validate inputs
    if (!isValidTagName(tag_name)) {
        return Status::Error("Invalid tag name: must be 1-64 characters, lowercase alphanumeric, "
                           "hyphens, underscores, and start with a letter");
    }
    
    if (!isValidDescription(description)) {
        return Status::Error("Invalid description: must be 0-500 characters");
    }

    // Check if tag already exists
    if (getTag(tag_name).has_value()) {
        return Status::Error("Tag already exists: " + tag_name);
    }

    // Get current sequence number from changefeed
    uint64_t sequence = changefeed_->getLatestSequence();
    
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    int64_t timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    // Create snapshot
    Snapshot snapshot{
        tag_name,
        sequence,
        timestamp_ms,
        description,
        created_by
    };

    // Serialize to JSON
    std::string value = snapshot.toJson().dump();
    
    // Store in RocksDB
    std::string key = makeKey(tag_name);
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s = db_->Put(write_opts, cf_, key, value);
    
    if (!s.ok()) {
        return Status::Error("Failed to create tag: " + s.ToString());
    }

    THEMIS_INFO("Snapshot created: tag={}, sequence={}, user={}", 
                tag_name, sequence, created_by);

    return Status::OK();
}

std::optional<SnapshotManager::Snapshot> SnapshotManager::getTag(const std::string& tag_name) const {
    std::string key = makeKey(tag_name);
    std::string value;
    
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s = db_->Get(read_opts, cf_, key, &value);
    
    if (s.IsNotFound()) {
        return std::nullopt;
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to get tag {}: {}", tag_name, s.ToString());
        return std::nullopt;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(value);
        return Snapshot::fromJson(j);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse snapshot JSON for tag {}: {}", tag_name, e.what());
        return std::nullopt;
    }
}

std::vector<SnapshotManager::Snapshot> SnapshotManager::listTags(bool sort_by_time) const {
    std::vector<Snapshot> snapshots;
    
    // Iterate over all keys with TAG_PREFIX
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_opts, cf_));
    
    std::string prefix = TAG_PREFIX;
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            snapshots.push_back(Snapshot::fromJson(j));
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to parse snapshot JSON: {}", e.what());
            // Continue with next snapshot
        }
    }

    // Sort by time if requested
    if (sort_by_time) {
        std::sort(snapshots.begin(), snapshots.end(),
            [](const Snapshot& a, const Snapshot& b) {
                return a.timestamp_ms > b.timestamp_ms; // Newest first
            });
    } else {
        std::sort(snapshots.begin(), snapshots.end(),
            [](const Snapshot& a, const Snapshot& b) {
                return a.tag_name < b.tag_name; // Alphabetical
            });
    }

    return snapshots;
}

SnapshotManager::Status SnapshotManager::deleteTag(const std::string& tag_name) {
    // Check if tag exists
    if (!getTag(tag_name).has_value()) {
        return Status::Error("Tag not found: " + tag_name);
    }

    // Delete from RocksDB
    std::string key = makeKey(tag_name);
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s = db_->Delete(write_opts, cf_, key);
    
    if (!s.ok()) {
        return Status::Error("Failed to delete tag: " + s.ToString());
    }

    THEMIS_INFO("Snapshot deleted: tag={}", tag_name);

    return Status::OK();
}

SnapshotManager::Stats SnapshotManager::getStats() const {
    Stats stats{0, 0, INT64_MAX, 0};
    
    std::vector<Snapshot> snapshots = listTags(false);
    stats.total_snapshots = snapshots.size();
    
    if (snapshots.empty()) {
        stats.oldest_timestamp_ms = 0;
        stats.newest_timestamp_ms = 0;
        return stats;
    }

    // Find oldest and newest timestamps
    for (const auto& snap : snapshots) {
        if (snap.timestamp_ms < stats.oldest_timestamp_ms) {
            stats.oldest_timestamp_ms = snap.timestamp_ms;
        }
        if (snap.timestamp_ms > stats.newest_timestamp_ms) {
            stats.newest_timestamp_ms = snap.timestamp_ms;
        }
        
        // Approximate size: key + value JSON size
        stats.total_size_bytes += makeKey(snap.tag_name).size() + snap.toJson().dump().size();
    }

    return stats;
}

bool SnapshotManager::isValidTagName(const std::string& tag_name) {
    // Check length
    if (tag_name.empty() || tag_name.size() > MAX_TAG_NAME_LENGTH) {
        return false;
    }

    // Check format: lowercase alphanumeric, hyphens, underscores, start with letter
    // Pattern: ^[a-z][a-z0-9_-]*$
    std::regex pattern("^[a-z][a-z0-9_-]*$");
    return std::regex_match(tag_name, pattern);
}

bool SnapshotManager::isValidDescription(const std::string& description) {
    return description.size() <= MAX_DESCRIPTION_LENGTH;
}

std::string SnapshotManager::makeKey(const std::string& tag_name) const {
    return std::string(TAG_PREFIX) + tag_name;
}

std::string SnapshotManager::extractTagName(const std::string& key) const {
    if (key.starts_with(TAG_PREFIX)) {
        return key.substr(std::strlen(TAG_PREFIX));
    }
    return "";
}

} // namespace themis
