/**
 * @file wal_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "sharding/wal_manager.h"
#include "storage/wal_storage.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis::sharding {

// ============================================================================
// LSN Implementation
// ============================================================================

LSN LSN::fromString(const std::string& str) {
    size_t pos = str.find('/');
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid LSN format: " + str);
    }

    uint64_t segment = std::stoull(str.substr(0, pos));
    uint64_t offset = std::stoull(str.substr(pos + 1));

    return LSN(segment, offset);
}

// ============================================================================
// WALEntry Implementation
// ============================================================================

std::vector<uint8_t> WALEntry::serialize() const {
    std::vector<uint8_t> result;

    // Format: [type:1][timestamp:8][lsn_segment:8][lsn_offset:8][tx_id_len:4][tx_id][data_len:4][data]
    result.push_back(static_cast<uint8_t>(type));

    for (int i = 7; i >= 0; --i) {
        result.push_back((timestamp >> (i * 8)) & 0xFF);
    }
    for (int i = 7; i >= 0; --i) {
        result.push_back((lsn.segment >> (i * 8)) & 0xFF);
    }
    for (int i = 7; i >= 0; --i) {
        result.push_back((lsn.offset >> (i * 8)) & 0xFF);
    }

    uint32_t tx_id_len = static_cast<uint32_t>(transaction_id.size());
    for (int i = 3; i >= 0; --i) {
        result.push_back((tx_id_len >> (i * 8)) & 0xFF);
    }
    result.insert(result.end(), transaction_id.begin(), transaction_id.end());

    std::string data_str = data.dump();
    uint32_t data_len = static_cast<uint32_t>(data_str.size());
    for (int i = 3; i >= 0; --i) {
        result.push_back((data_len >> (i * 8)) & 0xFF);
    }
    result.insert(result.end(), data_str.begin(), data_str.end());

    return result;
}

WALEntry WALEntry::deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 29) {
        throw std::runtime_error("WAL entry too small");
    }

    WALEntry entry;
    size_t pos = 0;

    entry.type = static_cast<WALEntryType>(bytes[pos++]);

    entry.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        entry.timestamp = (entry.timestamp << 8) | bytes[pos++];
    }

    entry.lsn.segment = 0;
    for (int i = 0; i < 8; ++i) {
        entry.lsn.segment = (entry.lsn.segment << 8) | bytes[pos++];
    }

    entry.lsn.offset = 0;
    for (int i = 0; i < 8; ++i) {
        entry.lsn.offset = (entry.lsn.offset << 8) | bytes[pos++];
    }

    uint32_t tx_id_len = 0;
    for (int i = 0; i < 4; ++i) {
        tx_id_len = (tx_id_len << 8) | bytes[pos++];
    }

    if (pos + tx_id_len > bytes.size()) {
        throw std::runtime_error("WAL entry truncated: transaction_id overruns buffer");
    }
    entry.transaction_id = std::string(bytes.begin() + pos, bytes.begin() + pos + tx_id_len);
    pos += tx_id_len;

    if (pos + 4 > bytes.size()) {
        throw std::runtime_error("WAL entry truncated: missing data_len field");
    }
    uint32_t data_len = 0;
    for (int i = 0; i < 4; ++i) {
        data_len = (data_len << 8) | bytes[pos++];
    }

    if (pos + data_len > bytes.size()) {
        throw std::runtime_error("WAL entry truncated: data overruns buffer");
    }
    std::string data_str(bytes.begin() + pos, bytes.begin() + pos + data_len);
    entry.data = nlohmann::json::parse(data_str);

    return entry;
}

size_t WALEntry::size() const {
    return 1 + 8 + 8 + 8 + 4 + transaction_id.size() + 4 + data.dump().size();
}

// ============================================================================
// WALManager Implementation
// ============================================================================

WALManager::WALManager(const WALManagerConfig& config)
    : config_(config), current_lsn_(1, 0), oldest_lsn_(1, 0) {
    std::error_code ec = {};
    std::filesystem::create_directories(config_.wal_directory, ec);

    themis::WALStorage::Config durability_cfg;
    durability_cfg.dir = config_.wal_directory;
    durability_cfg.rotation_threshold_bytes = config_.segment_size;
    durability_cfg.fsync_on_write = config_.sync_on_write;

    auto open_res = themis::WALStorage::open(
        durability_cfg,
        [this](const themis::WALStorage::Entry& recovered) {
            if (recovered.type != themis::WALStorage::EntryType::PUT) {
                return true;
            }

            std::vector<uint8_t> payload(recovered.value.begin(), recovered.value.end());
            WALEntry entry = WALEntry::deserialize(payload);
            entry.lsn = sequenceToLsn(recovered.sequence);
            entries_.push_back(entry);
            trimInMemoryEntries();
            current_lsn_ = sequenceToLsn(recovered.sequence + 1);
            total_entries_++;
            total_bytes_ += payload.size();
            return true;
        });

    if (!open_res) {
        throw std::runtime_error(
            "Failed to initialize WALStorage for sharding WAL manager: " +
            open_res.error().context());
    }

    durability_wal_ = std::move(*open_res);
    if (!entries_.empty()) {
        oldest_lsn_ = entries_.front().lsn;
    }
}

WALManager::~WALManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    flush();
}

LSN WALManager::append(const WALEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!durability_wal_) {
        throw std::runtime_error("WALManager append called before durability WAL initialization");
    }

    WALEntry entry_copy = entry;
    entry_copy.lsn = sequenceToLsn(durability_wal_->lastSequence() + 1);
    entry_copy.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    const auto payload = entry_copy.serialize();
    std::string value(payload.begin(), payload.end());
    auto append_res = durability_wal_->appendPut("wal_entry", value);
    if (!append_res) {
        throw std::runtime_error("WALManager durable append failed: " +
                                 append_res.error().context());
    }

    entry_copy.lsn = sequenceToLsn(*append_res);
    entries_.push_back(entry_copy);
    trimInMemoryEntries();
    current_lsn_ = sequenceToLsn(*append_res + 1);
    if (entries_.size() == 1) {
        oldest_lsn_ = entry_copy.lsn;
    }

    total_entries_++;
    total_bytes_ += payload.size();

    if (config_.sync_on_write) {
        flush();
    }

    return entry_copy.lsn;
}

std::optional<WALEntry> WALManager::read(const LSN& lsn) {
    auto entries = readRange(lsn, LSN(lsn.segment, lsn.offset + 1));
    if (entries.empty()) {
        return std::nullopt;
    }
    return entries[0];
}

std::vector<WALEntry> WALManager::readRange(const LSN& start_lsn,
                                            const std::optional<LSN>& end_lsn) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<WALEntry> result = {};
    result.reserve(entries_.size());
    for (const auto& entry : entries_) {
        if (entry.lsn < start_lsn) {
            continue;
        }
        if (end_lsn.has_value() && entry.lsn >= *end_lsn) {
            break;
        }
        result.push_back(entry);
    }
    return result;
}

LSN WALManager::getCurrentLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lsn_;
}

LSN WALManager::getOldestLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return oldest_lsn_;
}

void WALManager::flush() {
    if (!durability_wal_) {
        return;
    }
    auto flush_res = durability_wal_->flush();
    if (!flush_res) {
        THEMIS_WARN("Sharding WALManager flush failed: {}", flush_res.error().context());
    }
}

LSN WALManager::checkpoint() {
    WALEntry checkpoint_entry;
    checkpoint_entry.type = WALEntryType::CHECKPOINT;
    checkpoint_entry.data = {
        {"checkpoint_lsn", getCurrentLSN().toString()}
    };

    const LSN lsn = append(checkpoint_entry);

    std::lock_guard<std::mutex> lock(mutex_);
    if (durability_wal_) {
        auto res = durability_wal_->checkpoint(false);
        if (!res) {
            THEMIS_WARN("Sharding WALManager checkpoint durability marker failed: {}",
                        res.error().context());
        }
    }
    return lsn;
}

void WALManager::truncate(const LSN& lsn) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto first_to_keep = std::lower_bound(
        entries_.begin(), entries_.end(), lsn,
        [](const WALEntry& lhs, const LSN& rhs) { return lhs.lsn < rhs; });
    entries_.erase(entries_.begin(), first_to_keep);

    if (entries_.empty()) {
        oldest_lsn_ = lsn;
    } else {
        oldest_lsn_ = entries_.front().lsn;
    }
    trimInMemoryEntries();

    if (durability_wal_) {
        auto res = durability_wal_->checkpoint(true);
        if (!res) {
            THEMIS_WARN("Sharding WALManager truncate checkpoint failed: {}",
                        res.error().context());
        }
    }
}

WALManager::Statistics WALManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Statistics stats;
    stats.total_entries = total_entries_;
    stats.total_bytes = total_bytes_;
    stats.current_lsn = current_lsn_;
    stats.oldest_lsn = oldest_lsn_;
    stats.segments = durability_wal_ ? durability_wal_->segmentCount() : 0;
    return stats;
}

void WALManager::trimInMemoryEntries() {
    if (config_.max_in_memory_entries == 0) {
        entries_.clear();
        oldest_lsn_ = current_lsn_;
        return;
    }

    const auto max_entries = config_.max_in_memory_entries;
    if (entries_.size() <= max_entries) {
        if (!entries_.empty()) {
            oldest_lsn_ = entries_.front().lsn;
        }
        return;
    }

    const auto drop_count = entries_.size() - max_entries;
    entries_.erase(entries_.begin(), entries_.begin() + static_cast<std::ptrdiff_t>(drop_count));
    if (!entries_.empty()) {
        oldest_lsn_ = entries_.front().lsn;
    }
}

LSN WALManager::sequenceToLsn(uint64_t sequence) const {
    if (sequence == 0) {
        return LSN(1, 0);
    }
    return LSN(1, sequence);
}

} // namespace themis::sharding
