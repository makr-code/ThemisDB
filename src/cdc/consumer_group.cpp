/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consumer_group.cpp                                 ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cdc/consumer_group.h"
#include "utils/logger.h"

#include <rocksdb/utilities/transaction_db.h>
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace cdc {

// ============================================================
// Static helpers
// ============================================================

uint32_t ConsumerGroupManager::fnv1a32(const std::string& s) {
    // FNV-1a 32-bit: stable, fast, no external dependency
    uint32_t hash = 2166136261u;
    for (unsigned char c : s) {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

uint32_t ConsumerGroupManager::partitionForKey(const std::string& key,
                                                uint32_t partition_count) {
    if (partition_count == 0) return 0;
    return fnv1a32(key) % partition_count;
}

uint32_t ConsumerGroupManager::partitionForConsumer(const std::string& consumer_id,
                                                     uint32_t partition_count) {
    if (partition_count == 0) return 0;
    return fnv1a32(consumer_id) % partition_count;
}

// ============================================================
// Construction
// ============================================================

ConsumerGroupManager::ConsumerGroupManager(rocksdb::TransactionDB* db,
                                           rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {
    if (!db_) {
        throw error::invalidArgument("ConsumerGroupManager: db cannot be null");
    }
}

// ============================================================
// Key helpers
// ============================================================

std::string ConsumerGroupManager::makeConfigKey(const std::string& group_id) const {
    return std::string(GROUP_KEY_PREFIX) + group_id + CONFIG_SUFFIX;
}

std::string ConsumerGroupManager::makeOffsetKey(const std::string& group_id) const {
    return std::string(GROUP_KEY_PREFIX) + group_id + OFFSET_SUFFIX;
}

// ============================================================
// Internal RocksDB helpers (mutex must be held by caller)
// ============================================================

ConsumerGroupConfig ConsumerGroupManager::readConfigLocked(
        const std::string& group_id) const {
    rocksdb::ReadOptions opts;
    std::string value;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(opts, cf_, makeConfigKey(group_id), &value);
    } else {
        s = db_->Get(opts, makeConfigKey(group_id), &value);
    }

    if (s.IsNotFound()) {
        throw error::invalidArgument("Consumer group not found: " + group_id);
    }
    if (!s.ok()) {
        throw error::dbOperationFailed("Get group config", s.ToString());
    }

    try {
        return ConsumerGroupConfig::fromJson(nlohmann::json::parse(value));
    } catch (const std::exception& e) {
        throw error::internalError("Failed to parse group config for '" +
                                   group_id + "': " + e.what());
    }
}

uint64_t ConsumerGroupManager::readOffsetLocked(const std::string& group_id) const {
    rocksdb::ReadOptions opts;
    std::string value;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(opts, cf_, makeOffsetKey(group_id), &value);
    } else {
        s = db_->Get(opts, makeOffsetKey(group_id), &value);
    }

    if (s.IsNotFound() || value.empty()) return 0;
    if (!s.ok()) {
        throw error::dbOperationFailed("Get group offset", s.ToString());
    }

    try {
        return std::stoull(value);
    } catch (...) {
        return 0;
    }
}

void ConsumerGroupManager::writeConfigLocked(const ConsumerGroupConfig& config) {
    rocksdb::WriteOptions opts;
    std::string value = config.toJson().dump();
    rocksdb::Status s;

    if (cf_) {
        s = db_->Put(opts, cf_, makeConfigKey(config.group_id), value);
    } else {
        s = db_->Put(opts, makeConfigKey(config.group_id), value);
    }

    if (!s.ok()) {
        throw error::dbOperationFailed("Put group config", s.ToString());
    }
}

void ConsumerGroupManager::writeOffsetLocked(const std::string& group_id,
                                              uint64_t sequence) {
    rocksdb::WriteOptions opts;
    std::string value = std::to_string(sequence);
    rocksdb::Status s;

    if (cf_) {
        s = db_->Put(opts, cf_, makeOffsetKey(group_id), value);
    } else {
        s = db_->Put(opts, makeOffsetKey(group_id), value);
    }

    if (!s.ok()) {
        throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::ERROR,
                           "Failed to commit group offset", s.ToString());
    }
}

// ============================================================
// Group lifecycle
// ============================================================

void ConsumerGroupManager::createGroup(const ConsumerGroupConfig& config) {
    if (config.group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }
    if (config.consumer_count == 0) {
        throw error::invalidArgument("consumer_count", "must be >= 1");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    writeConfigLocked(config);
    THEMIS_INFO("CDC ConsumerGroup created: group={} partitions={}",
                config.group_id, config.consumer_count);
}

void ConsumerGroupManager::deleteGroup(const std::string& group_id) {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    rocksdb::WriteOptions opts;
    rocksdb::Status s1, s2;

    if (cf_) {
        s1 = db_->Delete(opts, cf_, makeConfigKey(group_id));
        s2 = db_->Delete(opts, cf_, makeOffsetKey(group_id));
    } else {
        s1 = db_->Delete(opts, makeConfigKey(group_id));
        s2 = db_->Delete(opts, makeOffsetKey(group_id));
    }

    // Tolerate NotFound (group might have no committed offset yet)
    if (!s1.ok() && !s1.IsNotFound()) {
        throw error::dbOperationFailed("Delete group config", s1.ToString());
    }
    if (!s2.ok() && !s2.IsNotFound()) {
        throw error::dbOperationFailed("Delete group offset", s2.ToString());
    }

    THEMIS_INFO("CDC ConsumerGroup deleted: group={}", group_id);
}

bool ConsumerGroupManager::groupExists(const std::string& group_id) const {
    if (group_id.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    rocksdb::ReadOptions opts;
    std::string value;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(opts, cf_, makeConfigKey(group_id), &value);
    } else {
        s = db_->Get(opts, makeConfigKey(group_id), &value);
    }

    return s.ok();
}

ConsumerGroupConfig ConsumerGroupManager::getGroupConfig(
        const std::string& group_id) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return readConfigLocked(group_id);
}

ConsumerGroupInfo ConsumerGroupManager::getGroupInfo(
        const std::string& group_id) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }
    std::lock_guard<std::mutex> lock(mutex_);

    ConsumerGroupInfo info;
    info.config             = readConfigLocked(group_id);
    info.committed_sequence = readOffsetLocked(group_id);
    return info;
}

std::vector<std::string> ConsumerGroupManager::listGroups() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> groups;
    rocksdb::ReadOptions opts;

    std::unique_ptr<rocksdb::Iterator> it;
    if (cf_) {
        it.reset(db_->NewIterator(opts, cf_));
    } else {
        it.reset(db_->NewIterator(opts));
    }

    const std::string prefix    = std::string(GROUP_KEY_PREFIX);
    const std::string config_sfx = CONFIG_SUFFIX;

    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.rfind(prefix, 0) != 0) break; // past prefix

        // Only emit config keys (not offset keys)
        if (key.size() > config_sfx.size() &&
            key.compare(key.size() - config_sfx.size(),
                        config_sfx.size(), config_sfx) == 0) {
            // Strip prefix and suffix to get group_id
            std::string gid = key.substr(
                prefix.size(),
                key.size() - prefix.size() - config_sfx.size());
            if (!gid.empty()) {
                groups.push_back(std::move(gid));
            }
        }
    }

    return groups;
}

// ============================================================
// Offset tracking
// ============================================================

uint64_t ConsumerGroupManager::getCommittedOffset(
        const std::string& group_id) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate group exists
    readConfigLocked(group_id); // throws if not found
    return readOffsetLocked(group_id);
}

void ConsumerGroupManager::commitOffset(const std::string& group_id,
                                         uint64_t sequence) {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Validate group exists
    readConfigLocked(group_id); // throws if not found

    // Offsets only advance
    uint64_t current = readOffsetLocked(group_id);
    if (sequence <= current) {
        return; // no-op: already at or ahead of this sequence
    }

    writeOffsetLocked(group_id, sequence);

    THEMIS_DEBUG("CDC ConsumerGroup offset committed: group={} sequence={}",
                 group_id, sequence);
}

// ============================================================
// Partition assignment
// ============================================================

uint32_t ConsumerGroupManager::getConsumerPartition(
        const std::string& group_id,
        const std::string& consumer_id) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ConsumerGroupConfig cfg = readConfigLocked(group_id);
    return partitionForConsumer(consumer_id, cfg.consumer_count);
}

bool ConsumerGroupManager::consumerHandlesKey(const std::string& group_id,
                                               const std::string& consumer_id,
                                               const std::string& event_key) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ConsumerGroupConfig cfg = readConfigLocked(group_id);

    uint32_t consumer_partition = partitionForConsumer(consumer_id, cfg.consumer_count);
    uint32_t key_partition      = partitionForKey(event_key, cfg.consumer_count);
    return consumer_partition == key_partition;
}

uint32_t ConsumerGroupManager::getPartitionForKey(const std::string& group_id,
                                                   const std::string& key) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ConsumerGroupConfig cfg = readConfigLocked(group_id);
    return partitionForKey(key, cfg.consumer_count);
}

// ============================================================
// Event fetching
// ============================================================

std::vector<Changefeed::ChangeEvent> ConsumerGroupManager::fetchEvents(
        const std::string& group_id,
        const std::string& consumer_id,
        const Changefeed& changefeed,
        size_t limit) const {
    if (group_id.empty()) {
        throw error::invalidArgument("group_id", "must not be empty");
    }

    // Read group config and committed offset under lock, then release
    ConsumerGroupConfig cfg;
    uint64_t committed = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cfg       = readConfigLocked(group_id);
        committed = readOffsetLocked(group_id);
    }

    const uint32_t consumer_partition =
        partitionForConsumer(consumer_id, cfg.consumer_count);

    // Fetch a broader batch from the changefeed; we may need to over-fetch
    // because some events belong to other partitions and are filtered out.
    // We fetch min(limit * consumer_count, 10000) events to bound memory.
    const size_t effective_limit = (limit == 0) ? 100 : limit;
    const size_t fetch_limit     = std::min<size_t>(
        effective_limit * static_cast<size_t>(cfg.consumer_count), 10000u);

    Changefeed::ListOptions opts;
    opts.from_sequence = committed;   // listEvents returns events *after* from_sequence
    opts.limit         = fetch_limit;

    std::vector<Changefeed::ChangeEvent> all_events = changefeed.listEvents(opts);

    // Filter to this consumer's partition
    std::vector<Changefeed::ChangeEvent> result;
    result.reserve(effective_limit);

    for (auto& ev : all_events) {
        uint32_t key_partition = partitionForKey(ev.key, cfg.consumer_count);
        if (key_partition == consumer_partition) {
            result.push_back(std::move(ev));
            if (result.size() >= effective_limit) break;
        }
    }

    return result;
}

} // namespace cdc
} // namespace themis
