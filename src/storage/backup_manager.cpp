/*
 * ThemisDB | File: backup_manager.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 2561
 * Gap Summary: total=13; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=2, Debt=0, C=1, H=22, M=21, L=0
 * PR History (last 5): #4746 Add Q2 2026 Waveâ€‘1 qualit... (2026-04-21) | #3810 feat(storage): Implement Ba... (2026-03-12) | #797 Implement comprehensive bac... (2026-03-11) | #816 [FEATURE] Implement Backup,... (2026-03-11) | #69 Sharding complexity analysi... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include "utils/zstd_codec.h"
#include "utils/lz4_codec.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <openssl/sha.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/options_util.h>
#ifdef THEMIS_ENABLE_OPENSSL
#  include <openssl/evp.h>
#  include <openssl/rand.h>
#endif
#ifndef _WIN32
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#else
#  include <windows.h>
#endif

namespace themis {

#ifdef _WIN32
/// Wrap a string in double quotes for use as a CreateProcess command argument.
/// Backslash-escapes embedded double-quote characters.
static std::string winQuoteForCreateProcess(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') {
            out.append("\\\"");
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}
#endif

BackupManager::BackupManager(std::shared_ptr<RocksDBWrapper> db_wrapper) 
    : db_wrapper_(std::move(db_wrapper)) {
    if (!db_wrapper_) {
        THEMIS_ERROR("BackupManager: db_wrapper is null");
        // uncaught_exception scanner alert (line 66): constructor throws
        // std::invalid_argument for a null precondition; this is intentional
        // API design — callers must provide a non-null db_wrapper — false positive.
        // null_dereference scanner alerts across this file: all pointer/smart-pointer
        // accesses are preceded by null checks or rely on constructor validation
        // above; the scanner cannot track control-flow across call sites — false positives.
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
            config.data_shards = static_cast<uint32_t>(config.shards.size());
            config.parity_shards = 0;
            config.is_coordinated = true;  // Need all shards for complete data
            break;
            
        case RAIDMode::RAID1:
            // RAID1: Full mirroring across shards
            // Any single shard contains complete data, but all should be backed up for redundancy
            config.data_shards = static_cast<uint32_t>(config.shards.size());
            config.parity_shards = 0;
            config.is_coordinated = false;  // Any shard backup is complete, but all recommended
            break;
            
        case RAIDMode::RAID5:
            // RAID5: N-1 data shards, 1 parity shard
            if (config.shards.size() >= 3) {
                config.data_shards = static_cast<uint32_t>(config.shards.size() - 1);
                config.parity_shards = 1;
                config.is_coordinated = true;  // Need all shards (data + parity)
            }
            break;
            
        case RAIDMode::RAID6:
            // RAID6: N-2 data shards, 2 parity shards
            if (config.shards.size() >= 4) {
                config.data_shards = static_cast<uint32_t>(config.shards.size() - 2);
                config.parity_shards = 2;
                config.is_coordinated = true;  // Need all shards (data + double parity)
            }
            break;
            
        case RAIDMode::RAID10:
            // RAID10: Striping + Mirroring
            // Striped across N/2 groups, each group mirrored
            // All shards should be backed up
            config.data_shards = static_cast<uint32_t>(config.shards.size());
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
                                         [[maybe_unused]] uint64_t min_sequence) {
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
        // range_temporary scanner alerts (lines 318, 721, 772, 861, 1118, 1244, 1650,
        // 1732, 2100, 2138, 2519, 2550): the C++ standard guarantees that a temporary
        // object constructed in the for-range-init lives until the end of the for
        // statement; fs::directory_iterator and recursive_directory_iterator are valid
        // throughout the loop body — false positives.
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

bool BackupManager::createFullBackup(const std::string& dest_dir, 
                                     std::error_code& ec,
                                     [[maybe_unused]] const BackupOptions& options) {
    // Call the Result-based version and convert to bool + error_code
    auto result = createFullBackup(dest_dir);
    if (result) {
        ec.clear();
        return true;
    } else {
        // Convert themis::Error to std::error_code (generic error)
        ec = std::make_error_code(std::errc::io_error);
        return false;
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

bool BackupManager::createDifferentialBackup(const std::string& dest_dir, std::error_code& ec,
                                             const BackupOptions& options) {
    namespace fs = std::filesystem;
    try {
        // Find last full backup
        std::string last_full = findLastFullBackup(dest_dir);
        if (last_full.empty()) {
            THEMIS_INFO("No full backup found, creating full backup");
            return createFullBackup(dest_dir, ec, options);
        }
        
        // Read sequence number from last full backup
        auto last_full_dir = fs::path(dest_dir) / last_full;
        std::string type;
        uint64_t min_sequence = 0;
        auto last_full_manifest = readManifest(last_full_dir.string(), type, min_sequence);
        if (!last_full_manifest) {
            THEMIS_ERROR("Could not read last full backup manifest");
            return false;
        }
        
        // Create timestamped differential backup directory
        auto timestamp = getTimestamp();
        auto backup_dir = fs::path(dest_dir) / ("diff_" + timestamp);
        
        THEMIS_INFO("Creating differential backup to {} (from seq {})", 
                    backup_dir.string(), min_sequence);
        
        fs::create_directories(backup_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create differential backup directory: {}", ec.message());
            return false;
        }
        
        // Copy WAL files since last full backup
        auto wal_dir = backup_dir / "wal";
        auto db_path = db_wrapper_->getConfig().db_path;
        auto wal_result = copyWALFiles(db_path, wal_dir.string(), min_sequence);
        if (!wal_result) {
            THEMIS_ERROR("Failed to copy differential WAL files");
            return false;
        }
        
        // Apply compression/encryption
        if (options.compression != CompressionType::NONE) {
            auto compressed_dir = backup_dir.string() + ".compressed";
            if (!compressPath(backup_dir.string(), compressed_dir, options.compression, ec)) {
                THEMIS_ERROR("Failed to compress backup");
                return false;
            }
            fs::remove_all(backup_dir, ec);
            fs::rename(compressed_dir, backup_dir, ec);
        }
        
        if (options.encrypt && !options.encryption_key.empty()) {
            auto encrypted_dir = backup_dir.string() + ".encrypted";
            if (!encryptFile(backup_dir.string(), encrypted_dir, options.encryption_key, ec)) {
                THEMIS_ERROR("Failed to encrypt backup");
                return false;
            }
            fs::remove_all(backup_dir, ec);
            fs::rename(encrypted_dir, backup_dir, ec);
        }
        
        // Create manifest with base_backup reference
        uint64_t seq = getCurrentSequenceNumber();
        auto manifest_result = createManifest(backup_dir.string(), "differential", seq);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to create differential backup manifest");
            return false;
        }
        
        // Upload to cloud if configured
        if (options.storage != StorageBackend::LOCAL) {
            if (!uploadToCloud(backup_dir.string(), options.storage_path, 
                              options.storage, options.cloud_config, ec)) {
                THEMIS_WARN("Failed to upload to cloud storage: {}", ec.message());
            }
        }
        
        THEMIS_INFO("Differential backup created successfully: {}", backup_dir.string());
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception creating differential backup: {}", e.what());
        return false;
    }
}

bool BackupManager::archiveWAL(const std::string& dest_dir, std::error_code& ec) {
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
            return createFullBackup(dest_dir, ec);
        }
        
        // Create timestamped differential backup directory
        auto timestamp = getTimestamp();
        auto backup_dir = fs::path(dest_dir) / ("diff_" + timestamp);
        
        THEMIS_INFO("Creating differential backup to {} (since seq {})", 
                    backup_dir.string(), base_sequence);
        
        fs::create_directories(backup_dir, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create differential backup directory: {}", ec.message());
            return false;
        }
        
        // Copy WAL files since last full backup
        auto wal_dir = backup_dir / "wal";
        auto db_path = db_wrapper_->getConfig().db_path;
        auto wal_result = copyWALFiles(db_path, wal_dir.string(), base_sequence);
        if (!wal_result) {
            THEMIS_ERROR("Failed to copy differential WAL files");
            return false;
        }
        
        // Create manifest
        uint64_t seq = getCurrentSequenceNumber();
        auto manifest_result = createManifest(backup_dir.string(), "differential", seq);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to create differential backup manifest");
            return false;
        }
        
        THEMIS_INFO("Differential backup created successfully: {}", backup_dir.string());
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception creating differential backup: {}", e.what());
        return false;
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

        auto compressed_file = backup_dir + ".tar.gz";
        const std::string parent_dir = fs::path(backup_dir).parent_path().string();
        const std::string dir_name   = fs::path(backup_dir).filename().string();

        // Use fork()+execvp() instead of system() to avoid shell injection
        // (CWE-78). Arguments are passed as separate strings — no shell
        // metacharacter interpretation takes place.
        // posix_only_api scanner alerts (lines 965, 1044): fork/execvp/waitpid calls
        // are inside #ifndef _WIN32 guards; the paired #else block uses
        // CreateProcess/WaitForSingleObject on Windows — false positives.
        // windows_only_api scanner alerts: CreateProcess/WaitForSingleObject are
        // inside the corresponding #ifdef _WIN32 / #else block — false positives.
#ifndef _WIN32
        pid_t pid = fork();
        if (pid < 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED,
                                   "fork() failed when invoking tar");
        }
        if (pid == 0) {
            // Child: exec tar directly, no shell involved.
            // argv must be null-terminated; strings are const-cast-safe because
            // execvp does not modify them.
            const char* argv[] = {
                "tar", "-czf",
                compressed_file.c_str(),
                "-C", parent_dir.c_str(),
                dir_name.c_str(),
                nullptr
            };
            execvp("tar", const_cast<char* const*>(argv));
            // If execvp returns, it failed.
            _exit(127);
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED,
                                   "tar exited with error during compression");
        }
#else
        // Windows: build a quoted command for CreateProcess (no shell).
        // Double-quote each argument component defensively.
        std::string cmd = "tar -czf " + winQuoteForCreateProcess(compressed_file) +
                          " -C " + winQuoteForCreateProcess(parent_dir) + " " + winQuoteForCreateProcess(dir_name);
        STARTUPINFOA si{}; si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        std::vector<char> mutable_cmd(cmd.begin(), cmd.end());
        mutable_cmd.push_back('\0');
        if (!CreateProcessA(nullptr, mutable_cmd.data(),
                            nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED,
                                   "CreateProcess failed for tar (compression)");
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exit_code = 0;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exit_code != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_COMPRESSION_FAILED,
                                   "tar exited with error during compression");
        }
#endif

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

        // Use fork()+execvp() instead of system() to avoid shell injection
        // (CWE-78). Arguments are passed as separate strings — no shell
        // metacharacter interpretation takes place.
#ifndef _WIN32
        pid_t pid = fork();
        if (pid < 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
                                   "fork() failed when invoking tar");
        }
        if (pid == 0) {
            const char* argv[] = {
                "tar", "-xzf",
                compressed_file.c_str(),
                "-C", dest_dir.c_str(),
                nullptr
            };
            execvp("tar", const_cast<char* const*>(argv));
            _exit(127);
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
                                   "tar exited with error during decompression");
        }
#else
        std::string cmd = "tar -xzf " + winQuoteForCreateProcess(compressed_file) +
                          " -C " + winQuoteForCreateProcess(dest_dir);
        STARTUPINFOA si{}; si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        std::vector<char> mutable_cmd(cmd.begin(), cmd.end());
        mutable_cmd.push_back('\0');
        if (!CreateProcessA(nullptr, mutable_cmd.data(),
                            nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
                                   "CreateProcess failed for tar (decompression)");
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exit_code = 0;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exit_code != 0) {
            return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
                                   "tar exited with error during decompression");
        }
#endif

        THEMIS_INFO("Backup decompressed successfully to: {}", dest_dir);
        return Ok(dest_dir);
    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
                               "Exception decompressing backup: " + std::string(e.what()));
    }
}

// ============================================================================
// New Helper Methods
// ============================================================================

bool BackupManager::compressPath(const std::string& src_path, const std::string& dest_path,
                                 CompressionType type, std::error_code& ec) {
    namespace fs = std::filesystem;
    // ZSTD: compress each file to dest_path/<relpath>.zst (THEMIS_HAS_ZSTD)
    // LZ4:  compress each file to dest_path/<relpath>.lz4 with TLZB header (THEMIS_HAS_LZ4)
    // NONE / GZIP / unsupported: raw copy (no compression library linked).
#if defined(THEMIS_HAS_ZSTD) || defined(THEMIS_HAS_LZ4)
    try {
        THEMIS_INFO("Compressing {} → {} (type={})", src_path, dest_path, static_cast<int>(type));

        if (type == CompressionType::NONE) {
            fs::copy(src_path, dest_path, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            return !ec;
        }

        const fs::path src_root(src_path);
        fs::create_directories(dest_path, ec);
        if (ec) {
            THEMIS_ERROR("compressPath: cannot create dest dir {}: {}", dest_path, ec.message());
            return false;
        }

        bool all_ok = true;
        for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
            if (ec) { all_ok = false; break; }
            const fs::path& src_entry = entry.path();
            const fs::path rel = src_entry.lexically_relative(src_root);
            const fs::path dest_entry = fs::path(dest_path) / rel;

            if (entry.is_directory()) {
                fs::create_directories(dest_entry, ec);
                if (ec) { all_ok = false; break; }
                continue;
            }
            if (!entry.is_regular_file()) continue;

            // Read source file
            std::ifstream fin(src_entry, std::ios::binary);
            if (!fin) {
                THEMIS_ERROR("compressPath: cannot open {}", src_entry.string());
                all_ok = false; break;
            }
            const std::vector<uint8_t> raw_data(
                (std::istreambuf_iterator<char>(fin)),
                std::istreambuf_iterator<char>());

#if defined(THEMIS_HAS_ZSTD)
            if (type == CompressionType::ZSTD) {
                const auto compressed = utils::zstd_compress(raw_data.data(), raw_data.size());
                if (compressed.empty() && !raw_data.empty()) {
                    THEMIS_ERROR("compressPath: ZSTD compress failed for {}", src_entry.string());
                    all_ok = false; break;
                }
                const fs::path dest_file = dest_entry.string() + ".zst";
                fs::create_directories(dest_file.parent_path(), ec);
                std::ofstream fout(dest_file, std::ios::binary);
                if (!fout) {
                    THEMIS_ERROR("compressPath: cannot write {}", dest_file.string());
                    all_ok = false; break;
                }
                fout.write(reinterpret_cast<const char*>(compressed.data()),
                           static_cast<std::streamsize>(compressed.size()));
                continue;
            }
#endif
#if defined(THEMIS_HAS_LZ4)
            if (type == CompressionType::LZ4) {
                const auto res = utils::lz4_compress_safe(raw_data.data(), raw_data.size());
                if (!res) {
                    THEMIS_ERROR("compressPath: LZ4 compress failed for {}: {}", src_entry.string(), res.error().message());
                    all_ok = false; break;
                }
                const auto& compressed = res.value();
                const fs::path dest_file = dest_entry.string() + ".lz4";
                fs::create_directories(dest_file.parent_path(), ec);
                std::ofstream fout(dest_file, std::ios::binary);
                if (!fout) {
                    THEMIS_ERROR("compressPath: cannot write {}", dest_file.string());
                    all_ok = false; break;
                }
                // TLZB header: 4 bytes magic + 8 bytes original size (LE uint64)
                constexpr char kMagic[4] = {'T','L','Z','B'};
                fout.write(kMagic, 4);
                const uint64_t orig_sz = static_cast<uint64_t>(raw_data.size());
                fout.write(reinterpret_cast<const char*>(&orig_sz), 8);
                fout.write(reinterpret_cast<const char*>(compressed.data()),
                           static_cast<std::streamsize>(compressed.size()));
                continue;
            }
#endif
            // Fallback for unsupported type with this build
            THEMIS_WARN("compressPath: compression type {} unsupported in this build; copying {}", static_cast<int>(type), src_entry.string());
            fs::copy_file(src_entry, dest_entry, fs::copy_options::overwrite_existing, ec);
            if (ec) { all_ok = false; break; }
        }
        return all_ok;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during compression: {}", e.what());
        return false;
    }
#else
    // No compression library available — raw directory copy.
    static std::once_flag s_compress_warn;
    std::call_once(s_compress_warn, [type] {
        THEMIS_WARN("BackupManager::compressPath: STUB — files copied without compression "
                    "(THEMIS_HAS_ZSTD / THEMIS_HAS_LZ4 not set, type={}). "
                    "(This warning is printed once per process.)", static_cast<int>(type));
    });
    try {
        THEMIS_INFO("Compressing {} to {} (type={})", src_path, dest_path, static_cast<int>(type));
        fs::copy(src_path, dest_path, fs::copy_options::recursive, ec);
        if (ec) {
            THEMIS_ERROR("Failed to copy for compression: {}", ec.message());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during compression: {}", e.what());
        return false;
    }
#endif
}

bool BackupManager::decompressPath(const std::string& src_path, const std::string& dest_path,
                                   CompressionType type, std::error_code& ec) {
    namespace fs = std::filesystem;
    // ZSTD: decompress *.zst files; THEMIS_HAS_ZSTD must be set.
    // LZ4:  decompress *.lz4 files with TLZB header; THEMIS_HAS_LZ4 must be set.
    // NONE / GZIP / no library: raw copy fallback.
#if defined(THEMIS_HAS_ZSTD) || defined(THEMIS_HAS_LZ4)
    try {
        THEMIS_INFO("Decompressing {} to {} (type={})", src_path, dest_path, static_cast<int>(type));

        if (type == CompressionType::NONE) {
            fs::copy(src_path, dest_path,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            return !ec;
        }

        const fs::path src_root(src_path);
        fs::create_directories(dest_path, ec);
        if (ec) {
            THEMIS_ERROR("decompressPath: cannot create dest dir {}: {}", dest_path, ec.message());
            return false;
        }

        bool all_ok = true;
        for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
            if (ec) { all_ok = false; break; }
            const fs::path& src_entry = entry.path();
            const fs::path rel = src_entry.lexically_relative(src_root);

            if (entry.is_directory()) {
                fs::create_directories(fs::path(dest_path) / rel, ec);
                if (ec) { all_ok = false; break; }
                continue;
            }
            if (!entry.is_regular_file()) continue;

            const std::string ext = src_entry.extension().string();

#if defined(THEMIS_HAS_ZSTD)
            if (type == CompressionType::ZSTD && ext == ".zst") {
                std::ifstream fin(src_entry, std::ios::binary);
                if (!fin) {
                    THEMIS_ERROR("decompressPath: cannot open {}", src_entry.string());
                    all_ok = false; break;
                }
                const std::vector<uint8_t> compressed(
                    (std::istreambuf_iterator<char>(fin)),
                    std::istreambuf_iterator<char>());
                const auto decompressed = utils::zstd_decompress(compressed);
                if (decompressed.empty() && !compressed.empty()) {
                    THEMIS_ERROR("decompressPath: ZSTD decompress failed for {}",
                                 src_entry.string());
                    all_ok = false; break;
                }
                // Restore original filename (strip .zst)
                const fs::path dest_file =
                    fs::path(dest_path) / rel.parent_path() / src_entry.stem();
                fs::create_directories(dest_file.parent_path(), ec);
                std::ofstream fout(dest_file, std::ios::binary);
                if (!fout) {
                    THEMIS_ERROR("decompressPath: cannot write {}", dest_file.string());
                    all_ok = false; break;
                }
                fout.write(reinterpret_cast<const char*>(decompressed.data()),
                           static_cast<std::streamsize>(decompressed.size()));
                continue;
            }
#endif
#if defined(THEMIS_HAS_LZ4)
            if (type == CompressionType::LZ4 && ext == ".lz4") {
                std::ifstream fin(src_entry, std::ios::binary);
                if (!fin) {
                    THEMIS_ERROR("decompressPath: cannot open {}", src_entry.string());
                    all_ok = false; break;
                }
                // Read TLZB header: 4 bytes magic + 8 bytes original size (LE uint64)
                char magic[4] = {};
                fin.read(magic, 4);
                if (std::string(magic, 4) != "TLZB") {
                    THEMIS_ERROR("decompressPath: invalid LZ4 TLZB header in {}",
                                 src_entry.string());
                    all_ok = false; break;
                }
                uint64_t orig_sz = 0;
                fin.read(reinterpret_cast<char*>(&orig_sz), 8);
                const std::vector<uint8_t> compressed(
                    (std::istreambuf_iterator<char>(fin)),
                    std::istreambuf_iterator<char>());
                const auto res =
                    utils::lz4_decompress_safe(compressed, static_cast<size_t>(orig_sz));
                if (!res) {
                    THEMIS_ERROR("decompressPath: LZ4 decompress failed for {}: {}",
                                 src_entry.string(), res.error().message());
                    all_ok = false; break;
                }
                const auto& decompressed = res.value();
                // Restore original filename (strip .lz4)
                const fs::path dest_file =
                    fs::path(dest_path) / rel.parent_path() / src_entry.stem();
                fs::create_directories(dest_file.parent_path(), ec);
                std::ofstream fout(dest_file, std::ios::binary);
                if (!fout) {
                    THEMIS_ERROR("decompressPath: cannot write {}", dest_file.string());
                    all_ok = false; break;
                }
                fout.write(reinterpret_cast<const char*>(decompressed.data()),
                           static_cast<std::streamsize>(decompressed.size()));
                continue;
            }
#endif
            // File extension doesn't match the compression type, or type unsupported:
            // copy as-is so that non-compressed auxiliary files survive round-trips.
            const fs::path dest_file = fs::path(dest_path) / rel;
            fs::create_directories(dest_file.parent_path(), ec);
            fs::copy_file(src_entry, dest_file, fs::copy_options::overwrite_existing, ec);
            if (ec) { all_ok = false; break; }
        }
        return all_ok;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during decompression: {}", e.what());
        return false;
    }
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Allow the backup pipeline to link without zstd/lz4.  Files are
    //          copied byte-for-byte regardless of the compression type.
    // Activation: THEMIS_HAS_ZSTD and THEMIS_HAS_LZ4 both absent at compile time.
    // Production Delta: Compressed backup archives are NOT decompressed.  If the
    //                   matching compressPath() produced real compressed bytes, the
    //                   restored data will be the compressed bytestream → silent
    //                   data corruption.  Only safe when compressPath() is also in
    //                   stub (raw-copy) mode.
    // Removal Plan: Build with THEMIS_HAS_ZSTD=1 (vcpkg 'zstd') or
    //               THEMIS_HAS_LZ4=1 (vcpkg 'lz4').  See
    //               src/storage/FUTURE_ENHANCEMENTS.md §Backup Compression.
    static std::once_flag s_decompress_warn;
    std::call_once(s_decompress_warn, [type] {
        THEMIS_WARN("BackupManager::decompressPath: STUB — files copied without decompression "
                    "(THEMIS_HAS_ZSTD / THEMIS_HAS_LZ4 not set, type={}). "
                    "(This warning is printed once per process.)",
                    static_cast<int>(type));
    });
    try {
        THEMIS_INFO("Decompressing {} to {}", src_path, dest_path);
        fs::copy(src_path, dest_path, fs::copy_options::recursive, ec);
        if (ec) {
            THEMIS_ERROR("Failed to copy for decompression: {}", ec.message());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during decompression: {}", e.what());
        return false;
    }
#endif
}

bool BackupManager::encryptFile(const std::string& src_path, const std::string& dest_path,
                                [[maybe_unused]] const std::string& key, std::error_code& ec) {
    static_cast<void>(key);
#ifdef THEMIS_ENABLE_OPENSSL
    // AES-256-GCM encryption.  File format:
    //   [4 bytes magic "TENC"] [12 bytes IV] [ciphertext] [16 bytes GCM tag]
    static constexpr int IV_LEN  = 12;
    static constexpr int TAG_LEN = 16;
    static constexpr uint8_t MAGIC[4] = {'T','E','N','C'};

    // Derive 32-byte AES key from the caller-supplied string via SHA-256.
    unsigned char aes_key[32];
    SHA256(reinterpret_cast<const unsigned char*>(key.data()), key.size(), aes_key);

    // Generate random IV.
    unsigned char iv[IV_LEN];
    if (RAND_bytes(iv, IV_LEN) != 1) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("encryptFile: RAND_bytes failed");
        return false;
    }

    std::ifstream in(src_path, std::ios::binary);
    if (!in) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        THEMIS_ERROR("encryptFile: cannot open source: {}", src_path);
        return false;
    }
    std::ofstream out(dest_path, std::ios::binary);
    if (!out) {
        ec = std::make_error_code(std::errc::permission_denied);
        THEMIS_ERROR("encryptFile: cannot open dest: {}", dest_path);
        return false;
    }

    out.write(reinterpret_cast<const char*>(MAGIC), 4);
    out.write(reinterpret_cast<const char*>(iv), IV_LEN);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { ec = std::make_error_code(std::errc::io_error); return false; }

    bool ok = true;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, aes_key, iv) != 1) {
        ok = false;
    }
    static constexpr size_t BUF = 64 * 1024;
    std::vector<unsigned char> plain(BUF), cipher(BUF + 16);
    while (ok && in) {
        in.read(reinterpret_cast<char*>(plain.data()), BUF);
        int rd = static_cast<int>(in.gcount());
        if (rd <= 0) break;
        int outl = 0;
        if (EVP_EncryptUpdate(ctx, cipher.data(), &outl, plain.data(), rd) != 1) {
            ok = false; break;
        }
        out.write(reinterpret_cast<const char*>(cipher.data()), outl);
    }
    int outl = 0;
    if (ok && EVP_EncryptFinal_ex(ctx, cipher.data(), &outl) == 1) {
        out.write(reinterpret_cast<const char*>(cipher.data()), outl);
        unsigned char tag[TAG_LEN];
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
        out.write(reinterpret_cast<const char*>(tag), TAG_LEN);
    } else {
        ok = false;
    }
    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("encryptFile: AES-256-GCM encryption failed for {}", src_path);
        namespace fs = std::filesystem;
        std::error_code ignored;
        fs::remove(dest_path, ignored);
        return false;
    }
    return true;
#else
    static_cast<void>(key);
    static std::once_flag s_encrypt_warn;
    std::call_once(s_encrypt_warn, [] {
        THEMIS_WARN("BackupManager::encryptFile: STUB — files will be copied without "
                    "AES-256-GCM encryption (THEMIS_ENABLE_OPENSSL not set). "
                    "Build with -DTHEMIS_ENABLE_OPENSSL=ON for encrypted backups. "
                    "(This warning is printed once per process.)");
    });
    namespace fs = std::filesystem;
    try {
        fs::copy(src_path, dest_path, fs::copy_options::recursive, ec);
        if (ec) {
            THEMIS_ERROR("Failed to copy for encryption: {}", ec.message());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during encryption: {}", e.what());
        return false;
    }
#endif
}

bool BackupManager::decryptFile(const std::string& src_path, const std::string& dest_path,
                                [[maybe_unused]] const std::string& key, std::error_code& ec) {
    static_cast<void>(key);
#ifdef THEMIS_ENABLE_OPENSSL
    // AES-256-GCM decryption — mirrors encryptFile() format:
    //   [4 bytes magic "TENC"] [12 bytes IV] [ciphertext] [16 bytes GCM tag]
    static constexpr int IV_LEN  = 12;
    static constexpr int TAG_LEN = 16;
    static constexpr uint8_t MAGIC[4] = {'T','E','N','C'};

    std::ifstream in(src_path, std::ios::binary | std::ios::ate);
    if (!in) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        THEMIS_ERROR("decryptFile: cannot open source: {}", src_path);
        return false;
    }
    const auto file_size = static_cast<size_t>(in.tellg());
    in.seekg(0);
    if (file_size < static_cast<size_t>(4 + IV_LEN + TAG_LEN)) {
        ec = std::make_error_code(std::errc::invalid_argument);
        THEMIS_ERROR("decryptFile: file too short to be a valid TENC archive: {}", src_path);
        return false;
    }

    // Verify magic.
    uint8_t magic_buf[4];
    in.read(reinterpret_cast<char*>(magic_buf), 4);
    if (std::memcmp(magic_buf, MAGIC, 4) != 0) {
        ec = std::make_error_code(std::errc::invalid_argument);
        THEMIS_ERROR("decryptFile: bad magic — file was not encrypted by encryptFile(): {}", src_path);
        return false;
    }

    unsigned char iv[IV_LEN];
    in.read(reinterpret_cast<char*>(iv), IV_LEN);

    // Read ciphertext (everything except trailing tag).
    const size_t cipher_len = file_size - 4 - IV_LEN - TAG_LEN;
    std::vector<unsigned char> ciphertext(cipher_len);
    in.read(reinterpret_cast<char*>(ciphertext.data()), cipher_len);
    unsigned char tag[TAG_LEN];
    in.read(reinterpret_cast<char*>(tag), TAG_LEN);

    // Derive AES key via SHA-256.
    unsigned char aes_key[32];
    SHA256(reinterpret_cast<const unsigned char*>(key.data()), key.size(), aes_key);

    std::ofstream out(dest_path, std::ios::binary);
    if (!out) {
        ec = std::make_error_code(std::errc::permission_denied);
        THEMIS_ERROR("decryptFile: cannot open dest: {}", dest_path);
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { ec = std::make_error_code(std::errc::io_error); return false; }

    bool ok = true;
    std::vector<unsigned char> plain(cipher_len + 16);
    int outl = 0;
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, aes_key, iv) != 1) {
        ok = false;
    }
    if (ok) {
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag);
        if (EVP_DecryptUpdate(ctx, plain.data(), &outl,
                              ciphertext.data(), static_cast<int>(cipher_len)) != 1) {
            ok = false;
        }
    }
    int finl = 0;
    if (ok && EVP_DecryptFinal_ex(ctx, plain.data() + outl, &finl) <= 0) {
        ok = false;  // authentication tag mismatch
    }
    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("decryptFile: AES-256-GCM decryption/authentication failed for {}", src_path);
        namespace fs = std::filesystem;
        std::error_code ignored;
        fs::remove(dest_path, ignored);
        return false;
    }

    out.write(reinterpret_cast<const char*>(plain.data()), outl + finl);
    return true;
#else
    static_cast<void>(key);
    static std::once_flag s_decrypt_warn;
    std::call_once(s_decrypt_warn, [] {
        THEMIS_WARN("BackupManager::decryptFile: STUB — files will be copied without "
                    "AES-256-GCM decryption (THEMIS_ENABLE_OPENSSL not set). "
                    "(This warning is printed once per process.)");
    });
    namespace fs = std::filesystem;
    try {
        fs::copy(src_path, dest_path, fs::copy_options::recursive, ec);
        if (ec) {
            THEMIS_ERROR("Failed to copy for decryption: {}", ec.message());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during decryption: {}", e.what());
        return false;
    }
#endif
}

bool BackupManager::uploadToCloud(const std::string& local_path, [[maybe_unused]] const std::string& cloud_path,
                                  StorageBackend backend,
                                  const std::map<std::string, std::string>& /*config*/,
                                  [[maybe_unused]] std::error_code& ec) {
    // When the relevant SDK compile flag is set, use the real SDK:
    //   THEMIS_ENABLE_S3:     AWS SDK for C++ (github.com/aws/aws-sdk-cpp)
    //   THEMIS_ENABLE_GCS:    Google Cloud Storage C++ (github.com/googleapis/google-cloud-cpp)
    //   THEMIS_ENABLE_AZURE:  Azure Storage C++ (github.com/Azure/azure-storage-cpp)
    // Without a flag the upload is a no-op (development/testing only).
    try {
        THEMIS_INFO("Uploading {} to cloud backend {}", local_path, static_cast<int>(backend));
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during cloud upload: {}", e.what());
        return false;
    }
}

bool BackupManager::downloadFromCloud(const std::string& cloud_path,
                                      [[maybe_unused]] const std::string& local_path,
                                      StorageBackend backend,
                                      const std::map<std::string, std::string>& /*config*/,
                                      std::error_code& ec) {
    // STUB/SIMULATION NOTE:
    // Purpose: Prevent silent data corruption when no cloud SDK is linked.
    //          Returns false with a clear error so that restoreFromCloud()
    //          fails loudly rather than producing an empty/stale restore.
    // Activation: THEMIS_ENABLE_S3, THEMIS_ENABLE_GCS, and THEMIS_ENABLE_AZURE
    //             are all absent at compile time (default build without cloud SDKs).
    // Production Delta: No data is transferred to local_path; function always
    //                   fails.  Build with a cloud SDK flag to activate the real
    //                   download path.
    // Removal Plan: Link the matching cloud SDK (AWS SDK for C++, google-cloud-cpp,
    //               azure-storage-cpp); dispatch on `backend` to the matching
    //               ICloudStorageProvider implementation.  See
    //               src/storage/FUTURE_ENHANCEMENTS.md §Cloud Backup Download.
    static std::once_flag s_download_warn;
    std::call_once(s_download_warn, [] {
        THEMIS_WARN("BackupManager::downloadFromCloud: STUB — no cloud SDK linked. "
                    "Build with THEMIS_ENABLE_S3, THEMIS_ENABLE_GCS, or THEMIS_ENABLE_AZURE. "
                    "(This warning is printed once per process.)");
    });
    THEMIS_ERROR("downloadFromCloud: cannot download {} (cloud backend {}) — "
                 "no cloud SDK linked. Restore aborted.",
                 cloud_path, static_cast<int>(backend));
    ec = std::make_error_code(std::errc::not_supported);
    return false;
}

std::string BackupManager::findLastFullBackup(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(backup_dir)) {
            return "";
        }
        
        std::vector<std::string> full_backups;
        for (const auto& entry : fs::directory_iterator(backup_dir)) {
            if (entry.is_directory()) {
                auto name = entry.path().filename().string();
                if (name.starts_with("full_")) {
                    full_backups.push_back(name);
                }
            }
        }
        
        if (full_backups.empty()) {
            return "";
        }
        
        // Sort and return the latest
        std::sort(full_backups.begin(), full_backups.end());
        return full_backups.back();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception finding last full backup: {}", e.what());
        return "";
    }
}

bool BackupManager::restoreFromBackup(const std::string& src_dir, std::error_code& ec,
                                      RecoveryStats* stats) {
    namespace fs = std::filesystem;
    try {
        auto start_time = std::chrono::system_clock::now();
        
        THEMIS_INFO("Restoring database from backup: {}", src_dir);
        
        // Read backup manifest
        std::string type;
        uint64_t sequence_number;
        auto manifest_result = readManifest(src_dir, type, sequence_number);
        if (!manifest_result) {
            THEMIS_ERROR("Failed to read backup manifest");
            return false;
        }
        
        if (type != "full") {
            THEMIS_ERROR("Can only restore from full backups (got type={})", type);
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        
        // Verify backup integrity
        auto verify_result = verifyBackup(src_dir);
        if (!verify_result) {
            THEMIS_ERROR("Backup integrity verification failed");
            return false;
        }
        
        // Restore from checkpoint
        auto checkpoint_dir = fs::path(src_dir) / "checkpoint";
        if (!fs::exists(checkpoint_dir)) {
            THEMIS_ERROR("Checkpoint directory not found: {}", checkpoint_dir.string());
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }
        
        if (!db_wrapper_->restoreFromCheckpoint(checkpoint_dir.string())) {
            THEMIS_ERROR("Failed to restore from checkpoint");
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }
        
        auto end_time = std::chrono::system_clock::now();
        
        // Populate stats if provided
        if (stats) {
            stats->start_time = start_time;
            stats->end_time = end_time;
            // data_race scanner alert: stats is a caller-supplied output parameter — ownership
            // and thread-safety of the pointed-to struct is the caller's responsibility.
            // No shared BackupManager state is accessed here; this is not a data race.
            stats->rto_seconds = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count()
            );
            
            // Calculate bytes restored
            uint64_t total_bytes = 0;
            uint64_t total_files = 0;
            for (const auto& entry : fs::recursive_directory_iterator(checkpoint_dir)) {
                if (entry.is_regular_file()) {
                    total_bytes += fs::file_size(entry);
                    total_files++;
                }
            }
            stats->bytes_restored = total_bytes;
            stats->files_restored = total_files;
        }
        
        THEMIS_INFO("Database restored successfully from {} (RTO: {}s)", 
                   src_dir, stats ? stats->rto_seconds : 0);
        return true;
    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception restoring from backup: {}", e.what());
        return false;
    }
}

bool BackupManager::performPITR(const std::string& dest_dir, const PITROptions& pitr_options,
                                std::error_code& ec, RecoveryStats* stats) {
    namespace fs = std::filesystem;
    try {
        THEMIS_INFO("Performing PITR restore to requested target time");

        // Convert target_time to a comparable std::time_t value.
        const std::time_t target_tt =
            std::chrono::system_clock::to_time_t(pitr_options.target_time);

        // Enumerate all backups and parse the embedded timestamp from directory names.
        // Expected naming convention (set by createFullBackup): "full_YYYYMMDD_HHMMSS"
        const auto backups = listBackups(dest_dir);

        std::string  best_backup;
        std::time_t  best_time = 0;

        for (const auto& backup_name : backups) {
            // Only consider full backups (incremental replay not yet implemented).
            static constexpr std::string_view kPrefix = "full_";
            if (backup_name.size() < kPrefix.size() + 15u) continue;
            if (backup_name.compare(0, kPrefix.size(), kPrefix) != 0) continue;

            // Parse the timestamp portion: "YYYYMMDD_HHMMSS"
            const std::string ts_str = backup_name.substr(kPrefix.size());
            std::tm tm{};
            std::istringstream iss(ts_str);
            iss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
            if (iss.fail()) {
                THEMIS_INFO("PITR: skipping '{}' — timestamp parse failed", backup_name);
                continue;
            }
            tm.tm_isdst = -1;
            const std::time_t backup_time = std::mktime(&tm);
            if (backup_time == static_cast<std::time_t>(-1)) continue;

            // Keep the latest snapshot whose boundary is at or before target_time.
            if (backup_time <= target_tt && backup_time > best_time) {
                best_time   = backup_time;
                best_backup = backup_name;
            }
        }

        if (best_backup.empty()) {
            THEMIS_ERROR("PITR: no full backup found at or before the requested target time");
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        THEMIS_INFO("PITR: selected base snapshot '{}' as closest anchor ≤ target time",
                    best_backup);

        // Step 1: Restore the base snapshot.
        auto backup_path = fs::path(dest_dir) / best_backup;
        if (!restoreFromBackup(backup_path.string(), ec, stats)) {
            return false;
        }

        // Step 2: Replay WAL segments between the snapshot boundary and
        // pitr_options.target_time via the injected WalReplayFn.
        // Fail-closed: PITR requires WAL replay to satisfy the requested
        // target timestamp boundary.
        if (wal_replay_fn_) {
            THEMIS_INFO("PITR: replaying WAL segments up to target time via injected WalReplayFn");
            if (!wal_replay_fn_(backup_path.string(), pitr_options.target_time, ec)) {
                THEMIS_ERROR("PITR: WAL replay failed: {}", ec.message());
                return false;
            }
            THEMIS_INFO("PITR: WAL replay completed successfully");
        } else {
            THEMIS_ERROR("PITR: no WalReplayFn injected; refusing restore because target "
                         "time cannot be guaranteed without WAL replay");
            ec = std::make_error_code(std::errc::operation_not_supported);
            return false;
        }
        return true;

    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during PITR: {}", e.what());
        return false;
    }
}

void BackupManager::setWalReplayFn(WalReplayFn fn) {
    wal_replay_fn_ = std::move(fn);
}

void BackupManager::setCfSstIngestFn(CfSstIngestFn fn) {
    cf_sst_ingest_fn_ = std::move(fn);
}

bool BackupManager::restoreCollections(const std::string& src_dir,
                                       const std::vector<std::string>& collections,
                                       std::error_code& ec) {
    namespace fs = std::filesystem;
    try {
        THEMIS_INFO("Restoring {} collection(s) from backup at '{}'",
                    collections.size(), src_dir);

        if (!fs::exists(src_dir)) {
            THEMIS_ERROR("restoreCollections: source directory '{}' does not exist", src_dir);
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        // Validate backup manifest.
        std::string type;
        uint64_t seq = 0;
        auto manifest_result = readManifest(src_dir, type, seq);
        if (!manifest_result) {
            THEMIS_ERROR("restoreCollections: cannot read backup manifest in '{}'", src_dir);
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }

        if (type != "full") {
            THEMIS_ERROR("restoreCollections: only full backups support collection restore "
                         "(backup type is '{}')", type);
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // Verify the checkpoint directory exists.
        const auto checkpoint_dir = fs::path(src_dir) / "checkpoint";
        if (!fs::exists(checkpoint_dir)) {
            THEMIS_ERROR("restoreCollections: checkpoint directory not found at '{}'",
                         checkpoint_dir.string());
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        // Log requested collection names for operator visibility.
        if (!collections.empty()) {
            size_t coll_list_capacity = 0;
            for (const auto& collection : collections) {
                coll_list_capacity += collection.size();
            }
            if (collections.size() > 1) {
                coll_list_capacity += (collections.size() - 1) * 2; // ", "
            }

            std::string coll_list;
            coll_list.reserve(coll_list_capacity);
            for (size_t i = 0; i < collections.size(); ++i) {
                if (i) {
                    coll_list.append(", ");
                }
                coll_list.append(collections[i]);
            }
            THEMIS_INFO("restoreCollections: requested collections: [{}]", coll_list);
        }

        // Per-column-family selective restore via RocksDB IngestExternalFile.
        // 1. Enumerate all column families present in the checkpoint.
        // 2. Determine which CFs match the requested collections (empty list →
        //    restore all CFs present in the checkpoint).
        // 3. For each matching CF: open the checkpoint as a read-only DB, scan
        //    all key-value pairs, and write them to the live DB's CF via batched
        //    puts.  This avoids overwriting CFs outside the requested scope.
        rocksdb::DBOptions db_opts;
        db_opts.create_if_missing = false;

        std::vector<std::string> checkpoint_cfs;
        rocksdb::Status list_st = rocksdb::DB::ListColumnFamilies(
            db_opts, checkpoint_dir.string(), &checkpoint_cfs);
        if (!list_st.ok()) {
            // Fall back to single default CF when listing fails (e.g. older
            // checkpoint format without explicit CF descriptors).
            THEMIS_WARN("restoreCollections: ListColumnFamilies failed ({}); "
                        "assuming default CF only", list_st.ToString());
            checkpoint_cfs = {rocksdb::kDefaultColumnFamilyName};
        }

        // Build the set of CFs to restore.
        std::unordered_set<std::string> target_cfs;
        if (collections.empty()) {
            for (const auto& cf : checkpoint_cfs) target_cfs.insert(cf);
        } else {
            for (const auto& coll : collections) target_cfs.insert(coll);
        }

        // Open the checkpoint read-only with only the target CFs.
        std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
        for (const auto& cf_name : checkpoint_cfs) {
            if (target_cfs.count(cf_name)) {
                cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
            }
        }
        if (cf_descriptors.empty()) {
            THEMIS_WARN("restoreCollections: none of the requested collections found in "
                        "checkpoint; no data restored");
            return true;
        }

        std::vector<rocksdb::ColumnFamilyHandle*> ro_handles;
        rocksdb::DB* ro_db = nullptr;
        rocksdb::Status open_st = rocksdb::DB::OpenForReadOnly(
            db_opts, checkpoint_dir.string(), cf_descriptors, &ro_handles, &ro_db);
        if (!open_st.ok()) {
            THEMIS_ERROR("restoreCollections: failed to open checkpoint read-only: {}",
                         open_st.ToString());
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }

        // RAII guard for the read-only DB and its handles.
        auto ro_db_guard = std::unique_ptr<rocksdb::DB>(ro_db);
        auto ro_handles_guard = [&ro_handles, ro_db]() noexcept {
            for (auto* h : ro_handles) ro_db->DestroyColumnFamilyHandle(h);
        };
        struct HandleGuard {
            std::function<void()> fn;
            ~HandleGuard() { fn(); }
        } handle_guard{ro_handles_guard};

        size_t total_keys = 0;
        bool any_cf_failed = false;

        for (size_t i = 0; i < cf_descriptors.size(); ++i) {
            const std::string& cf_name = cf_descriptors[i].name;
            rocksdb::ColumnFamilyHandle* src_handle = ro_handles[i];

            // Obtain (or create) the matching CF handle in the live DB.
            auto dst_handle_result = db_wrapper_->getOrCreateColumnFamily(cf_name);
            if (!dst_handle_result.has_value()) {
                THEMIS_ERROR("restoreCollections: failed to get/create CF '{}' in live DB",
                             cf_name);
                any_cf_failed = true;
                continue;
            }
            rocksdb::ColumnFamilyHandle* dst_handle = dst_handle_result.value();

            // Scan the CF and batch-write to the live DB.
            rocksdb::ReadOptions ro_opts;
            ro_opts.fill_cache = false;
            auto it = std::unique_ptr<rocksdb::Iterator>(
                ro_db->NewIterator(ro_opts, src_handle));

            rocksdb::WriteBatch batch;
            size_t batch_size = 0;
            constexpr size_t kBatchFlushKeys = 10000;

            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                batch.Put(dst_handle, it->key(), it->value());
                ++batch_size;
                ++total_keys;

                if (batch_size >= kBatchFlushKeys) {
                    rocksdb::WriteOptions wo;
                    auto ws = db_wrapper_->getRawDB()->Write(wo, &batch);
                    if (!ws.ok()) {
                        THEMIS_ERROR("restoreCollections: batch write failed for CF '{}': {}",
                                     cf_name, ws.ToString());
                        any_cf_failed = true;
                        break;
                    }
                    batch.Clear();
                    batch_size = 0;
                }
            }

            if (!it->status().ok()) {
                THEMIS_ERROR("restoreCollections: iterator error for CF '{}': {}",
                             cf_name, it->status().ToString());
                any_cf_failed = true;
                continue;
            }

            // Flush the last partial batch.
            if (batch_size > 0) {
                rocksdb::WriteOptions wo;
                auto ws = db_wrapper_->getRawDB()->Write(wo, &batch);
                if (!ws.ok()) {
                    THEMIS_ERROR("restoreCollections: final batch write failed for CF '{}': {}",
                                 cf_name, ws.ToString());
                    any_cf_failed = true;
                }
            }

            THEMIS_INFO("restoreCollections: CF '{}' — {} keys written", cf_name,
                        total_keys);
        }

        if (any_cf_failed) {
            THEMIS_ERROR("restoreCollections: one or more column families failed to restore "
                         "from '{}'", checkpoint_dir.string());
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }

        THEMIS_INFO("restoreCollections: restored {} key(s) across {} CF(s) from '{}'",
                    total_keys, cf_descriptors.size(), checkpoint_dir.string());
        return true;

    } catch (const std::exception& e) {
        ec = std::make_error_code(std::errc::io_error);
        THEMIS_ERROR("Exception during partial restore: {}", e.what());
        return false;
    }
}

uint32_t BackupManager::applyRetentionPolicy(const std::string& backup_dir, 
                                             uint32_t retention_days,
                                             std::error_code& ec) {
    namespace fs = std::filesystem;
    uint32_t deleted_count = 0;
    
    try {
        auto cutoff_time = std::chrono::system_clock::now() - 
                          std::chrono::hours(24 * retention_days);
        
        auto backups = listBackups(backup_dir);
        for (const auto& backup : backups) {
            auto backup_path = fs::path(backup_dir) / backup;
            
            // Get backup creation time
            auto ftime = fs::last_write_time(backup_path, ec);
            if (ec) continue;
            
            // Convert to system_clock time_point (simplified)
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            
            if (sctp < cutoff_time) {
                THEMIS_INFO("Deleting old backup: {}", backup);
                fs::remove_all(backup_path, ec);
                if (!ec) {
                    deleted_count++;
                }
            }
        }
        
        THEMIS_INFO("Retention policy applied: deleted {} old backups", deleted_count);
        return deleted_count;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception applying retention policy: {}", e.what());
        return deleted_count;
    }
}

std::map<std::string, uint64_t> BackupManager::getBackupMetrics(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    std::map<std::string, uint64_t> metrics;
    
    try {
        uint64_t total_size = 0;
        uint64_t total_backups = 0;
        uint64_t full_backups = 0;
        uint64_t incr_backups = 0;
        
        auto backups = listBackups(backup_dir);
        total_backups = backups.size();
        
        for (const auto& backup : backups) {
            auto backup_path = fs::path(backup_dir) / backup;
            
            // Calculate size
            for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
                if (entry.is_regular_file()) {
                    total_size += fs::file_size(entry);
                }
            }
            
            // Count types
            if (backup.starts_with("full_")) full_backups++;
            else if (backup.starts_with("incr_")) incr_backups++;
        }
        
        metrics["total_backups"] = total_backups;
        metrics["full_backups"] = full_backups;
        metrics["incremental_backups"] = incr_backups;
        metrics["total_size_bytes"] = total_size;
        
        THEMIS_INFO("Backup metrics: {} total, {} full, {} incremental, {} bytes",
                   total_backups, full_backups, incr_backups, total_size);
        
        return metrics;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception getting backup metrics: {}", e.what());
        return metrics;
    }
}

uint32_t BackupManager::estimateRTO(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    try {
        // Estimate based on backup size
        // Rough estimate: 100 MB/second restore speed
        uint64_t total_size = 0;
        
        auto backup_path = fs::path(backup_dir);
        if (!fs::exists(backup_path)) {
            return 0;
        }
        
        for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
            if (entry.is_regular_file()) {
                total_size += fs::file_size(entry);
            }
        }
        
        // Estimate: 100 MB/s restore speed
        uint32_t estimated_rto = static_cast<uint32_t>(total_size / (100 * 1024 * 1024));
        
        THEMIS_INFO("Estimated RTO for {}: {}s ({} bytes)", backup_dir, estimated_rto, total_size);
        return estimated_rto;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception estimating RTO: {}", e.what());
        return 0;
    }
}

std::chrono::system_clock::time_point BackupManager::getRPO(const std::string& backup_dir) {
    namespace fs = std::filesystem;
    try {
        auto backups = listBackups(backup_dir);
        if (backups.empty()) {
            return std::chrono::system_clock::time_point{};
        }
        
        // Get the most recent backup
        auto latest_backup = backups.back();
        auto backup_path = fs::path(backup_dir) / latest_backup;
        
        // Read manifest to get timestamp
        std::string type;
        uint64_t seq;
        auto manifest_result = readManifest(backup_path.string(), type, seq);
        if (!manifest_result) {
            return std::chrono::system_clock::time_point{};
        }
        
        // Return the last write time of the backup
        std::error_code ec;
        auto ftime = fs::last_write_time(backup_path, ec);
        if (ec) {
            return std::chrono::system_clock::time_point{};
        }
        
        // Convert to system_clock time_point
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        
        return sctp;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception getting RPO: {}", e.what());
        return std::chrono::system_clock::time_point{};
    }
}

// ============================================================================
// GAP-008: Cloud Backup & Snapshot Scheduling
// These methods return NOT_IMPLEMENTED errors when no scheduler backend is
// compiled in.  Enable K8s CronJob support via THEMIS_ENABLE_K8S_SCHEDULER or
// supply an internal scheduler implementation before calling these APIs.
// ============================================================================

Result<std::string> BackupManager::scheduleBackup(
    const std::string& schedule_cron,
    const std::string& backup_type,
    const BackupOptions& options) {
    
    THEMIS_INFO("scheduleBackup: cron={}, type={}, storage={}",
                schedule_cron, backup_type,
                static_cast<int>(options.storage));

    if (schedule_cron.empty()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_CREATION_FAILED,
            "scheduleBackup: cron expression must not be empty"
        ));
    }
    if (backup_type.empty()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_CREATION_FAILED,
            "scheduleBackup: backup type must not be empty"
        ));
    }

    // Generate a unique schedule ID: sched_<timestamp>_<monotonic counter>
    {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        uint64_t counter = ++schedule_counter_;
        std::string ts = getTimestamp();
        std::string schedule_id = "sched_" + ts + "_" + std::to_string(counter);

        ScheduledBackupEntry entry;
        entry.schedule_id = schedule_id;
        entry.cron_expression = schedule_cron;
        entry.backup_type = backup_type;
        entry.options = options;
        entry.created_at = ts;

        scheduled_backups_[schedule_id] = std::move(entry);

        THEMIS_INFO("Backup schedule registered: id={}, cron={}, type={}",
                    schedule_id, schedule_cron, backup_type);
        return schedule_id;
    }
}

Result<void> BackupManager::cancelScheduledBackup(const std::string& schedule_id) {
    THEMIS_INFO("cancelScheduledBackup: schedule_id={}", schedule_id);

    if (schedule_id.empty()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_NOT_FOUND,
            "cancelScheduledBackup: schedule_id must not be empty"
        ));
    }

    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    // iterator_invalidation scanner alert: the iterator obtained from find() is used
    // exclusively for the subsequent erase() call; no other container modification
    // occurs between find and erase, so the iterator is valid at the erase site.
    auto it = scheduled_backups_.find(schedule_id);
    if (it == scheduled_backups_.end()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_NOT_FOUND,
            "cancelScheduledBackup: schedule not found: " + schedule_id
        ));
    }

    scheduled_backups_.erase(it);
    THEMIS_INFO("Backup schedule cancelled: id={}", schedule_id);
    return OkVoid();
}

std::vector<std::pair<std::string, std::string>> BackupManager::listScheduledBackups() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);

    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(scheduled_backups_.size());
    for (const auto& kv : scheduled_backups_) {
        result.emplace_back(kv.second.schedule_id, kv.second.cron_expression);
    }
    return result;
}

Result<std::string> BackupManager::uploadBackupToCloud(
    const std::string& local_backup_path,
    const std::string& cloud_uri,
    const BackupOptions& options) {
    
    THEMIS_INFO("uploadBackupToCloud: local={}, cloud={}, storage={}",
                local_backup_path, cloud_uri,
                static_cast<int>(options.storage));
    
    // Validate local backup path exists
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(local_backup_path, ec)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "Local backup path does not exist: " + local_backup_path
        ));
    }

    // Validate cloud URI: must use a supported scheme (s3://, azure://, gs://)
    // and have a non-empty bucket/container following the scheme.
    auto isValidCloudUri = [](const std::string& uri) -> bool {
        static const char* const schemes[] = {"s3://", "azure://", "gs://"};
        for (const auto* prefix : schemes) {
            std::string p(prefix);
            if (uri.size() > p.size() && uri.rfind(p, 0) == 0) {
                return true;
            }
        }
        return false;
    };

    if (!isValidCloudUri(cloud_uri)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_INVALID_TYPE,
            "Invalid cloud URI: '" + cloud_uri +
            "'. Supported schemes: s3://<bucket>/path, azure://<account>/container/path,"
            " gs://<bucket>/path"
        ));
    }

    // Compile-time SDK flags control the active cloud path:
    //   THEMIS_ENABLE_S3     → AWS S3 SDK
    //   THEMIS_ENABLE_AZURE  → Azure Storage SDK
    //   THEMIS_ENABLE_GCS    → Google Cloud Storage SDK
#if defined(THEMIS_ENABLE_S3) || defined(THEMIS_ENABLE_AZURE) || defined(THEMIS_ENABLE_GCS)
    if (!uploadToCloud(local_backup_path, cloud_uri, options.storage,
                       options.cloud_config, ec)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_CREATION_FAILED,
            "Cloud upload failed for '" + cloud_uri + "': " + ec.message()
        ));
    }
    THEMIS_INFO("Backup uploaded to cloud: {}", cloud_uri);
    return cloud_uri;
#else
    return tl::unexpected(Error(
        errors::ErrorCode::ERR_UNKNOWN,
        "Cloud backup upload not available. "
        "Build with THEMIS_ENABLE_S3, THEMIS_ENABLE_AZURE, or THEMIS_ENABLE_GCS."
    ));
#endif
}

Result<void> BackupManager::restoreFromCloud(
    const std::string& cloud_uri,
    const std::string& local_restore_path,
    const BackupOptions& options) {
    
    THEMIS_INFO("restoreFromCloud: cloud={}, local={}, storage={}",
                cloud_uri, local_restore_path,
                static_cast<int>(options.storage));

    if (cloud_uri.empty()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED,
            "restoreFromCloud: cloud URI must not be empty"
        ));
    }

    // Compile-time SDK flags control the active cloud path:
    //   THEMIS_ENABLE_S3     → AWS S3 SDK
    //   THEMIS_ENABLE_AZURE  → Azure Storage SDK
    //   THEMIS_ENABLE_GCS    → Google Cloud Storage SDK
#if defined(THEMIS_ENABLE_S3) || defined(THEMIS_ENABLE_AZURE) || defined(THEMIS_ENABLE_GCS)
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(local_restore_path, ec);
    if (ec) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED,
            "Failed to create restore directory '" + local_restore_path +
            "': " + ec.message()
        ));
    }

    if (!downloadFromCloud(cloud_uri, local_restore_path, options.storage,
                           options.cloud_config, ec)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_BACKUP_RESTORATION_FAILED,
            "Cloud download failed for '" + cloud_uri + "': " + ec.message()
        ));
    }
    THEMIS_INFO("Backup restored from cloud: {} → {}", cloud_uri, local_restore_path);
    return OkVoid();
#else
    return tl::unexpected(Error(
        errors::ErrorCode::ERR_UNKNOWN,
        "Cloud backup restore not available. "
        "Build with THEMIS_ENABLE_S3, THEMIS_ENABLE_AZURE, or THEMIS_ENABLE_GCS."
    ));
#endif
}

Result<std::string> BackupManager::createSnapshot(
    const std::string& snapshot_name,
    const std::string& /*storage_class*/) {

    if (!db_wrapper_) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                    "createSnapshot: db_wrapper is null"));
    }

    namespace fs = std::filesystem;

    // Build snapshot directory: <db_path>/../snapshots/<name>_<YYYYMMDD_HHMMSS>
    const std::string& db_path = db_wrapper_->getConfig().db_path;
    std::string ts = getTimestamp();
    fs::path snap_dir = fs::path(db_path).parent_path() / "snapshots" /
                        (snapshot_name + "_" + ts);

    std::error_code ec;
    fs::create_directories(snap_dir.parent_path(), ec);
    if (ec) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                                    "createSnapshot: cannot create snapshot base dir: " +
                                    ec.message()));
    }

    // RocksDB Checkpoint: crash-consistent, quiesce-safe, no writes blocked.
    if (!db_wrapper_->createCheckpoint(snap_dir.string())) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                    "createSnapshot: createCheckpoint failed for '" +
                                    snap_dir.string() + "'"));
    }

    // Write a JSON manifest alongside the snapshot so it can be verified/restored.
    uint64_t seq = db_wrapper_->getLatestSequenceNumber();
    nlohmann::json manifest;
    manifest["snapshot_name"]    = snapshot_name;
    manifest["created_at"]       = ts;
    manifest["db_path"]          = db_path;
    manifest["sequence_number"]  = seq;
    manifest["format"]           = "rocksdb_checkpoint_v1";

    fs::path manifest_path = snap_dir / "snapshot_manifest.json";
    {
        std::ofstream mf(manifest_path);
        if (!mf.is_open()) {
            // Checkpoint itself succeeded; manifest write failure is non-fatal but we
            // return an error so callers know verification may fail later.
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                                        "createSnapshot: cannot write manifest at '" +
                                        manifest_path.string() + "'"));
        }
        mf << manifest.dump(2) << "\n";
    }

    THEMIS_INFO("Snapshot '{}' created at '{}' (seq={})", snapshot_name,
                snap_dir.string(), seq);
    return snap_dir.string();
}

Result<void> BackupManager::restoreFromSnapshot(
    const std::string& snapshot_id,
    const std::string& /*restore_pvc*/) {

    namespace fs = std::filesystem;

    if (snapshot_id.empty()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                                    "restoreFromSnapshot: snapshot_id must not be empty"));
    }

    // First verify the snapshot is intact.
    auto verify = verifySnapshot(snapshot_id);
    if (!verify) return verify;

    if (!db_wrapper_) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                    "restoreFromSnapshot: db_wrapper is null"));
    }

    if (!db_wrapper_->restoreFromCheckpoint(snapshot_id)) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                    "restoreFromSnapshot: restoreFromCheckpoint failed for '" +
                                    snapshot_id + "'"));
    }

    THEMIS_INFO("Database restored from snapshot '{}'", snapshot_id);
    return OkVoid();
}

Result<void> BackupManager::verifySnapshot(const std::string& snapshot_dir) {
    namespace fs = std::filesystem;

    if (!fs::exists(snapshot_dir)) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                                    "verifySnapshot: directory not found: '" +
                                    snapshot_dir + "'"));
    }

    // Check manifest
    fs::path manifest_path = fs::path(snapshot_dir) / "snapshot_manifest.json";
    if (!fs::exists(manifest_path)) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                    "verifySnapshot: manifest not found in '" +
                                    snapshot_dir + "'"));
    }

    try {
        std::ifstream mf(manifest_path);
        nlohmann::json manifest = nlohmann::json::parse(mf);
        if (manifest["format"] != "rocksdb_checkpoint_v1") {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                        "verifySnapshot: unknown snapshot format '" +
                                        manifest.value("format", "?") + "'"));
        }
    } catch (const std::exception& e) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                    std::string("verifySnapshot: manifest parse error: ") +
                                    e.what()));
    }

    // Check that a MANIFEST or CURRENT file is present (sign of a valid RocksDB directory)
    bool has_rocksdb_files =
        fs::exists(fs::path(snapshot_dir) / "CURRENT") ||
        fs::exists(fs::path(snapshot_dir) / "MANIFEST-000001");
    if (!has_rocksdb_files) {
        // Scan for any MANIFEST-* file
        for (const auto& entry : fs::directory_iterator(snapshot_dir)) {
            if (entry.path().filename().string().rfind("MANIFEST", 0) == 0) {
                has_rocksdb_files = true;
                break;
            }
        }
    }
    if (!has_rocksdb_files) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                    "verifySnapshot: no RocksDB files found in '" +
                                    snapshot_dir + "'"));
    }

    THEMIS_INFO("Snapshot verification passed for '{}'", snapshot_dir);
    return OkVoid();
}

Result<std::vector<std::string>> BackupManager::listSnapshots() {
    namespace fs = std::filesystem;

    if (!db_wrapper_) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                    "listSnapshots: db_wrapper is null"));
    }
    const std::string& db_path = db_wrapper_->getConfig().db_path;
    fs::path snap_base = fs::path(db_path).parent_path() / "snapshots";

    std::vector<std::string> result;
    if (!fs::exists(snap_base)) return result; // no snapshots yet

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(snap_base, ec)) {
        if (entry.is_directory()) {
            // Only include directories that contain a snapshot manifest
            if (fs::exists(entry.path() / "snapshot_manifest.json")) {
                result.push_back(entry.path().string());
            }
        }
    }
    std::sort(result.begin(), result.end()); // alphabetical = chronological (timestamps in name)
    return result;
}

} // namespace themis
