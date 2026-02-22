/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changefeed.cpp                                     ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     628                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cdc/changefeed.h"
#include "cdc/cdc_error.h"
#include "utils/logger.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace themis {
using namespace themis::cdc;

// ===== ChangeEvent JSON Serialization =====

nlohmann::json Changefeed::ChangeEvent::toJson() const {
    nlohmann::json j;
    j["sequence"] = sequence;
    
    // Convert ChangeEventType to string
    switch (type) {
        case ChangeEventType::EVENT_PUT: j["type"] = "PUT"; break;
        case ChangeEventType::EVENT_DELETE: j["type"] = "DELETE"; break;
        case ChangeEventType::EVENT_TRANSACTION_COMMIT: j["type"] = "TRANSACTION_COMMIT"; break;
        case ChangeEventType::EVENT_TRANSACTION_ROLLBACK: j["type"] = "TRANSACTION_ROLLBACK"; break;
    }
    
    j["key"] = key;
    if (value.has_value()) {
        j["value"] = *value;
    } else {
        j["value"] = nullptr;
    }
    j["timestamp_ms"] = timestamp_ms;
    j["metadata"] = metadata;
    
    return j;
}

Changefeed::ChangeEvent Changefeed::ChangeEvent::fromJson(const nlohmann::json& j) {
    ChangeEvent event;
    event.sequence = j.value("sequence", uint64_t(0));
    
    // Parse ChangeEventType
    std::string type_str = j.value("type", "PUT");
    if (type_str == "PUT") event.type = ChangeEventType::EVENT_PUT;
    else if (type_str == "DELETE") event.type = ChangeEventType::EVENT_DELETE;
    else if (type_str == "TRANSACTION_COMMIT") event.type = ChangeEventType::EVENT_TRANSACTION_COMMIT;
    else if (type_str == "TRANSACTION_ROLLBACK") event.type = ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    else event.type = ChangeEventType::EVENT_PUT; // default
    
    event.key = j.value("key", "");
    
    if (j.contains("value") && !j["value"].is_null()) {
        event.value = j["value"].get<std::string>();
    }
    
    event.timestamp_ms = j.value("timestamp_ms", int64_t(0));
    
    if (j.contains("metadata")) {
        event.metadata = j["metadata"];
    }
    
    return event;
}

// ===== Changefeed Implementation =====

Changefeed::Changefeed(rocksdb::TransactionDB* db, 
                       rocksdb::ColumnFamilyHandle* cf,
                       RetentionPolicy retention)
    : db_(db), cf_(cf), retention_policy_(std::move(retention)) {
    if (!db_) {
        throw std::invalid_argument("Changefeed: db cannot be null");
    }
    
    // Start retention cleanup if enabled
    if (retention_policy_.enabled) {
        startRetentionCleanup();
    }
}

Changefeed::~Changefeed() {
    stopRetentionCleanup();
}

std::string Changefeed::makeKey(uint64_t sequence) const {
    // Zero-pad sequence for lexicographic ordering
    char buf[128];
    snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, (unsigned long long)sequence);
    return std::string(buf);
}

uint64_t Changefeed::nextSequence() {
    // Protect read-modify-write sequence with mutex to prevent race conditions
    // TODO: Consider using RocksDB merge operator for better performance
    std::lock_guard<std::mutex> lock(sequence_mutex_);
    
    std::string seq_value;
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
    } else {
        s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
    }
    
    uint64_t next_seq = 1;
    if (s.ok() && !seq_value.empty()) {
        next_seq = std::stoull(seq_value) + 1;
    }
    
    // Write back incremented sequence
    rocksdb::WriteOptions write_opts;
    std::string next_seq_str = std::to_string(next_seq);
    
    rocksdb::Status write_status;
    if (cf_) {
        write_status = db_->Put(write_opts, cf_, SEQUENCE_KEY, next_seq_str);
    } else {
        write_status = db_->Put(write_opts, SEQUENCE_KEY, next_seq_str);
    }
    
    if (!write_status.ok()) {
        THEMIS_ERROR("Failed to update sequence counter: {}", write_status.ToString());
        throw error::sequenceGenerationFailed(write_status.ToString());
    }
    
    return next_seq;
}

Changefeed::ChangeEvent Changefeed::recordEvent(ChangeEvent event) {
    // Assign sequence number
    event.sequence = nextSequence();
    
    // Set timestamp if not set
    if (event.timestamp_ms == 0) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }
    
    // Serialize to JSON
    std::string value = event.toJson().dump();
    std::string key = makeKey(event.sequence);
    
    // Store in RocksDB
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to record change event {}: {}", event.sequence, s.ToString());
        throw error::eventRecordFailed(s.ToString());
    }
    
    THEMIS_DEBUG("Recorded change event {} (type={}, key={})", 
                 event.sequence, static_cast<int>(event.type), event.key);
    
    return event;
}

std::vector<Changefeed::ChangeEvent> Changefeed::listEvents(const ListOptions& options) const {
    std::vector<ChangeEvent> results;
    
    // Long-poll support: wait for events if none available
    if (options.long_poll_ms > 0) {
        uint64_t latest = getLatestSequence();
        if (latest <= options.from_sequence) {
            // No new events, wait
            waitForEvents(options.from_sequence, options.long_poll_ms);
        }
    }
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    // Start from requested sequence
    std::string start_key = makeKey(options.from_sequence + 1);
    it->Seek(start_key);
    
    size_t count = 0;
    for (; it->Valid() && count < options.limit; it->Next()) {
        std::string key = it->key().ToString();
        
        // Stop if we've left the changefeed prefix
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson(j);
            
            // Apply filters
            bool matches = true;
            
            if (options.key_prefix.has_value() &&
                event.key.find(*options.key_prefix) != 0) {
                matches = false;
            }
            
            if (options.event_type.has_value() &&
                event.type != *options.event_type) {
                matches = false;
            }
            
            if (matches) {
                results.push_back(event);
                count++;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse change event at key {}: {}", key, e.what());
            continue;
        }
    }
    
    return results;
}

std::vector<Changefeed::ChangeEvent> Changefeed::listEvents() const {
    return listEvents(ListOptions{});
}

uint64_t Changefeed::getLatestSequence() const {
    std::string seq_value;
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
    } else {
        s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
    }
    
    if (s.ok() && !seq_value.empty()) {
        return std::stoull(seq_value);
    }
    
    return 0;
}

bool Changefeed::waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const {
    // Simple polling implementation (production would use event notifications)
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (true) {
        uint64_t latest = getLatestSequence();
        if (latest > from_sequence) {
            return true; // New events available
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false; // Timeout
        }
        
        // Sleep for a short interval before checking again
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

Changefeed::Stats Changefeed::getStats() const {
    Stats stats{};
    stats.latest_sequence = getLatestSequence();
    stats.watermarks = getWatermarks();
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    for (; it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        stats.total_events++;
        stats.total_size_bytes += it->key().size() + it->value().size();
    }
    
    return stats;
}

void Changefeed::clear() {
    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    size_t count = 0;
    for (; it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        rocksdb::Status s;
        if (cf_) {
            s = db_->Delete(write_opts, cf_, key);
        } else {
            s = db_->Delete(write_opts, key);
        }
        
        if (s.ok()) {
            count++;
        }
    }
    
    // Reset sequence counter
    if (cf_) {
        db_->Put(write_opts, cf_, SEQUENCE_KEY, "0");
    } else {
        db_->Put(write_opts, SEQUENCE_KEY, "0");
    }
    
    THEMIS_INFO("Cleared {} change events", count);
}

Changefeed::ChangeEvent Changefeed::getEvent(uint64_t sequence) const {
    std::string key = makeKey(sequence);
    std::string value;
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(read_opts, cf_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }

    if (!s.ok()) {
        throw error::dbOperationFailed("getEvent", s.ToString());
    }

    nlohmann::json j = nlohmann::json::parse(value);
    return ChangeEvent::fromJson(j);
}

Changefeed::CompactionResult Changefeed::compactByKey() {
    CompactionResult result;

    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;

    // Phase 1: Scan all events and record the latest sequence per document key.
    // Use the rocksdb key string as the storage key (not the document key) to
    // iterate in sequence order so we naturally see events oldest-first.
    std::unordered_map<std::string, uint64_t> latest_seq_per_doc_key;

    {
        std::unique_ptr<rocksdb::Iterator> it;
        if (cf_) {
            it.reset(db_->NewIterator(read_opts, cf_));
        } else {
            it.reset(db_->NewIterator(read_opts));
        }

        it->Seek(KEY_PREFIX);
        for (; it->Valid(); it->Next()) {
            std::string k = it->key().ToString();
            if (k.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
                break;
            }

            result.events_scanned++;
            try {
                nlohmann::json j = nlohmann::json::parse(it->value().ToString());
                ChangeEvent ev = ChangeEvent::fromJson(j);
                // Always overwrite: later iterations have higher sequence numbers
                // because we iterate in key (sequence) order.
                latest_seq_per_doc_key[ev.key] = ev.sequence;
            } catch (const std::exception& e) {
                THEMIS_WARN("compactByKey: failed to parse event at {}: {}", k, e.what());
            }
        }
    }

    // Phase 2: Re-scan and delete any event that is NOT the latest for its key,
    // unless it is a DELETE event (tombstone must be preserved for consumers).
    {
        std::unique_ptr<rocksdb::Iterator> it;
        if (cf_) {
            it.reset(db_->NewIterator(read_opts, cf_));
        } else {
            it.reset(db_->NewIterator(read_opts));
        }

        it->Seek(KEY_PREFIX);
        std::unordered_set<std::string> compacted_keys;

        for (; it->Valid(); it->Next()) {
            std::string k = it->key().ToString();
            if (k.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
                break;
            }

            try {
                nlohmann::json j = nlohmann::json::parse(it->value().ToString());
                ChangeEvent ev = ChangeEvent::fromJson(j);

                auto it_latest = latest_seq_per_doc_key.find(ev.key);
                if (it_latest == latest_seq_per_doc_key.end()) {
                    // This can happen if the event was added after Phase 1 scan,
                    // or if Phase 1 failed to parse it.  Conservatively keep it.
                    THEMIS_WARN("compactByKey: doc key '{}' not found in phase-1 map; "
                                "retaining event seq={}", ev.key, ev.sequence);
                    result.events_retained++;
                    continue;
                }

                bool is_latest = (ev.sequence == it_latest->second);
                bool is_delete = (ev.type == ChangeEventType::EVENT_DELETE);

                if (!is_latest && !is_delete) {
                    // Superseded — remove it
                    rocksdb::Status s;
                    if (cf_) {
                        s = db_->Delete(write_opts, cf_, k);
                    } else {
                        s = db_->Delete(write_opts, k);
                    }

                    if (s.ok()) {
                        result.events_deleted++;
                        compacted_keys.insert(ev.key);
                    } else {
                        THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
                        result.events_retained++;
                    }
                } else {
                    result.events_retained++;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("compactByKey: failed to parse event at {}: {}", k, e.what());
                result.events_retained++;
            }
        }

        result.keys_compacted = compacted_keys.size();
    }

    THEMIS_INFO("compactByKey: scanned={} deleted={} keys_compacted={} retained={}",
                result.events_scanned, result.events_deleted,
                result.keys_compacted, result.events_retained);
    return result;
}

size_t Changefeed::deleteOldEvents(uint64_t before_sequence) {
    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    size_t count = 0;
    for (; it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson(j);
            
            if (event.sequence < before_sequence) {
                rocksdb::Status s;
                if (cf_) {
                    s = db_->Delete(write_opts, cf_, key);
                } else {
                    s = db_->Delete(write_opts, key);
                }
                
                if (s.ok()) {
                    count++;
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse change event for deletion: {}", e.what());
            continue;
        }
    }
    
    THEMIS_INFO("Deleted {} old change events (before sequence {})", count, before_sequence);
    return count;
}

Changefeed::Watermarks Changefeed::getWatermarks() const {
    Watermarks wm{};
    wm.low_watermark = 0;
    wm.high_watermark = 0;
    wm.oldest_timestamp_ms = 0;
    wm.newest_timestamp_ms = 0;
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    // Find first (oldest) event
    it->Seek(KEY_PREFIX);
    if (it->Valid()) {
        std::string key = it->key().ToString();
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) == 0) {
            try {
                nlohmann::json j = nlohmann::json::parse(it->value().ToString());
                ChangeEvent event = ChangeEvent::fromJson(j);
                wm.low_watermark = event.sequence;
                wm.oldest_timestamp_ms = event.timestamp_ms;
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse oldest event: {}", e.what());
            }
        }
    }
    
    // Find last (newest) event - iterate to end
    it->SeekToLast();
    while (it->Valid()) {
        std::string key = it->key().ToString();
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) == 0) {
            try {
                nlohmann::json j = nlohmann::json::parse(it->value().ToString());
                ChangeEvent event = ChangeEvent::fromJson(j);
                wm.high_watermark = event.sequence;
                wm.newest_timestamp_ms = event.timestamp_ms;
                break;
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse newest event: {}", e.what());
                it->Prev();
            }
        } else {
            it->Prev();
        }
    }
    
    return wm;
}

size_t Changefeed::deleteOldEventsByTimestamp(int64_t before_timestamp_ms) {
    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    size_t count = 0;
    for (; it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson(j);
            
            if (event.timestamp_ms < before_timestamp_ms) {
                rocksdb::Status s;
                if (cf_) {
                    s = db_->Delete(write_opts, cf_, key);
                } else {
                    s = db_->Delete(write_opts, key);
                }
                
                if (s.ok()) {
                    count++;
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse change event for deletion: {}", e.what());
            continue;
        }
    }
    
    THEMIS_INFO("Deleted {} old change events (before timestamp {})", count, before_timestamp_ms);
    return count;
}

size_t Changefeed::applyRetentionPolicy() {
    // Take a local snapshot of the policy under the lock to avoid data races
    // with concurrent calls to updateRetentionPolicy().
    RetentionPolicy policy;
    {
        std::lock_guard<std::mutex> plk(retention_mutex_);
        policy = retention_policy_;
    }

    if (!policy.enabled) {
        return 0;
    }
    
    size_t total_deleted = 0;
    
    // Get current stats
    auto stats = getStats();
    auto wm = stats.watermarks;
    
    // Apply time-based retention
    if (policy.max_age_hours.count() > 0) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        auto cutoff_ms = now_ms - std::chrono::duration_cast<std::chrono::milliseconds>(
            policy.max_age_hours
        ).count();
        
        size_t deleted_by_time = deleteOldEventsByTimestamp(cutoff_ms);
        total_deleted += deleted_by_time;
        THEMIS_DEBUG("Retention: deleted {} events older than {}ms", deleted_by_time, cutoff_ms);
    }
    
    // Apply count-based retention
    if (stats.total_events > policy.max_event_count) {
        // Delete oldest events to get under the limit
        size_t to_delete = stats.total_events - policy.max_event_count;
        uint64_t cutoff_sequence = wm.low_watermark + to_delete;
        
        size_t deleted_by_count = deleteOldEvents(cutoff_sequence);
        total_deleted += deleted_by_count;
        THEMIS_DEBUG("Retention: deleted {} events to maintain count limit", deleted_by_count);
    }
    
    // Apply size-based retention
    if (stats.total_size_bytes > policy.max_size_bytes) {
        // Estimate how many events to delete based on average event size
        size_t avg_event_size = stats.total_events > 0 ? 
            (stats.total_size_bytes / stats.total_events) : 1024;
        size_t excess_bytes = stats.total_size_bytes - policy.max_size_bytes;
        
        // Add buffer to ensure we get under the limit (account for estimation error)
        constexpr size_t SIZE_RETENTION_BUFFER_EVENTS = 100;
        size_t events_to_delete = (excess_bytes / avg_event_size) + SIZE_RETENTION_BUFFER_EVENTS;
        
        uint64_t cutoff_sequence = wm.low_watermark + events_to_delete;
        size_t deleted_by_size = deleteOldEvents(cutoff_sequence);
        total_deleted += deleted_by_size;
        THEMIS_DEBUG("Retention: deleted {} events to maintain size limit", deleted_by_size);
    }
    
    if (total_deleted > 0) {
        THEMIS_INFO("Retention policy applied: deleted {} total events", total_deleted);
    }
    
    return total_deleted;
}

void Changefeed::updateRetentionPolicy(const RetentionPolicy& policy) {
    std::lock_guard<std::mutex> lock(retention_mutex_);
    retention_policy_ = policy;
    THEMIS_INFO("RetentionPolicy updated: enabled={} max_age_hours={} max_event_count={} compact_on_cleanup={}",
                policy.enabled, policy.max_age_hours.count(),
                policy.max_event_count, policy.compact_on_cleanup);
}

Changefeed::RetentionPolicy Changefeed::getRetentionPolicy() const {
    std::lock_guard<std::mutex> lock(retention_mutex_);
    return retention_policy_;
}

void Changefeed::startRetentionCleanup() {
    if (retention_thread_running_.exchange(true)) {
        THEMIS_WARN("Retention cleanup thread already running");
        return;
    }
    
    retention_thread_ = std::thread(&Changefeed::retentionCleanupThread, this);
    {
        std::lock_guard<std::mutex> lk(retention_mutex_);
        THEMIS_INFO("Started retention cleanup thread (interval: {}min)", 
                    retention_policy_.cleanup_interval.count());
    }
}

void Changefeed::stopRetentionCleanup() {
    if (!retention_thread_running_.exchange(false)) {
        return;
    }
    
    retention_cv_.notify_all();
    
    if (retention_thread_.joinable()) {
        retention_thread_.join();
    }
    
    THEMIS_INFO("Stopped retention cleanup thread");
}

void Changefeed::retentionCleanupThread() {
    constexpr int ERROR_RETRY_DELAY_SECONDS = 60;
    
    while (retention_thread_running_.load()) {
        try {
            // Apply TTL/count/size-based retention
            applyRetentionPolicy();

            // Take a local copy of the policy fields we need, holding the lock
            bool do_compact;
            {
                std::lock_guard<std::mutex> plk(retention_mutex_);
                do_compact = retention_policy_.compact_on_cleanup;
            }

            // If configured, also compact superseded entries by key
            if (do_compact) {
                auto cr = compactByKey();
                THEMIS_DEBUG("Retention compact pass: deleted={} keys_compacted={}",
                             cr.events_deleted, cr.keys_compacted);
            }
            
            // Wait for next cleanup interval
            std::unique_lock<std::mutex> lock(retention_mutex_);
            retention_cv_.wait_for(lock, retention_policy_.cleanup_interval, [this]() {
                return !retention_thread_running_.load();
            });
        } catch (const std::exception& e) {
            THEMIS_ERROR("Error in retention cleanup thread: {}", e.what());
            // Sleep before retrying to avoid tight loop on persistent errors
            std::this_thread::sleep_for(std::chrono::seconds(ERROR_RETRY_DELAY_SECONDS));
        }
    }
    
    THEMIS_DEBUG("Retention cleanup thread exiting");
}

} // namespace themis
