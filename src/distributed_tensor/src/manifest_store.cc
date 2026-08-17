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

using json = nlohmann::json;

namespace themis {
namespace distributed_tensor {

// In-memory store implementation (mock backend for now)
// TODO: Replace with actual RocksDB integration when available
static std::map<std::string, std::string> g_manifest_store;
static std::map<std::string, std::vector<ManifestVersion>> g_version_history;
static std::map<std::string, std::pair<std::string, int64_t>> g_locks;  // artifact_id -> (lock_holder, expire_ms)
static std::map<std::string, uint64_t> g_version_counters;
static std::mutex g_store_mutex;

// ============================================================================
// ManifestStore Methods
// ============================================================================

ManifestStoreStatus ManifestStore::open(const std::string& db_path) {
  db_path_ = db_path;
  is_open_ = true;
  // TODO: Replace with actual RocksDB initialization
  // For now, clear in-memory store on open
  {
    std::lock_guard<std::mutex> lock(g_store_mutex);
    g_manifest_store.clear();
    g_version_history.clear();
    g_locks.clear();
    g_version_counters.clear();
  }
  return ManifestStoreStatus::OK;
}

ManifestStoreStatus ManifestStore::close() {
  is_open_ = false;
  // TODO: Replace with actual RocksDB cleanup
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

  // Create version record
  ManifestVersion version;
  version.version_id = getCurrentVersion(artifact_id) + 1;
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
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
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
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
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
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
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
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  auto it = g_version_counters.find(artifact_id);
  if (it != g_version_counters.end()) {
    return it->second;
  }
  return 0;
}

std::vector<ManifestVersion> ManifestStore::getVersionHistory(const std::string& artifact_id) {
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  auto it = g_version_history.find(artifact_id);
  if (it != g_version_history.end()) {
    return it->second;
  }
  return {};
}

ManifestStoreStatus ManifestStore::deleteManifest(const std::string& artifact_id) {
  if (!is_open_) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  g_manifest_store.erase(artifact_id);
  g_version_history.erase(artifact_id);
  g_version_counters.erase(artifact_id);
  g_locks.erase(artifact_id);
  
  return ManifestStoreStatus::OK;
}

ManifestStore::Stats ManifestStore::getStats() const {
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  Stats stats;
  stats.total_artifacts = g_manifest_store.size();
  
  for (const auto& hist : g_version_history) {
    stats.total_versions += hist.second.size();
  }
  
  for (const auto& lock : g_locks) {
    if (lock.second.second == 0) {
      stats.locked_artifacts++;
    } else {
      int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
      if (lock.second.second > now_ms) {
        stats.locked_artifacts++;
      }
    }
  }
  
  // Approximate storage size
  for (const auto& manifest : g_manifest_store) {
    stats.storage_size_bytes += manifest.second.size();
  }
  
  return stats;
}

ManifestStoreStatus ManifestStore::internalRead(const std::string& artifact_id,
                                                uint64_t version_id,
                                                ArtifactManifest& manifest) {
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  // If version_id == 0, get the latest version
  if (version_id == 0) {
    auto it = g_manifest_store.find(artifact_id);
    if (it == g_manifest_store.end()) {
      return ManifestStoreStatus::NOT_FOUND;
    }
    
    try {
      auto parsed = json::parse(it->second);
      manifest = parsed.get<ArtifactManifest>();
      return ManifestStoreStatus::OK;
    } catch (...) {
      return ManifestStoreStatus::STORAGE_ERROR;
    }
  }
  
  // Get specific version from history
  auto hist_it = g_version_history.find(artifact_id);
  if (hist_it == g_version_history.end()) {
    return ManifestStoreStatus::VERSION_MISMATCH;
  }
  
  for (const auto& ver : hist_it->second) {
    if (ver.version_id == version_id) {
      manifest = ver.manifest;
      return ManifestStoreStatus::OK;
    }
  }
  
  return ManifestStoreStatus::VERSION_MISMATCH;
}

ManifestStoreStatus ManifestStore::internalWrite(const std::string& artifact_id,
                                                 const ArtifactManifest& manifest,
                                                 const ManifestVersion& version_info) {
  std::lock_guard<std::mutex> lock(g_store_mutex);
  
  try {
    // Store current manifest
    g_manifest_store[artifact_id] = json(manifest).dump();
    
    // Add to version history
    g_version_history[artifact_id].push_back(version_info);
    
    // Update version counter
    g_version_counters[artifact_id] = version_info.version_id;
    
    return ManifestStoreStatus::OK;
  } catch (...) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }
}

}  // namespace distributed_tensor
}  // namespace themis
