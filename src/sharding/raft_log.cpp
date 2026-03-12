/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_log.cpp                                       ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:00:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_log.h"
#include "utils/zstd_codec.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace themisdb {
namespace sharding {

RaftLog::RaftLog() : commit_index_(0) {}

uint64_t RaftLog::append(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_[entry.index] = entry;
    return entry.index;
}

std::optional<LogEntry> RaftLog::getEntry(uint64_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = log_.find(index);
    if (it != log_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LogEntry> RaftLog::getEntries(uint64_t start_index, uint64_t end_index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> entries;
    
    for (uint64_t i = start_index; i <= end_index; ++i) {
        auto it = log_.find(i);
        if (it != log_.end()) {
            entries.push_back(it->second);
        } else {
            break;  // Stop at first missing entry
        }
    }
    
    return entries;
}

bool RaftLog::hasEntry(uint64_t index, uint64_t term) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Special case: index 0 always matches (no previous entry)
    if (index == 0) {
        return true;
    }

    // If the index is exactly the snapshot boundary, match against snapshot term
    if (index == snapshot_index_) {
        return term == snapshot_term_;
    }
    
    auto it = log_.find(index);
    if (it == log_.end()) {
        return false;
    }
    
    return it->second.term == term;
}

void RaftLog::truncateFrom(uint64_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Erase all entries from index onward
    auto it = log_.lower_bound(index);
    log_.erase(it, log_.end());
    
    // If we truncated past the commit index, adjust it
    if (commit_index_ >= index) {
        commit_index_ = (index > 0) ? index - 1 : 0;
    }
}

void RaftLog::setCommitIndex(uint64_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    commit_index_ = index;
}

uint64_t RaftLog::getCommitIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commit_index_;
}

uint64_t RaftLog::getLastLogIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_.empty()) {
        return 0;
    }
    return log_.rbegin()->first;
}

uint64_t RaftLog::getLastLogTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_.empty()) {
        return 0;
    }
    return log_.rbegin()->second.term;
}

size_t RaftLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.size();
}

size_t RaftLog::estimatedSizeBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Fixed overhead per entry: two uint64_t (term + index) + one uint64_t (timestamp)
    constexpr size_t kEntryOverhead = sizeof(uint64_t) * 3;
    size_t total = 0;
    for (const auto& [idx, entry] : log_) {
        total += kEntryOverhead + entry.command.size();
    }
    return total;
}

void RaftLog::compactUpTo(uint64_t snapshot_index, uint64_t snapshot_term) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Erase all entries at or below snapshot_index
    auto it = log_.upper_bound(snapshot_index);
    log_.erase(log_.begin(), it);

    // Record snapshot anchor so hasEntry() and prevLogIndex checks still work
    snapshot_index_ = snapshot_index;
    snapshot_term_  = snapshot_term;

    // Commit index cannot go below the snapshot
    if (commit_index_ < snapshot_index) {
        commit_index_ = snapshot_index;
    }
}

void RaftLog::setSnapshotMeta(uint64_t index, uint64_t term) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_index_ = index;
    snapshot_term_  = term;
}

uint64_t RaftLog::getSnapshotIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_index_;
}

uint64_t RaftLog::getSnapshotTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_term_;
}

void RaftLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    log_.clear();
    commit_index_ = 0;
    snapshot_index_ = 0;
    snapshot_term_ = 0;
}

// ============================================================================
// RaftSnapshotManager
// ============================================================================

namespace {

/// Compute SHA-256 of the given buffer and return a lowercase hex string.
std::string sha256Hex(const uint8_t* data, size_t size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, size, hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

}  // namespace

RaftSnapshotManager::RaftSnapshotManager(const Config& config)
    : config_(config) {
    std::filesystem::create_directories(config_.snapshot_directory);
}

/* static */ std::string RaftSnapshotManager::computeChecksum(const uint8_t* data, size_t size) {
    return sha256Hex(data, size);
}

std::string RaftSnapshotManager::snapshotPath(uint64_t snapshot_index) const {
    return config_.snapshot_directory + "/raft_snapshot_" +
           std::to_string(snapshot_index) + ".bin";
}

bool RaftSnapshotManager::shouldCompact(const RaftLog& log) const {
    return log.estimatedSizeBytes() >= config_.compaction_threshold_bytes;
}

bool RaftSnapshotManager::createAndInstall(RaftLog& log,
                                            uint64_t snapshot_index,
                                            uint64_t snapshot_term,
                                            const std::vector<uint8_t>& state_data) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // 1. Validate preconditions
        if (snapshot_index == 0) {
            spdlog::error("RaftSnapshotManager: snapshot_index must be > 0");
            return false;
        }
        if (state_data.empty()) {
            spdlog::warn("RaftSnapshotManager: creating snapshot with empty state data");
        }

        // 2. Compute checksum of raw (pre-compression) state data
        const std::string checksum = computeChecksum(state_data.data(), state_data.size());

        // 3. Compress with ZSTD level 3
        auto compressed = themis::utils::zstd_compress(state_data, config_.compression_level);
        if (compressed.empty() && !state_data.empty()) {
            spdlog::error("RaftSnapshotManager: ZSTD compression failed for snapshot {}",
                          snapshot_index);
            return false;
        }

        // 4. Build binary snapshot file layout:
        //    [snapshot_index: 8B][snapshot_term: 8B][uncompressed_size: 8B]
        //    [checksum_len: 4B][checksum: N bytes][compressed_data: rest]
        const uint32_t checksum_len = static_cast<uint32_t>(checksum.size());
        const uint64_t uncompressed_size = state_data.size();
        const uint64_t ts = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        std::string path = snapshotPath(snapshot_index);
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for writing", path);
            return false;
        }

        auto write64 = [&](uint64_t v) {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
        };
        auto write32 = [&](uint32_t v) {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
        };

        write64(snapshot_index);
        write64(snapshot_term);
        write64(uncompressed_size);
        write64(ts);
        write32(checksum_len);
        file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
        if (!compressed.empty()) {
            file.write(reinterpret_cast<const char*>(compressed.data()),
                       static_cast<std::streamsize>(compressed.size()));
        }
        file.close();

        const double ratio = uncompressed_size > 0
            ? static_cast<double>(uncompressed_size) / std::max<size_t>(1, compressed.size())
            : 1.0;

        spdlog::info("RaftSnapshotManager: created snapshot index={} term={} "
                     "uncompressed={}B compressed={}B ratio={:.2f}x path={}",
                     snapshot_index, snapshot_term,
                     uncompressed_size, compressed.size(), ratio, path);

        // 5. Compact the in-memory Raft log
        log.compactUpTo(snapshot_index, snapshot_term);

        // 6. Remove old snapshots beyond retention limit
        cleanupOldSnapshots();

        return true;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: exception creating snapshot: {}", e.what());
        return false;
    }
}

std::optional<RaftSnapshot> RaftSnapshotManager::loadSnapshot(uint64_t snapshot_index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for reading", path);
            return std::nullopt;
        }

        auto read64 = [&]() -> uint64_t {
            uint64_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            return v;
        };
        auto read32 = [&]() -> uint32_t {
            uint32_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            return v;
        };

        RaftSnapshot snap;
        snap.snapshot_index    = read64();
        snap.snapshot_term     = read64();
        snap.uncompressed_size = read64();
        snap.timestamp         = read64();

        const uint32_t checksum_len = read32();
        snap.checksum.resize(checksum_len);
        file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));

        // Read remaining bytes as compressed data
        const std::streampos data_start = file.tellg();
        file.seekg(0, std::ios::end);
        const std::streampos data_end = file.tellg();
        const size_t data_size = static_cast<size_t>(data_end - data_start);
        file.seekg(data_start);

        snap.data.resize(data_size);
        if (data_size > 0) {
            file.read(reinterpret_cast<char*>(snap.data.data()),
                      static_cast<std::streamsize>(data_size));
        }
        file.close();

        // Decompress to verify checksum
        if (!snap.data.empty()) {
            auto decompressed = themis::utils::zstd_decompress(snap.data);
            if (decompressed.empty() && snap.uncompressed_size > 0) {
                spdlog::error("RaftSnapshotManager: decompression failed for snapshot {}",
                              snapshot_index);
                return std::nullopt;
            }
            const std::string actual_checksum =
                computeChecksum(decompressed.data(), decompressed.size());
            if (actual_checksum != snap.checksum) {
                spdlog::error("RaftSnapshotManager: checksum mismatch for snapshot {} "
                              "(expected={} actual={})",
                              snapshot_index, snap.checksum, actual_checksum);
                return std::nullopt;
            }
        }

        return snap;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: exception loading snapshot {}: {}",
                      snapshot_index, e.what());
        return std::nullopt;
    }
}

std::optional<RaftSnapshot> RaftSnapshotManager::loadLatestSnapshot() const {
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        return std::nullopt;
    }
    return loadSnapshot(snapshots.front());
}

std::vector<uint64_t> RaftSnapshotManager::listSnapshots() const {
    std::vector<uint64_t> ids;
    try {
        if (!std::filesystem::exists(config_.snapshot_directory)) {
            return ids;
        }
        for (const auto& entry :
             std::filesystem::directory_iterator(config_.snapshot_directory)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            // Format: raft_snapshot_<index>.bin
            if (name.rfind("raft_snapshot_", 0) == 0 && name.ends_with(".bin")) {
                const std::string id_str = name.substr(14, name.size() - 18);
                try {
                    ids.push_back(std::stoull(id_str));
                } catch (...) {}
            }
        }
        std::sort(ids.begin(), ids.end(), std::greater<uint64_t>());
    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: listSnapshots error: {}", e.what());
    }
    return ids;
}

size_t RaftSnapshotManager::getChunkCount(uint64_t snapshot_index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return 0;
    }
    const size_t file_size = std::filesystem::file_size(path);
    if (file_size == 0) {
        return 0;
    }
    return (file_size + config_.chunk_size_bytes - 1) / config_.chunk_size_bytes;
}

std::optional<RaftSnapshotChunk> RaftSnapshotManager::getChunk(uint64_t snapshot_index,
                                                                 uint64_t chunk_index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        const size_t file_size = std::filesystem::file_size(path);
        if (file_size == 0) {
            return std::nullopt;
        }

        const size_t total_chunks =
            (file_size + config_.chunk_size_bytes - 1) / config_.chunk_size_bytes;
        if (chunk_index >= total_chunks) {
            return std::nullopt;
        }

        const size_t offset = chunk_index * config_.chunk_size_bytes;
        const size_t length = std::min(config_.chunk_size_bytes, file_size - offset);

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return std::nullopt;
        }
        file.seekg(static_cast<std::streamoff>(offset));

        RaftSnapshotChunk chunk;
        chunk.snapshot_index = snapshot_index;
        chunk.chunk_index    = chunk_index;
        chunk.total_chunks   = total_chunks;
        chunk.last_chunk     = (chunk_index + 1 == total_chunks);
        chunk.data.resize(length);
        file.read(reinterpret_cast<char*>(chunk.data.data()),
                  static_cast<std::streamsize>(length));
        file.close();

        chunk.checksum = computeChecksum(chunk.data.data(), chunk.data.size());
        return chunk;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: getChunk error snapshot={} chunk={}: {}",
                      snapshot_index, chunk_index, e.what());
        return std::nullopt;
    }
}

void RaftSnapshotManager::cleanupOldSnapshots() {
    // NOTE: called with mutex_ held
    try {
        // Build list without taking the lock again (already held by caller)
        std::vector<uint64_t> ids;
        if (std::filesystem::exists(config_.snapshot_directory)) {
            for (const auto& entry :
                 std::filesystem::directory_iterator(config_.snapshot_directory)) {
                if (!entry.is_regular_file()) continue;
                const std::string name = entry.path().filename().string();
                if (name.rfind("raft_snapshot_", 0) == 0 && name.ends_with(".bin")) {
                    const std::string id_str = name.substr(14, name.size() - 18);
                    try {
                        ids.push_back(std::stoull(id_str));
                    } catch (...) {}
                }
            }
        }
        std::sort(ids.begin(), ids.end(), std::greater<uint64_t>());
        for (size_t i = config_.max_snapshots; i < ids.size(); ++i) {
            const std::string p = snapshotPath(ids[i]);
            std::filesystem::remove(p);
            spdlog::info("RaftSnapshotManager: removed old snapshot {}", ids[i]);
        }
    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: cleanupOldSnapshots error: {}", e.what());
    }
}

}  // namespace sharding
}  // namespace themisdb
