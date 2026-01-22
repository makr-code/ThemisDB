#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <openssl/sha.h>

namespace themis {

BackupManager::BackupManager(std::shared_ptr<RocksDBWrapper> db_wrapper) 
    : db_wrapper_(std::move(db_wrapper)) {
    if (!db_wrapper_) {
        THEMIS_ERROR("BackupManager: db_wrapper is null");
        throw std::invalid_argument("db_wrapper cannot be null");
    }
    
    // Detect RAID configuration on construction
    raid_config_ = detectRAIDConfiguration();
    
    if (raid_config_.mode != RAIDMode::NONE) {
        THEMIS_INFO("BackupManager initialized with RAID mode: {}, group: {}, shards: {}", 
                    raidModeToString(raid_config_.mode),
                    raid_config_.raid_group,
                    raid_config_.shards.size());
    }
}

BackupManager::~BackupManager() = default;

// Static helper to parse RAID mode from string
RAIDMode BackupManager::parseRAIDMode(const std::string& mode_str) {
    std::string mode_lower = mode_str;
    std::transform(mode_lower.begin(), mode_lower.end(), mode_lower.begin(), ::tolower);
    
    if (mode_lower == "raid0") return RAIDMode::RAID0;
    if (mode_lower == "raid1") return RAIDMode::RAID1;
    if (mode_lower == "raid5") return RAIDMode::RAID5;
    if (mode_lower == "raid6") return RAIDMode::RAID6;
    if (mode_lower == "raid10") return RAIDMode::RAID10;
    
    return RAIDMode::NONE;
}

// Static helper to convert RAID mode to string
std::string BackupManager::raidModeToString(RAIDMode mode) {
    switch (mode) {
        case RAIDMode::RAID0: return "RAID0";
        case RAIDMode::RAID1: return "RAID1";
        case RAIDMode::RAID5: return "RAID5";
        case RAIDMode::RAID6: return "RAID6";
        case RAIDMode::RAID10: return "RAID10";
        default: return "NONE";
    }
}

// Detect RAID configuration from environment
RAIDConfig BackupManager::detectRAIDConfiguration() {
    RAIDConfig config;
    
    // Read RAID group from environment (e.g., "raid0", "raid1", "raid5")
    const char* raid_group_env = std::getenv("THEMIS_RAID_GROUP");
    if (!raid_group_env) {
        // No RAID configuration
        return config;
    }
    
    config.raid_group = raid_group_env;
    config.mode = parseRAIDMode(config.raid_group);
    
    // Read shard ID
    const char* shard_id_env = std::getenv("THEMIS_SHARD_ID");
    std::string shard_id = shard_id_env ? shard_id_env : "unknown";
    
    // Read all shards in this RAID group (comma-separated list)
    // Format: "themis-raid5-shard1:18765,themis-raid5-shard2:18765,themis-raid5-shard3:18765"
    const char* shards_env = std::getenv("THEMIS_SHARDS");
    if (shards_env) {
        std::string shards_str = shards_env;
        std::istringstream ss(shards_str);
        std::string shard;
        uint32_t index = 0;
        
        while (std::getline(ss, shard, ',')) {
            ShardInfo info;
            info.shard_id = shard;
            info.endpoint = shard;
            info.shard_index = index++;
            info.is_parity_shard = false;  // Will be set below for RAID5/6
            config.shards.push_back(info);
        }
    }
    
    // Determine backup requirements based on RAID type
    switch (config.mode) {
        case RAIDMode::RAID0:
            // RAID0: Data striped across all shards, no redundancy
            // All shards required for complete backup
            config.data_shards = config.shards.size();
            config.parity_shards = 0;
            config.is_coordinated = true;  // Need all shards for complete data
            break;
            
        case RAIDMode::RAID1:
            // RAID1: Full mirroring across shards
            // Any single shard contains complete data, but all should be backed up for redundancy
            config.data_shards = config.shards.size();
            config.parity_shards = 0;
            config.is_coordinated = false;  // Any shard backup is complete, but all recommended
            break;
            
        case RAIDMode::RAID5:
            // RAID5: N-1 data shards, 1 parity shard
            if (config.shards.size() >= 3) {
                config.data_shards = config.shards.size() - 1;
                config.parity_shards = 1;
                config.is_coordinated = true;  // Need all shards (data + parity)
            }
            break;
            
        case RAIDMode::RAID6:
            // RAID6: N-2 data shards, 2 parity shards
            if (config.shards.size() >= 4) {
                config.data_shards = config.shards.size() - 2;
                config.parity_shards = 2;
                config.is_coordinated = true;  // Need all shards (data + double parity)
            }
            break;
            
        case RAIDMode::RAID10:
            // RAID10: Striping + Mirroring
            // Striped across N/2 groups, each group mirrored
            // All shards should be backed up
            config.data_shards = config.shards.size();
            config.parity_shards = 0;
            config.is_coordinated = true;  // Need all shards for complete striped data
            break;
            
        case RAIDMode::NONE:
        default:
            // No RAID configuration
            break;
    }
    
    return config;
}

std::string BackupManager::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

Result<void> BackupManager::createManifest(const std::string& backup_dir, const std::string& type,
                                           uint64_t sequence_number) {
    namespace fs = std::filesystem;
    try {
        nlohmann::json manifest;
        manifest["type"] = type;
        manifest["timestamp"] = getTimestamp();
        manifest["sequence_number"] = sequence_number;
        manifest["db_path"] = db_wrapper_->getConfig().db_path;
        
        // Add RAID configuration information
        manifest["raid"]["mode"] = raidModeToString(raid_config_.mode);
        manifest["raid"]["raid_group"] = raid_config_.raid_group;
        manifest["raid"]["is_coordinated"] = raid_config_.is_coordinated;
        
        if (raid_config_.mode == RAIDMode::RAID5 || raid_config_.mode == RAIDMode::RAID6) {
            manifest["raid"]["data_shards"] = raid_config_.data_shards;
            manifest["raid"]["parity_shards"] = raid_config_.parity_shards;
            manifest["raid"]["total_shards"] = raid_config_.shards.size();
            
            // List all shards for verification
            nlohmann::json shards_array = nlohmann::json::array();
            for (const auto& shard : raid_config_.shards) {
                nlohmann::json shard_obj;
                shard_obj["shard_id"] = shard.shard_id;
                shard_obj["shard_index"] = shard.shard_index;
                shard_obj["is_parity"] = shard.is_parity_shard;
                shards_array.push_back(shard_obj);
            }
            manifest["raid"]["shards"] = shards_array;
            
            // Add important warning about backup completeness for RAID5/6
            manifest["raid"]["backup_note"] = 
                "For RAID5/6: This backup MUST include ALL shards (data + parity) for complete recovery. "
                "Each shard backup is essential - losing even one shard may prevent full data recovery.";
        }
        
        auto manifest_path = fs::path(backup_dir) / "MANIFEST.json";
        std::ofstream out(manifest_path);
        if (!out) {
            THEMIS_ERROR("Failed to create manifest file: {}", manifest_path.string());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                          "Failed to open manifest file: " + manifest_path.string());
        }
        out << manifest.dump(2);
        out.close();
        
        THEMIS_INFO("Created backup manifest: type={}, seq={}, RAID={}, path={}", 
                    type, sequence_number, raidModeToString(raid_config_.mode), 
                    manifest_path.string());
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception creating manifest: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                      "Exception creating manifest: " + std::string(e.what()));
    }
}

Result<void> BackupManager::readManifest(const std::string& backup_dir, std::string& type,
                                         uint64_t& sequence_number) {
    namespace fs = std::filesystem;
    try {
        auto manifest_path = fs::path(backup_dir) / "MANIFEST.json";
        if (!fs::exists(manifest_path)) {
            THEMIS_ERROR("Manifest not found: {}", manifest_path.string());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                          "Manifest not found: " + manifest_path.string());
        }
        
        std::ifstream in(manifest_path);
        if (!in) {
            THEMIS_ERROR("Failed to read manifest: {}", manifest_path.string());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                          "Failed to read manifest: " + manifest_path.string());
        }
        
        nlohmann::json manifest;
        in >> manifest;
        
        type = manifest.value("type", "unknown");
        sequence_number = manifest.value("sequence_number", 0ULL);
        
        THEMIS_INFO("Read backup manifest: type={}, seq={}", type, sequence_number);
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception reading manifest: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                      "Exception reading manifest: " + std::string(e.what()));
    }
}

uint64_t BackupManager::getCurrentSequenceNumber() const {
    // RocksDB exposes sequence number via GetLatestSequenceNumber()
    // For now, we use a simplified approach with timestamp
    // In production, integrate with RocksDB's sequence number API
    return static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
}

Result<void> BackupManager::copyWALFiles(const std::string& src_dir, const std::string& dest_dir,
                                         uint64_t min_sequence) {
    (void)min_sequence;
    namespace fs = std::filesystem;
    try {
        std::error_code ec;
        fs::create_directories(dest_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create WAL dest directory: {}", ec.message());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED, 
                          "Failed to create WAL dest directory: " + ec.message());
        }
        
        int count = 0;
        for (const auto& entry : fs::directory_iterator(src_dir)) {
            auto path = entry.path();
            auto ext = path.extension().string();
            
            // Copy .log files (RocksDB WAL files) and .sst files (SST files)
            if (ext == ".log" || ext == ".sst") {
                auto dest_path = fs::path(dest_dir) / path.filename();
                fs::copy_file(path, dest_path, fs::copy_options::overwrite_existing, ec);
                if (ec) {
                    THEMIS_ERROR("Failed to copy WAL file {}: {}", path.string(), ec.message());
                    return ErrVoid(errors::ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED, 
                                  "Failed to copy WAL file: " + path.string());
                }
                count++;
            }
        }
        
        THEMIS_INFO("Copied {} WAL files from {} to {}", count, src_dir, dest_dir);
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception copying WAL files: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED, 
                      "Exception copying WAL files: " + std::string(e.what()));
    }
}

Result<std::string> BackupManager::createFullBackup(const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        // Create timestamped backup directory
        auto timestamp = getTimestamp();
        auto backup_dir = fs::path(dest_dir) / ("full_" + timestamp);
        
        THEMIS_INFO("Creating full backup to {}", backup_dir.string());
        
        std::error_code ec;
        fs::create_directories(backup_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create backup directory: {}", ec.message());
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create backup directory: " + ec.message());
        }
        
        // Create RocksDB checkpoint
        auto checkpoint_dir = backup_dir / "checkpoint";
        if (!db_wrapper_->createCheckpoint(checkpoint_dir.string())) {
            THEMIS_ERROR("Failed to create RocksDB checkpoint");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create RocksDB checkpoint");
        }
        
        // Archive current WAL files
        auto wal_dir = backup_dir / "wal";
        auto db_path = db_wrapper_->getConfig().db_path;
        auto wal_result = copyWALFiles(db_path, wal_dir.string(), 0);
        if (!wal_result) {
            THEMIS_ERROR("Failed to copy WAL files");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to copy WAL files: " + wal_result.error().message());
        }
        
        // Create manifest
        uint64_t seq = getCurrentSequenceNumber();
        auto manifest_result = createManifest(backup_dir.string(), "full", seq);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to create backup manifest");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create backup manifest: " + manifest_result.error().message());
        }
        
        // Update 'latest' symlink
        auto latest_link = fs::path(dest_dir) / "latest";
        if (fs::exists(latest_link)) {
            fs::remove(latest_link, ec);
        }
        fs::create_symlink(backup_dir.filename(), latest_link, ec);
        if (ec) {
            THEMIS_WARN("Failed to create 'latest' symlink: {}", ec.message());
            // Non-critical, continue
        }
        
        THEMIS_INFO("Full backup created successfully: {}", backup_dir.string());
        return Ok(backup_dir.string());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception creating full backup: {}", e.what());
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                               "Exception creating full backup: " + std::string(e.what()));
    }
}

Result<std::string> BackupManager::createIncrementalBackup(const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        // Find last backup to determine min sequence number
        auto backups = listBackups(dest_dir);
        uint64_t min_sequence = 0;
        
        if (!backups.empty()) {
            auto last_backup_dir = fs::path(dest_dir) / backups.back();
            std::string type;
            auto result = readManifest(last_backup_dir.string(), type, min_sequence);
            if (!result) {
                THEMIS_WARN("Could not read last backup manifest, creating full backup instead");
                return createFullBackup(dest_dir);
            }
        } else {
            THEMIS_INFO("No previous backups found, creating full backup");
            return createFullBackup(dest_dir);
        }
        
        // Create timestamped incremental backup directory
        auto timestamp = getTimestamp();
        auto backup_dir = fs::path(dest_dir) / ("incr_" + timestamp);
        
        THEMIS_INFO("Creating incremental backup to {} (seq >= {})", 
                    backup_dir.string(), min_sequence);
        
        std::error_code ec;
        fs::create_directories(backup_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create incremental backup directory: {}", ec.message());
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create incremental backup directory: " + ec.message());
        }
        
        // Copy WAL files since last backup
        auto wal_dir = backup_dir / "wal";
        auto db_path = db_wrapper_->getConfig().db_path;
        auto wal_result = copyWALFiles(db_path, wal_dir.string(), min_sequence);
        if (!wal_result) {
            THEMIS_ERROR("Failed to copy incremental WAL files");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to copy incremental WAL files: " + wal_result.error().message());
        }
        
        // Create manifest
        uint64_t seq = getCurrentSequenceNumber();
        auto manifest_result = createManifest(backup_dir.string(), "incremental", seq);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to create incremental backup manifest");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create incremental backup manifest: " + manifest_result.error().message());
        }
        
        THEMIS_INFO("Incremental backup created successfully: {}", backup_dir.string());
        return Ok(backup_dir.string());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception creating incremental backup: {}", e.what());
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                               "Exception creating incremental backup: " + std::string(e.what()));
    }
}

Result<std::string> BackupManager::createDifferentialBackup(const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        // Find last FULL backup to determine base point
        auto backups = listBackups(dest_dir);
        uint64_t base_sequence = 0;
        bool found_full = false;
        
        // Search for the last full backup
        for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
            if (it->starts_with("full_")) {
                auto full_backup_dir = fs::path(dest_dir) / *it;
                std::string type;
                auto result = readManifest(full_backup_dir.string(), type, base_sequence);
                if (result && type == "full") {
                    found_full = true;
                    break;
                }
            }
        }
        
        if (!found_full) {
            THEMIS_INFO("No full backup found, creating full backup instead");
            return createFullBackup(dest_dir);
        }
        
        // Create timestamped differential backup directory
        auto timestamp = getTimestamp();
        auto backup_dir = fs::path(dest_dir) / ("diff_" + timestamp);
        
        THEMIS_INFO("Creating differential backup to {} (since seq {})", 
                    backup_dir.string(), base_sequence);
        
        std::error_code ec;
        fs::create_directories(backup_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create differential backup directory: {}", ec.message());
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create differential backup directory: " + ec.message());
        }
        
        // Copy WAL files since last full backup
        auto wal_dir = backup_dir / "wal";
        auto db_path = db_wrapper_->getConfig().db_path;
        auto wal_result = copyWALFiles(db_path, wal_dir.string(), base_sequence);
        if (!wal_result) {
            THEMIS_ERROR("Failed to copy differential WAL files");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to copy differential WAL files: " + wal_result.error().message());
        }
        
        // Create manifest
        uint64_t seq = getCurrentSequenceNumber();
        auto manifest_result = createManifest(backup_dir.string(), "differential", seq);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to create differential backup manifest");
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                                   "Failed to create differential backup manifest: " + manifest_result.error().message());
        }
        
        THEMIS_INFO("Differential backup created successfully: {}", backup_dir.string());
        return Ok(backup_dir.string());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception creating differential backup: {}", e.what());
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_CREATION_FAILED, 
                               "Exception creating differential backup: " + std::string(e.what()));
    }
}

Result<void> BackupManager::archiveWAL(const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        std::error_code ec;
        fs::create_directories(dest_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create WAL archive directory: {}", ec.message());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED, 
                          "Failed to create WAL archive directory: " + ec.message());
        }
        
        auto db_path = db_wrapper_->getConfig().db_path;
        return copyWALFiles(db_path, dest_dir, 0);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception archiving WAL: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED, 
                      "Exception archiving WAL: " + std::string(e.what()));
    }
}

Result<void> BackupManager::restoreFromBackup(const std::string& src_dir) {
    namespace fs = std::filesystem;
    try {
        THEMIS_INFO("Restoring database from backup: {}", src_dir);
        
        // Read backup manifest
        std::string type;
        uint64_t sequence_number;
        auto manifest_result = readManifest(src_dir, type, sequence_number);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to read backup manifest");
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED, 
                          "Failed to read backup manifest: " + manifest_result.error().message());
        }
        
        if (type != "full") {
            THEMIS_ERROR("Can only restore from full backups (got type={})", type);
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_INVALID_TYPE, 
                          "Can only restore from full backups, got type: " + type);
        }
        
        // Verify backup integrity
        auto verify_result = verifyBackup(src_dir);
        if (!verify_result) {
            THEMIS_ERROR("Backup integrity verification failed");
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                          "Backup integrity verification failed: " + verify_result.error().message());
        }
        
        // Restore from checkpoint
        auto checkpoint_dir = fs::path(src_dir) / "checkpoint";
        if (!fs::exists(checkpoint_dir)) {
            THEMIS_ERROR("Checkpoint directory not found: {}", checkpoint_dir.string());
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, 
                          "Checkpoint directory not found: " + checkpoint_dir.string());
        }
        
        if (!db_wrapper_->restoreFromCheckpoint(checkpoint_dir.string())) {
            THEMIS_ERROR("Failed to restore from checkpoint");
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED, 
                          "Failed to restore from checkpoint");
        }
        
        THEMIS_INFO("Database restored successfully from {}", src_dir);
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception restoring from backup: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED, 
                      "Exception restoring from backup: " + std::string(e.what()));
    }
}

std::vector<std::string> BackupManager::listBackups(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    std::vector<std::string> backups;
    
    try {
        if (!fs::exists(backup_dir)) {
            return backups;
        }
        
        for (const auto& entry : fs::directory_iterator(backup_dir)) {
            if (entry.is_directory()) {
                auto name = entry.path().filename().string();
                if (name.starts_with("full_") || name.starts_with("incr_") || name.starts_with("diff_")) {
                    backups.push_back(name);
                }
            }
        }
        
        // Sort by timestamp (filename format ensures correct sort order)
        std::sort(backups.begin(), backups.end());
        
        THEMIS_INFO("Found {} backups in {}", backups.size(), backup_dir);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception listing backups: {}", e.what());
    }
    
    return backups;
}

Result<void> BackupManager::verifyBackup(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    try {
        // Verify manifest exists
        auto manifest_path = fs::path(backup_dir) / "MANIFEST.json";
        if (!fs::exists(manifest_path)) {
            THEMIS_ERROR("Backup manifest missing: {}", manifest_path.string());
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                          "Backup manifest missing: " + manifest_path.string());
        }
        
        // Read manifest
        std::string type;
        uint64_t seq;
        auto result = readManifest(backup_dir, type, seq);
        if (!result) {
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                          "Failed to read manifest: " + result.error().message());
        }
        
        // Verify checkpoint directory exists for full backups
        if (type == "full") {
            auto checkpoint_dir = fs::path(backup_dir) / "checkpoint";
            if (!fs::exists(checkpoint_dir)) {
                THEMIS_ERROR("Checkpoint directory missing: {}", checkpoint_dir.string());
                return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                              "Checkpoint directory missing: " + checkpoint_dir.string());
            }
            
            // Verify checkpoint has RocksDB files
            bool has_files = false;
            for (const auto& entry : fs::directory_iterator(checkpoint_dir)) {
                if (entry.is_regular_file()) {
                    has_files = true;
                    break;
                }
            }
            if (!has_files) {
                THEMIS_ERROR("Checkpoint directory is empty: {}", checkpoint_dir.string());
                return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                              "Checkpoint directory is empty: " + checkpoint_dir.string());
            }
        }
        
        // Verify WAL directory exists
        auto wal_dir = fs::path(backup_dir) / "wal";
        if (!fs::exists(wal_dir)) {
            THEMIS_WARN("WAL directory missing (non-critical): {}", wal_dir.string());
        }
        
        // RAID5/6 specific verification: Check that all required shards are backed up
        if (raid_config_.mode == RAIDMode::RAID5 || raid_config_.mode == RAIDMode::RAID6) {
            auto raid_result = verifyRAIDShardsInBackup(backup_dir, raid_config_);
            if (!raid_result) {
                THEMIS_ERROR("RAID5/6 backup incomplete: not all shards are backed up");
                return ErrVoid(errors::ErrorCode::ERR_BACKUP_INCOMPLETE, 
                              "RAID5/6 backup incomplete: " + raid_result.error().message());
            }
        }
        
        THEMIS_INFO("Backup verification passed: {}", backup_dir);
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception verifying backup: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                      "Exception verifying backup: " + std::string(e.what()));
    }
}

Result<void> BackupManager::verifyRAIDShardsInBackup(const std::string& backup_dir, 
                                                     const RAIDConfig& raid_config) {
    namespace fs = std::filesystem;
    
    if (raid_config.shards.empty()) {
        THEMIS_WARN("No RAID shards configured, skipping shard verification");
        return OkVoid();
    }
    
    // Read the manifest to get expected shards
    std::ifstream manifest_file(fs::path(backup_dir) / "MANIFEST.json");
    if (!manifest_file) {
        THEMIS_ERROR("Cannot read manifest for RAID verification");
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                      "Cannot read manifest for RAID verification");
    }
    
    nlohmann::json manifest;
    try {
        manifest_file >> manifest;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse manifest: {}", e.what());
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT, 
                      "Failed to parse manifest: " + std::string(e.what()));
    }
    
    // Check if RAID info exists in manifest
    if (!manifest.contains("raid") || !manifest["raid"].contains("total_shards")) {
        THEMIS_WARN("Manifest missing RAID information for RAID5/6 backup - skipping shard verification");
        THEMIS_WARN("This may indicate an older backup format without RAID metadata");
        // Cannot verify without RAID info, but log warning and continue
        return OkVoid();
    }
    
    uint32_t expected_shards = manifest["raid"]["total_shards"];
    uint32_t data_shards = manifest["raid"].value("data_shards", 0);
    uint32_t parity_shards = manifest["raid"].value("parity_shards", 0);
    
    // Log the requirement
    THEMIS_INFO("Verifying RAID5/6 backup completeness: expecting {} total shards ({} data + {} parity)",
                expected_shards, data_shards, parity_shards);
    
    // For distributed RAID, each node should have its own backup
    // Here we verify that this node's backup is complete
    // A coordinated backup system would verify all nodes have backed up
    
    // Check raid_topology directory if it exists (for coordinated backups)
    auto raid_topology_dir = fs::path(backup_dir) / "raid_topology";
    if (fs::exists(raid_topology_dir)) {
        // Count shard backup directories
        uint32_t found_shards = 0;
        for (const auto& entry : fs::directory_iterator(raid_topology_dir)) {
            if (entry.is_directory() && entry.path().filename().string().find("shard") != std::string::npos) {
                found_shards++;
            }
        }
        
        if (found_shards < expected_shards) {
            THEMIS_ERROR("Incomplete RAID5/6 backup: found {} of {} required shards", 
                        found_shards, expected_shards);
            THEMIS_ERROR("  For RAID5/6, ALL shards (data + parity) must be backed up for complete recovery!");
            return ErrVoid(errors::ErrorCode::ERR_BACKUP_INCOMPLETE, 
                          "Incomplete RAID5/6 backup: found " + std::to_string(found_shards) + 
                          " of " + std::to_string(expected_shards) + " required shards");
        }
        
        THEMIS_INFO("RAID5/6 backup verification passed: all {} shards present", found_shards);
    }
    
    return OkVoid();
}

Result<void> BackupManager::isBackupComplete(const std::string& backup_dir, 
                                             const RAIDConfig& raid_config) {
    // For RAID5/6, verify all shards are backed up
    if (raid_config.mode == RAIDMode::RAID5 || raid_config.mode == RAIDMode::RAID6) {
        return verifyRAIDShardsInBackup(backup_dir, raid_config);
    }
    
    // For non-RAID or RAID0/1/10, standard verification is sufficient
    return verifyBackup(backup_dir);
}

Result<std::string> BackupManager::calculateChecksum(const std::string& file_path) {
    namespace fs = std::filesystem;
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, 
                                   "Failed to open file for checksum: " + file_path);
        }
        
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        constexpr size_t buffer_size = 8192;
        std::vector<char> buffer(buffer_size);
        
        while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
            SHA256_Update(&sha256, buffer.data(), file.gcount());
        }
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);
        
        // Convert to hex string
        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        
        return Ok(oss.str());
    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                               "Exception calculating checksum: " + std::string(e.what()));
    }
}

Result<void> BackupManager::verifyChecksum(const std::string& file_path, 
                                           const std::string& expected_checksum) {
    auto result = calculateChecksum(file_path);
    if (!result) {
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_VERIFICATION_FAILED, 
                      "Failed to calculate checksum: " + result.error().message());
    }
    
    if (*result != expected_checksum) {
        return ErrVoid(errors::ErrorCode::ERR_BACKUP_CHECKSUM_MISMATCH, 
                      "Checksum mismatch for file: " + file_path);
    }
    
    return OkVoid();
}

Result<std::string> BackupManager::compressBackup(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(backup_dir)) {
            return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, 
                                   "Backup directory not found: " + backup_dir);
        }
        
        // Create compressed file path
        auto compressed_file = backup_dir + ".tar.gz";
        
        // Use system tar command for compression
        std::string cmd = "tar -czf \"" + compressed_file + "\" -C \"" + 
                         fs::path(backup_dir).parent_path().string() + "\" \"" + 
                         fs::path(backup_dir).filename().string() + "\"";
        
        int result = system(cmd.c_str());
        if (result != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED, 
                                   "Failed to compress backup directory");
        }
        
        THEMIS_INFO("Backup compressed successfully: {}", compressed_file);
        return Ok(compressed_file);
    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED, 
                               "Exception compressing backup: " + std::string(e.what()));
    }
}

Result<std::string> BackupManager::decompressBackup(const std::string& compressed_file, 
                                                    const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(compressed_file)) {
            return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, 
                                   "Compressed file not found: " + compressed_file);
        }
        
        std::error_code ec;
        fs::create_directories(dest_dir, ec);
        if (ec) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED, 
                                   "Failed to create destination directory: " + ec.message());
        }
        
        // Use system tar command for decompression
        std::string cmd = "tar -xzf \"" + compressed_file + "\" -C \"" + dest_dir + "\"";
        
        int result = system(cmd.c_str());
        if (result != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED, 
                                   "Failed to decompress backup file");
        }
        
        THEMIS_INFO("Backup decompressed successfully to: {}", dest_dir);
        return Ok(dest_dir);
    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED, 
                               "Exception decompressing backup: " + std::string(e.what()));
    }
}

} // namespace themis
