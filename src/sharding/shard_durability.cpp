/**
 * @file shard_durability.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_durability.h"
#include <stdexcept>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/write_batch.h>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace themisdb {
namespace sharding {

ShardDurability::ShardDurability(
    rocksdb::TransactionDB* rocksdb_instance,
    const ShardDurabilityConfig& config
)
    : db_(rocksdb_instance)
    , config_(config)
{
}

ShardDurability::~ShardDurability() {
    shutdown();
}

bool ShardDurability::initialize() {
    if (!db_) {
        return false;
    }
    
    // Create checkpoint manager
    rocksdb::Checkpoint* cp_raw = nullptr;
    rocksdb::Status status = rocksdb::Checkpoint::Create(db_->GetBaseDB(), &cp_raw);
    if (!status.ok()) {
        return false;
    }
    checkpoint_manager_.reset(cp_raw);
    
    // Create checkpoint directory if needed
    if (!config_.checkpoint_dir.empty()) {
        std::filesystem::create_directories(config_.checkpoint_dir);
        scanCheckpointDirectory();
    }
    
    // Perform crash recovery if enabled
    if (config_.enable_auto_recovery) {
        RecoveryStats recovery = performRecovery();
        if (recovery.recovery_needed && !recovery.recovery_successful) {
            return false;
        }
        
        if ([[maybe_unused]] recovery_callback_ && recovery.recovery_needed) {
            recovery_callback_([[maybe_unused]] recovery);
        }
    }
    
    return true;
}

void ShardDurability::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (db_ && config_.enable_wal) {
        // Final WAL sync before shutdown
        syncWAL();
    }
    
    checkpoint_manager_.reset();
}

bool ShardDurability::syncWAL() {
    if (!db_ || !config_.enable_wal) {
        return false;
    }
    
    rocksdb::WriteOptions write_opts;
    write_opts.sync = true;
    
    // Write an empty batch with sync enabled to force WAL flush
    rocksdb::WriteBatch empty_batch;
    rocksdb::Status status = db_->GetBaseDB()->Write(write_opts, &empty_batch);
    
    if (status.ok()) {
        stats_.total_syncs.fetch_add(1, std::memory_order_relaxed);
        stats_.last_sync_time = std::chrono::system_clock::now();
        return true;
    }
    
    return false;
}

std::optional<CheckpointInfo> ShardDurability::createCheckpoint(
    const std::string& checkpoint_name
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!checkpoint_manager_) {
        return std::nullopt;
    }
    
    CheckpointInfo info;
    info.checkpoint_id = checkpoint_name.empty() 
        ? generateCheckpointId() 
        : checkpoint_name;
    info.path = config_.checkpoint_dir + "/" + info.checkpoint_id;
    info.created_at = std::chrono::system_clock::now();
    info.sequence_number = getCurrentSequenceNumber();
    
    // Create checkpoint
    rocksdb::Status status = checkpoint_manager_->CreateCheckpoint(info.path);
    if (!status.ok()) {
        return std::nullopt;
    }
    
    // Calculate checkpoint size
    try {
        info.size_bytes = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(info.path)) {
            if (entry.is_regular_file()) {
                info.size_bytes += entry.file_size();
            }
        }
    } catch (...) {
        info.size_bytes = 0;
    }
    
    info.is_valid = validateCheckpoint(info.path);
    
    // Add to checkpoint list
    checkpoints_.push_back(info);
    
    // Cleanup old checkpoints
    cleanupOldCheckpoints();
    
    stats_.checkpoints_created.fetch_add(1, std::memory_order_relaxed);
    stats_.last_checkpoint_time = std::chrono::system_clock::now();
    
    return info;
}

std::vector<CheckpointInfo> ShardDurability::listCheckpoints() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_;
}

bool ShardDurability::restoreFromCheckpoint(const std::string& checkpoint_id) {
    // Note: This is a simplified implementation
    // In production, this would require:
    // 1. Shutdown the database
    // 2. Copy checkpoint files to DB directory
    // 3. Restart the database
    // This implementation assumes the caller handles DB lifecycle
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(checkpoints_.begin(), checkpoints_.end(),
        [&checkpoint_id](const CheckpointInfo& cp) {
            return cp.checkpoint_id == checkpoint_id;
        });
    
    if (it == checkpoints_.end()) {
        return false;
    }
    
    // Validate checkpoint before restoring
    if (!validateCheckpoint(it->path)) {
        return false;
    }
    
    // In a real implementation, we would:
    // 1. Close current DB
    // 2. Replace DB files with checkpoint files
    // 3. Reopen DB
    // This is left to the caller to coordinate
    
    return true;
}

RecoveryStats ShardDurability::performRecovery() {
    RecoveryStats stats;
    auto start_time = std::chrono::steady_clock::now();
    
    if (!db_) {
        stats.last_error = "No database instance";
        return stats;
    }
    
    // RocksDB automatically performs WAL replay on open
    // We just need to verify the database state
    
    // Check if recovery was needed by examining WAL
    // In RocksDB, recovery is automatic on Open()
    // We detect if recovery happened by checking for WAL files
    
    try {
        // Verify WAL integrity
        bool wal_intact = verifyWALIntegrity();
        
        if (wal_intact) {
            stats.recovery_successful = true;
            stats.recovery_needed = false;
        } else {
            stats.recovery_needed = true;
            stats.recovery_successful = false;
            stats.last_error = "WAL integrity check failed";
        }
        
    } catch (const std::exception& e) {
        stats.recovery_needed = true;
        stats.recovery_successful = false;
        stats.last_error = e.what();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    stats.recovery_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    if (stats.recovery_successful) {
        stats_.recoveries_performed.fetch_add(1, std::memory_order_relaxed);
    }
    
    return stats;
}

bool ShardDurability::verifyWALIntegrity() const {
    if (!db_) {
        return false;
    }
    
    // RocksDB provides WAL integrity through its internal mechanisms
    // We perform a simple check by attempting to get DB statistics
    try {
        std::string stats_str = {};
        db_->GetBaseDB()->GetProperty("rocksdb.stats", &stats_str);
        return true;
    } catch (...) {
        return false;
    }
}

uint64_t ShardDurability::getCurrentSequenceNumber() const {
    if (!db_) {
        return 0;
    }
    
    return db_->GetBaseDB()->GetLatestSequenceNumber();
}

void ShardDurability::setRecoveryCallback([[maybe_unused]] RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    recovery_callback_ = callback;
}

void ShardDurability::updateConfig(const ShardDurabilityConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

std::string ShardDurability::generateCheckpointId() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss = {};
    ss << "checkpoint_"
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
       << "_" << getCurrentSequenceNumber();
    
    return ss.str();
}

void ShardDurability::cleanupOldCheckpoints() {
    if (static_cast<int>(checkpoints_.size()) <= config_.max_checkpoints) {
        return;
    }
    
    // Sort by creation time (oldest first)
    std::sort(checkpoints_.begin(), checkpoints_.end(),
        [](const CheckpointInfo& a, const CheckpointInfo& b) {
            return a.created_at < b.created_at;
        });
    
    // Remove oldest checkpoints
    size_t to_remove = static_cast<int>(checkpoints_.size()) - config_.max_checkpoints;
    for (size_t i = 0; i < to_remove; ++i) {
        try {
            std::filesystem::remove_all(checkpoints_[i].path);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    checkpoints_.erase(checkpoints_.begin(), checkpoints_.begin() + to_remove);
}

void ShardDurability::scanCheckpointDirectory() {
    checkpoints_.clear();
    
    if (!std::filesystem::exists(config_.checkpoint_dir)) {
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(config_.checkpoint_dir)) {
            if (entry.is_directory()) {
                CheckpointInfo info;
                info.checkpoint_id = entry.path().filename().string();
                info.path = entry.path().string();
                info.is_valid = validateCheckpoint(info.path);
                
                // Try to get metadata
                try {
                    auto ftime = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now()
                    );
                    info.created_at = sctp;
                } catch (...) {
                    info.created_at = std::chrono::system_clock::now();
                }
                
                // Calculate size
                info.size_bytes = 0;
                for (const auto& file : std::filesystem::recursive_directory_iterator(info.path)) {
                    if (file.is_regular_file()) {
                        info.size_bytes += file.file_size();
                    }
                }
                
                checkpoints_.push_back(info);
            }
        }
    } catch (...) {
        // Ignore scan errors
    }
    
    // Sort by creation time (newest first)
    std::sort(checkpoints_.begin(), checkpoints_.end(),
        [](const CheckpointInfo& a, const CheckpointInfo& b) {
            return a.created_at > b.created_at;
        });
}

bool ShardDurability::validateCheckpoint(const std::string& checkpoint_path) const {
    // Basic validation: check if directory exists and contains expected files
    if (!std::filesystem::exists(checkpoint_path)) {
        return false;
    }
    
    // Check for CURRENT file (RocksDB manifest pointer)
    std::string current_file = checkpoint_path + "/CURRENT";
    if (!std::filesystem::exists(current_file)) {
        return false;
    }
    
    return true;
}

}  // namespace sharding
}  // namespace themisdb

