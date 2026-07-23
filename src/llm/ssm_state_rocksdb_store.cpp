/**
 * @file ssm_state_rocksdb_store.cpp
 * @brief RocksDB-backed SSM state persistence implementation.
 * @version 0.1.0-beta
 */

#include "llm/ssm_state_rocksdb_store.h"
#include "storage/hlc.h"

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>

#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <nlohmann/json.hpp>

namespace themis::llm {

SSMStateRocksDBStore::SSMStateRocksDBStore(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf,
    const Config& config)
    : db_(db), cf_(cf), config_(config) {
    
    if (!db_) {
        throw std::invalid_argument("RocksDB TransactionDB pointer cannot be nullptr");
    }
}

SSMStateRocksDBStore::~SSMStateRocksDBStore() {
    // DB and CF are not owned; cleanup is caller's responsibility
}

bool SSMStateRocksDBStore::checkpoint(
    const std::string& session_id,
    const SSMStateSnapshot& snapshot) {
    
    if (session_id.empty()) {
        ++failed_checkpoints_;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Verify snapshot has HLC timestamp
        // Note: Using snapshot_ts from SSMStateSnapshot
        std::string key = makeSSMStateKey(session_id, snapshot.snapshot_ts);
        std::string value = serializeSnapshot(snapshot);

        rocksdb::WriteOptions write_opts;
        write_opts.sync = config_.sync_on_checkpoint;

        rocksdb::Status status;
        if (cf_) {
            status = db_->Put(write_opts, cf_, key, value);
        } else {
            status = db_->Put(write_opts, key, value);
        }

        if (!status.ok()) {
            ++failed_checkpoints_;
            return false;
        }

        ++successful_checkpoints_;
        ++total_checkpoints_;
        return true;
    } catch (...) {
        ++failed_checkpoints_;
        return false;
    }
}

std::optional<SSMStateSnapshot> SSMStateRocksDBStore::resume(
    const std::string& session_id,
    const std::optional<HLCTimestamp>& snapshot_ts) {
    
    if (session_id.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        if (snapshot_ts.has_value()) {
            // Resume specific snapshot by HLC timestamp
            std::string key = makeSSMStateKey(session_id, *snapshot_ts);
            std::string value;

            rocksdb::Status status;
            if (cf_) {
                status = db_->Get(rocksdb::ReadOptions(), cf_, key, &value);
            } else {
                status = db_->Get(rocksdb::ReadOptions(), key, &value);
            }

            if (status.ok()) {
                return deserializeSnapshot(value);
            }
            return std::nullopt;
        } else {
            // Resume most recent snapshot
            auto recent = findMostRecentSnapshot(session_id);
            if (recent.has_value()) {
                return deserializeSnapshot(recent->second);
            }
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }
}

bool SSMStateRocksDBStore::invalidate(const std::string& session_id) {
    if (session_id.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Delete all snapshots for this session
        // Key format: ssm_state:{session_id}:*
        std::string prefix = "ssm_state:" + session_id + ":";

        // Use prefix iterator to find all matching keys
        rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
        if (!it) {
            return false;
        }

        std::vector<std::string> keys_to_delete;
        for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
            keys_to_delete.push_back(it->key().ToString());
        }
        delete it;

        // Delete all found keys
        rocksdb::WriteOptions write_opts;
        for (const auto& key : keys_to_delete) {
            rocksdb::Status status;
            if (cf_) {
                status = db_->Delete(write_opts, cf_, key);
            } else {
                status = db_->Delete(write_opts, key);
            }
            if (!status.ok()) {
                return false;
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

uint64_t SSMStateRocksDBStore::compact(uint64_t retention_window_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (retention_window_ms == 0) {
        retention_window_ms = config_.retention_window_ms;
    }

    try {
        int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        int64_t cutoff_ms = now_ms - retention_window_ms;

        // Scan all snapshots and delete expired ones
        rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
        if (!it) {
            return 0;
        }

        std::vector<std::string> keys_to_delete;
        it->Seek("ssm_state:");

        while (it->Valid() && it->key().starts_with("ssm_state:")) {
            auto ts = parseTimestampFromKey(it->key().ToString());
            if (ts.has_value()) {
                // Convert HLC physical time to milliseconds and compare
                // TODO: Proper HLC comparison
                // For now, simple heuristic: if physical_time < cutoff, delete
                if (ts->physical() < static_cast<uint64_t>(cutoff_ms)) {
                    keys_to_delete.push_back(it->key().ToString());
                }
            }
            it->Next();
        }
        delete it;

        // Delete expired snapshots
        rocksdb::WriteOptions write_opts;
        for (const auto& key : keys_to_delete) {
            rocksdb::Status status;
            if (cf_) {
                status = db_->Delete(write_opts, cf_, key);
            } else {
                status = db_->Delete(write_opts, key);
            }
            if (!status.ok()) {
                return keys_to_delete.size() - 1;  // Return partial count
            }
        }

        return keys_to_delete.size();
    } catch (...) {
        return 0;
    }
}

std::string SSMStateRocksDBStore::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json stats;
    stats["total_checkpoints"] = total_checkpoints_;
    stats["successful_checkpoints"] = successful_checkpoints_;
    stats["failed_checkpoints"] = failed_checkpoints_;
    if (total_checkpoints_ > 0) {
        stats["success_rate"] = 
            static_cast<double>(successful_checkpoints_) / total_checkpoints_;
    }

    return stats.dump();
}

std::string SSMStateRocksDBStore::makeSSMStateKey(
    const std::string& session_id,
    const HLCTimestamp& ts) {
    
    std::ostringstream key;
    key << "ssm_state:" << session_id << ":"
        << ts.physical() << ":"
        << ts.logical();
    
    return key.str();
}

std::string SSMStateRocksDBStore::serializeSnapshot(
    const SSMStateSnapshot& snapshot) {
    
    // Format: [version:1][data...]
    std::string result;
    result.push_back(1);  // Version 1

    // Serialize snapshot to JSON and then to binary
    // TODO: Use protobuf or binary serialization for efficiency
    nlohmann::json j;
    j["snapshot_ts_physical"] = snapshot.snapshot_ts.physical();
    j["snapshot_ts_logical"] = snapshot.snapshot_ts.logical();
    j["state_fingerprint"] = snapshot.state_fingerprint;
    j["sequence_counter"] = snapshot.sequence_counter;

    // Serialize binary state as hex string
    std::ostringstream hex;
    for (auto b : snapshot.state_data) {
        hex << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(b) & 0xFF);
    }
    j["state_data_hex"] = hex.str();

    std::string state_json = j.dump();
    result.append(state_json);

    return result;
}

std::optional<SSMStateSnapshot> SSMStateRocksDBStore::deserializeSnapshot(
    const std::string& data) {
    
    if (data.empty()) {
        return std::nullopt;
    }

    try {
        // Version check
        uint8_t version = static_cast<uint8_t>(data[0]);
        if (version != 1) {
            return std::nullopt;
        }

        // Parse JSON (skip version byte)
        nlohmann::json j = nlohmann::json::parse(data.substr(1));

        SSMStateSnapshot snapshot;
        int64_t physical = j["snapshot_ts_physical"].get<int64_t>();
        int64_t logical = j["snapshot_ts_logical"].get<int64_t>();
        snapshot.snapshot_ts = HLCTimestamp::from(static_cast<uint64_t>(physical), static_cast<uint32_t>(logical));
        snapshot.state_fingerprint = j.value("state_fingerprint", std::string());
        snapshot.sequence_counter = j.value("sequence_counter", 0ULL);
        std::string hex = j.value("state_data_hex", std::string());
        snapshot.state_data.clear();
        snapshot.state_data.reserve(hex.size() / 2);
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            unsigned int byte = 0;
            std::istringstream iss(hex.substr(i,2));
            iss >> std::hex >> byte;
            snapshot.state_data.push_back(static_cast<uint8_t>(byte));
        }

        return snapshot;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<HLCTimestamp> SSMStateRocksDBStore::parseTimestampFromKey(
    const std::string& key) {
    
    // Format: ssm_state:{session_id}:{physical}:{logical}
    // Parse the numeric components at the end
    
    size_t last_colon = key.rfind(':');
    if (last_colon == std::string::npos) {
        return std::nullopt;
    }

    try {
        int64_t logical = std::stoll(key.substr(last_colon + 1));

        size_t second_last_colon = key.rfind(':', last_colon - 1);
        if (second_last_colon == std::string::npos) {
            return std::nullopt;
        }

        int64_t physical = std::stoll(key.substr(second_last_colon + 1, 
                                                  last_colon - second_last_colon - 1));

        return HLCTimestamp::from(static_cast<uint64_t>(physical), static_cast<uint32_t>(logical));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::pair<HLCTimestamp, std::string>>
SSMStateRocksDBStore::findMostRecentSnapshot(const std::string& session_id) {
    
    // Prefix scan for all snapshots with this session_id
    std::string prefix = "ssm_state:" + session_id + ":";
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    if (!it) {
        return std::nullopt;
    }

    std::optional<std::pair<HLCTimestamp, std::string>> most_recent;

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        auto ts = parseTimestampFromKey(it->key().ToString());
        if (ts.has_value()) {
            if (!most_recent.has_value() ||
                ts->physical() > most_recent->first.physical() ||
                (ts->physical() == most_recent->first.physical() &&
                 ts->logical() > most_recent->first.logical())) {
                most_recent = std::make_pair(*ts, it->value().ToString());
            }
        }
    }

    delete it;
    return most_recent;
}

} // namespace themis::llm
