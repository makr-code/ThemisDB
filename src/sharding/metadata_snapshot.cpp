/**
 * @file metadata_snapshot.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License
// Phase 2.2: Metadata Shard Durability

#include "sharding/metadata_snapshot.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace themisdb {
namespace sharding {

/** @brief Compute SHA-256 checksum for serialized snapshot payload. */
std::string MetadataSnapshot::calculateChecksum() const {
    // Serialize snapshot to JSON string (excluding checksum field)
    nlohmann::json j = toJson();
    std::string data = j.dump();
    
    // Calculate SHA-256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),static_cast<int>(data.size()), hash);
    
    // Convert to hex string
    std::stringstream ss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

/** @brief Construct snapshot manager and ensure snapshot directory exists. */
MetadataSnapshotManager::MetadataSnapshotManager(
    const std::string& snapshot_directory,
    size_t max_snapshots
)
    : snapshot_directory_(snapshot_directory)
    , max_snapshots_(max_snapshots) {
    
    // Create snapshot directory if it doesn't exist
    if (!std::filesystem::exists(snapshot_directory_)) {
        std::filesystem::create_directories(snapshot_directory_);
    }
}

/** @brief Create snapshot file, persist JSON, and trigger retention cleanup. */
std::optional<uint64_t> MetadataSnapshotManager::createSnapshot(
    const std::string& shard_id,
    const LSN& last_lsn,
    const std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>>& storage
) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    try {
        // Create snapshot
        MetadataSnapshot snapshot;
        snapshot.snapshot_id = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        snapshot.last_applied_lsn = last_lsn;
        snapshot.shard_id = shard_id;
        snapshot.timestamp = snapshot.snapshot_id;
        snapshot.total_entries = 0;
        
        // Copy metadata storage
        for (const auto& [partition_key, entries] : storage) {
            std::map<std::string, nlohmann::json> partition_data = {};

            for (const auto& [key, metadata_entry] : entries) {
                partition_data[key] = metadata_entry.toJson();
                snapshot.total_entries++;
            }
            snapshot.partitions[partition_key] = partition_data;
        }
        
        // Calculate checksum
        snapshot.checksum = snapshot.calculateChecksum();
        
        // Serialize to JSON
        nlohmann::json j = snapshot.toJson();
        j["checksum"] = snapshot.checksum;
        
        // Write to file
        std::string snapshot_path = getSnapshotPath(snapshot.snapshot_id);
        std::ofstream file(snapshot_path);
        if (!file.is_open()) {
            spdlog::error("Failed to create snapshot file: {}", snapshot_path);
            return std::nullopt;
        }
        
        file << j.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        spdlog::info("Created metadata snapshot: id={}, entries={}, size={}KB, path={}",
                    snapshot.snapshot_id,
                    snapshot.total_entries,
                    std::filesystem::file_size(snapshot_path) / 1024,
                    snapshot_path);
        
        // Cleanup old snapshots
        cleanupOldSnapshots();
        
        return snapshot.snapshot_id;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception creating metadata snapshot: {}", e.what());
        return std::nullopt;
    }
}

/** @brief Load newest snapshot if available. */
std::optional<MetadataSnapshot> MetadataSnapshotManager::loadLatestSnapshot() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        spdlog::info("No metadata snapshots found");
        return std::nullopt;
    }
    
    // Load the newest snapshot
    uint64_t latest_id = snapshots[0];
    return loadSnapshot(latest_id);
}

/** @brief Load snapshot by id and validate checksum integrity. */
std::optional<MetadataSnapshot> MetadataSnapshotManager::loadSnapshot([[maybe_unused]] uint64_t snapshot_id) {
    try {
        std::string snapshot_path = getSnapshotPath(snapshot_id);
        
        if (!std::filesystem::exists(snapshot_path)) {
            spdlog::warn("Snapshot file not found: {}", snapshot_path);
            return std::nullopt;
        }
        
        // Read file
        std::ifstream file(snapshot_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open snapshot file: {}", snapshot_path);
            return std::nullopt;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Deserialize
        MetadataSnapshot snapshot = MetadataSnapshot::fromJson(j);
        snapshot.checksum = j["checksum"];
        
        // Verify checksum
        if (!snapshot.verifyChecksum()) {
            spdlog::error("Snapshot checksum verification failed: {}", snapshot_path);
            return std::nullopt;
        }
        
        spdlog::info("Loaded metadata snapshot: id={}, entries={}, shard={}",
                    snapshot.snapshot_id,
                    snapshot.total_entries,
                    snapshot.shard_id);
        
        return snapshot;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception loading metadata snapshot {}: {}", snapshot_id, e.what());
        return std::nullopt;
    }
}

/** @brief Enumerate snapshot files and return IDs sorted newest-first. */
std::vector<uint64_t> MetadataSnapshotManager::listSnapshots() const {
    std::vector<uint64_t> snapshot_ids;
    
    try {
        if (!std::filesystem::exists(snapshot_directory_)) {
            return snapshot_ids;
        }
        
        // Scan directory for snapshot files
        for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // Parse filename: metadata_snapshot_<id>.json
                if (filename.find("metadata_snapshot_") == 0 &&
                    filename.find(".json") != std::string::npos) {
                    
                    std::string id_str = filename.substr(18);  // After "metadata_snapshot_"
                    id_str = id_str.substr(0, id_str.find(".json"));
                    
                    try {
                        uint64_t snapshot_id = std::stoull(id_str);
                        snapshot_ids.push_back(snapshot_id);
                    } catch (...) {
                        // Skip invalid filenames
                    }
                }
            }
        }
        
        // Sort by ID (newest first)
        std::sort(snapshot_ids.begin(), snapshot_ids.end(), std::greater<uint64_t>());
        
    } catch (const std::exception& e) {
        spdlog::error("Exception listing metadata snapshots: {}", e.what());
    }
    
    return snapshot_ids;
}

/** @brief Enforce snapshot retention by deleting oldest surplus snapshots. */
void MetadataSnapshotManager::cleanupOldSnapshots() {
    try {
        auto snapshots = listSnapshots();
        
        // Delete old snapshots beyond max_snapshots
        if (static_cast<int>(snapshots.size()) > max_snapshots_) {
            for (size_t i = max_snapshots_; i < snapshots.size(); ++i) {
                deleteSnapshot(snapshots[i]);
            }
            
            spdlog::info("Cleaned up {} old metadata snapshots",
                        static_cast<int>(snapshots.size()) - max_snapshots_);
        }
    } catch (const std::exception& e) {
        spdlog::error("Exception cleaning up metadata snapshots: {}", e.what());
    }
}

/** @brief Delete one snapshot file from disk when present. */
bool MetadataSnapshotManager::deleteSnapshot([[maybe_unused]] uint64_t snapshot_id) {
    try {
        std::string snapshot_path = getSnapshotPath(snapshot_id);
        
        if (std::filesystem::exists(snapshot_path)) {
            std::filesystem::remove(snapshot_path);
            spdlog::debug("Deleted metadata snapshot: {}", snapshot_id);
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Exception deleting metadata snapshot {}: {}", snapshot_id, e.what());
        return false;
    }
}

/** @brief Build full file path for snapshot id. */
std::string MetadataSnapshotManager::getSnapshotPath([[maybe_unused]] uint64_t snapshot_id) const {
    return snapshot_directory_ + "/metadata_snapshot_" + std::to_string(snapshot_id) + ".json";
}

} // namespace sharding
} // namespace themisdb


