/**
 * @file changefeed.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=16, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/changefeed.h"
#include <stdexcept>
#include "cdc/cdc_error.h"
#include "utils/logger.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/merge_operator.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <rocksdb/merge_operator.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "cdc/cdc_error.h"
#include "utils/logger.h"

namespace themis {
using namespace themis::cdc;

// ---------------------------------------------------------------------------
// SequenceIncrementOperator
//
// An AssociativeMergeOperator that treats SEQUENCE_KEY as a little-endian
// uint64.  Each Merge operand is a little-endian uint64 delta (always 1 in
// practice).  Handles legacy decimal-string base values for backward
// compatibility with existing deployments.
// ---------------------------------------------------------------------------
namespace {

class SequenceIncrementOperator : public rocksdb::AssociativeMergeOperator {
  public:
    // Stateless RocksDB merge operator: no owned resources beyond the base class.
    ~SequenceIncrementOperator() override = default;

    bool Merge(const rocksdb::Slice & /*key*/, const rocksdb::Slice *existing_value, const rocksdb::Slice &value,
               std::string *new_value, rocksdb::Logger * /*logger*/) const override {
        // RocksDB passes immutable operand slices scoped to this callback. The
        // operator stores no mutable shared state, so these reads are thread-safe
        // without additional locking.
        uint64_t base = 0;
        if (existing_value != nullptr && !existing_value->empty()) {
            if ([[maybe_unused]] existing_value->size() == sizeof(uint64_t)) {
                // Binary little-endian uint64 (new format)
                memcpy(&base, existing_value->data(), sizeof(uint64_t));
            } else {
                // Legacy decimal-string format (backward compatibility)
                try {
                    base = std::stoull(std::string(existing_value->data(),
                                                   existing_value->size()));
                } catch (const std::string&) {
                    base = 0;
                } catch (const char*) {
                    base = 0;
                } catch (...) {
                    base = 0;
                }
            }
        }

        uint64_t delta = 0;
        if ([[maybe_unused]] value.size() == sizeof(uint64_t)) {
            memcpy(&delta, value.data(), sizeof(uint64_t));
        }

        const uint64_t result = base + delta;
        new_value->resize(sizeof(uint64_t));
        memcpy(&(*new_value)[0], &result, sizeof(uint64_t));
        return true;
    }

    const char *Name() const override {
        return "SequenceIncrementOperator";
    }
};

} // anonymous namespace

// ===== ChangeEvent JSON Serialization =====

nlohmann::json Changefeed::ChangeEvent::toJson() const {
    nlohmann::json j;
    j["sequence"] = sequence;

    // Convert ChangeEventType to string
    switch (type) {
        case ChangeEventType::EVENT_PUT:
            j["type"] = "PUT";
            break;
        case ChangeEventType::EVENT_DELETE:
            j["type"] = "DELETE";
            break;
        case ChangeEventType::EVENT_TRANSACTION_COMMIT:
            j["type"] = "TRANSACTION_COMMIT";
            break;
        case ChangeEventType::EVENT_TRANSACTION_ROLLBACK:
            j["type"] = "TRANSACTION_ROLLBACK";
            break;
    }

    j["key"] = key;
    if (value.has_value()) {
        j["value"] = *value;
    } else {
        j["value"] = nullptr;
    }
    j["timestamp_ms"] = timestamp_ms;
    j["metadata"]     = metadata;

    // Before/after document snapshots (optional; only present when enriched)
    if (before_snapshot.has_value()) {
        j["before_snapshot"] = *before_snapshot;
    }
    if (after_snapshot.has_value()) {
        j["after_snapshot"] = *after_snapshot;
    }

    // GDPR redaction marker (only written when true to keep the common case compact)
    if (redacted) {
        j["redacted"] = true;
    }

    return j;
}

Changefeed::ChangeEvent Changefeed::ChangeEvent::fromJson([[maybe_unused]] const nlohmann::json &j) {
    ChangeEvent event;
    event.sequence = j.value("sequence", uint64_t(0));

    // Parse ChangeEventType
    std::string type_str = j.value("type", "PUT");
    if (type_str == "PUT") {
        event.type = ChangeEventType::EVENT_PUT;
    } else if (type_str == "DELETE") {
        event.type = ChangeEventType::EVENT_DELETE;
    } else if (type_str == "TRANSACTION_COMMIT") {
        event.type = ChangeEventType::EVENT_TRANSACTION_COMMIT;
    } else if (type_str == "TRANSACTION_ROLLBACK") {
        event.type = ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    } else {
        event.type = ChangeEventType::EVENT_PUT; // default
    }

    event.key = j.value("key", "");

    if (j.contains("value") && !j["value"].is_null()) {
        event.value = j["value"].get<std::string>();
    }

    event.timestamp_ms = j.value("timestamp_ms", int64_t(0));

    if (j.contains("metadata")) {
        event.metadata = j["metadata"];
    }

    // Before/after document snapshots (optional enrichment fields)
    if (j.contains("before_snapshot") && j["before_snapshot"].is_string()) {
        event.before_snapshot = j["before_snapshot"].get<std::string>();
    }
    if (j.contains("after_snapshot") && j["after_snapshot"].is_string()) {
        event.after_snapshot = j["after_snapshot"].get<std::string>();
    }

    // GDPR redaction marker
    event.redacted = j.value("redacted", false);

    return event;
}

// ===== Changefeed Implementation =====

std::shared_ptr<rocksdb::MergeOperator> Changefeed::makeSequenceMergeOperator() {
    return std::make_shared<SequenceIncrementOperator>();
}

uint64_t Changefeed::loadInitialSequence() const {
    std::string seq_value;
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
    } else {
        s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
    }

    if (s.ok() && !seq_value.empty()) {
        // Binary little-endian uint64 format (new)
        if ([[maybe_unused]] seq_value.size() == sizeof(uint64_t)) {
            uint64_t val;
            memcpy(&val, seq_value.data(), sizeof(val));
            return val;
        }
        // Legacy decimal-string format (backward compatibility)
        try {
            return std::stoull(seq_value);
        } catch (const std::exception&) {
        } catch (...) {
        }
    }

    if (s.IsNotFound()) {
        return 0;
    }

    // Get failed (possibly due to unresolved Merge operands when the merge
    // operator was not registered at DB open time).  Fall back to scanning
    // stored events for the highest sequence number.
    THEMIS_WARN([[maybe_unused]] "Changefeed: Get(SEQUENCE_KEY) failed ({}); scanning events for max sequence", s.ToString());
    return scanMaxSequence();
}

uint64_t Changefeed::scanMaxSequence() const {
    uint64_t max_seq = 0;

    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }

    it->Seek(KEY_PREFIX);
    for (; it->Valid(); it->Next()) {
        const std::string k = it->key().ToString();
        if (k.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        try {
            const nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            const uint64_t seq     = j.value("sequence", uint64_t(0));
            if (seq > max_seq) {
                max_seq = seq;
            }
        } catch (const nlohmann::json::exception&) {
            // Skip unparseable entries
        } catch (const std::string&) {
            // Skip unparseable entries
        } catch (const char*) {
            // Skip unparseable entries
        } catch (...) {
            // Skip unparseable entries
        }
    }

    return max_seq;
}

Changefeed::Changefeed(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf, RetentionPolicy retention)
    : db_(db), cf_(cf), retention_policy_(std::move(retention)) {
    if (!db_) {
        throw std::invalid_argument("Changefeed: db cannot be null");
    }

    // Initialise the in-process atomic counter from the persisted value so
    // that sequence numbers are monotonically increasing across restarts.
    const uint64_t initial_sequence = loadInitialSequence();
    sequence_counter_.store(initial_sequence, std::memory_order_relaxed);
    persisted_sequence_.store(initial_sequence, std::memory_order_relaxed);

    // Keep Merge enabled by default and rely on the first Merge() status in
    // nextSequence() to self-disable when the DB/CF was opened without a merge
    // operator. TransactionDB option introspection can be unreliable across
    // wrappers and may falsely report no merge operator.
    // Detect merge operator support at construction time by querying the DB's
    // ColumnFamilyOptions.  This MUST happen before the first nextSequence()
    // call: calling db_->Merge() on a CF without a merge_operator triggers
    // RocksDB's background error handler (stop=1) which permanently blocks
    // all subsequent writes — even plain Put() calls.
    {
        bool merge_available = false;
        try {
            rocksdb::Options opts = cf_
                ? db_->GetOptions(cf_)
                : db_->GetOptions();
            merge_available = (opts.merge_operator != nullptr);
        } catch (const std::exception&) {
            // If introspection fails, default to disabled (safe — prevents error state).
            merge_available = false;
        } catch (const std::string&) {
            // If introspection fails, default to disabled (safe — prevents error state).
            merge_available = false;
        } catch (const char*) {
            // If introspection fails, default to disabled (safe — prevents error state).
            merge_available = false;
        }
        sequence_merge_supported_.store(merge_available, std::memory_order_relaxed);
    }

    // Start retention cleanup if enabled
    if (retention_policy_.enabled) {
        startRetentionCleanup();
    }
}

Changefeed::~Changefeed() noexcept {
    stopRetentionCleanup();
}

std::string Changefeed::makeKey([[maybe_unused]] uint64_t sequence) const {
    // Zero-pad sequence for lexicographic ordering
    char buf[128];
    snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(sequence));
    return std::string(buf);
}

uint64_t Changefeed::nextSequence() {
    // Atomically increment the in-process counter — lock-free, O(1).
    // No mutex needed; std::atomic<uint64_t> guarantees uniqueness across threads.
    const uint64_t seq = sequence_counter_.fetch_add(1, std::memory_order_relaxed) + 1;

    // Persist the increment to RocksDB via the SequenceIncrementOperator for
    // crash recovery.  Merge() is non-blocking from the caller perspective
    // (the operand is buffered in the LSM write-path and applied lazily).
    rocksdb::WriteOptions write_opts;

    if (sequence_merge_supported_.load(std::memory_order_acquire)) {
        const uint64_t delta = 1;
        const rocksdb::Slice delta_slice(reinterpret_cast<const char *>(&delta), sizeof(delta));

        rocksdb::Status s;
        if (cf_) {
            s = db_->Merge(write_opts, cf_, SEQUENCE_KEY, delta_slice);
        } else {
            s = db_->Merge(write_opts, SEQUENCE_KEY, delta_slice);
        }

        if (s.ok()) {
            uint64_t persisted = persisted_sequence_.load(std::memory_order_relaxed);
            while (persisted < seq
                   && !persisted_sequence_.compare_exchange_weak(persisted, seq, std::memory_order_relaxed)) {
            }
            return seq;
        }

        // Disable Merge path after first failure to avoid repeated write-path
        // errors on DBs/CFs opened without a merge operator.
        sequence_merge_supported_.store(false, std::memory_order_release);
        THEMIS_ERROR("Changefeed: failed to persist sequence via Merge: {}", s.ToString());
    }

    uint64_t persisted = persisted_sequence_.load(std::memory_order_acquire);
    if (seq <= persisted) {
        return seq;
    }

    std::lock_guard<std::mutex> lock(sequence_persist_mutex_);
    persisted = persisted_sequence_.load(std::memory_order_relaxed);
    if (seq <= persisted) {
        return seq;
    }

    std::string seq_value(sizeof(uint64_t), '\0');
    std::memcpy(seq_value.data(), &seq, sizeof(uint64_t));

    rocksdb::Status persist_status;
    if (cf_) {
        persist_status = db_->Put(write_opts, cf_, SEQUENCE_KEY, seq_value);
    } else {
        persist_status = db_->Put(write_opts, SEQUENCE_KEY, seq_value);
    }

    if (!persist_status.ok()) {
        THEMIS_ERROR("Changefeed: fallback Put persistence failed: {}", persist_status.ToString());
    } else {
        persisted_sequence_.store(seq, std::memory_order_release);
    }

    return seq;
}

Changefeed::ChangeEvent Changefeed::recordEvent([[maybe_unused]] ChangeEvent event) {
    // Assign sequence number
    event.sequence = nextSequence();

    // Set timestamp if not set
    if ([[maybe_unused]] event.timestamp_ms == 0) {
        auto now           = std::chrono::system_clock::now().time_since_epoch();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>([[maybe_unused]] now).count();
    }

    // Serialize to JSON
    std::string value = event.toJson().dump();
    std::string key   = makeKey([[maybe_unused]] event.sequence);

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
        throw error::eventRecordFailed([[maybe_unused]] s.ToString());
    }

    THEMIS_DEBUG("Recorded change event {} (type={}, key={})", event.sequence, static_cast<int>(event.type), event.key);

    notifySubscribers([[maybe_unused]] event);

    return event;
}

std::vector<Changefeed::ChangeEvent> Changefeed::listEvents([[maybe_unused]] const ListOptions &options) const {
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
    for (; it->Valid(); it->Next()) {
        std::string key = it->key().ToString();

        // Stop if we've left the changefeed prefix
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }

        // Check limit before parsing
        if (count >= options.limit) {
            break;
        }

        // Apply to_sequence upper bound using the RocksDB key (no JSON parse needed).
        // Keys are formatted as KEY_PREFIX + zero-padded 20-digit sequence, and RocksDB
        // iterates in lexicographic (== numeric) order, so we can break early here.
        if (options.to_sequence > 0) {
            const char *seq_start = key.c_str() + strlen(KEY_PREFIX);
            char *end_ptr         = nullptr;
            uint64_t key_seq      = std::strtoull(seq_start, &end_ptr, 10);
            if (end_ptr != seq_start && key_seq > options.to_sequence) {
                break;
            }
        }

        try {
            nlohmann::json j  = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson([[maybe_unused]] j);

            // Apply filters
            bool matches = true;

            if ([[maybe_unused]] options.key_prefix.has_value() && event.key.find(*options.key_prefix) != 0) {
                matches = false;
            }

            // Multi-type filter takes precedence; fall back to legacy single-type filter
            if ([[maybe_unused]] !options.event_types.empty()) {
                if ([[maybe_unused]] options.event_types.find(event.type) == options.event_types.end()) {
                    matches = false;
                }
            } else if ([[maybe_unused]] options.event_type.has_value() && event.type != *options.event_type) {
                matches = false;
            }

            if (matches) {
                results.push_back([[maybe_unused]] event);
                count++;
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to parse change event at key {}: {}", key, e.what());
        } catch (...) {
            THEMIS_WARN("Failed to parse change event at key {} due to unknown exception", key);
        }
    }

    return results;
}

std::vector<Changefeed::ChangeEvent> Changefeed::listEvents() const {
    return listEvents([[maybe_unused]] ListOptions{});
}

uint64_t Changefeed::getLatestSequence() const {
    // Return the in-process atomic counter directly — no DB round-trip needed.
    return sequence_counter_.load(std::memory_order_relaxed);
}

bool Changefeed::waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const {
    // Simple polling implementation (production would use event notifications)
    auto start   = std::chrono::steady_clock::now();
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
    stats.watermarks      = getWatermarks();

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

    // Reset the in-process counter and the persisted RocksDB base value.
    // Using Put() overwrites any prior Merge operands so that subsequent
    // Merge(+1) calls start from zero again.
    sequence_counter_.store(0, std::memory_order_relaxed);
    const uint64_t zero = 0;
    const std::string zero_bytes(reinterpret_cast<const char *>(&zero), sizeof(zero));
    if (cf_) {
        db_->Put(write_opts, cf_, SEQUENCE_KEY, zero_bytes);
    } else {
        db_->Put(write_opts, SEQUENCE_KEY, zero_bytes);
    }

    THEMIS_INFO("Cleared {} change events", count);
}

Changefeed::ChangeEvent Changefeed::getEvent([[maybe_unused]] uint64_t sequence) const {
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
    return ChangeEvent::fromJson([[maybe_unused]] j);
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
                ChangeEvent ev   = ChangeEvent::fromJson([[maybe_unused]] j);
                // Always overwrite: later iterations have higher sequence numbers
                // because we iterate in key (sequence) order.
                latest_seq_per_doc_key[ev.key] = ev.sequence;
            } catch (const std::exception &e) {
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
                ChangeEvent ev   = ChangeEvent::fromJson([[maybe_unused]] j);

                auto it_latest = latest_seq_per_doc_key.find(ev.key);
                if (it_latest == latest_seq_per_doc_key.end()) {
                    // This can happen if the event was added after Phase 1 scan,
                    // or if Phase 1 failed to parse it.  Conservatively keep it.
                    THEMIS_WARN("compactByKey: doc key '{}' not found in phase-1 map; "
                                "retaining event seq={}",
                                ev.key, ev.sequence);
                    result.events_retained++;
                    continue;
                }

                bool is_latest = (ev.sequence == it_latest->second);
                bool is_delete = ([[maybe_unused]] ev.type == ChangeEventType::EVENT_DELETE);

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
            } catch (const std::exception &e) {
                THEMIS_WARN("compactByKey: failed to parse event at {}: {}", k, e.what());
                result.events_retained++;
            }
        }

        result.keys_compacted = compacted_keys.size();
    }

    THEMIS_INFO("compactByKey: scanned={} deleted={} keys_compacted={} retained={}", result.events_scanned,
                result.events_deleted, result.keys_compacted, result.events_retained);
    return result;
}

Changefeed::RedactionResult Changefeed::redactByKeyPrefix(const std::string &key_prefix) {
    if (key_prefix.empty()) {
        throw error::invalidArgument("redactByKeyPrefix: key_prefix cannot be empty");
    }

    RedactionResult result;

    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;
    std::unique_ptr<rocksdb::Iterator> it;

    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }

    // Seek to the first changefeed entry (sequence 1)
    it->Seek(makeKey(1));

    for (; it->Valid(); it->Next()) {
        const std::string rocksdb_key = it->key().ToString();

        // Stop as soon as we leave the changefeed key-space
        if (rocksdb_key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }

        result.events_scanned++;

        try {
            nlohmann::json j  = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson([[maybe_unused]] j);

            // Skip events whose key does not start with key_prefix
            if (event.key.compare(0, key_prefix.size(), key_prefix) != 0) {
                continue;
            }

            // Skip events that are already redacted
            if ([[maybe_unused]] event.redacted) {
                continue;
            }

            // Scrub PII-bearing fields; preserve audit-critical fields
            const std::string affected_key = event.key;
            event.value                    = "[REDACTED]";
            event.before_snapshot          = std::nullopt;
            event.after_snapshot           = std::nullopt;
            event.redacted                 = true;

            const std::string new_value = event.toJson().dump();
            rocksdb::Status s;
            if (cf_) {
                s = db_->Put(write_opts, cf_, rocksdb_key, new_value);
            } else {
                s = db_->Put(write_opts, rocksdb_key, new_value);
            }

            if (!s.ok()) {
                THEMIS_ERROR("redactByKeyPrefix: failed to overwrite event {}: {}", rocksdb_key, s.ToString());
                continue;
            }

            result.events_redacted++;
            result.affected_keys.push_back(affected_key);

        } catch (const std::exception &e) {
            THEMIS_WARN("redactByKeyPrefix: failed to parse event at {}: {}", rocksdb_key, e.what());
        }
    }

    THEMIS_INFO("redactByKeyPrefix: key_prefix='{}' scanned={} redacted={}", key_prefix, result.events_scanned,
                result.events_redacted);
    return result;
}

size_t Changefeed::deleteOldEvents([[maybe_unused]] uint64_t before_sequence) {
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
            nlohmann::json j  = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson([[maybe_unused]] j);

            if ([[maybe_unused]] event.sequence < before_sequence) {
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
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to parse change event for deletion: {}", e.what());
            continue;
        }
    }

    THEMIS_INFO([[maybe_unused]] "Deleted {} old change events (before sequence {})", count, before_sequence);
    return count;
}

Changefeed::Watermarks Changefeed::getWatermarks() const {
    Watermarks wm{};
    wm.low_watermark       = 0;
    wm.high_watermark      = 0;
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
                nlohmann::json j       = nlohmann::json::parse(it->value().ToString());
                ChangeEvent event      = ChangeEvent::fromJson([[maybe_unused]] j);
                wm.low_watermark       = event.sequence;
                wm.oldest_timestamp_ms = event.timestamp_ms;
            } catch (const std::exception &e) {
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
                nlohmann::json j       = nlohmann::json::parse(it->value().ToString());
                ChangeEvent event      = ChangeEvent::fromJson([[maybe_unused]] j);
                wm.high_watermark      = event.sequence;
                wm.newest_timestamp_ms = event.timestamp_ms;
                break;
            } catch (const std::exception &e) {
                THEMIS_WARN("Failed to parse newest event: {}", e.what());
                it->Prev();
            }
        } else {
            it->Prev();
        }
    }

    return wm;
}

size_t Changefeed::deleteOldEventsByTimestamp([[maybe_unused]] int64_t before_timestamp_ms) {
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
            nlohmann::json j  = nlohmann::json::parse(it->value().ToString());
            ChangeEvent event = ChangeEvent::fromJson([[maybe_unused]] j);

            if ([[maybe_unused]] event.timestamp_ms < before_timestamp_ms) {
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
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to parse change event for deletion: {}", e.what());
            continue;
        }
    }

    THEMIS_INFO([[maybe_unused]] "Deleted {} old change events (before timestamp {})", count, before_timestamp_ms);
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
    auto wm    = stats.watermarks;

    // Apply time-based retention
    if (policy.max_age_hours.count() > 0) {
        auto now       = std::chrono::system_clock::now().time_since_epoch();
        auto now_ms    = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        auto cutoff_ms = now_ms - std::chrono::duration_cast<std::chrono::milliseconds>(policy.max_age_hours).count();

        size_t deleted_by_time = deleteOldEventsByTimestamp([[maybe_unused]] cutoff_ms);
        total_deleted += deleted_by_time;
        THEMIS_DEBUG("Retention: deleted {} events older than {}ms", deleted_by_time, cutoff_ms);
    }

    // Apply count-based retention
    if ([[maybe_unused]] stats.total_events > policy.max_event_count) {
        // Delete oldest events to get under the limit
        size_t to_delete         = stats.total_events - policy.max_event_count;
        uint64_t cutoff_sequence = wm.low_watermark + to_delete;

        size_t deleted_by_count = deleteOldEvents([[maybe_unused]] cutoff_sequence);
        total_deleted += deleted_by_count;
        THEMIS_DEBUG("Retention: deleted {} events to maintain count limit", deleted_by_count);
    }

    // Apply size-based retention
    if (stats.total_size_bytes > policy.max_size_bytes) {
        // Estimate how many events to delete based on average event size
        size_t avg_event_size = stats.total_events > 0 ? (stats.total_size_bytes / stats.total_events) : 1024;
        size_t excess_bytes   = stats.total_size_bytes - policy.max_size_bytes;

        // Add buffer to ensure we get under the limit (account for estimation error)
        constexpr size_t SIZE_RETENTION_BUFFER_EVENTS = 100;
        size_t events_to_delete                       = (excess_bytes / avg_event_size) + SIZE_RETENTION_BUFFER_EVENTS;

        uint64_t cutoff_sequence = wm.low_watermark + events_to_delete;
        size_t deleted_by_size   = deleteOldEvents([[maybe_unused]] cutoff_sequence);
        total_deleted += deleted_by_size;
        THEMIS_DEBUG("Retention: deleted {} events to maintain size limit", deleted_by_size);
    }

    if (total_deleted > 0) {
        THEMIS_INFO("Retention policy applied: deleted {} total events", total_deleted);
    }

    return total_deleted;
}

void Changefeed::updateRetentionPolicy(const RetentionPolicy &policy) {
    bool was_enabled;
    {
        std::lock_guard<std::mutex> lock(retention_mutex_);
        was_enabled       = retention_policy_.enabled;
        retention_policy_ = policy;
    }
    THEMIS_INFO("RetentionPolicy updated: enabled={} max_age_hours={} max_event_count={} compact_on_cleanup={}",
                policy.enabled, policy.max_age_hours.count(), policy.max_event_count, policy.compact_on_cleanup);

    if (policy.enabled && !was_enabled) {
        // Retention was disabled; start the background cleanup thread now.
        startRetentionCleanup();
    } else if (!policy.enabled && was_enabled) {
        // Retention was enabled; stop the background cleanup thread.
        stopRetentionCleanup();
    } else if (policy.enabled) {
        // Policy remains enabled but settings may have changed (e.g. new interval).
        // Wake the thread so it picks up the updated configuration immediately.
        retention_cv_.notify_all();
    }
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
        THEMIS_INFO("Started retention cleanup thread (interval: {}min)", retention_policy_.cleanup_interval.count());
    }
}

void Changefeed::stopRetentionCleanup() {
    if (!retention_thread_running_.exchange(false)) {
        return;
    }

    retention_cv_.notify_all();

    if (retention_thread_.joinable()) {
        // Safe blocking join: the stop flag is already cleared and notify_all()
        // wakes both the normal cleanup wait and the error-backoff wait below,
        // so the worker has no indefinite blocking point after shutdown begins.
        retention_thread_.join();
    }

    THEMIS_INFO("Stopped retention cleanup thread");
}

bool Changefeed::isRetentionCleanupRunning() const noexcept {
    return retention_thread_running_.load();
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
                THEMIS_DEBUG("Retention compact pass: deleted={} keys_compacted={}", cr.events_deleted,
                             cr.keys_compacted);
            }

            // Wait for next cleanup interval
            std::unique_lock<std::mutex> lock(retention_mutex_);
            retention_cv_.wait_for(lock, retention_policy_.cleanup_interval,
                                   [this]() { return !retention_thread_running_.load(); });
        } catch (const std::exception &e) {
            THEMIS_ERROR("Error in retention cleanup thread: {}", e.what());
            // Wait with the condition variable so shutdown can interrupt the
            // retry delay instead of leaving stopRetentionCleanup() blocked.
            std::unique_lock<std::mutex> lock(retention_mutex_);
            retention_cv_.wait_for(lock, std::chrono::seconds(ERROR_RETRY_DELAY_SECONDS),
                                   [this]() { return !retention_thread_running_.load(); });
        }
    }

    THEMIS_DEBUG("Retention cleanup thread exiting");
}

// ---------------------------------------------------------------------------
// Push-based subscription API
// ---------------------------------------------------------------------------

bool Changefeed::SubscriptionFilter::matches(const ChangeEvent &ev) const noexcept {
    if (!key_prefix.empty() && ev.key.rfind(key_prefix, 0) != 0) {
        return false;
    }
    if ([[maybe_unused]] !event_types.empty() && event_types.find(ev.type) == event_types.end()) {
        return false;
    }
    return true;
}

Changefeed::SubscriptionHandle Changefeed::subscribe(SubscriptionFilter filter, SubscriptionCallback callback) {
    const uint64_t id = next_subscription_id_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lk(subscriptions_mutex_);
        subscriptions_.emplace(id, SubscriptionEntry{std::move(filter), std::move(callback)});
        subscription_count_.fetch_add(1, std::memory_order_release);
    }
    THEMIS_DEBUG("Changefeed: subscription {} registered", id);
    return SubscriptionHandle{this, id};
}

void Changefeed::unsubscribe([[maybe_unused]] uint64_t subscription_id) noexcept {
    std::lock_guard<std::mutex> lk(subscriptions_mutex_);
    if (subscriptions_.erase(subscription_id) > 0) {
        subscription_count_.fetch_sub(1, std::memory_order_release);
    }
    THEMIS_DEBUG("Changefeed: subscription {} cancelled", subscription_id);
}

void Changefeed::notifySubscribers([[maybe_unused]] const ChangeEvent &event) {
    // Fast path: skip snapshot + mutex acquisition when no subscribers registered.
    if (subscription_count_.load(std::memory_order_acquire) == 0) {
        return;
    }

    // Take a snapshot of the current subscriber map under the lock so that
    // callbacks can themselves call subscribe/unsubscribe without deadlocking.
    std::vector<SubscriptionEntry> snapshot;
    {
        std::lock_guard<std::mutex> lk(subscriptions_mutex_);
        snapshot.reserve(subscriptions_.size());
        for (const auto &[id, entry] : subscriptions_) {
            snapshot.push_back(entry);
        }
    }

    for (const auto &entry : snapshot) {
        if ([[maybe_unused]] entry.filter.matches(event)) {
            try {
                entry.callback([[maybe_unused]] event);
            } catch (const std::exception &ex) {
                // Callbacks must not throw; log and continue.
                THEMIS_WARN("Changefeed: subscriber callback threw an exception: {} - ignored", ex.what());
            } catch (const char *ex) {
                // Callbacks must not throw; log and continue.
                THEMIS_WARN("Changefeed: subscriber callback threw an exception: {} - ignored",
                            (ex ? ex : "<null>"));
            } catch (const std::string &ex) {
                // Callbacks must not throw; log and continue.
                THEMIS_WARN("Changefeed: subscriber callback threw an exception: {} - ignored", ex);
            } catch (...) {
                // Callbacks must not throw; log and continue.
                THEMIS_WARN([[maybe_unused]] "Changefeed: subscriber callback threw an unknown exception - ignored");
            }
        }
    }
}

void Changefeed::SubscriptionHandle::cancel() noexcept {
    if (feed_) {
        feed_->unsubscribe(id_);
        feed_ = nullptr;
        id_   = 0;
    }
}

} // namespace themis

