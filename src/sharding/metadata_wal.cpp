/*
 * ThemisDB | File: metadata_wal.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 182
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=44 | delta=41 | status=divergent
 * External Severity (v3): C=5, H=30, M=9
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2026 ThemisDB
// Licensed under MIT License
// Phase 2.2: Metadata Shard Durability

#include "sharding/metadata_wal.h"
#include <spdlog/spdlog.h>
#include <filesystem>

namespace themisdb {
namespace sharding {

MetadataWAL::MetadataWAL(const MetadataWALConfig& config)
    : config_(config) {
}

MetadataWAL::~MetadataWAL() {
}

bool MetadataWAL::initialize() {
    try {
        // Create WAL directory if it doesn't exist
        if (!std::filesystem::exists(config_.wal_directory)) {
            std::filesystem::create_directories(config_.wal_directory);
        }
        
        // Create snapshot directory if it doesn't exist
        if (!std::filesystem::exists(config_.snapshot_directory)) {
            std::filesystem::create_directories(config_.snapshot_directory);
        }
        
        // Initialize WAL manager
        themis::sharding::WALManagerConfig wal_config;
        wal_config.wal_directory = config_.wal_directory;
        wal_config.segment_size = config_.segment_size;
        wal_config.sync_on_write = config_.sync_on_write;
        wal_config.write_buffer_size = config_.write_buffer_size;
        
        wal_manager_ = std::make_unique<themis::sharding::WALManager>(wal_config);
        
        spdlog::info("MetadataWAL initialized: wal_dir={}, snapshot_dir={}",
                    config_.wal_directory, config_.snapshot_directory);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Exception initializing MetadataWAL: {}", e.what());
        return false;
    }
}

LSN MetadataWAL::logPut(
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value,
    uint64_t version
) {
    MetadataWALEntry entry;
    entry.type = MetadataWALEntryType::PUT;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.partition = partition;
    entry.key = key;
    entry.value = value;
    entry.version = version;
    
    return writeEntry(entry);
}

LSN MetadataWAL::logDelete(
    MetadataPartitionKey partition,
    const std::string& key,
    uint64_t version
) {
    MetadataWALEntry entry;
    entry.type = MetadataWALEntryType::DELETE_OP;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.partition = partition;
    entry.key = key;
    entry.value = nullptr;  // No value for delete
    entry.version = version;
    
    return writeEntry(entry);
}

LSN MetadataWAL::logUpdate(
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value,
    uint64_t version
) {
    MetadataWALEntry entry;
    entry.type = MetadataWALEntryType::UPDATE;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.partition = partition;
    entry.key = key;
    entry.value = value;
    entry.version = version;
    
    return writeEntry(entry);
}

std::vector<MetadataWALEntry> MetadataWAL::readEntries(const LSN& start_lsn) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    
    std::vector<MetadataWALEntry> entries;
    
    if (!wal_manager_) {
        spdlog::warn("WAL manager not initialized, cannot read entries");
        return entries;
    }
    
    try {
        // Read WAL entries from WALManager
        auto wal_entries = wal_manager_->readRange(start_lsn, std::nullopt);
        
        // Convert to MetadataWALEntry
        for (const auto& wal_entry : wal_entries) {
            // Only process metadata-specific entry types (120-122)
            const int wal_type = static_cast<int>(wal_entry.type);
            if (wal_type >= 120 && wal_type <= 122) {
                entries.push_back(MetadataWALEntry::fromWALEntry(wal_entry));
            }
        }
        
        spdlog::debug("Read {} metadata entries from WAL starting at LSN ({}, {})",
                     entries.size(), start_lsn.segment, start_lsn.offset);
    } catch (const std::exception& e) {
        spdlog::error("Exception reading metadata WAL entries: {}", e.what());
    }
    
    return entries;
}

void MetadataWAL::flush() {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    
    if (wal_manager_) {
        wal_manager_->flush();
    }
}

LSN MetadataWAL::writeEntry(const MetadataWALEntry& entry) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    
    if (!wal_manager_) {
        spdlog::error("WAL manager not initialized");
        return LSN(0, 0);
    }
    
    try {
        // Convert to WALEntry and write
        WALEntry wal_entry = entry.toWALEntry();
        LSN lsn = wal_manager_->append(wal_entry);
        
        spdlog::debug("Logged metadata {} to WAL: partition={}, key={}, version={}, LSN=({}, {})",
                     entry.type == MetadataWALEntryType::PUT ? "PUT" :
                     entry.type == MetadataWALEntryType::DELETE_OP ? "DELETE" : "UPDATE",
                     static_cast<int>(entry.partition),
                     entry.key,
                     entry.version,
                     lsn.segment,
                     lsn.offset);
        
        return lsn;
    } catch (const std::exception& e) {
        spdlog::error("Exception writing metadata WAL entry: {}", e.what());
        return LSN(0, 0);
    }
}

} // namespace sharding
} // namespace themisdb

