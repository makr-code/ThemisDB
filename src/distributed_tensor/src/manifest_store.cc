/// @file manifest_store.cc
/// @brief Implementation of manifest persistence store for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// Error codes for this module use the distributed_tensor range [7000-7099]:
///   - 7000-7009: Storage errors (open, close, I/O failures)
///   - 7010-7019: Manifest validation errors (invalid_manifest, schema violation)
///   - 7020-7029: CAS (Compare-And-Swap) failures (version mismatch)
///   - 7030-7039: Locking errors (lock_timeout, lock_not_held)
///   - 7040-7099: Reserved for future use
///
/// SG-DT-01 (Fail-Closed Invariant): All error paths must fail closed. The
/// validate() method call on line 80 and 114 enforces the SG-DT-01 invariant.

#include "src/distributed_tensor/include/manifest_store.h"
#include <map>
#include <mutex>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

using json = nlohmann::json;

namespace themis {
namespace distributed_tensor {

// RocksDB instance for persistent manifest storage
static std::unique_ptr<rocksdb::DB> g_manifest_db;
static std::mutex g_db_mutex;
static bool g_db_initialized = false;

// In-memory lock storage (locks are ephemeral and don't need to be persisted)
static std::map<std::string, std::pair<std::string, int64_t>> g_locks;  // artifact_id -> (lock_holder, expire_ms)

// ============================================================================
// Error mapping: RocksDB::Status to ManifestStoreStatus
// ============================================================================
// Error code ranges for distributed_tensor module [7000-7099]:
//   - 7000-7009: Storage errors (open, close, I/O failures)
//   - 7010-7019: Manifest validation errors (invalid_manifest, schema violation)
//   - 7020-7029: CAS (Compare-And-Swap) failures (version mismatch)
//   - 7030-7039: Locking errors (lock_timeout, lock_not_held)
//   - 7040-7099: Future use (error codes for aborted, timeout, lock conflicts)

ManifestStoreStatus mapRocksDBStatusToManifestStatus(const rocksdb::Status& status) {
  if (status.ok()) {
    return ManifestStoreStatus::OK;
  }
  
  if (status.IsNotFound()) {
    return ManifestStoreStatus::NOT_FOUND;
  }
  
  if (status.IsInvalidArgument()) {
    return ManifestStoreStatus::INVALID_MANIFEST;
  }
  
  if (status.IsIOError()) {
    spdlog::error("RocksDB I/O error: {}", status.ToString());
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  if (status.IsCorruption()) {
    spdlog::error("RocksDB corruption detected: {}", status.ToString());
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  // Handle Aborted status (operation aborted, possibly async/concurrent issue)
  if (status.IsAborted()) {
    spdlog::warn("RocksDB operation aborted: {}", status.ToString());
    // Map to STORAGE_ERROR (code 7001) - operation could not complete due to abort
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  // Handle TimedOut status (operation exceeded time limit)
  if (status.IsTimedOut()) {
    spdlog::warn("RocksDB operation timed out: {}", status.ToString());
    // Map to STORAGE_ERROR (code 7002) - operation could not complete within time limit
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  // Handle Locked status (resource locked by another operation)
  if (status.IsLocked()) {
    spdlog::warn("RocksDB resource locked: {}", status.ToString());
    return ManifestStoreStatus::ARTIFACT_LOCKED;
  }
  
  // Default: treat as storage error
  spdlog::error("RocksDB error (unmapped status): {}", status.ToString());
  return ManifestStoreStatus::STORAGE_ERROR;
}

// ============================================================================
// ManifestStore Methods
// ============================================================================

ManifestStoreStatus ManifestStore::open(const std::string& db_path) {
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  // Check if already initialized
  if (g_db_initialized && g_manifest_db) {
    // If opening with a different path, close the old DB and open the new one
    if (db_path_ != db_path) {
      spdlog::warn("ManifestStore::open: Path mismatch - closing old database at '{}' "
                   "and opening new database at '{}'",
                   db_path_, db_path);
      
      // Flush pending writes before closing
      rocksdb::Status flush_status = g_manifest_db->Flush(rocksdb::FlushOptions());
      if (!flush_status.ok()) {
        spdlog::warn("ManifestStore::open: Flush failed during path switch: {}", flush_status.ToString());
      }
      
      // Close the old database
      rocksdb::Status close_status = g_manifest_db->Close();
      g_manifest_db.reset();
      g_db_initialized = false;
      
      if (!close_status.ok()) {
        spdlog::error("ManifestStore::open: Failed to close old RocksDB: {}", close_status.ToString());
        return mapRocksDBStatusToManifestStatus(close_status);
      }
      
      // Continue to open the new path
    } else {
      // Same path already open - return success
      db_path_ = db_path;
      is_open_ = true;
      return ManifestStoreStatus::OK;
    }
  }

  // Configure RocksDB options
  rocksdb::Options options;
  options.create_if_missing = true;
  options.compression = rocksdb::kLZ4Compression;
  options.max_open_files = 256;
  options.write_buffer_size = 64 * 1024 * 1024;  // 64 MB
  options.target_file_size_base = 64 * 1024 * 1024;  // 64 MB
  
  // Open RocksDB instance
  rocksdb::DB* db = nullptr;
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
  
  if (!status.ok()) {
    spdlog::error("ManifestStore::open: Failed to open RocksDB at path '{}': {}",
                 db_path, status.ToString());
    return mapRocksDBStatusToManifestStatus(status);
  }
  
  g_manifest_db.reset(db);
  g_db_initialized = true;
  db_path_ = db_path;
  is_open_ = true;
  
  spdlog::info("ManifestStore::open: Successfully opened RocksDB at path '{}'", db_path);
  return ManifestStoreStatus::OK;
}

ManifestStoreStatus ManifestStore::close() {
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  if (!g_manifest_db) {
    is_open_ = false;
    return ManifestStoreStatus::OK;
  }

  // Flush pending writes before closing
  rocksdb::Status flush_status = g_manifest_db->Flush(rocksdb::FlushOptions());
  if (!flush_status.ok()) {
    spdlog::warn("ManifestStore::close: Flush failed: {}", flush_status.ToString());
  }

  // Close database
  rocksdb::Status close_status = g_manifest_db->Close();
  g_manifest_db.reset();
  g_db_initialized = false;
  is_open_ = false;

  if (!close_status.ok()) {
    spdlog::error("ManifestStore::close: Failed to close RocksDB: {}", close_status.ToString());
    return mapRocksDBStatusToManifestStatus(close_status);
  }

  spdlog::info("ManifestStore::close: Successfully closed RocksDB");
  return ManifestStoreStatus::OK;
}

ManifestStoreStatus ManifestStore::read(const std::string& artifact_id, ArtifactManifest& manifest) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  // Read latest version (version_id = 0 means latest)
  return internalRead(artifact_id, 0, manifest);
}

ManifestStoreStatus ManifestStore::readVersion(const std::string& artifact_id,
                                               uint64_t version_id,
                                               ArtifactManifest& manifest) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  return internalRead(artifact_id, version_id, manifest);
}

ManifestStoreStatus ManifestStore::write(const std::string& artifact_id,
                                         const ArtifactManifest& manifest,
                                         uint64_t expected_version,
                                         const std::string& change_reason,
                                         const std::string& changed_by) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  // Validate manifest
  if (!manifest.validate()) {
    return ManifestStoreStatus::INVALID_MANIFEST;
  }

  // Check CAS condition if expected_version is specified
  if (expected_version != 0) {
    uint64_t current_version = getCurrentVersion(artifact_id);
    if (current_version != expected_version) {
      return ManifestStoreStatus::CAS_FAILED;
    }
  }

  // Get current version and validate that the counter is accessible
  uint64_t current_version = getCurrentVersion(artifact_id);
  
  // Validate version counter is available (cannot be 0 if we've written before,
  // but for first write we need to ensure counter logic is sound)
  // If current_version is 0, this is either a new artifact or counter is corrupted
  // Check if artifact exists to distinguish between new artifact and corrupted counter
  if (current_version == 0) {
    ArtifactManifest existing;
    ManifestStoreStatus check_status = internalRead(artifact_id, 0, existing);
    if (check_status == ManifestStoreStatus::OK) {
      // Artifact exists but counter is corrupted/missing - this is an error
      spdlog::error("ManifestStore::write: Version counter corrupted for existing artifact_id={}", 
                   artifact_id);
      return ManifestStoreStatus::STORAGE_ERROR;
    } else if (check_status != ManifestStoreStatus::NOT_FOUND) {
      // Some other error reading the artifact
      spdlog::error("ManifestStore::write: Error reading artifact_id={} to validate counter: status={}",
                   artifact_id, static_cast<int>(check_status));
      return check_status;
    }
    // If NOT_FOUND, this is a new artifact - proceed with version 1
    spdlog::debug("ManifestStore::write: Creating new version counter for artifact_id={}", artifact_id);
  }

  // Create version record
  ManifestVersion version;
  version.version_id = current_version + 1;
  version.created_at_unix_sec =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  version.manifest = manifest;
  version.change_reason = change_reason;
  version.changed_by = changed_by;

  return internalWrite(artifact_id, manifest, version);
}

ManifestStoreStatus ManifestStore::swapManifest(const std::string& artifact_id,
                                                const ArtifactManifest& new_manifest,
                                                uint64_t current_manifest_version,
                                                const std::string& reason) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  // Validate new manifest
  if (!new_manifest.validate()) {
    return ManifestStoreStatus::INVALID_MANIFEST;
  }

  // Use CAS to ensure atomic swap
  return write(artifact_id, new_manifest, current_manifest_version, reason, "update_worker");
}

ManifestStoreStatus ManifestStore::invalidate(const std::string& artifact_id,
                                              InvalidationReason reason) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  ArtifactManifest manifest;
  ManifestStoreStatus status = read(artifact_id, manifest);
  if (status != ManifestStoreStatus::OK) {
    return status;
  }

  // Mark as invalidated
  manifest.lifecycle_state = LifecycleState::INVALIDATED;
  manifest.invalidation_reason = reason;

  return write(artifact_id, manifest, 0, "invalidated", "invalidation_manager");
}

ManifestStoreStatus ManifestStore::acquireLock(const std::string& artifact_id,
                                               const std::string& lock_holder,
                                               int64_t timeout_ms) {
  if (!is_open_) {
    return ManifestStoreStatus::STORE_NOT_OPEN;
  }
  
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  auto lock_it = g_locks.find(artifact_id);
  if (lock_it != g_locks.end()) {
    // Lock exists - check if it has expired
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (lock_it->second.second > now_ms) {
      // Lock still active
      return ManifestStoreStatus::ARTIFACT_LOCKED;
    }
    // Lock expired, remove it
    g_locks.erase(lock_it);
  }
  
  // Acquire lock
  int64_t expire_ms = 0;
  if (timeout_ms > 0) {
    expire_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + timeout_ms;
  }
  
  g_locks[artifact_id] = {lock_holder, expire_ms};
  return ManifestStoreStatus::OK;
}

ManifestStoreStatus ManifestStore::releaseLock(const std::string& artifact_id,
                                               const std::string& lock_holder) {
  if (!is_open_) {
    return ManifestStoreStatus::STORE_NOT_OPEN;
  }
  
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  auto lock_it = g_locks.find(artifact_id);
  if (lock_it == g_locks.end()) {
    return ManifestStoreStatus::OK;  // Not locked, no-op
  }
  
  // Verify lock holder matches
  if (lock_it->second.first != lock_holder) {
    return ManifestStoreStatus::ARTIFACT_LOCKED;  // Different holder
  }
  
  g_locks.erase(lock_it);
  return ManifestStoreStatus::OK;
}

bool ManifestStore::isLocked(const std::string& artifact_id) const {
  if (!is_open_) {
    return false;  // Store not open, consider artifact as not locked
  }
  
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  auto lock_it = g_locks.find(artifact_id);
  if (lock_it == g_locks.end()) {
    return false;
  }
  
  // Check if lock has expired
  if (lock_it->second.second == 0) {
    return true;  // No expiration
  }
  
  int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  
  return lock_it->second.second > now_ms;
}

uint64_t ManifestStore::getCurrentVersion(const std::string& artifact_id) const {
  std::lock_guard<std::mutex> lock(g_db_mutex);

  if (!g_manifest_db) {
    return 0;
  }

  std::string counter_key = artifact_id + ":counter";
  std::string value;
  rocksdb::Status status = g_manifest_db->Get(rocksdb::ReadOptions(), counter_key, &value);
  
  if (!status.ok()) {
    return 0;
  }

  try {
    return std::stoull(value);
  } catch (...) {
    return 0;
  }
}

std::vector<ManifestVersion> ManifestStore::getVersionHistory(const std::string& artifact_id) {
  std::lock_guard<std::mutex> lock(g_db_mutex);

  std::vector<ManifestVersion> history;
  
  if (!g_manifest_db) {
    return history;
  }

  try {
    // Get current version count
    uint64_t current_version = 0;
    std::string counter_key = artifact_id + ":counter";
    std::string counter_value;
    rocksdb::Status counter_status = g_manifest_db->Get(rocksdb::ReadOptions(), counter_key, &counter_value);
    
    if (counter_status.ok()) {
      try {
        current_version = std::stoull(counter_value);
      } catch (...) {
        return history;
      }
    }

    // Iterate through versions 1 to current_version
    for (uint64_t v = 1; v <= current_version; ++v) {
      std::string version_key = artifact_id + ":version:" + std::to_string(v);
      std::string version_json;
      rocksdb::Status status = g_manifest_db->Get(rocksdb::ReadOptions(), version_key, &version_json);
      
      if (!status.ok()) {
        continue;  // Skip missing versions
      }

      try {
        auto parsed = json::parse(version_json);
        ManifestVersion ver = parsed.get<ManifestVersion>();
        history.push_back(ver);
      } catch (...) {
        continue;  // Skip malformed versions
      }
    }
  } catch (...) {
    // Return partial history
  }

  return history;
}

ManifestStoreStatus ManifestStore::deleteManifest(const std::string& artifact_id) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  std::lock_guard<std::mutex> lock(g_db_mutex);

  if (!g_manifest_db) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  try {
    rocksdb::WriteBatch batch;
    
    // Delete current manifest
    batch.Delete(artifact_id);
    
    // Delete counter
    batch.Delete(artifact_id + ":counter");
    
    // Delete all versions (get count first)
    uint64_t current_version = 0;
    std::string counter_value;
    rocksdb::Status counter_status = g_manifest_db->Get(rocksdb::ReadOptions(), 
                                                        artifact_id + ":counter", 
                                                        &counter_value);
    if (counter_status.ok()) {
      try {
        current_version = std::stoull(counter_value);
      } catch (...) {
        // Ignore parse error
      }
    }
    
    for (uint64_t v = 1; v <= current_version; ++v) {
      batch.Delete(artifact_id + ":version:" + std::to_string(v));
    }
    
    // Execute batch delete
    rocksdb::Status status = g_manifest_db->Write(rocksdb::WriteOptions(), &batch);
    
    if (!status.ok()) {
      spdlog::error("ManifestStore::deleteManifest: Failed to delete manifest for artifact_id={}: {}",
                   artifact_id, status.ToString());
      return mapRocksDBStatusToManifestStatus(status);
    }
    
    spdlog::debug("ManifestStore::deleteManifest: Successfully deleted manifest for artifact_id={}",
                 artifact_id);
    return ManifestStoreStatus::OK;
  } catch (const std::exception& e) {
    spdlog::error("ManifestStore::deleteManifest: Exception deleting manifest for artifact_id={}: {}",
                 artifact_id, e.what());
    return ManifestStoreStatus::STORAGE_ERROR;
  }
}

ManifestStore::Stats ManifestStore::getStats() const {
  std::lock_guard<std::mutex> lock(g_db_mutex);
  
  Stats stats;
  stats.total_artifacts = 0;
  stats.total_versions = 0;
  stats.locked_artifacts = 0;
  stats.storage_size_bytes = 0;

  if (!g_manifest_db) {
    return stats;
  }

  try {
    // Use RocksDB iterator to count artifacts and calculate stats
    rocksdb::Iterator* it = g_manifest_db->NewIterator(rocksdb::ReadOptions());
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string key = it->key().ToString();
      
      // Count manifest entries (keys without ":" are manifests)
      if (key.find(':') == std::string::npos) {
        stats.total_artifacts++;
        stats.storage_size_bytes += it->value().size();
      }
      
      // Count versions (keys with ":version:")
      if (key.find(":version:") != std::string::npos) {
        stats.total_versions++;
      }
    }
    
    delete it;
  } catch (const std::exception& e) {
    spdlog::warn("ManifestStore::getStats: Exception during stats collection: {}", e.what());
  }

  // Lock info is still maintained in-memory for now
  // Note: In future, locking information could also be persisted in RocksDB
  
  return stats;
}

ManifestStoreStatus ManifestStore::internalRead(const std::string& artifact_id,
                                                uint64_t version_id,
                                                ArtifactManifest& manifest) {
  std::lock_guard<std::mutex> lock(g_db_mutex);

  if (!g_manifest_db) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  if (version_id == 0) {
    // Read latest manifest for this artifact
    std::string value;
    rocksdb::Status status = g_manifest_db->Get(rocksdb::ReadOptions(), 
                                                artifact_id, &value);
    
    if (!status.ok()) {
      return mapRocksDBStatusToManifestStatus(status);
    }

    try {
      auto parsed = json::parse(value);
      manifest = parsed.get<ArtifactManifest>();
      return ManifestStoreStatus::OK;
    } catch (const std::exception& e) {
      spdlog::error("ManifestStore::internalRead: JSON parse error for artifact_id={}: {}",
                   artifact_id, e.what());
      return ManifestStoreStatus::STORAGE_ERROR;
    }
  }

  // Read specific version from version history key
  std::string version_key = artifact_id + ":version:" + std::to_string(version_id);
  std::string value;
  rocksdb::Status status = g_manifest_db->Get(rocksdb::ReadOptions(),
                                              version_key, &value);
  
  if (!status.ok()) {
    return mapRocksDBStatusToManifestStatus(status);
  }

  try {
    auto parsed = json::parse(value);
    ManifestVersion ver = parsed.get<ManifestVersion>();
    manifest = ver.manifest;
    return ManifestStoreStatus::OK;
  } catch (const std::exception& e) {
    spdlog::error("ManifestStore::internalRead: Version parse error for artifact_id={} version_id={}: {}",
                 artifact_id, version_id, e.what());
    return ManifestStoreStatus::STORAGE_ERROR;
  }
}

ManifestStoreStatus ManifestStore::internalWrite(const std::string& artifact_id,
                                                 const ArtifactManifest& manifest,
                                                 const ManifestVersion& version_info) {
  std::lock_guard<std::mutex> lock(g_db_mutex);

  if (!g_manifest_db) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }

  try {
    // Use batch for atomic writes
    rocksdb::WriteBatch batch;
    
    // Write latest manifest
    std::string manifest_json = json(manifest).dump();
    batch.Put(artifact_id, manifest_json);
    
    // Write version entry
    std::string version_key = artifact_id + ":version:" + std::to_string(version_info.version_id);
    std::string version_json = json(version_info).dump();
    batch.Put(version_key, version_json);
    
    // Write version counter
    std::string counter_key = artifact_id + ":counter";
    std::string counter_value = std::to_string(version_info.version_id);
    batch.Put(counter_key, counter_value);
    
    // Execute batch write
    rocksdb::Status status = g_manifest_db->Write(rocksdb::WriteOptions(), &batch);
    
    if (!status.ok()) {
      spdlog::error("ManifestStore::internalWrite: Failed to write manifest for artifact_id={}: {}",
                   artifact_id, status.ToString());
      return mapRocksDBStatusToManifestStatus(status);
    }
    
    spdlog::debug("ManifestStore::internalWrite: Successfully wrote manifest for artifact_id={} version={}",
                 artifact_id, version_info.version_id);
    return ManifestStoreStatus::OK;
  } catch (const std::exception& e) {
    spdlog::error("ManifestStore::internalWrite: Exception writing manifest for artifact_id={}: {}",
                 artifact_id, e.what());
    return ManifestStoreStatus::STORAGE_ERROR;
  }
}

}  // namespace distributed_tensor
}  // namespace themis
