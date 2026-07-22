/// @file artifact_manifest.cc
/// @brief Implementation of derived-artifact lifecycle and staleness management.
///
/// Implements the extended ArtifactManifest methods, ArtifactLifecyclePolicy,
/// ArtifactClassifier, and the utility string-conversion helpers for RebuildState,
/// UpdateMode, and InvalidationReason (issue #5442).

#include "src/distributed_tensor/include/artifact_manifest.h"
#include "src/distributed_tensor/include/tensor_artifact_classes.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <functional>

using json = nlohmann::json;

namespace themis {
namespace distributed_tensor {

// ============================================================================
// ArtifactLifecyclePolicy — serialization / usability helpers
// ============================================================================

std::string ArtifactLifecyclePolicy::stateToString(LifecycleState state) {
  switch (state) {
    case LifecycleState::READY:       return "READY";
    case LifecycleState::STALE:       return "STALE";
    case LifecycleState::INVALIDATED: return "INVALIDATED";
    case LifecycleState::REBUILDING:  return "REBUILDING";
    case LifecycleState::FAILED:      return "FAILED";
    default:                          return "UNKNOWN";
  }
}

std::optional<LifecycleState> ArtifactLifecyclePolicy::stringToState(
    const std::string& state_str) {
  if (state_str == "READY"  || state_str == "ACTIVE") return LifecycleState::READY;
  if (state_str == "STALE")       return LifecycleState::STALE;
  if (state_str == "INVALIDATED") return LifecycleState::INVALIDATED;
  if (state_str == "REBUILDING")  return LifecycleState::REBUILDING;
  if (state_str == "FAILED")      return LifecycleState::FAILED;
  return std::nullopt;
}

bool ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState state) noexcept {
  return state == LifecycleState::READY || state == LifecycleState::STALE;
}

// ============================================================================
// ArtifactClassifier — class/semantic validation and string conversion
// ============================================================================

bool ArtifactClassifier::isValidCombination(ArtifactClass klass,
                                             TruthSemantic  semantic) noexcept {
  switch (klass) {
    case ArtifactClass::SOURCE_OF_TRUTH:
      return semantic == TruthSemantic::GROUND_TRUTH;
    case ArtifactClass::DERIVED:
    case ArtifactClass::EPHEMERAL:
      return semantic == TruthSemantic::ADVISORY_ONLY;
    default:
      return false;
  }
}

std::string ArtifactClassifier::classToString(ArtifactClass klass) {
  switch (klass) {
    case ArtifactClass::SOURCE_OF_TRUTH: return "SOURCE_OF_TRUTH";
    case ArtifactClass::DERIVED:         return "DERIVED";
    case ArtifactClass::EPHEMERAL:       return "EPHEMERAL";
    default:                             return "UNKNOWN";
  }
}

std::optional<ArtifactClass> ArtifactClassifier::stringToClass(
    const std::string& class_str) {
  if (class_str == "SOURCE_OF_TRUTH") return ArtifactClass::SOURCE_OF_TRUTH;
  if (class_str == "DERIVED")         return ArtifactClass::DERIVED;
  if (class_str == "EPHEMERAL")       return ArtifactClass::EPHEMERAL;
  return std::nullopt;
}

std::string ArtifactClassifier::semanticToString(TruthSemantic semantic) {
  switch (semantic) {
    case TruthSemantic::ADVISORY_ONLY: return "ADVISORY_ONLY";
    case TruthSemantic::GROUND_TRUTH:  return "GROUND_TRUTH";
    default:                           return "UNKNOWN";
  }
}

std::optional<TruthSemantic> ArtifactClassifier::stringToSemantic(
    const std::string& semantic_str) {
  if (semantic_str == "ADVISORY_ONLY") return TruthSemantic::ADVISORY_ONLY;
  if (semantic_str == "GROUND_TRUTH")  return TruthSemantic::GROUND_TRUTH;
  return std::nullopt;
}

// ============================================================================
// RebuildStateUtils
// ============================================================================

std::string RebuildStateUtils::stateToString(RebuildState state) {
  switch (state) {
    case RebuildState::PRISTINE:         return "PRISTINE";
    case RebuildState::PATCHED:          return "PATCHED";
    case RebuildState::PARTIAL_REFITTED: return "PARTIAL_REFITTED";
    case RebuildState::REBUILT:          return "REBUILT";
    default:                             return "UNKNOWN";
  }
}

std::optional<RebuildState> RebuildStateUtils::stringToState(
    const std::string& state_str) {
  if (state_str == "PRISTINE")         return RebuildState::PRISTINE;
  if (state_str == "PATCHED")          return RebuildState::PATCHED;
  if (state_str == "PARTIAL_REFITTED") return RebuildState::PARTIAL_REFITTED;
  if (state_str == "REBUILT")          return RebuildState::REBUILT;
  return std::nullopt;
}

// ============================================================================
// UpdateModeUtils
// ============================================================================

std::string UpdateModeUtils::modeToString(UpdateMode mode) {
  switch (mode) {
    case UpdateMode::PATCH:         return "patch";
    case UpdateMode::PARTIAL_REFIT: return "partial_refit";
    case UpdateMode::REBUILD:       return "rebuild";
    default:                        return "unknown";
  }
}

std::optional<UpdateMode> UpdateModeUtils::stringToMode(
    const std::string& mode_str) {
  if (mode_str == "patch")         return UpdateMode::PATCH;
  if (mode_str == "partial_refit") return UpdateMode::PARTIAL_REFIT;
  if (mode_str == "rebuild")       return UpdateMode::REBUILD;
  return std::nullopt;
}

// ============================================================================
// InvalidationReasonUtils
// ============================================================================

std::string InvalidationReasonUtils::reasonToString(InvalidationReason reason) {
  switch (reason) {
    case InvalidationReason::UNKNOWN:                  return "UNKNOWN";
    case InvalidationReason::INTEGRITY_CHECK_FAILED:   return "INTEGRITY_CHECK_FAILED";
    case InvalidationReason::STALENESS_EXCEEDED:       return "STALENESS_EXCEEDED";
    case InvalidationReason::SOURCE_INVALIDATED:       return "SOURCE_INVALIDATED";
    case InvalidationReason::SOURCE_LINEAGE_CORRUPTED: return "SOURCE_LINEAGE_CORRUPTED";
    case InvalidationReason::POLICY_VIOLATION:         return "POLICY_VIOLATION";
    case InvalidationReason::ADMIN_REQUESTED:          return "ADMIN_REQUESTED";
    case InvalidationReason::SHARD_UNAVAILABLE:        return "SHARD_UNAVAILABLE";
    default:                                           return "UNKNOWN";
  }
}

std::optional<InvalidationReason> InvalidationReasonUtils::stringToReason(
    const std::string& reason_str) {
  if (reason_str == "UNKNOWN")                  return InvalidationReason::UNKNOWN;
  if (reason_str == "INTEGRITY_CHECK_FAILED")   return InvalidationReason::INTEGRITY_CHECK_FAILED;
  if (reason_str == "STALENESS_EXCEEDED")       return InvalidationReason::STALENESS_EXCEEDED;
  if (reason_str == "SOURCE_INVALIDATED")       return InvalidationReason::SOURCE_INVALIDATED;
  if (reason_str == "SOURCE_LINEAGE_CORRUPTED") return InvalidationReason::SOURCE_LINEAGE_CORRUPTED;
  if (reason_str == "POLICY_VIOLATION")         return InvalidationReason::POLICY_VIOLATION;
  if (reason_str == "ADMIN_REQUESTED")          return InvalidationReason::ADMIN_REQUESTED;
  if (reason_str == "SHARD_UNAVAILABLE")        return InvalidationReason::SHARD_UNAVAILABLE;
  return std::nullopt;
}

// ============================================================================
// ArtifactManifest methods
// ============================================================================

bool ArtifactManifest::validate() const {
  // artifact_id must be non-empty
  if (artifact_id.empty()) {
    return false;
  }

  // Validate content_hash format if set (SHA-256 = 64 hex, SHA-1 = 40, MD5 = 32)
  if (!content_hash.empty()) {
    if (content_hash.length() < 32 || content_hash.length() > 128) {
      return false;
    }
    for (char c : content_hash) {
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        return false;
      }
    }
  }

  // Timestamp consistency: updated_at >= created_at
  if (created_at_unix_sec > 0 && updated_at_unix_sec > 0) {
    if (updated_at_unix_sec < created_at_unix_sec) {
      return false;
    }
  }

  // last_verified >= created_at
  if (created_at_unix_sec > 0 && last_verified_unix_sec > 0) {
    if (last_verified_unix_sec < created_at_unix_sec) {
      return false;
    }
  }

  // Sequence range consistency
  if (source_seq_end > 0 && source_seq_start > 0) {
    if (source_seq_end < source_seq_start) {
      return false;
    }
  }

  // Residual must be in a reasonable range
  if (residual < 0.0 || residual > 1000.0) {
    return false;
  }

  // rank_status must not exceed rank_cap (when rank_cap is non-zero)
  if (rank_status > 0 && rank_cap > 0) {
    if (rank_status > rank_cap) {
      return false;
    }
  }

  // Replication factor must be at least 1
  if (replication_factor < 1) {
    return false;
  }

  // Artifact class and truth-semantic combination must be valid
  if (!ArtifactClassifier::isValidCombination(artifact_class, truth_semantic)) {
    return false;
  }

  return true;
}

bool ArtifactManifest::isUsable(int64_t /*now_unix_sec*/) const {
  return ArtifactLifecyclePolicy::isUsableForPlanning(lifecycle_state);
}

bool ArtifactManifest::isStale(int64_t now_unix_sec) const {
  if (staleness_threshold_sec == 0) {
    return false;  // No threshold configured; artifact never considered stale
  }
  if (last_verified_unix_sec == 0) {
    return true;   // Never verified with a threshold set → stale
  }
  const int64_t age_sec = now_unix_sec - last_verified_unix_sec;
  return age_sec > staleness_threshold_sec;
}

double ArtifactManifest::getFreshnessScore(int64_t now_unix_sec) const {
  if (last_verified_unix_sec == 0) {
    return 0.0;  // Never verified → not fresh
  }
  if (staleness_threshold_sec == 0) {
    return 1.0;  // No threshold configured → always fresh
  }

  int64_t age_sec = now_unix_sec - last_verified_unix_sec;
  if (age_sec < 0) age_sec = 0;
  if (age_sec > staleness_threshold_sec) age_sec = staleness_threshold_sec;

  return 1.0 - static_cast<double>(age_sec) / static_cast<double>(staleness_threshold_sec);
}

bool ArtifactManifest::isCorrupted() const {
  if (manifest_hash.empty()) {
    return false;  // No stored hash → cannot detect corruption
  }

  // Recompute a hash of the core manifest fields (excluding manifest_hash itself)
  json j;
  j["artifact_id"]           = artifact_id;
  j["version"]               = version;
  j["content_hash"]          = content_hash;
  j["artifact_class"]        = static_cast<int>(artifact_class);
  j["truth_semantic"]        = static_cast<int>(truth_semantic);
  j["lifecycle_state"]       = static_cast<int>(lifecycle_state);
  j["created_at_unix_sec"]   = created_at_unix_sec;
  j["updated_at_unix_sec"]   = updated_at_unix_sec;
  j["last_verified_unix_sec"]= last_verified_unix_sec;
  j["last_rebuild_at_unix_sec"] = last_rebuild_at_unix_sec;
  j["residual"]              = residual;
  j["rank_cap"]              = rank_cap;
  j["rank_status"]           = rank_status;
  j["rebuild_state"]         = static_cast<int>(rebuild_state);
  j["update_mode"]           = static_cast<int>(update_mode);
  j["invalidation_reason"]   = static_cast<int>(invalidation_reason);

  const std::string serialized = j.dump();
  const std::size_t hash_val   = std::hash<std::string>{}(serialized);

  std::ostringstream oss;
  oss << std::hex << hash_val;
  return oss.str() != manifest_hash;
}

std::string ArtifactManifest::toJSON() const {
  json j;

  // Identity & Classification
  j["artifact_id"]    = artifact_id;
  j["artifact_class"] = ArtifactClassifier::classToString(artifact_class);
  j["truth_semantic"] = ArtifactClassifier::semanticToString(truth_semantic);
  j["lifecycle_state"]= ArtifactLifecyclePolicy::stateToString(lifecycle_state);

  // Versioning & Integrity
  j["version"]       = version;
  j["content_hash"]  = content_hash;
  j["manifest_hash"] = manifest_hash;

  // Temporal Metadata
  j["created_at_unix_sec"]      = created_at_unix_sec;
  j["updated_at_unix_sec"]      = updated_at_unix_sec;
  j["last_verified_unix_sec"]   = last_verified_unix_sec;
  j["last_rebuild_at_unix_sec"] = last_rebuild_at_unix_sec;
  j["staleness_threshold_sec"]  = staleness_threshold_sec;
  j["artifact_age_ms"]          = artifact_age_ms;

  // Sequence & Lag Tracking
  j["source_seq_start"] = source_seq_start;
  j["source_seq_end"]   = source_seq_end;
  j["delta_lag"]        = delta_lag;

  // Approximation & Quality Metrics
  j["residual"]     = residual;
  j["rank_cap"]     = rank_cap;
  j["rank_status"]  = rank_status;

  // Rebuild & Update Tracking
  j["rebuild_state"]       = RebuildStateUtils::stateToString(rebuild_state);
  j["update_mode"]         = UpdateModeUtils::modeToString(update_mode);
  j["invalidation_reason"] = InvalidationReasonUtils::reasonToString(invalidation_reason);

  // Provenance & Reconstruction
  j["source_artifact_id"]         = source_artifact_id;
  j["provenance_chain"]           = provenance_chain;
  j["reconstruction_instructions"]= reconstruction_instructions;

  // Placement & Distribution
  j["shard_placements"]         = shard_placements;
  j["requires_full_replication"]= requires_full_replication;
  j["is_rebuildable"]           = is_rebuildable;

  // Replication & Redundancy Strategy
  j["replication_factor"]       = replication_factor;
  j["erasure_coding_scheme"]    = erasure_coding_scheme;
  j["backup_shard_placements"]  = backup_shard_placements;

  // Compatibility & Constraints
  j["compatibility_metadata"] = compatibility_metadata;
  j["min_planner_version"]    = min_planner_version;
  j["advisory_only"]          = advisory_only;

  // Metadata & Extensibility
  j["custom_attributes"] = custom_attributes;
  j["description"]       = description;

  return j.dump(2);
}

std::optional<ArtifactManifest> ArtifactManifest::fromJSON(
    const std::string& json_str) {
  try {
    json j = json::parse(json_str);
    ArtifactManifest manifest;

    // Identity & Classification
    if (j.contains("artifact_id"))
      manifest.artifact_id = j["artifact_id"].get<std::string>();
    if (j.contains("artifact_class")) {
      auto ac = ArtifactClassifier::stringToClass(j["artifact_class"].get<std::string>());
      if (ac) manifest.artifact_class = *ac;
    }
    if (j.contains("truth_semantic")) {
      auto ts = ArtifactClassifier::stringToSemantic(j["truth_semantic"].get<std::string>());
      if (ts) manifest.truth_semantic = *ts;
    }
    if (j.contains("lifecycle_state")) {
      auto ls = ArtifactLifecyclePolicy::stringToState(j["lifecycle_state"].get<std::string>());
      if (ls) manifest.lifecycle_state = *ls;
    }

    // Versioning & Integrity
    if (j.contains("version"))
      manifest.version = j["version"].get<uint64_t>();
    if (j.contains("content_hash"))
      manifest.content_hash = j["content_hash"].get<std::string>();
    if (j.contains("manifest_hash"))
      manifest.manifest_hash = j["manifest_hash"].get<std::string>();

    // Temporal Metadata
    if (j.contains("created_at_unix_sec"))
      manifest.created_at_unix_sec = j["created_at_unix_sec"].get<int64_t>();
    if (j.contains("updated_at_unix_sec"))
      manifest.updated_at_unix_sec = j["updated_at_unix_sec"].get<int64_t>();
    if (j.contains("last_verified_unix_sec"))
      manifest.last_verified_unix_sec = j["last_verified_unix_sec"].get<int64_t>();
    if (j.contains("last_rebuild_at_unix_sec"))
      manifest.last_rebuild_at_unix_sec = j["last_rebuild_at_unix_sec"].get<int64_t>();
    if (j.contains("staleness_threshold_sec"))
      manifest.staleness_threshold_sec = j["staleness_threshold_sec"].get<int64_t>();
    if (j.contains("artifact_age_ms"))
      manifest.artifact_age_ms = j["artifact_age_ms"].get<uint64_t>();

    // Sequence & Lag Tracking
    if (j.contains("source_seq_start"))
      manifest.source_seq_start = j["source_seq_start"].get<uint64_t>();
    if (j.contains("source_seq_end"))
      manifest.source_seq_end = j["source_seq_end"].get<uint64_t>();
    if (j.contains("delta_lag"))
      manifest.delta_lag = j["delta_lag"].get<uint64_t>();

    // Approximation & Quality Metrics
    if (j.contains("residual"))
      manifest.residual = j["residual"].get<double>();
    if (j.contains("rank_cap"))
      manifest.rank_cap = j["rank_cap"].get<uint32_t>();
    if (j.contains("rank_status"))
      manifest.rank_status = j["rank_status"].get<uint32_t>();

    // Rebuild & Update Tracking
    if (j.contains("rebuild_state")) {
      auto rs = RebuildStateUtils::stringToState(j["rebuild_state"].get<std::string>());
      if (rs) manifest.rebuild_state = *rs;
    }
    if (j.contains("update_mode")) {
      auto um = UpdateModeUtils::stringToMode(j["update_mode"].get<std::string>());
      if (um) manifest.update_mode = *um;
    }
    if (j.contains("invalidation_reason")) {
      auto ir = InvalidationReasonUtils::stringToReason(j["invalidation_reason"].get<std::string>());
      if (ir) manifest.invalidation_reason = *ir;
    }

    // Provenance & Reconstruction
    if (j.contains("source_artifact_id"))
      manifest.source_artifact_id = j["source_artifact_id"].get<std::string>();
    if (j.contains("provenance_chain"))
      manifest.provenance_chain = j["provenance_chain"].get<std::vector<std::string>>();
    if (j.contains("reconstruction_instructions"))
      manifest.reconstruction_instructions = j["reconstruction_instructions"].get<std::string>();

    // Placement & Distribution
    if (j.contains("shard_placements"))
      manifest.shard_placements = j["shard_placements"].get<std::vector<std::string>>();
    if (j.contains("requires_full_replication"))
      manifest.requires_full_replication = j["requires_full_replication"].get<bool>();
    if (j.contains("is_rebuildable"))
      manifest.is_rebuildable = j["is_rebuildable"].get<bool>();

    // Replication & Redundancy Strategy
    if (j.contains("replication_factor"))
      manifest.replication_factor = j["replication_factor"].get<uint32_t>();
    if (j.contains("erasure_coding_scheme"))
      manifest.erasure_coding_scheme = j["erasure_coding_scheme"].get<std::string>();
    if (j.contains("backup_shard_placements"))
      manifest.backup_shard_placements = j["backup_shard_placements"].get<std::vector<std::string>>();

    // Compatibility & Constraints
    if (j.contains("compatibility_metadata"))
      manifest.compatibility_metadata =
        j["compatibility_metadata"].get<std::map<std::string, std::string>>();
    if (j.contains("min_planner_version"))
      manifest.min_planner_version = j["min_planner_version"].get<std::string>();
    if (j.contains("advisory_only"))
      manifest.advisory_only = j["advisory_only"].get<bool>();

    // Metadata & Extensibility
    if (j.contains("custom_attributes"))
      manifest.custom_attributes =
        j["custom_attributes"].get<std::map<std::string, std::string>>();
    if (j.contains("description"))
      manifest.description = j["description"].get<std::string>();

    return manifest;
  } catch (...) {
    return std::nullopt;
  }
}

std::string ArtifactManifest::toYAML() const {
  std::ostringstream oss;
  oss << "# ArtifactManifest\n";
  oss << "artifact_id: "           << artifact_id << "\n";
  oss << "version: "               << version << "\n";
  oss << "artifact_class: "        << ArtifactClassifier::classToString(artifact_class) << "\n";
  oss << "truth_semantic: "        << ArtifactClassifier::semanticToString(truth_semantic) << "\n";
  oss << "lifecycle_state: "       << ArtifactLifecyclePolicy::stateToString(lifecycle_state) << "\n";
  oss << "content_hash: "          << content_hash << "\n";
  oss << "manifest_hash: "         << manifest_hash << "\n";
  oss << "created_at_unix_sec: "   << created_at_unix_sec << "\n";
  oss << "updated_at_unix_sec: "   << updated_at_unix_sec << "\n";
  oss << "last_verified_unix_sec: "<< last_verified_unix_sec << "\n";
  oss << "last_rebuild_at_unix_sec: " << last_rebuild_at_unix_sec << "\n";
  oss << "source_seq_start: "      << source_seq_start << "\n";
  oss << "source_seq_end: "        << source_seq_end << "\n";
  oss << "delta_lag: "             << delta_lag << "\n";
  oss << "artifact_age_ms: "       << artifact_age_ms << "\n";
  oss << "residual: "              << std::scientific << residual << "\n";
  oss << "rank_cap: "              << rank_cap << "\n";
  oss << "rank_status: "           << rank_status << "\n";
  oss << "rebuild_state: "         << RebuildStateUtils::stateToString(rebuild_state) << "\n";
  oss << "update_mode: "           << UpdateModeUtils::modeToString(update_mode) << "\n";
  oss << "invalidation_reason: "   << InvalidationReasonUtils::reasonToString(invalidation_reason) << "\n";
  return oss.str();
}

std::optional<ArtifactManifest> ArtifactManifest::fromYAML(
    const std::string& yaml_str) {
  // Attempt JSON parse first (covers YAML supersets with JSON content)
  if (yaml_str.find('{') != std::string::npos) {
    return fromJSON(yaml_str);
  }

  // Parse simple "key: value" YAML line-by-line into a JSON object
  json j;
  std::istringstream iss(yaml_str);
  std::string line;
  const auto isWhitespace = [](char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  };
  const auto trim = [&](std::string& s) {
    while (!s.empty() && isWhitespace(s.front())) {
      s.erase(s.begin());
    }
    while (!s.empty() && isWhitespace(s.back())) {
      s.pop_back();
    }
  };
  const auto toLower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    return s;
  };
  const auto parseYamlScalar = [&](const std::string& value) -> json {
    const std::string lowered = toLower(value);
    if (lowered == "true") {
      return true;
    }
    if (lowered == "false") {
      return false;
    }

    if (value.find_first_of(".eE") != std::string::npos) {
      char* end_ptr = nullptr;
      errno = 0;
      const double parsed = std::strtod(value.c_str(), &end_ptr);
      if (end_ptr != value.c_str() && end_ptr != nullptr && *end_ptr == '\0' && errno != ERANGE) {
        return parsed;
      }
    }

    if (!value.empty() && value.front() == '-') {
      char* end_ptr = nullptr;
      errno = 0;
      const long long parsed = std::strtoll(value.c_str(), &end_ptr, 10);
      if (end_ptr != value.c_str() && end_ptr != nullptr && *end_ptr == '\0' && errno != ERANGE) {
        return parsed;
      }
    } else {
      char* end_ptr = nullptr;
      errno = 0;
      const unsigned long long parsed = std::strtoull(value.c_str(), &end_ptr, 10);
      if (end_ptr != value.c_str() && end_ptr != nullptr && *end_ptr == '\0' && errno != ERANGE) {
        return parsed;
      }
    }

    return value;
  };

  while (std::getline(iss, line)) {
    if (line.empty() || line[0] == '#') continue;
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) continue;

    std::string key   = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    trim(key);
    trim(value);

    if (!value.empty()) {
      j[key] = parseYamlScalar(value);
    }
  }

  // Reconstruct via JSON round-trip (type coercion handled in fromJSON)
  return fromJSON(j.dump());
}

void ArtifactManifest::markPublished(UpdateMode mode, RebuildState rebuild_state,
                                     uint64_t new_source_seq) {
  // Record the update mode
  update_mode = mode;
  this->rebuild_state = rebuild_state;

  // Update source sequence to reflect the new end point
  if (new_source_seq > source_seq_end) {
    source_seq_end = new_source_seq;
  }

  // Update the last verification time to now
  last_verified_unix_sec = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  // Recalculate delta lag based on new source_seq_end
  // (Delta lag would typically be computed from the current exact-graph sequence,
  // but for now we reset it to 0 to indicate a fresh update)
  delta_lag = 0;

  // Mark lifecycle as READY after successful publish
  lifecycle_state = LifecycleState::READY;
}

}  // namespace distributed_tensor
}  // namespace themis
