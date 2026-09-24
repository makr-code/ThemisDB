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

SSMStateRocksDBStore::SSMStateRocksDBStore(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf)
    : SSMStateRocksDBStore(db, cf, Config{}) {
}

SSMStateRocksDBStore::~SSMStateRocksDBStore() {
    // DB and CF are not owned; cleanup is caller's responsibility
}

/**
 * @brief Checkpoint.
 * @param[in] session_id Identifier of the session.
 * @param[in] snapshot Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lock(), makeSSMStateKey(), serializeSnapshot(), Put(), ok().
 */
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

/**
 * @brief Resume.
 * @param[in] session_id Identifier of the session.
 * @param[in] snapshot_ts Input parameter.
 * @return Return value.
 * @details Calls: empty(), lock(), has_value(), makeSSMStateKey(), Get(), rocksdb::ReadOptions(), ok(), deserializeSnapshot().
 */
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
            std::string value = {};

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

/**
 * @brief Invalidate.
 * @param[in] session_id Identifier of the session.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lock(), NewIterator(), rocksdb::ReadOptions(), Seek(), Valid(), key(), starts_with().
 */
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

        std::vector<std::string> keys_to_delete = {};

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

uint64_t SSMStateRocksDBStore::compact([[maybe_unused]] uint64_t retention_window_ms) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
                // ✅ PRODUCTION FIX: Proper HLC timestamp comparison
                // Create a cutoff timestamp with logical counter = 0 for boundary comparison
                // This ensures we delete all snapshots with timestamps < cutoff_ms
                auto cutoff_ts = HLCTimestamp::from(static_cast<uint64_t>(cutoff_ms), 0);
                
                // Use HLC comparison operators to properly compare both physical and logical components
                if (*ts < cutoff_ts) {
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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

/**
 * @brief Make SSMState Key.
 * @param[in] session_id Identifier of the session.
 * @param[in] ts Input parameter.
 * @return Return value.
 * @details Calls: physical(), logical(), str().
 */
std::string SSMStateRocksDBStore::makeSSMStateKey(
    const std::string& session_id,
    const HLCTimestamp& ts) {
    
    std::ostringstream key = {};
    key << "ssm_state:" << session_id << ":"
        << ts.physical() << ":"
        << ts.logical();
    
    return key.str();
}

/**
 * @brief Serialize Snapshot.
 * @param[in] snapshot Input parameter.
 * @return Return value.
 * @details Binary serialization format: version(1) | physical(8) | logical(8) | fingerprint_size(4) | fingerprint | seq_counter(8) | data_size(4) | data
 */
std::string SSMStateRocksDBStore::serializeSnapshot(
    const SSMStateSnapshot& snapshot) {
    
    // Format: [version:1][physical:8][logical:8][fp_len:4][fingerprint][seq:8][data_len:4][data...]
    std::string result = {};
    result.push_back(2);  // Version 2 (binary format)
    
    // Serialize HLC timestamp (physical + logical components)
    uint64_t physical = snapshot.snapshot_ts.physical();
    uint64_t logical = snapshot.snapshot_ts.logical();
    
    // Add physical timestamp (8 bytes, big-endian)
    for (int i = 7; i >= 0; --i) {
        result.push_back(static_cast<char>((physical >> (i * 8)) & 0xFF));
    }
    
    // Add logical timestamp (8 bytes, big-endian)
    for (int i = 7; i >= 0; --i) {
        result.push_back(static_cast<char>((logical >> (i * 8)) & 0xFF));
    }
    
    // Add fingerprint length (4 bytes, big-endian)
    uint32_t fp_len = snapshot.state_fingerprint.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((fp_len >> (i * 8)) & 0xFF));
    }
    
    // Add fingerprint data
    result.append(snapshot.state_fingerprint);
    
    // Add sequence counter (8 bytes, big-endian)
    uint64_t seq_counter = snapshot.sequence_counter;
    for (int i = 7; i >= 0; --i) {
        result.push_back(static_cast<char>((seq_counter >> (i * 8)) & 0xFF));
    }
    
    // Add state data length (4 bytes, big-endian)
    uint32_t data_len = snapshot.state_data.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((data_len >> (i * 8)) & 0xFF));
    }
    
    // Add state data
    for (auto b : snapshot.state_data) {
        result.push_back(static_cast<char>(b));
    }
    
    return result;
}

/**
 * @brief Deserialize Snapshot.
 * @param[in] data Input parameter.
 * @return Return value.
 * @details Supports both version 1 (JSON, legacy) and version 2 (binary, current).
 */
std::optional<SSMStateSnapshot> SSMStateRocksDBStore::deserializeSnapshot(
    const std::string& data) {
    
    if (data.empty()) {
        return std::nullopt;
    }

    try {
        // Version check
        uint8_t version = static_cast<uint8_t>(data[0]);
        
        if (version == 1) {
            // Legacy JSON format (for backward compatibility)
            nlohmann::json j = nlohmann::json::parse(data.substr(1));

            SSMStateSnapshot snapshot;
            int64_t physical = j["snapshot_ts_physical"].get<int64_t>();
            int64_t logical = j["snapshot_ts_logical"].get<int64_t>();
            snapshot.snapshot_ts = HLCTimestamp::from(static_cast<uint64_t>(physical), static_cast<uint32_t>(logical));
            snapshot.state_fingerprint = j.value("state_fingerprint", std::string());
            snapshot.sequence_counter = j.value("sequence_counter", 0);
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
        } else if (version == 2) {
            // Binary format (current)
            if (data.size() < 1 + 8 + 8 + 4 + 8 + 4) {
                return std::nullopt;  // Not enough data for header
            }
            
            SSMStateSnapshot snapshot;
            size_t offset = 1;  // Skip version byte
            
            // Parse physical timestamp (8 bytes, big-endian)
            uint64_t physical = 0;
            for (int i = 0; i < 8; ++i) {
                physical = (physical << 8) | static_cast<uint8_t>(data[offset++]);
            }
            
            // Parse logical timestamp (8 bytes, big-endian)
            uint64_t logical = 0;
            for (int i = 0; i < 8; ++i) {
                logical = (logical << 8) | static_cast<uint8_t>(data[offset++]);
            }
            snapshot.snapshot_ts = HLCTimestamp::from(physical, static_cast<uint32_t>(logical));
            
            // Parse fingerprint length (4 bytes, big-endian)
            uint32_t fp_len = 0;
            for (int i = 0; i < 4; ++i) {
                fp_len = (fp_len << 8) | static_cast<uint8_t>(data[offset++]);
            }
            
            // Parse fingerprint data
            if (offset + fp_len > data.size()) {
                return std::nullopt;
            }
            snapshot.state_fingerprint = data.substr(offset, fp_len);
            offset += fp_len;
            
            // Parse sequence counter (8 bytes, big-endian)
            uint64_t seq_counter = 0;
            if (offset + 8 > data.size()) {
                return std::nullopt;
            }
            for (int i = 0; i < 8; ++i) {
                seq_counter = (seq_counter << 8) | static_cast<uint8_t>(data[offset++]);
            }
            snapshot.sequence_counter = seq_counter;
            
            // Parse state data length (4 bytes, big-endian)
            uint32_t data_len = 0;
            if (offset + 4 > data.size()) {
                return std::nullopt;
            }
            for (int i = 0; i < 4; ++i) {
                data_len = (data_len << 8) | static_cast<uint8_t>(data[offset++]);
            }
            
            // Parse state data
            if (offset + data_len != data.size()) {
                return std::nullopt;
            }
            snapshot.state_data.clear();
            snapshot.state_data.reserve(data_len);
            for (uint32_t i = 0; i < data_len; ++i) {
                snapshot.state_data.push_back(static_cast<uint8_t>(data[offset + i]));
            }
            
            return snapshot;
        } else {
            return std::nullopt;  // Unsupported version
        }
    } catch (...) {
        return std::nullopt;
    }
}

/**
 * @brief Parse Timestamp From Key.
 * @param[in] key Input parameter.
 * @return Return value.
 * @details Calls: rfind(), std::stoll(), substr(), HLCTimestamp::from().
 */
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
    it = nullptr;
    return most_recent;
}

} // namespace themis::llm
