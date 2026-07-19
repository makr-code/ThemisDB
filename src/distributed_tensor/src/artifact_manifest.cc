/// @file artifact_manifest.cc
/// @brief Implementation of distributed tensor artifact manifest schema
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

#include "include/artifact_manifest.h"
#include "include/tensor_artifact_classes.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

namespace themis {
namespace distributed_tensor {

// ============================================================================
// ArtifactManifest Methods
// ============================================================================

bool ArtifactManifest::validate() const {
  // Validate artifact_id is not empty
  if (artifact_id.empty()) {
    return false;
  }

  // Validate content_hash format (should be non-empty if set)
  if (!content_hash.empty()) {
    // Basic check: SHA-256 hex strings are 64 chars, MD5 32 chars, SHA-1 40 chars
    if (content_hash.length() < 32 || content_hash.length() > 128) {
      return false;
    }
    // Check if all characters are valid hex
    for (char c : content_hash) {
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        return false;
      }
    }
  }

  // Validate timestamps are consistent
  if (created_at_unix_sec > 0 && updated_at_unix_sec > 0) {
    if (updated_at_unix_sec < created_at_unix_sec) {
      return false;
    }
  }

  if (created_at_unix_sec > 0 && last_verified_unix_sec > 0) {
    if (last_verified_unix_sec < created_at_unix_sec) {
      return false;
    }
  }

  // Validate sequence ranges
  if (source_seq_end > 0 && source_seq_start > 0) {
    if (source_seq_end < source_seq_start) {
      return false;
    }
  }

  // Validate residual is in reasonable range [0.0, 1.0] or [0, 100]
  if (residual < 0.0 || residual > 1000.0) {
    return false;
  }

  // Validate rank values
  if (rank_status > 0 && rank_cap > 0) {
    if (rank_status > rank_cap) {
      return false;
    }
  }

  // Validate replication factor is at least 1
  if (replication_factor < 1) {
    return false;
  }

  // Validate artifact class and truth semantic combination
  if (!ArtifactClassifier::isValidCombination(artifact_class, truth_semantic)) {
    return false;
  }

  return true;
}

bool ArtifactManifest::isUsable(int64_t now_unix_sec) const {
  // Only ACTIVE and STALE states are usable
  if (lifecycle_state != LifecycleState::ACTIVE && lifecycle_state != LifecycleState::STALE) {
    return false;
  }

  // If STALE, still usable but caller should be aware
  return true;
}

bool ArtifactManifest::isStale(int64_t now_unix_sec) const {
  if (staleness_threshold_sec == 0) {
    // No staleness threshold set; not stale by default
    return false;
  }

  if (last_verified_unix_sec == 0) {
    // Never verified; consider it stale if threshold is set
    return true;
  }

  int64_t age_sec = now_unix_sec - last_verified_unix_sec;
  // Consider artifact stale when age is greater than or equal to the threshold.
  // Using >= makes the boundary case explicit and matches planner expectations.
  return age_sec >= staleness_threshold_sec;
}

double ArtifactManifest::getFreshnessScore(int64_t now_unix_sec) const {
  // If never verified, return 0.0 (not fresh)
  if (last_verified_unix_sec == 0) {
    return 0.0;
  }

  // If no staleness threshold, return 1.0 (always fresh)
  if (staleness_threshold_sec == 0) {
    return 1.0;
  }

  int64_t age_sec = now_unix_sec - last_verified_unix_sec;

  // Clamp age to [0, staleness_threshold_sec]
  if (age_sec < 0) {
    age_sec = 0;
  }
  if (age_sec > staleness_threshold_sec) {
    age_sec = staleness_threshold_sec;
  }

  // Linear freshness: 1.0 when age==0, 0.0 when age>=threshold.
  // freshness = clamp(1.0 - (age / threshold), 0.0, 1.0)
  double denom = static_cast<double>(staleness_threshold_sec);
  double freshness = 1.0 - (static_cast<double>(age_sec) / denom);
  if (freshness < 0.0) freshness = 0.0;
  if (freshness > 1.0) freshness = 1.0;
  return freshness;
}

bool ArtifactManifest::isCorrupted() const {
  // If no manifest_hash is set, we can't verify corruption
  if (manifest_hash.empty()) {
    return false;
  }

  // Recalculate expected manifest_hash and compare
  // For now, this is a placeholder; real implementation would:
  // 1. Serialize manifest fields (excluding manifest_hash itself)
  // 2. Hash the serialized data
  // 3. Compare with stored manifest_hash
  return false;
}

std::string ArtifactManifest::toJSON() const {
  json j;

  // Identity & Classification
  j["artifact_id"] = artifact_id;
  j["artifact_class"] = ArtifactClassifier::classToString(artifact_class);
  j["truth_semantic"] = ArtifactClassifier::semanticToString(truth_semantic);
  j["lifecycle_state"] = ArtifactLifecyclePolicy::stateToString(lifecycle_state);

  // Versioning & Integrity
  j["version"] = version;
  j["content_hash"] = content_hash;
  j["manifest_hash"] = manifest_hash;

  // Temporal Metadata
  j["created_at_unix_sec"] = created_at_unix_sec;
  j["updated_at_unix_sec"] = updated_at_unix_sec;
  j["last_verified_unix_sec"] = last_verified_unix_sec;
  j["last_rebuild_at_unix_sec"] = last_rebuild_at_unix_sec;
  j["staleness_threshold_sec"] = staleness_threshold_sec;
  j["artifact_age_ms"] = artifact_age_ms;

  // Sequence & Lag Tracking
  j["source_seq_start"] = source_seq_start;
  j["source_seq_end"] = source_seq_end;
  j["delta_lag"] = delta_lag;

  // Approximation & Quality Metrics
  j["residual"] = residual;
  j["rank_cap"] = rank_cap;
  j["rank_status"] = rank_status;

  // Rebuild & Update Tracking
  j["rebuild_state"] = RebuildStateUtils::stateToString(rebuild_state);
  j["update_mode"] = UpdateModeUtils::modeToString(update_mode);
  j["invalidation_reason"] = InvalidationReasonUtils::reasonToString(invalidation_reason);

  // Provenance & Reconstruction
  j["source_artifact_id"] = source_artifact_id;
  j["provenance_chain"] = provenance_chain;
  j["reconstruction_instructions"] = reconstruction_instructions;

  // Placement & Distribution
  j["shard_placements"] = shard_placements;
  j["requires_full_replication"] = requires_full_replication;
  j["is_rebuildable"] = is_rebuildable;

  // Replication & Redundancy Strategy
  j["replication_factor"] = replication_factor;
  j["erasure_coding_scheme"] = erasure_coding_scheme;
  j["backup_shard_placements"] = backup_shard_placements;

  // Compatibility & Constraints
  j["compatibility_metadata"] = compatibility_metadata;
  j["min_planner_version"] = min_planner_version;
  j["advisory_only"] = advisory_only;

  // Metadata & Extensibility
  j["custom_attributes"] = custom_attributes;
  j["description"] = description;

  return j.dump(2);
}

std::optional<ArtifactManifest> ArtifactManifest::fromJSON(const std::string& json_str) {
  try {
    json j = json::parse(json_str);
    ArtifactManifest manifest;

    // Identity & Classification
    if (j.contains("artifact_id")) manifest.artifact_id = j["artifact_id"].get<std::string>();
    if (j.contains("artifact_class")) {
      auto artifact_class = ArtifactClassifier::stringToClass(j["artifact_class"].get<std::string>());
      if (artifact_class) manifest.artifact_class = *artifact_class;
    }
    if (j.contains("truth_semantic")) {
      auto truth_semantic = ArtifactClassifier::stringToSemantic(j["truth_semantic"].get<std::string>());
      if (truth_semantic) manifest.truth_semantic = *truth_semantic;
    }
    if (j.contains("lifecycle_state")) {
      auto lifecycle_state = ArtifactLifecyclePolicy::stringToState(j["lifecycle_state"].get<std::string>());
      if (lifecycle_state) manifest.lifecycle_state = *lifecycle_state;
    }

    // Versioning & Integrity
    if (j.contains("version")) manifest.version = j["version"].get<std::string>();
    if (j.contains("content_hash")) manifest.content_hash = j["content_hash"].get<std::string>();
    if (j.contains("manifest_hash")) manifest.manifest_hash = j["manifest_hash"].get<std::string>();

    // Temporal Metadata
    if (j.contains("created_at_unix_sec")) manifest.created_at_unix_sec = j["created_at_unix_sec"].get<int64_t>();
    if (j.contains("updated_at_unix_sec")) manifest.updated_at_unix_sec = j["updated_at_unix_sec"].get<int64_t>();
    if (j.contains("last_verified_unix_sec")) manifest.last_verified_unix_sec = j["last_verified_unix_sec"].get<int64_t>();
    if (j.contains("last_rebuild_at_unix_sec")) manifest.last_rebuild_at_unix_sec = j["last_rebuild_at_unix_sec"].get<int64_t>();
    if (j.contains("staleness_threshold_sec")) manifest.staleness_threshold_sec = j["staleness_threshold_sec"].get<int64_t>();
    if (j.contains("artifact_age_ms")) manifest.artifact_age_ms = j["artifact_age_ms"].get<uint64_t>();

    // Sequence & Lag Tracking
    if (j.contains("source_seq_start")) manifest.source_seq_start = j["source_seq_start"].get<uint64_t>();
    if (j.contains("source_seq_end")) manifest.source_seq_end = j["source_seq_end"].get<uint64_t>();
    if (j.contains("delta_lag")) manifest.delta_lag = j["delta_lag"].get<uint64_t>();

    // Approximation & Quality Metrics
    if (j.contains("residual")) manifest.residual = j["residual"].get<double>();
    if (j.contains("rank_cap")) manifest.rank_cap = j["rank_cap"].get<uint32_t>();
    if (j.contains("rank_status")) manifest.rank_status = j["rank_status"].get<uint32_t>();

    // Rebuild & Update Tracking
    if (j.contains("rebuild_state")) {
      auto rebuild_state = RebuildStateUtils::stringToState(j["rebuild_state"].get<std::string>());
      if (rebuild_state) manifest.rebuild_state = *rebuild_state;
    }
    if (j.contains("update_mode")) {
      auto update_mode = UpdateModeUtils::stringToMode(j["update_mode"].get<std::string>());
      if (update_mode) manifest.update_mode = *update_mode;
    }
    if (j.contains("invalidation_reason")) {
      auto invalidation_reason = InvalidationReasonUtils::stringToReason(j["invalidation_reason"].get<std::string>());
      if (invalidation_reason) manifest.invalidation_reason = *invalidation_reason;
    }

    // Provenance & Reconstruction
    if (j.contains("source_artifact_id")) manifest.source_artifact_id = j["source_artifact_id"].get<std::string>();
    if (j.contains("provenance_chain")) manifest.provenance_chain = j["provenance_chain"].get<std::vector<std::string>>();
    if (j.contains("reconstruction_instructions")) manifest.reconstruction_instructions = j["reconstruction_instructions"].get<std::string>();

    // Placement & Distribution
    if (j.contains("shard_placements")) manifest.shard_placements = j["shard_placements"].get<std::vector<std::string>>();
    if (j.contains("requires_full_replication")) manifest.requires_full_replication = j["requires_full_replication"].get<bool>();
    if (j.contains("is_rebuildable")) manifest.is_rebuildable = j["is_rebuildable"].get<bool>();

    // Replication & Redundancy Strategy
    if (j.contains("replication_factor")) manifest.replication_factor = j["replication_factor"].get<uint32_t>();
    if (j.contains("erasure_coding_scheme")) manifest.erasure_coding_scheme = j["erasure_coding_scheme"].get<std::string>();
    if (j.contains("backup_shard_placements")) manifest.backup_shard_placements = j["backup_shard_placements"].get<std::vector<std::string>>();

    // Compatibility & Constraints
    if (j.contains("compatibility_metadata")) manifest.compatibility_metadata = j["compatibility_metadata"].get<std::map<std::string, std::string>>();
    if (j.contains("min_planner_version")) manifest.min_planner_version = j["min_planner_version"].get<std::string>();
    if (j.contains("advisory_only")) manifest.advisory_only = j["advisory_only"].get<bool>();

    // Metadata & Extensibility
    if (j.contains("custom_attributes")) manifest.custom_attributes = j["custom_attributes"].get<std::map<std::string, std::string>>();
    if (j.contains("description")) manifest.description = j["description"].get<std::string>();

    return manifest;
  } catch (...) {
    return std::nullopt;
  }
}

std::string ArtifactManifest::toYAML() const {
  // For now, serialize to JSON and note that YAML serialization would require a YAML library
  // This is a placeholder that returns JSON; real YAML support would use yaml-cpp or similar
  std::ostringstream oss;
  oss << "# ArtifactManifest (JSON representation)\n";
  oss << toJSON();
  return oss.str();
}

std::optional<ArtifactManifest> ArtifactManifest::fromYAML(const std::string& yaml_str) {
  // For now, attempt to parse as JSON
  // Real YAML support would require a YAML library
  return fromJSON(yaml_str);
}

// ============================================================================
// RebuildStateUtils Implementation
// ============================================================================

std::string RebuildStateUtils::stateToString(RebuildState state) {
  switch (state) {
    case RebuildState::PRISTINE:
      return "PRISTINE";
    case RebuildState::PATCHED:
      return "PATCHED";
    case RebuildState::PARTIAL_REFITTED:
      return "PARTIAL_REFITTED";
    case RebuildState::REBUILT:
      return "REBUILT";
    default:
      return "UNKNOWN";
  }
}

std::optional<RebuildState> RebuildStateUtils::stringToState(const std::string& state_str) {
  if (state_str == "PRISTINE") return RebuildState::PRISTINE;
  if (state_str == "PATCHED") return RebuildState::PATCHED;
  if (state_str == "PARTIAL_REFITTED") return RebuildState::PARTIAL_REFITTED;
  if (state_str == "REBUILT") return RebuildState::REBUILT;
  return std::nullopt;
}

// ============================================================================
// UpdateModeUtils Implementation
// ============================================================================

std::string UpdateModeUtils::modeToString(UpdateMode mode) {
  switch (mode) {
    case UpdateMode::PATCH:
      return "patch";
    case UpdateMode::PARTIAL_REFIT:
      return "partial_refit";
    case UpdateMode::REBUILD:
      return "rebuild";
    default:
      return "unknown";
  }
}

std::optional<UpdateMode> UpdateModeUtils::stringToMode(const std::string& mode_str) {
  if (mode_str == "patch") return UpdateMode::PATCH;
  if (mode_str == "partial_refit") return UpdateMode::PARTIAL_REFIT;
  if (mode_str == "rebuild") return UpdateMode::REBUILD;
  return std::nullopt;
}

// ============================================================================
// InvalidationReasonUtils Implementation
// ============================================================================

std::string InvalidationReasonUtils::reasonToString(InvalidationReason reason) {
  switch (reason) {
    case InvalidationReason::UNKNOWN:
      return "UNKNOWN";
    case InvalidationReason::INTEGRITY_CHECK_FAILED:
      return "INTEGRITY_CHECK_FAILED";
    case InvalidationReason::STALENESS_EXCEEDED:
      return "STALENESS_EXCEEDED";
    case InvalidationReason::SOURCE_INVALIDATED:
      return "SOURCE_INVALIDATED";
    case InvalidationReason::SOURCE_LINEAGE_CORRUPTED:
      return "SOURCE_LINEAGE_CORRUPTED";
    case InvalidationReason::POLICY_VIOLATION:
      return "POLICY_VIOLATION";
    case InvalidationReason::ADMIN_REQUESTED:
      return "ADMIN_REQUESTED";
    case InvalidationReason::SHARD_UNAVAILABLE:
      return "SHARD_UNAVAILABLE";
    default:
      return "UNKNOWN";
  }
}

std::optional<InvalidationReason> InvalidationReasonUtils::stringToReason(const std::string& reason_str) {
  if (reason_str == "UNKNOWN") return InvalidationReason::UNKNOWN;
  if (reason_str == "INTEGRITY_CHECK_FAILED") return InvalidationReason::INTEGRITY_CHECK_FAILED;
  if (reason_str == "STALENESS_EXCEEDED") return InvalidationReason::STALENESS_EXCEEDED;
  if (reason_str == "SOURCE_INVALIDATED") return InvalidationReason::SOURCE_INVALIDATED;
  if (reason_str == "SOURCE_LINEAGE_CORRUPTED") return InvalidationReason::SOURCE_LINEAGE_CORRUPTED;
  if (reason_str == "POLICY_VIOLATION") return InvalidationReason::POLICY_VIOLATION;
  if (reason_str == "ADMIN_REQUESTED") return InvalidationReason::ADMIN_REQUESTED;
  if (reason_str == "SHARD_UNAVAILABLE") return InvalidationReason::SHARD_UNAVAILABLE;
  return std::nullopt;
}

}  // namespace distributed_tensor
}  // namespace themis
