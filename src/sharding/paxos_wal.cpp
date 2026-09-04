/**
 * @file paxos_wal.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "sharding/paxos_wal.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <chrono>

namespace themis {
namespace sharding {

// ============================================================================
// PaxosWALEntry Implementation
// ============================================================================

WALEntry PaxosWALEntry::toWALEntry() const {
    WALEntry entry;
    entry.lsn = lsn;
    entry.timestamp = timestamp;
    
    // Encode Paxos-specific type as a custom WAL type
    // We use type 100+ for Paxos entries to avoid conflicts
    entry.type = static_cast<WALEntryType>(static_cast<uint8_t>(type) + 100);
    
    // Pack Paxos-specific fields into JSON
    nlohmann::json paxos_data = {
        {"slot", slot},
        {"round", round},
        {"node_id", node_id},
        {"data", data}
    };
    
    entry.data = paxos_data;
    entry.transaction_id = "paxos_slot_" + std::to_string(slot);
    
    return entry;
}

PaxosWALEntry PaxosWALEntry::fromWALEntry(const WALEntry& entry) {
    PaxosWALEntry paxos_entry;
    paxos_entry.lsn = entry.lsn;
    paxos_entry.timestamp = entry.timestamp;
    
    // Decode Paxos type from WAL type
    uint8_t wal_type = static_cast<uint8_t>(entry.type);
    if (wal_type < 100) {
        throw std::runtime_error("Not a Paxos WAL entry");
    }
    paxos_entry.type = static_cast<PaxosWALEntryType>(wal_type - 100);
    
    // Unpack Paxos-specific fields from JSON
    if (entry.data.contains("slot")) {
        paxos_entry.slot = entry.data["slot"].get<uint64_t>();
    }
    if (entry.data.contains("round")) {
        paxos_entry.round = entry.data["round"].get<uint64_t>();
    }
    if (entry.data.contains("node_id")) {
        paxos_entry.node_id = entry.data["node_id"].get<std::string>();
    }
    if (entry.data.contains("data")) {
        paxos_entry.data = entry.data["data"];
    }
    
    return paxos_entry;
}

size_t PaxosWALEntry::size() const {
    // Approximate size: type + timestamp + slot + round + node_id + data
    return sizeof(type) + sizeof(timestamp) + sizeof(slot) + sizeof(round) +
           node_id.size() + data.dump().size();
}

// ============================================================================
// PaxosWAL Implementation
// ============================================================================

PaxosWAL::PaxosWAL(const PaxosWALConfig& config)
    : config_(config) {
}

PaxosWAL::~PaxosWAL() {
    if (wal_manager_) {
        wal_manager_->flush();
    }
}

bool PaxosWAL::initialize() {
    try {
        // Create directories if they don't exist
        std::filesystem::create_directories(config_.wal_directory);
        std::filesystem::create_directories(config_.snapshot_directory);
        
        // Initialize WAL manager
        WALManagerConfig wal_config;
        wal_config.wal_directory = config_.wal_directory;
        wal_config.segment_size = config_.segment_size;
        wal_config.max_segments = config_.max_segments;
        wal_config.sync_on_write = config_.sync_on_write;
        wal_config.write_buffer_size = config_.write_buffer_size;
        
        wal_manager_ = std::make_unique<WALManager>(wal_config);
        
        spdlog::info("PaxosWAL initialized: wal_dir={}, snapshot_dir={}",
                     config_.wal_directory, config_.snapshot_directory);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize PaxosWAL: {}", e.what());
        return false;
    }
}

LSN PaxosWAL::logEntry(const PaxosWALEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!wal_manager_) {
        throw std::runtime_error("PaxosWAL not initialized");
    }
    
    WALEntry wal_entry = entry.toWALEntry();
    LSN lsn = wal_manager_->append(wal_entry);
    
    spdlog::debug("PaxosWAL: Logged entry type={} slot={} round={} at LSN={}",
                  static_cast<int>(entry.type), entry.slot, entry.round, lsn.toString());
    
    return lsn;
}

LSN PaxosWAL::logPrepare(uint64_t slot, uint64_t round, const std::string& node_id) {
    nlohmann::json data = nlohmann::json::object();
    PaxosWALEntry entry = createEntry(PaxosWALEntryType::PREPARE, slot, round, node_id, data);
    return logEntry(entry);
}

LSN PaxosWAL::logPromise(uint64_t slot, uint64_t round, const std::string& node_id,
                         uint64_t accepted_round, const nlohmann::json& accepted_value) {
    nlohmann::json data = {
        {"accepted_round", accepted_round},
        {"accepted_value", accepted_value}
    };
    PaxosWALEntry entry = createEntry(PaxosWALEntryType::PROMISE, slot, round, node_id, data);
    return logEntry(entry);
}

LSN PaxosWAL::logAccept(uint64_t slot, uint64_t round, const std::string& node_id,
                        const ConsensusLogEntry& value) {
    nlohmann::json data = {
        {"value", {
            {"index", value.index},
            {"term", value.term},
            {"operation", value.operation},
            {"data", value.data}
        }}
    };
    PaxosWALEntry entry = createEntry(PaxosWALEntryType::ACCEPT, slot, round, node_id, data);
    return logEntry(entry);
}

LSN PaxosWAL::logAccepted(uint64_t slot, uint64_t round, const std::string& node_id) {
    nlohmann::json data = nlohmann::json::object();
    PaxosWALEntry entry = createEntry(PaxosWALEntryType::ACCEPTED, slot, round, node_id, data);
    return logEntry(entry);
}

LSN PaxosWAL::logCommit(uint64_t slot, const ConsensusLogEntry& value) {
    nlohmann::json data = {
        {"value", {
            {"index", value.index},
            {"term", value.term},
            {"operation", value.operation},
            {"data", value.data}
        }}
    };
    PaxosWALEntry entry = createEntry(PaxosWALEntryType::COMMIT, slot, 0, "", data);
    return logEntry(entry);
}

std::vector<PaxosWALEntry> PaxosWAL::readEntries(const LSN& start_lsn,
                                                  const std::optional<LSN>& end_lsn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!wal_manager_) {
        throw std::runtime_error("PaxosWAL not initialized");
    }
    
    std::vector<WALEntry> wal_entries = wal_manager_->readRange(start_lsn, end_lsn);
    std::vector<PaxosWALEntry> paxos_entries = {};

    paxos_entries.reserve(wal_entries.size());
    
    for (const auto& wal_entry : wal_entries) {
        try {
            PaxosWALEntry paxos_entry = PaxosWALEntry::fromWALEntry(wal_entry);
            paxos_entries.push_back(std::move(paxos_entry));
        } catch (const std::exception& e) {
            spdlog::warn("Skipping non-Paxos WAL entry at LSN {}: {}",
                        wal_entry.lsn.toString(), e.what());
        }
    }
    
    spdlog::debug("PaxosWAL: Read {} entries from LSN {} to {}",
                  paxos_entries.size(), start_lsn.toString(),
                  end_lsn ? end_lsn->toString() : "end");
    
    return paxos_entries;
}

LSN PaxosWAL::getCurrentLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!wal_manager_) {
        return LSN(0, 0);
    }
    
    return wal_manager_->getCurrentLSN();
}

LSN PaxosWAL::getOldestLSN() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!wal_manager_) {
        return LSN(0, 0);
    }
    
    return wal_manager_->getOldestLSN();
}

void PaxosWAL::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (wal_manager_) {
        wal_manager_->flush();
        spdlog::debug("PaxosWAL: Flushed to disk");
    }
}

bool PaxosWAL::shouldCreateSnapshot([[maybe_unused]] size_t operations_since_last) const {
    return operations_since_last >= config_.snapshot_interval;
}

bool PaxosWAL::compact(const LSN& up_to_lsn, const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!wal_manager_) {
        spdlog::error("PaxosWAL::compact: WAL not initialized");
        return false;
    }

    try {
        // 1. Write a SNAPSHOT marker entry so replay code can detect the
        //    compaction boundary.
        nlohmann::json marker_data = {
            {"compacted_up_to_lsn", up_to_lsn.toString()},
            {"compaction_ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
        PaxosWALEntry marker = createEntry(
            PaxosWALEntryType::SNAPSHOT, 0, 0, node_id, marker_data);
        wal_manager_->append(marker.toWALEntry());

        // 2. Delegate to WALManager for the actual segment truncation
        wal_manager_->truncate(up_to_lsn);

        spdlog::info("PaxosWAL: compacted WAL up to LSN={} node={}",
                     up_to_lsn.toString(), node_id);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("PaxosWAL::compact: exception: {}", e.what());
        return false;
    }
}

PaxosWALEntry PaxosWAL::createEntry(PaxosWALEntryType type, uint64_t slot,
                                     uint64_t round, const std::string& node_id,
                                     const nlohmann::json& data) {
    PaxosWALEntry entry;
    entry.type = type;
    entry.slot = slot;
    entry.round = round;
    entry.node_id = node_id;
    entry.data = data;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return entry;
}

} // namespace sharding
} // namespace themis

