/**
 * @file index_manifest_v1.cpp
 * @brief Index Manifest v1.0 Implementation
 *
 * Implements JSON serialization, RocksDB persistence, and version history
 * rotation for IndexManifestV1.
 *
 * @date 2026-09-24
 */

#include "index/index_manifest_v1.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis::index {

// ============================================================================
// IndexVersionInfo Implementation
// ============================================================================

nlohmann::json IndexVersionInfo::to_json() const {
  return nlohmann::json{
      {"version_number", version_number},
      {"timestamp", timestamp},
      {"embedding_metadata",
       nlohmann::json{{"embedding_model_id", embedding_model_id},
                      {"embedding_dim", embedding_dim}}},
      {"chunking_metadata", nlohmann::json{{"chunking_profile_id", chunking_profile_id}}},
      {"index_schema", nlohmann::json{{"schema_version", schema_version}}},
      {"document_count", document_count},
      {"index_size_bytes", index_size_bytes},
      {"last_indexed_at", last_indexed_at},
  };
}

IndexVersionInfo IndexVersionInfo::from_json(const nlohmann::json& j) {
  IndexVersionInfo result;
  if (j.contains("version_number")) result.version_number = j["version_number"];
  if (j.contains("timestamp")) result.timestamp = j["timestamp"];
  if (j.contains("embedding_metadata")) {
    const auto& meta = j["embedding_metadata"];
    if (meta.contains("embedding_model_id")) {
      result.embedding_model_id = meta["embedding_model_id"];
    }
    if (meta.contains("embedding_dim")) result.embedding_dim = meta["embedding_dim"];
  }
  if (j.contains("chunking_metadata")) {
    const auto& meta = j["chunking_metadata"];
    if (meta.contains("chunking_profile_id")) {
      result.chunking_profile_id = meta["chunking_profile_id"];
    }
  }
  if (j.contains("index_schema")) {
    const auto& schema = j["index_schema"];
    if (schema.contains("schema_version")) result.schema_version = schema["schema_version"];
  }
  if (j.contains("document_count")) result.document_count = j["document_count"];
  if (j.contains("index_size_bytes")) result.index_size_bytes = j["index_size_bytes"];
  if (j.contains("last_indexed_at")) result.last_indexed_at = j["last_indexed_at"];
  return result;
}

// ============================================================================
// VersionHistoryEntry Implementation
// ============================================================================

nlohmann::json VersionHistoryEntry::to_json() const {
  return nlohmann::json{
      {"version_number", version_number},
      {"timestamp", timestamp},
      {"embedding_model_id", embedding_model_id},
      {"chunking_profile_id", chunking_profile_id},
      {"schema_version", schema_version},
      {"action", action},
      {"reason", reason},
      {"documents_added", documents_added},
      {"documents_removed", documents_removed},
  };
}

VersionHistoryEntry VersionHistoryEntry::from_json(const nlohmann::json& j) {
  VersionHistoryEntry result;
  if (j.contains("version_number")) result.version_number = j["version_number"];
  if (j.contains("timestamp")) result.timestamp = j["timestamp"];
  if (j.contains("embedding_model_id")) result.embedding_model_id = j["embedding_model_id"];
  if (j.contains("chunking_profile_id")) result.chunking_profile_id = j["chunking_profile_id"];
  if (j.contains("schema_version")) result.schema_version = j["schema_version"];
  if (j.contains("action")) result.action = j["action"];
  if (j.contains("reason")) result.reason = j["reason"];
  if (j.contains("documents_added")) result.documents_added = j["documents_added"];
  if (j.contains("documents_removed")) result.documents_removed = j["documents_removed"];
  return result;
}

// ============================================================================
// ReindexTrackingInfo Implementation
// ============================================================================

nlohmann::json ReindexTrackingInfo::to_json() const {
  return nlohmann::json{
      {"last_reindex_reason", last_reindex_reason},
      {"last_reindex_start", last_reindex_start},
      {"last_reindex_end", last_reindex_end},
      {"last_reindex_status", last_reindex_status},
      {"atomic_rollback_available", atomic_rollback_available},
      {"previous_version_accessible", previous_version_accessible},
      {"error_message", error_message},
  };
}

ReindexTrackingInfo ReindexTrackingInfo::from_json(const nlohmann::json& j) {
  ReindexTrackingInfo result;
  if (j.contains("last_reindex_reason")) result.last_reindex_reason = j["last_reindex_reason"];
  if (j.contains("last_reindex_start")) result.last_reindex_start = j["last_reindex_start"];
  if (j.contains("last_reindex_end")) result.last_reindex_end = j["last_reindex_end"];
  if (j.contains("last_reindex_status")) result.last_reindex_status = j["last_reindex_status"];
  if (j.contains("atomic_rollback_available")) {
    result.atomic_rollback_available = j["atomic_rollback_available"];
  }
  if (j.contains("previous_version_accessible")) {
    result.previous_version_accessible = j["previous_version_accessible"];
  }
  if (j.contains("error_message")) result.error_message = j["error_message"];
  return result;
}

// ============================================================================
// IndexManifestGovernance Implementation
// ============================================================================

nlohmann::json IndexManifestGovernance::to_json() const {
  return nlohmann::json{
      {"contract_version", contract_version},
      {"audit_trail_enabled", audit_trail_enabled},
      {"canary_deployments_enabled", canary_deployments_enabled},
  };
}

IndexManifestGovernance IndexManifestGovernance::from_json(const nlohmann::json& j) {
  IndexManifestGovernance result;
  if (j.contains("contract_version")) result.contract_version = j["contract_version"];
  if (j.contains("audit_trail_enabled")) result.audit_trail_enabled = j["audit_trail_enabled"];
  if (j.contains("canary_deployments_enabled")) {
    result.canary_deployments_enabled = j["canary_deployments_enabled"];
  }
  return result;
}

// ============================================================================
// IndexManifestV1 Implementation
// ============================================================================

nlohmann::json IndexManifestV1::to_json() const {
  nlohmann::json history_array = nlohmann::json::array();
  for (const auto& entry : version_history) {
    history_array.push_back(entry.to_json());
  }

  return nlohmann::json{
      {"manifest_version", manifest_version},
      {"index_id", index_id},
      {"created_at", created_at},
      {"last_updated_at", last_updated_at},
      {"current_version", current_version.to_json()},
      {"version_history", history_array},
      {"reindex_tracking", reindex_tracking.to_json()},
      {"governance", governance.to_json()},
  };
}

IndexManifestV1 IndexManifestV1::from_json(const nlohmann::json& j) {
  IndexManifestV1 result;
  if (j.contains("manifest_version")) result.manifest_version = j["manifest_version"];
  if (j.contains("index_id")) result.index_id = j["index_id"];
  if (j.contains("created_at")) result.created_at = j["created_at"];
  if (j.contains("last_updated_at")) result.last_updated_at = j["last_updated_at"];

  if (j.contains("current_version")) {
    result.current_version = IndexVersionInfo::from_json(j["current_version"]);
  }

  if (j.contains("version_history")) {
    for (const auto& entry : j["version_history"]) {
      result.version_history.push_back(VersionHistoryEntry::from_json(entry));
    }
  }

  if (j.contains("reindex_tracking")) {
    result.reindex_tracking = ReindexTrackingInfo::from_json(j["reindex_tracking"]);
  }

  if (j.contains("governance")) {
    result.governance = IndexManifestGovernance::from_json(j["governance"]);
  }

  return result;
}

std::string IndexManifestV1::to_rocksdb_value() const {
  // Store as minified JSON to save space in RocksDB
  return to_json().dump(-1);
}

IndexManifestV1 IndexManifestV1::from_rocksdb_value(const std::string& rocksdb_value) {
  try {
    auto json = nlohmann::json::parse(rocksdb_value);
    return from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[IndexManifestV1] Failed to parse manifest from RocksDB: {}", e.what());
    throw;
  }
}

void IndexManifestV1::add_version_history(const VersionHistoryEntry& entry) {
  version_history.push_back(entry);

  // Maintain FIFO rotation: keep only the most recent MAX_HISTORY_SIZE entries
  if (version_history.size() > MAX_HISTORY_SIZE) {
    version_history.erase(version_history.begin());
    spdlog::debug(
        "[IndexManifestV1] Version history rotated (size: {} -> {})",
        version_history.size() + 1, version_history.size());
  }

  // Update last_updated_at
  auto now = std::time(nullptr);
  auto tm = std::gmtime(&now);
  std::ostringstream oss;
  oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
  last_updated_at = oss.str();
}

uint32_t IndexManifestV1::get_previous_version_number() const {
  if (version_history.size() < 2) {
    return 0;  // No previous version
  }
  // Second-to-last entry is the previous version
  return version_history[version_history.size() - 2].version_number;
}

bool IndexManifestV1::can_rollback_to_previous() const {
  return reindex_tracking.atomic_rollback_available &&
         reindex_tracking.previous_version_accessible && version_history.size() >= 2;
}

}  // namespace themis::index
