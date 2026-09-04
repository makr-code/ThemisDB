/**
 * @file paxos_snapshot.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "sharding/paxos_snapshot.h"
#include <stdexcept>
#include "sharding/paxos_consensus.h"
#include "utils/zstd_codec.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace sharding {

// ============================================================================
// PaxosSnapshot Implementation
// ============================================================================

nlohmann::json PaxosSnapshot::toJSON() const {
    nlohmann::json json;
    
    json["snapshot_id"] = snapshot_id;
    json["last_applied_lsn"] = last_applied_lsn.toString();
    json["last_committed_slot"] = last_committed_slot;
    json["current_round"] = current_round;
    json["node_id"] = node_id;
    json["timestamp"] = timestamp;
    json["instances"] = instances;
    json["committed_log"] = committed_log;
    json["checksum"] = checksum;
    
    return json;
}

PaxosSnapshot PaxosSnapshot::fromJSON(const nlohmann::json& json) {
    PaxosSnapshot snapshot;
    
    snapshot.snapshot_id = json["snapshot_id"].get<uint64_t>();
    snapshot.last_applied_lsn = LSN::fromString(json["last_applied_lsn"].get<std::string>());
    snapshot.last_committed_slot = json["last_committed_slot"].get<uint64_t>();
    snapshot.current_round = json["current_round"].get<uint64_t>();
    snapshot.node_id = json["node_id"].get<std::string>();
    snapshot.timestamp = json["timestamp"].get<uint64_t>();
    
    if (json.contains("instances")) {
        snapshot.instances = json["instances"].get<std::map<uint64_t, nlohmann::json>>();
    }
    
    if (json.contains("committed_log")) {
        snapshot.committed_log = json["committed_log"].get<std::map<uint64_t, nlohmann::json>>();
    }
    
    if (json.contains("checksum")) {
        snapshot.checksum = json["checksum"].get<std::string>();
    }
    
    return snapshot;
}

std::string PaxosSnapshot::calculateChecksum() const {
    // Create JSON without checksum field
    nlohmann::json data_json;
    data_json["snapshot_id"] = snapshot_id;
    data_json["last_applied_lsn"] = last_applied_lsn.toString();
    data_json["last_committed_slot"] = last_committed_slot;
    data_json["current_round"] = current_round;
    data_json["node_id"] = node_id;
    data_json["timestamp"] = timestamp;
    data_json["instances"] = instances;
    data_json["committed_log"] = committed_log;
    
    // Calculate SHA-256 hash
    std::string data_str = data_json.dump();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data_str.c_str()), 
           data_str.size(), hash);
    
    // Convert to hex string
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

bool PaxosSnapshot::verifyChecksum() const {
    std::string calculated = calculateChecksum();
    return calculated == checksum;
}

std::vector<uint8_t> PaxosSnapshot::compress([[maybe_unused]] int level) const {
    const std::string json_str = toJSON().dump();
    return themis::utils::zstd_compress(
        reinterpret_cast<const uint8_t*>(json_str.data()),static_cast<int>(json_str.size()), level);
}

/* static */ std::optional<PaxosSnapshot> PaxosSnapshot::decompress(
        const std::vector<uint8_t>& compressed) {
    if (compressed.empty()) {
        return std::nullopt;
    }
    auto decompressed = themis::utils::zstd_decompress(compressed);
    if (decompressed.empty()) {
        spdlog::error("PaxosSnapshot::decompress: ZSTD decompression failed");
        return std::nullopt;
    }
    try {
        const std::string json_str(decompressed.begin(), decompressed.end());
        auto json = nlohmann::json::parse(json_str);
        PaxosSnapshot snap = PaxosSnapshot::fromJSON(json);
        if (!snap.verifyChecksum()) {
            spdlog::error("PaxosSnapshot::decompress: checksum mismatch after decompression");
            return std::nullopt;
        }
        return snap;
    } catch (const std::exception& e) {
        spdlog::error("PaxosSnapshot::decompress: parse error: {}", e.what());
        return std::nullopt;
    }
}

// ============================================================================
// PaxosSnapshotManager Implementation
// ============================================================================

PaxosSnapshotManager::PaxosSnapshotManager(const std::string& snapshot_directory,
                                           size_t max_snapshots,
                                           int compression_level)
    : snapshot_directory_(snapshot_directory)
    , max_snapshots_(max_snapshots)
    , compression_level_(compression_level) {
    
    // Create snapshot directory if it doesn't exist
    try {
        std::filesystem::create_directories(snapshot_directory_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to create snapshot directory {}: {}", 
                     snapshot_directory_, e.what());
    }
}

std::optional<uint64_t> PaxosSnapshotManager::createSnapshot(
    const std::string& node_id,
    const LSN& last_applied_lsn,
    uint64_t last_committed_slot,
    uint64_t current_round,
    const std::map<uint64_t, PaxosInstance>& instances,
    const std::map<uint64_t, ConsensusLogEntry>& committed_log
) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string filepath = getSnapshotPath(generateSnapshotId());
    const std::string temp_filepath = filepath + ".tmp";
    
    try {
        // Create snapshot
        PaxosSnapshot snapshot;
        snapshot.snapshot_id = last_snapshot_id_;
        snapshot.last_applied_lsn = last_applied_lsn;
        snapshot.last_committed_slot = last_committed_slot;
        snapshot.current_round = current_round;
        snapshot.node_id = node_id;
        snapshot.timestamp = snapshot.snapshot_id;  // Same as ID (timestamp)
        
        // Serialize Paxos instances
        for (const auto& [slot, instance] : instances) {
            nlohmann::json instance_json;
            instance_json["slot"] = instance.slot;
            instance_json["is_committed"] = instance.is_committed;
            
            if (instance.promised_proposal.round > 0) {
                instance_json["promised_proposal"] = {
                    {"round", instance.promised_proposal.round},
                    {"node_id", instance.promised_proposal.node_id}
                };
            }
            
            if (instance.accepted_proposal.round > 0) {
                instance_json["accepted_proposal"] = {
                    {"round", instance.accepted_proposal.round},
                    {"node_id", instance.accepted_proposal.node_id}
                };
                
                instance_json["accepted_value"] = {
                    {"index", instance.accepted_value.index},
                    {"term", instance.accepted_value.term},
                    {"operation", instance.accepted_value.operation},
                    {"data", instance.accepted_value.data}
                };
            }
            
            snapshot.instances[slot] = instance_json;
        }
        
        // Serialize committed log
        for (const auto& [index, entry] : committed_log) {
            nlohmann::json entry_json;
            entry_json["index"] = entry.index;
            entry_json["term"] = entry.term;
            entry_json["operation"] = entry.operation;
            entry_json["data"] = entry.data;
            
            snapshot.committed_log[index] = entry_json;
        }
        
        // Calculate checksum
        snapshot.checksum = snapshot.calculateChecksum();
        
        // Compress and write to file
        auto compressed = snapshot.compress(compression_level_);
        const bool compression_succeeded = !compressed.empty();
        const double ratio = compression_succeeded
            ? static_cast<double>(snapshot.toJSON().dump().size()) /
              std::max<size_t>(1,static_cast<int>(compressed.size()))
            : 1.0;

        std::ofstream file(temp_filepath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            spdlog::error("Failed to open snapshot file for writing: {}", temp_filepath);
            return std::nullopt;
        }

        if (compression_succeeded) {
            // Binary format: "PAXZ" magic + compressed bytes
            const char magic[4] = {'P', 'A', 'X', 'Z'};
            file.write(magic, 4);
            file.write(reinterpret_cast<const char*>(compressed.data()),
                       static_cast<std::streamsize>(compressed.size()));
        } else {
            // Fallback: plain JSON text
            nlohmann::json json = snapshot.toJSON();
            const std::string text = json.dump(2);
            file.write(text.data(), static_cast<std::streamsize>(text.size()));
        }
        file.flush();
        if (!file.good()) {
            file.close();
            std::error_code ec = {};
            std::filesystem::remove(temp_filepath, ec);
            spdlog::error("Failed to fully write snapshot file: {}", temp_filepath);
            return std::nullopt;
        }
        file.close();
        if (!file) {
            std::error_code ec = {};
            std::filesystem::remove(temp_filepath, ec);
            spdlog::error("Failed to close snapshot file cleanly: {}", temp_filepath);
            return std::nullopt;
        }
        std::error_code rename_ec = {};
        std::filesystem::rename(temp_filepath, filepath, rename_ec);
        if (rename_ec) {
            std::error_code cleanup_ec = {};
            std::filesystem::remove(temp_filepath, cleanup_ec);
            spdlog::error("Failed to publish snapshot file {}: {}", filepath, rename_ec.message());
            return std::nullopt;
        }
        
        spdlog::info("Created Paxos snapshot: id={} slot={} instances={} log_entries={} "
                     "compressed={} ratio={:.2f}x",
                    snapshot.snapshot_id, last_committed_slot,
                    instances.size(),static_cast<int>(committed_log.size()),
                    compression_succeeded, ratio);
        
        // Cleanup old snapshots
        cleanupOldSnapshots(max_snapshots_);
        
        return snapshot.snapshot_id;
        
    } catch (const std::exception& e) {
        std::error_code ec = {};
        std::filesystem::remove(temp_filepath, ec);
        spdlog::error("Failed to create Paxos snapshot: {}", e.what());
        return std::nullopt;
    }
}

std::optional<PaxosSnapshot> PaxosSnapshotManager::loadLatestSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        spdlog::info("No Paxos snapshots found");
        return std::nullopt;
    }
    
    // Load the most recent snapshot (first in list)
    return loadSnapshot(snapshots[0]);
}

std::optional<PaxosSnapshot> PaxosSnapshotManager::loadSnapshot([[maybe_unused]] uint64_t snapshot_id) {
    try {
        std::string filepath = getSnapshotPath(snapshot_id);
        
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            spdlog::warn("Snapshot file not found: {}", filepath);
            return std::nullopt;
        }

        // Detect format: binary ZSTD ("PAXZ" magic) or legacy plain JSON
        char magic[4] = {0, 0, 0, 0};
        file.read(magic, 4);
        if (file.gcount() != 4) {
            spdlog::error("Snapshot file is truncated before header read: {}", filepath);
            return std::nullopt;
        }
        const bool is_compressed = (magic[0] == 'P' && magic[1] == 'A' &&
                                    magic[2] == 'X' && magic[3] == 'Z');

        PaxosSnapshot snapshot = {};

        if (is_compressed) {
            // Read remaining bytes as compressed data
            const std::streampos data_start = file.tellg();
            file.seekg(0, std::ios::end);
            const size_t data_size =
                static_cast<size_t>(file.tellg() - data_start);
            file.seekg(data_start);
            if (data_size == 0) {
                spdlog::error("Compressed Paxos snapshot has no payload: id={}", snapshot_id);
                return std::nullopt;
            }

            std::vector<uint8_t> compressed(data_size);
            file.read(reinterpret_cast<char*>(compressed.data()),
                      static_cast<std::streamsize>(data_size));
            if (file.gcount() != static_cast<std::streamsize>(data_size)) {
                spdlog::error("Compressed Paxos snapshot is truncated: id={}", snapshot_id);
                return std::nullopt;
            }
            file.close();

            auto opt = PaxosSnapshot::decompress(compressed);
            if (!opt) {
                spdlog::error("Failed to decompress Paxos snapshot: id={}", snapshot_id);
                return std::nullopt;
            }
            snapshot = std::move(*opt);
        } else {
            // Legacy plain-JSON format – rewind and parse as text
            file.seekg(0);
            nlohmann::json json;
            file >> json;
            file.close();
            snapshot = PaxosSnapshot::fromJSON(json);

            // Verify checksum
            if (!snapshot.verifyChecksum()) {
                spdlog::error("Snapshot checksum verification failed: id={}", snapshot_id);
                return std::nullopt;
            }
        }
        if (snapshot.snapshot_id != snapshot_id) {
            spdlog::error("Snapshot payload ID mismatch: requested={} actual={}",
                          snapshot_id, snapshot.snapshot_id);
            return std::nullopt;
        }
        
        spdlog::info("Loaded Paxos snapshot: id={} slot={} instances={} log_entries={}",
                    snapshot.snapshot_id, snapshot.last_committed_slot,
                    snapshot.instances.size(),static_cast<int>(snapshot.committed_log.size()));
        
        return snapshot;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load Paxos snapshot {}: {}", snapshot_id, e.what());
        return std::nullopt;
    }
}

std::vector<uint64_t> PaxosSnapshotManager::listSnapshots() const {
    std::vector<uint64_t> snapshots;
    
    try {
        if (!std::filesystem::exists(snapshot_directory_)) {
            return snapshots;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // Parse snapshot ID from filename (format: paxos_snapshot_<id>.json)
                if (filename.find("paxos_snapshot_") == 0 && filename.ends_with(".json")) {
                    std::string id_str = filename.substr(15, static_cast<int>(filename.size()) - 20);
                    try {
                        uint64_t snapshot_id = std::stoull(id_str);
                        snapshots.push_back(snapshot_id);
                    } catch (...) {
                        // Skip invalid filenames
                    }
                }
            }
        }
        
        // Sort by ID (newest first)
        std::sort(snapshots.begin(), snapshots.end(), std::greater<uint64_t>());
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to list snapshots: {}", e.what());
    }
    
    return snapshots;
}

void PaxosSnapshotManager::cleanupOldSnapshots([[maybe_unused]] size_t keep_count) {
    try {
        auto snapshots = listSnapshots();
        
        if (static_cast<int>(snapshots.size()) <= keep_count) {
            return;  // Nothing to cleanup
        }
        
        // Delete old snapshots beyond keep_count
        for (size_t i = keep_count; i < snapshots.size(); ++i) {
            std::string filepath = getSnapshotPath(snapshots[i]);
            std::filesystem::remove(filepath);
            spdlog::info("Deleted old Paxos snapshot: id={}", snapshots[i]);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to cleanup old snapshots: {}", e.what());
    }
}

std::string PaxosSnapshotManager::getSnapshotPath([[maybe_unused]] uint64_t snapshot_id) const {
    return snapshot_directory_ + "/paxos_snapshot_" + std::to_string(snapshot_id) + ".json";
}

uint64_t PaxosSnapshotManager::generateSnapshotId() const {
    // Use current timestamp as snapshot ID (milliseconds since epoch)
    auto now = std::chrono::system_clock::now();
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    const uint64_t candidate = static_cast<uint64_t>(timestamp);
    if (candidate <= last_snapshot_id_) {
        ++last_snapshot_id_;
    } else {
        last_snapshot_id_ = candidate;
    }
    return last_snapshot_id_;
}

} // namespace sharding
} // namespace themis

