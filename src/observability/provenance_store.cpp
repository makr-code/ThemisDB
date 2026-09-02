/**
 * @file provenance_store.cpp
 * @brief RocksDB-backed implementation for persistent provenance storage (GAP-4.1).
 */

#include "observability/provenance_store.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace themis::observability {

using json = nlohmann::json;

namespace {

// Key format: "provenance:<query_id>:<step_number:08d>"
[[nodiscard]] std::string makeProvenanceKey(const std::string& query_id, int step_number) {
    std::ostringstream oss;
    oss << "provenance:" << query_id << ":" << std::setfill('0') << std::setw(8)
        << step_number;
    return oss.str();
}

// Time-index key: "provenance_ts:<timestamp_ms:016x>:<query_id>:<step_number:08d>"
[[nodiscard]] std::string makeTimeIndexKey(int64_t timestamp_ms,
                                           const std::string& query_id,
                                           int step_number) {
    std::ostringstream oss;
    oss << "provenance_ts:" << std::setfill('0') << std::setw(16) << std::hex << timestamp_ms
        << ":" << query_id << ":" << std::setfill('0') << std::setw(8) << std::dec
        << step_number;
    return oss.str();
}

[[nodiscard]] int64_t nowMsSinceEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

struct ParsedTimeIndexKey {
    int64_t timestamp_ms = 0;
    std::string query_id;
    int step_number = -1;
};

[[nodiscard]] std::optional<ParsedTimeIndexKey> parseTimeIndexKey(const std::string& key) {
    constexpr char kPrefix[] = "provenance_ts:";
    if (key.rfind(kPrefix, 0) != 0) {
        return std::nullopt;
    }

    const auto first_colon_after_ts = key.find(':', sizeof(kPrefix) - 1);
    if (first_colon_after_ts == std::string::npos) {
        return std::nullopt;
    }

    ParsedTimeIndexKey parsed;
    try {
        const auto ts_hex = key.substr(sizeof(kPrefix) - 1,
                                       first_colon_after_ts - (sizeof(kPrefix) - 1));
        parsed.timestamp_ms = std::stoll(ts_hex, nullptr, 16);
    } catch (...) {
        return std::nullopt;
    }

    const auto last_colon = key.rfind(':');
    if (last_colon == std::string::npos || last_colon <= first_colon_after_ts) {
        parsed.query_id = key.substr(first_colon_after_ts + 1);
        return parsed;
    }

    parsed.query_id = key.substr(first_colon_after_ts + 1,
                                 last_colon - first_colon_after_ts - 1);
    try {
        parsed.step_number = std::stoi(key.substr(last_colon + 1));
    } catch (...) {
        parsed.step_number = -1;
    }
    return parsed;
}

// Serialize a provenance record to JSON
[[nodiscard]] json serializeRecord(const ProvenanceStepRecord& record) {
    json j;
    j["query_id"]                 = record.query_id;
    j["step_number"]              = record.step_number;
    j["layer_name"]               = record.layer_name;
    j["timestamp_ms"]             = record.timestamp_ms;
    j["correlation_id"]           = record.correlation_id;
    j["source_layer"]             = record.source_layer;
    j["input_vector_hash"]        = record.input_vector_hash;
    j["num_candidates"]           = record.num_candidates;
    j["num_selected"]             = record.num_selected;
    j["shard_id"]                 = record.shard_id;
    j["backend_name"]             = record.backend_name;
    j["routing_reason_code"]      = record.routing_reason_code;
    j["fallback_mode"]            = record.fallback_mode;
    j["confidence_policy_version"]= record.confidence_policy_version;
    j["decision_duration_us"]     = record.decision_duration_us;
    return j;
}

// Deserialize a provenance record from JSON
[[nodiscard]] ProvenanceStepRecord deserializeRecord(const std::string& json_str) {
    ProvenanceStepRecord record;
    try {
        const auto j = json::parse(json_str);
        record.query_id                  = j.value("query_id", "");
        record.step_number               = j.value("step_number", 0);
        record.layer_name                = j.value("layer_name", "");
        record.timestamp_ms              = j.value("timestamp_ms", int64_t{0});
        record.correlation_id            = j.value("correlation_id", "");
        record.source_layer              = j.value("source_layer", "");
        record.input_vector_hash         = j.value("input_vector_hash", "");
        record.num_candidates            = j.value("num_candidates", int64_t{0});
        record.num_selected              = j.value("num_selected", int64_t{0});
        record.shard_id                  = j.value("shard_id", "");
        record.backend_name              = j.value("backend_name", "");
        record.routing_reason_code       = j.value("routing_reason_code", "");
        record.fallback_mode             = j.value("fallback_mode", "");
        record.confidence_policy_version = j.value("confidence_policy_version", "");
        record.decision_duration_us      = j.value("decision_duration_us", int64_t{0});
    } catch (const std::exception&) {
        // Gracefully handle parse errors
    }
    return record;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RocksDBProvenanceStore::Impl (private pimpl)
// ─────────────────────────────────────────────────────────────────────────────

/** @brief RocksDBProvenanceStore::Impl (private pimpl). */
class RocksDBProvenanceStore::Impl {
public:
    explicit Impl(const RocksDBProvenanceStore::Config& config) {
        rocksdb::Options options;
        options.create_if_missing = true;

        if (config.enable_compression) {
            options.compression = rocksdb::kSnappyCompression;
        }

        std::unique_ptr<rocksdb::DB> db_raw;
        const auto status = rocksdb::DB::Open(options, config.db_path, &db_raw);

        if (!status.ok()) {
            throw std::runtime_error(std::string("Failed to open RocksDB: ") + status.ToString());
        }

        db_ = std::move(db_raw);
        config_ = config;
    }

    [[nodiscard]] rocksdb::DB* getDb() const { return db_.get(); }

    [[nodiscard]] const RocksDBProvenanceStore::Config& config() const { return config_; }

private:
    std::unique_ptr<rocksdb::DB> db_;
    RocksDBProvenanceStore::Config config_;
};

namespace {

void addDeletionForTimeIndexKey(rocksdb::WriteBatch& batch, const std::string& time_index_key) {
    const auto parsed = parseTimeIndexKey(time_index_key);
    if (!parsed.has_value()) {
        return;
    }

    batch.Delete(time_index_key);
    if (parsed->step_number >= 0) {
        batch.Delete(makeProvenanceKey(parsed->query_id, parsed->step_number));
    }
}

[[nodiscard]] bool applyRetentionPolicies(rocksdb::DB* db,
                                          const RocksDBProvenanceStore::Config& config) {
    if (config.retention_max_records == 0 && config.retention_max_age_ms == 0) {
        return true;
    }

    rocksdb::WriteBatch batch;
    bool has_changes = false;

    constexpr char kTimePrefix[] = "provenance_ts:";
    if (config.retention_max_age_ms > 0) {
        const auto cutoff_ts = nowMsSinceEpoch() - config.retention_max_age_ms;
        std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
        for (it->Seek(kTimePrefix); it->Valid(); it->Next()) {
            const auto key = it->key().ToString();
            const auto parsed = parseTimeIndexKey(key);
            if (!parsed.has_value()) {
                if (key.rfind(kTimePrefix, 0) != 0) {
                    break;
                }
                continue;
            }
            if (parsed->timestamp_ms > cutoff_ts) {
                break;
            }
            addDeletionForTimeIndexKey(batch, key);
            has_changes = true;
        }
    }

    if (config.retention_max_records > 0) {
        std::vector<std::string> time_keys;
        std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
        for (it->Seek(kTimePrefix); it->Valid(); it->Next()) {
            const auto key = it->key().ToString();
            if (key.rfind(kTimePrefix, 0) != 0) {
                break;
            }
            if (parseTimeIndexKey(key).has_value()) {
                time_keys.push_back(key);
            }
        }

        if (time_keys.size() > config.retention_max_records) {
            const auto to_delete = time_keys.size() - config.retention_max_records;
            for (std::size_t i = 0; i < to_delete; ++i) {
                addDeletionForTimeIndexKey(batch, time_keys[i]);
                has_changes = true;
            }
        }
    }

    if (!has_changes) {
        return true;
    }

    const auto status = db->Write(rocksdb::WriteOptions(), &batch);
    return status.ok();
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RocksDBProvenanceStore (public interface)
// ─────────────────────────────────────────────────────────────────────────────

RocksDBProvenanceStore::RocksDBProvenanceStore(Config config)
    : impl_(std::make_unique<Impl>(config)) {}

RocksDBProvenanceStore::~RocksDBProvenanceStore() = default;

bool RocksDBProvenanceStore::storeRecord(const std::string& query_id,
                                          int step_number,
                                          const ProvenanceStepRecord& record) {
    try {
        auto* db = impl_->getDb();
        if (!db) {
            return false;
        }

        const auto prov_key = makeProvenanceKey(query_id, step_number);
        const auto time_key = makeTimeIndexKey(record.timestamp_ms, query_id, step_number);

        const auto json_str = serializeRecord(record).dump();

        // Store main record
        rocksdb::WriteOptions write_opts;
        write_opts.sync = false;  // Async writes for performance
        auto status = db->Put(write_opts, prov_key, json_str);

        if (!status.ok()) {
            return false;
        }

        // Store time-index entry (value can be empty; key is sufficient for range queries)
        status = db->Put(write_opts, time_key, "");
        if (!status.ok()) {
            return false;
        }

        return applyRetentionPolicies(db, impl_->config());
    } catch (...) {
        return false;
    }
}

std::optional<ProvenanceStepRecord> RocksDBProvenanceStore::getRecord(
    const std::string& query_id, int step_number) {
    try {
        auto* db = impl_->getDb();
        if (!db) {
            return std::nullopt;
        }

        const auto key = makeProvenanceKey(query_id, step_number);
        std::string value;
        const auto status = db->Get(rocksdb::ReadOptions(), key, &value);

        if (!status.ok()) {
            return std::nullopt;
        }

        return deserializeRecord(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<ProvenanceStepRecord> RocksDBProvenanceStore::getProvenanceChain(
    const std::string& query_id) {
    std::vector<ProvenanceStepRecord> records;

    try {
        auto* db = impl_->getDb();
        if (!db) {
            return records;
        }

        // Range scan: "provenance:<query_id>:00000000" to "provenance:<query_id>:99999999"
        const auto prefix = "provenance:" + query_id + ":";
        // Wrap in unique_ptr: iterator freed on all paths including exceptions (Phase 8.4).
        auto it = std::unique_ptr<rocksdb::Iterator>(db->NewIterator(rocksdb::ReadOptions()));

        for (it->Seek(prefix); it->Valid() && it->key().ToString().find(prefix) == 0;
             it->Next()) {
            const auto value_str = it->value().ToString();
            records.push_back(deserializeRecord(value_str));
        }

        // Sort by step_number (should already be in order due to key format, but ensure it)
        std::sort(records.begin(), records.end(),
                  [](const ProvenanceStepRecord& a, const ProvenanceStepRecord& b) {
                      return a.step_number < b.step_number;
                  });
    } catch (...) {
        // Silently return what we have on error
    }

    return records;
}

std::vector<ProvenanceStepRecord> RocksDBProvenanceStore::getRecordsByTimeRange(
    int64_t start_ts_ms, int64_t end_ts_ms) {
    std::vector<ProvenanceStepRecord> records;

    try {
        auto* db = impl_->getDb();
        if (!db) {
            return records;
        }

        // Range scan on time-index: "provenance_ts:<start_ts:016x>:..."
        std::ostringstream oss_start, oss_end;
        oss_start << "provenance_ts:" << std::setfill('0') << std::setw(16) << std::hex
                  << start_ts_ms;
        oss_end << "provenance_ts:" << std::setfill('0') << std::setw(16) << std::hex
                << end_ts_ms;

        const auto start_prefix = oss_start.str();
        const auto end_prefix   = oss_end.str();

        auto it = std::unique_ptr<rocksdb::Iterator>(db->NewIterator(rocksdb::ReadOptions()));

        for (it->Seek(start_prefix); it->Valid(); it->Next()) {
            const auto key = it->key().ToString();

            // Check if we've gone past the end range (lexicographic comparison)
            if (key.compare(0, 13, "provenance_ts") != 0) {
                break;
            }
            if (key > end_prefix + "~") {  // "~" is past most printable chars
                break;
            }

            const auto parsed = parseTimeIndexKey(key);
            if (!parsed.has_value()) {
                continue;
            }

            // Check if within range
            if (parsed->timestamp_ms < start_ts_ms || parsed->timestamp_ms > end_ts_ms) {
                continue;
            }

            if (parsed->step_number >= 0) {
                const auto record = getRecord(parsed->query_id, parsed->step_number);
                if (record.has_value()) {
                    records.push_back(*record);
                }
                continue;
            }

            // Backward-compat fallback for legacy time-index keys without step numbers.
            const auto chain = getProvenanceChain(parsed->query_id);
            for (const auto& rec : chain) {
                if (rec.timestamp_ms >= start_ts_ms && rec.timestamp_ms <= end_ts_ms) {
                    records.push_back(rec);
                }
            }
        }

        // iterator freed automatically by unique_ptr destructor

        // Sort by timestamp
        std::sort(records.begin(), records.end(),
                  [](const ProvenanceStepRecord& a, const ProvenanceStepRecord& b) {
                      return a.timestamp_ms < b.timestamp_ms;
                  });

        // Remove duplicates (from overlapping query chains)
        records.erase(
            std::unique(records.begin(), records.end(),
                        [](const ProvenanceStepRecord& a, const ProvenanceStepRecord& b) {
                            return a.query_id == b.query_id && a.step_number == b.step_number;
                        }),
            records.end());
    } catch (...) {
        // Silently return what we have on error
    }

    return records;
}

std::vector<std::string> RocksDBProvenanceStore::listQueryIds() {
    std::vector<std::string> query_ids;

    try {
        auto* db = impl_->getDb();
        if (!db) {
            return query_ids;
        }

        auto it = std::unique_ptr<rocksdb::Iterator>(db->NewIterator(rocksdb::ReadOptions()));

        std::string last_query_id;
        for (it->Seek("provenance:"); it->Valid(); it->Next()) {
            const auto key = it->key().ToString();

            if (key.compare(0, 11, "provenance:") != 0) {
                break;
            }

            // Parse query_id from key: "provenance:<query_id>:<step>"
            const auto first_colon = 11;
            const auto second_colon = key.find(':', first_colon);

            if (second_colon == std::string::npos) {
                continue;
            }

            const auto query_id = key.substr(first_colon, second_colon - first_colon);

            if (query_id != last_query_id) {
                query_ids.push_back(query_id);
                last_query_id = query_id;
            }
        }
        // iterator freed automatically by unique_ptr destructor
    } catch (...) {
        // Silently return what we have on error
    }

    return query_ids;
}

bool RocksDBProvenanceStore::deleteQuery(const std::string& query_id) {
    try {
        auto* db = impl_->getDb();
        if (!db) {
            return false;
        }

        rocksdb::WriteBatch batch;
        const auto prefix = "provenance:" + query_id + ":";

        // Delete main provenance entries; unique_ptr ensures no leak on throw (Phase 8.4).
        {
            auto it = std::unique_ptr<rocksdb::Iterator>(db->NewIterator(rocksdb::ReadOptions()));
            for (it->Seek(prefix); it->Valid() && it->key().ToString().find(prefix) == 0;
                 it->Next()) {
                batch.Delete(it->key().ToString());
            }
        }

        // Delete time-index entries for this query
        const auto time_prefix = "provenance_ts:";
        {
            auto it = std::unique_ptr<rocksdb::Iterator>(db->NewIterator(rocksdb::ReadOptions()));
            for (it->Seek(time_prefix); it->Valid() && it->key().ToString().find(time_prefix) == 0;
                 it->Next()) {
                const auto key = it->key().ToString();
                const auto parsed = parseTimeIndexKey(key);
                if (parsed.has_value() && parsed->query_id == query_id) {
                    batch.Delete(key);
                }
            }
        }

        const auto status = db->Write(rocksdb::WriteOptions(), &batch);
        return status.ok();
    } catch (...) {
        return false;
    }
}

bool RocksDBProvenanceStore::flush() {
    try {
        auto* db = impl_->getDb();
        if (!db) {
            return false;
        }

        rocksdb::FlushOptions flush_opts;
        flush_opts.wait = true;
        const auto status = db->Flush(flush_opts);
        return status.ok();
    } catch (...) {
        return false;
    }
}

bool RocksDBProvenanceStore::compact() {
    try {
        auto* db = impl_->getDb();
        if (!db) {
            return false;
        }

        rocksdb::CompactRangeOptions compact_opts;
        const auto status = db->CompactRange(compact_opts, nullptr, nullptr);
        return status.ok();
    } catch (...) {
        return false;
    }
}

}  // namespace themis::observability
