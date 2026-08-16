/**
 * @file raft_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=5, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_log.h"
#include <stdexcept>
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
#include <cstdlib>

namespace themisdb {
namespace sharding {

// ============================================================================
// TIMEOUT CONFIGURATION FOR CONSENSUS OPERATIONS
// ============================================================================
// Environment variable: THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS
// Default: 10000 ms (10 seconds) - consensus ops must complete or timeout quickly
// This prevents indefinite blocking during network partitions or node failures.
// ============================================================================

namespace {
    /**
     * @brief Get consensus timeout from environment or use default
     * @return Timeout duration in milliseconds
     */
    std::chrono::milliseconds getConsensusTimeout() {
        const char* env_timeout = std::getenv("THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS");
        if (env_timeout) {
            try {
                long timeout_ms = std::stol(env_timeout);
                if (timeout_ms > 0 && timeout_ms <= 120000) {  // 2 minute max
                    return std::chrono::milliseconds(timeout_ms);
                }
            } catch (...) {
                spdlog::warn("Invalid THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS: {}; using default", env_timeout);
            }
        }
        return std::chrono::milliseconds(10000);  // Default: 10 seconds
    }
}

/** @brief Construct empty in-memory Raft log state. */
RaftLog::RaftLog() : commit_index_(0) {}

/** @brief Append entry at its declared index, replacing existing slot if present. */
uint64_t RaftLog::append(const LogEntry& entry) {
    // TIMEOUT ENFORCEMENT: Use try_lock_for to prevent indefinite blocking
    // during consensus operations. If timeout expires, throw exception to
    // allow caller to handle consensus abort.
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        std::string error_msg = "RaftLog::append timeout after " + 
                               std::to_string(timeout.count()) + "ms";
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    log_[entry.index] = entry;
    return entry.index;
}

/** @brief Return entry at index when present. */
std::optional<LogEntry> RaftLog::getEntry(uint64_t index) const {
    // TIMEOUT ENFORCEMENT: Use try_lock_for for consistent timeout behavior
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getEntry timeout after {}ms retrieving index {}",
                     timeout.count(), index);
        return std::nullopt;
    }
    
    auto it = log_.find(index);
    if (it != log_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/** @brief Return contiguous available entry range from start to end index. */
std::vector<LogEntry> RaftLog::getEntries(uint64_t start_index, uint64_t end_index) const {
    // TIMEOUT ENFORCEMENT: Batch read operation with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getEntries timeout after {}ms retrieving range [{}, {}]",
                     timeout.count(), start_index, end_index);
        return {};  // Return empty vector on timeout
    }
    
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

/** @brief Check whether index/term pair is known, including snapshot anchor. */
bool RaftLog::hasEntry(uint64_t index, uint64_t term) const {
    // TIMEOUT ENFORCEMENT: Consensus query with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::hasEntry timeout after {}ms checking index={} term={}",
                     timeout.count(), index, term);
        return false;
    }
    
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

/** @brief Delete all log entries from index onward and clamp commit index. */
void RaftLog::truncateFrom(uint64_t index) {
    // TIMEOUT ENFORCEMENT: Consensus write with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::truncateFrom timeout after {}ms truncating from index={}",
                     timeout.count(), index);
        return;  // Log truncation timeout - caller should handle
    }
    
    // Erase all entries from index onward
    auto it = log_.lower_bound(index);
    log_.erase(it, log_.end());
    
    // If we truncated past the commit index, adjust it
    if (commit_index_ >= index) {
        commit_index_ = (index > 0) ? index - 1 : 0;
    }
}

/** @brief Advance commit index if monotonic and bounded by last known index. */
void RaftLog::setCommitIndex(uint64_t index) {
    // TIMEOUT ENFORCEMENT: Consensus commit update with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::setCommitIndex timeout after {}ms setting index={}",
                     timeout.count(), index);
        return;
    }
    
    // RLOG-2: Reject attempts to regress or jump past the last appended entry.
    if (index < commit_index_) {
        spdlog::warn("RaftLog::setCommitIndex: rejecting regression from {} to {}",
                     commit_index_, index);
        return;
    }
    const uint64_t last = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
    if (index > last) {
        spdlog::error("RaftLog::setCommitIndex: index {} exceeds last log entry {} – ignored",
                      index, last);
        return;
    }
    commit_index_ = index;
}

/** @brief Return current committed log index. */
uint64_t RaftLog::getCommitIndex() const {
    // TIMEOUT ENFORCEMENT: Fast read with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getCommitIndex timeout after {}ms", timeout.count());
        return 0;  // Return 0 on timeout (nothing committed)
    }
    return commit_index_;
}

/** @brief Return last available log index or snapshot index when compacted. */
uint64_t RaftLog::getLastLogIndex() const {
    // TIMEOUT ENFORCEMENT: Fast read with timeout
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getLastLogIndex timeout after {}ms", timeout.count());
        return snapshot_index_;  // Return snapshot index on timeout
    }
    if (log_.empty()) {
        // RLOG-1: After snapshot compaction the in-memory log is empty but
        // snapshot_index_ marks the last included entry. Return it so that
        // callers (e.g. propose()) compute the correct next log slot.
        return snapshot_index_;
    }
    return log_.rbegin()->first;
}

/** @brief Return term of last in-memory entry or snapshot anchor term. */
uint64_t RaftLog::getLastLogTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_.empty()) {
        return snapshot_term_;
    }
    return log_.rbegin()->second.term;
}

/** @brief Return number of in-memory log entries. */
size_t RaftLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.size();
}

/** @brief Estimate in-memory footprint of current log entries in bytes. */
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

/** @brief Compact committed entries up to snapshot boundary and set anchor. */
void RaftLog::compactUpTo(uint64_t snapshot_index, uint64_t snapshot_term) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Safety: never compact past committed entries.  The caller must ensure
    // that snapshot_index <= commit_index before invoking this method.
    if (snapshot_index > commit_index_) {
        spdlog::error("RaftLog::compactUpTo: snapshot_index ({}) > commit_index ({}) – "
                      "refusing to compact uncommitted entries",
                      snapshot_index, commit_index_);
        return;
    }

    // Erase all entries at or below snapshot_index
    auto it = log_.upper_bound(snapshot_index);
    log_.erase(log_.begin(), it);

    // Record snapshot anchor so hasEntry() and prevLogIndex checks still work
    snapshot_index_ = snapshot_index;
    snapshot_term_  = snapshot_term;
    // commit_index_ is deliberately NOT advanced here: the caller already
    // verified snapshot_index <= commit_index, so commit_index_ is already
    // correct and we must not move it forward.
}

/** @brief Update snapshot anchor metadata used for post-compaction lookups. */
void RaftLog::setSnapshotMeta(uint64_t index, uint64_t term) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_index_ = index;
    snapshot_term_  = term;
}

/** @brief Return installed snapshot index anchor. */
uint64_t RaftLog::getSnapshotIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_index_;
}

/** @brief Return installed snapshot term anchor. */
uint64_t RaftLog::getSnapshotTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_term_;
}

/** @brief Clear all log entries and reset commit/snapshot state. */
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
/// Handles the empty-buffer case (size == 0) safely without dereferencing
/// a potentially null pointer returned by std::vector::data().
std::string sha256Hex(const uint8_t* data, size_t size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (size == 0) {
        // SHA-256 of empty input is well-defined; use a static 0-length buffer.
        static const uint8_t kEmpty[1] = {0};
        SHA256(kEmpty, 0, hash);
    } else {
        SHA256(data, size, hash);
    }
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

}  // namespace

/** @brief Construct snapshot manager and ensure storage directory exists. */
RaftSnapshotManager::RaftSnapshotManager(const Config& config)
    : config_(config) {
    if (config_.chunk_size_bytes == 0) {
        spdlog::warn("RaftSnapshotManager: chunk_size_bytes=0 is invalid; "
                     "clamping to default 4 MB");
        config_.chunk_size_bytes = kDefaultChunkSizeBytes;
    }
    std::filesystem::create_directories(config_.snapshot_directory);
}

/** @brief Compute SHA-256 checksum helper wrapper. */
/* static */ std::string RaftSnapshotManager::computeChecksum(const uint8_t* data, size_t size) {
    return sha256Hex(data, size);
}

/** @brief Build on-disk snapshot file path for snapshot index. */
std::string RaftSnapshotManager::snapshotPath(uint64_t snapshot_index) const {
    return config_.snapshot_directory + "/raft_snapshot_" +
           std::to_string(snapshot_index) + ".bin";
}

/** @brief Return true when log size reaches configured compaction threshold. */
bool RaftSnapshotManager::shouldCompact(const RaftLog& log) const {
    return log.estimatedSizeBytes() >= config_.compaction_threshold_bytes;
}

/**
 * @brief Create, persist, and install snapshot, then compact log.
 * @param log Raft log to compact after successful snapshot write.
 * @param snapshot_index Last included index.
 * @param snapshot_term Last included term.
 * @param state_data Uncompressed state-machine bytes.
 * @return True when snapshot was persisted and compaction completed.
 */
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
        // Compaction must only cover committed entries; refuse if the caller
        // attempts to snapshot beyond what has been committed.
        if (snapshot_index > log.getCommitIndex()) {
            spdlog::error("RaftSnapshotManager: snapshot_index ({}) > commit_index ({}) – "
                          "refusing to snapshot uncommitted entries",
                          snapshot_index, log.getCommitIndex());
            return false;
        }
        // The snapshot_term must match the term of the entry at snapshot_index
        // (or the existing snapshot anchor if the entry was already compacted).
        if (auto entry = log.getEntry(snapshot_index)) {
            if (entry->term != snapshot_term) {
                spdlog::error("RaftSnapshotManager: snapshot_term ({}) does not match "
                              "log[{}].term ({}) – Raft safety violation",
                              snapshot_term, snapshot_index, entry->term);
                return false;
            }
        } else if (log.getSnapshotIndex() == snapshot_index &&
                   log.getSnapshotTerm() != snapshot_term) {
            spdlog::error("RaftSnapshotManager: snapshot_term ({}) does not match "
                          "existing snapshot anchor term ({}) for index {}",
                          snapshot_term, log.getSnapshotTerm(), snapshot_index);
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

        const std::string path = snapshotPath(snapshot_index);
        const std::string temp_path = path + ".tmp";

        std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for writing", temp_path);
            return false;
        }

        auto write64 = [&](uint64_t v) -> bool {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
            return file.good();
        };
        auto write32 = [&](uint32_t v) -> bool {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
            return file.good();
        };

        if (!write64(snapshot_index) ||
            !write64(snapshot_term) ||
            !write64(uncompressed_size) ||
            !write64(ts) ||
            !write32(checksum_len)) {
            spdlog::error("RaftSnapshotManager: header write failed for {}", temp_path);
            file.close();
            std::error_code ec;
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
        if (!file.good()) {
            spdlog::error("RaftSnapshotManager: checksum write failed for {}", temp_path);
            file.close();
            std::error_code ec;
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        if (!compressed.empty()) {
            file.write(reinterpret_cast<const char*>(compressed.data()),
                       static_cast<std::streamsize>(compressed.size()));
            if (!file.good()) {
                spdlog::error("RaftSnapshotManager: payload write failed for {}", temp_path);
                file.close();
                std::error_code ec;
                std::filesystem::remove(temp_path, ec);
                return false;
            }
        }

        file.flush();
        if (!file.good()) {
            spdlog::error("RaftSnapshotManager: flush failed for {}", temp_path);
            file.close();
            std::error_code ec;
            std::filesystem::remove(temp_path, ec);
            return false;
        }
        file.close();
        if (file.fail()) {
            spdlog::error("RaftSnapshotManager: close failed for {}", temp_path);
            std::error_code ec;
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        {
            std::error_code ec_remove;
            std::filesystem::remove(path, ec_remove);
        }
        std::error_code ec_rename;
        std::filesystem::rename(temp_path, path, ec_rename);
        if (ec_rename) {
            spdlog::error("RaftSnapshotManager: atomic install failed {} -> {}: {}",
                          temp_path, path, ec_rename.message());
            std::error_code ec;
            std::filesystem::remove(temp_path, ec);
            return false;
        }

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

/**
 * @brief Load and verify snapshot file by index.
 * @param snapshot_index Snapshot identifier.
 * @return Snapshot payload when found and checksum-valid.
 */
std::optional<RaftSnapshot> RaftSnapshotManager::loadSnapshot(uint64_t snapshot_index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        // Minimum valid file: 4×8B (index/term/uncompressed_size/timestamp) + 4B (checksum_len)
        constexpr size_t kMinHeaderBytes = 4 * sizeof(uint64_t) + sizeof(uint32_t);
        // Maximum checksum length we will accept (SHA-256 hex is exactly 64 chars)
        constexpr uint32_t kExpectedChecksumLen = 64;

        const size_t file_size = std::filesystem::file_size(path);
        if (file_size < kMinHeaderBytes) {
            spdlog::error("RaftSnapshotManager: snapshot file too small ({} bytes): {}",
                          file_size, path);
            return std::nullopt;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for reading", path);
            return std::nullopt;
        }

        // Helper lambdas that check stream state after every read
        auto read64 = [&](const char* field) -> std::optional<uint64_t> {
            uint64_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            if (!file.good() || file.gcount() != static_cast<std::streamsize>(sizeof(v))) {
                spdlog::error("RaftSnapshotManager: failed to read {} from {}", field, path);
                return std::nullopt;
            }
            return v;
        };
        auto read32 = [&](const char* field) -> std::optional<uint32_t> {
            uint32_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            if (!file.good() || file.gcount() != static_cast<std::streamsize>(sizeof(v))) {
                spdlog::error("RaftSnapshotManager: failed to read {} from {}", field, path);
                return std::nullopt;
            }
            return v;
        };

        RaftSnapshot snap;

        auto idx = read64("snapshot_index");  if (!idx) return std::nullopt;
        snap.snapshot_index = *idx;

        auto term = read64("snapshot_term");  if (!term) return std::nullopt;
        snap.snapshot_term = *term;

        auto usize = read64("uncompressed_size"); if (!usize) return std::nullopt;
        snap.uncompressed_size = *usize;

        auto ts = read64("timestamp");  if (!ts) return std::nullopt;
        snap.timestamp = *ts;

        auto clen = read32("checksum_len"); if (!clen) return std::nullopt;
        const uint32_t checksum_len = *clen;

        // Enforce expected checksum length to prevent large allocations from
        // a corrupt or malicious file.
        if (checksum_len != kExpectedChecksumLen) {
            spdlog::error("RaftSnapshotManager: unexpected checksum_len {} (expected {}) in {}",
                          checksum_len, kExpectedChecksumLen, path);
            return std::nullopt;
        }
        // Verify the file contains enough bytes for the checksum + any data
        const size_t bytes_consumed_so_far = kMinHeaderBytes;
        if (file_size < bytes_consumed_so_far + checksum_len) {
            spdlog::error("RaftSnapshotManager: file too small for checksum in {}", path);
            return std::nullopt;
        }

        snap.checksum.resize(checksum_len);
        file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));
        if (!file.good() || file.gcount() != static_cast<std::streamsize>(checksum_len)) {
            spdlog::error("RaftSnapshotManager: failed to read checksum from {}", path);
            return std::nullopt;
        }

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
            if (file.gcount() != static_cast<std::streamsize>(data_size)) {
                spdlog::error("RaftSnapshotManager: truncated data read from {}", path);
                return std::nullopt;
            }
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
            // Verify decompressed size matches the stored metadata
            if (decompressed.size() != snap.uncompressed_size) {
                spdlog::error("RaftSnapshotManager: decompressed size mismatch for snapshot {}: "
                              "expected {} got {}",
                              snapshot_index, snap.uncompressed_size, decompressed.size());
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

/** @brief Load newest available snapshot from storage, if any. */
std::optional<RaftSnapshot> RaftSnapshotManager::loadLatestSnapshot() const {
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        return std::nullopt;
    }
    return loadSnapshot(snapshots.front());
}

/** @brief Enumerate persisted snapshot indices from disk sorted descending. */
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
                } catch (const std::invalid_argument&) {
                    continue;
                } catch (const std::out_of_range&) {
                    continue;
                }
            }
        }
        std::sort(ids.begin(), ids.end(), std::greater<uint64_t>());
    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: listSnapshots error: {}", e.what());
    }
    return ids;
}

/** @brief Return number of transfer chunks for stored snapshot file. */
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

/**
 * @brief Read and checksum one snapshot chunk for network transfer.
 * @param snapshot_index Snapshot identifier.
 * @param chunk_index Zero-based chunk offset.
 * @return Snapshot chunk when available.
 */
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
        if (!file.good()) {
            return std::nullopt;
        }

        RaftSnapshotChunk chunk;
        chunk.snapshot_index = snapshot_index;
        chunk.chunk_index    = chunk_index;
        chunk.total_chunks   = total_chunks;
        chunk.last_chunk     = (chunk_index + 1 == total_chunks);
        chunk.data.resize(length);
        file.read(reinterpret_cast<char*>(chunk.data.data()),
                  static_cast<std::streamsize>(length));
        if (!file.good() || file.gcount() != static_cast<std::streamsize>(length)) {
            return std::nullopt;
        }
        file.close();

        chunk.checksum = computeChecksum(chunk.data.data(), chunk.data.size());
        return chunk;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: getChunk error snapshot={} chunk={}: {}",
                      snapshot_index, chunk_index, e.what());
        return std::nullopt;
    }
}

/** @brief Prune old snapshot files beyond retention limit. */
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
                    } catch (const std::invalid_argument&) {
                        continue;
                    } catch (const std::out_of_range&) {
                        continue;
                    }
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
