/// @file tensor_artifact_classes.cc
/// @brief Implementation of tensor artifact classification and lifecycle management
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-01

#include "src/distributed_tensor/include/tensor_artifact_classes.h"

#include <algorithm>
#include <cassert>
#include <ctime>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// ArtifactLifecyclePolicy: State Transition Validation
// ============================================================================

bool ArtifactLifecyclePolicy::isValidTransition(LifecycleState source_state,
                                                LifecycleState target_state) {
  // State transition matrix: defines which transitions are allowed.
  // Reference: DISTRIBUTED_TENSOR_SHARDING.md § 10 Recovery and Rebuild
  switch (source_state) {
    case LifecycleState::CREATED:
      // From CREATED, we can transition to ACTIVE or INVALIDATED (failed creation)
      return target_state == LifecycleState::ACTIVE ||
             target_state == LifecycleState::INVALIDATED;

    case LifecycleState::ACTIVE:
      // From ACTIVE, we can transition to STALE, INVALIDATED, or DELETED
      return target_state == LifecycleState::STALE ||
             target_state == LifecycleState::INVALIDATED ||
             target_state == LifecycleState::DELETED;

    case LifecycleState::STALE:
      // From STALE, we can transition to REBUILT, INVALIDATED, or DELETED
      return target_state == LifecycleState::REBUILT ||
             target_state == LifecycleState::INVALIDATED ||
             target_state == LifecycleState::DELETED;

    case LifecycleState::INVALIDATED:
      // From INVALIDATED, we can transition to REBUILT or DELETED
      return target_state == LifecycleState::REBUILT ||
             target_state == LifecycleState::DELETED;

    case LifecycleState::REBUILT:
      // From REBUILT, we should transition to ACTIVE (after verification)
      return target_state == LifecycleState::ACTIVE;

    case LifecycleState::DELETED:
      // DELETED is terminal; no transitions allowed
      return false;

    default:
      return false;
  }
}

std::string ArtifactLifecyclePolicy::stateToString(LifecycleState state) {
  switch (state) {
    case LifecycleState::CREATED:
      return "CREATED";
    case LifecycleState::ACTIVE:
      return "ACTIVE";
    case LifecycleState::STALE:
      return "STALE";
    case LifecycleState::INVALIDATED:
      return "INVALIDATED";
    case LifecycleState::REBUILT:
      return "REBUILT";
    case LifecycleState::DELETED:
      return "DELETED";
    default:
      return "UNKNOWN";
  }
}

std::optional<LifecycleState> ArtifactLifecyclePolicy::stringToState(
    const std::string& state_str) {
  if (state_str == "CREATED") return LifecycleState::CREATED;
  if (state_str == "ACTIVE") return LifecycleState::ACTIVE;
  if (state_str == "STALE") return LifecycleState::STALE;
  if (state_str == "INVALIDATED") return LifecycleState::INVALIDATED;
  if (state_str == "REBUILT") return LifecycleState::REBUILT;
  if (state_str == "DELETED") return LifecycleState::DELETED;
  return std::nullopt;
}

bool ArtifactLifecyclePolicy::isUsable(LifecycleState state) {
  return state == LifecycleState::ACTIVE || state == LifecycleState::STALE;
}

bool ArtifactLifecyclePolicy::isTerminal(LifecycleState state) {
  return state == LifecycleState::DELETED;
}

bool ArtifactLifecyclePolicy::requiresVerification(LifecycleState state) {
  return state == LifecycleState::CREATED || state == LifecycleState::REBUILT;
}

// ============================================================================
// ArtifactClassifier: Classification and Semantic Validation
// ============================================================================

std::string ArtifactClassifier::classToString(ArtifactClass artifact_class) {
  switch (artifact_class) {
    case ArtifactClass::PRIMARY:
      return "PRIMARY";
    case ArtifactClass::DERIVED:
      return "DERIVED";
    case ArtifactClass::EPHEMERAL:
      return "EPHEMERAL";
    case ArtifactClass::ADVISORY_ONLY:
      return "ADVISORY_ONLY";
    default:
      return "UNKNOWN";
  }
}

std::optional<ArtifactClass> ArtifactClassifier::stringToClass(
    const std::string& class_str) {
  if (class_str == "PRIMARY") return ArtifactClass::PRIMARY;
  if (class_str == "DERIVED") return ArtifactClass::DERIVED;
  if (class_str == "EPHEMERAL") return ArtifactClass::EPHEMERAL;
  if (class_str == "ADVISORY_ONLY") return ArtifactClass::ADVISORY_ONLY;
  return std::nullopt;
}

std::string ArtifactClassifier::semanticToString(TruthSemantic semantic) {
  switch (semantic) {
    case TruthSemantic::SOURCE_OF_TRUTH:
      return "SOURCE_OF_TRUTH";
    case TruthSemantic::TRUTH_ADJACENT:
      return "TRUTH_ADJACENT";
    case TruthSemantic::ADVISORY:
      return "ADVISORY";
    default:
      return "UNKNOWN";
  }
}

std::optional<TruthSemantic> ArtifactClassifier::stringToSemantic(
    const std::string& semantic_str) {
  if (semantic_str == "SOURCE_OF_TRUTH") return TruthSemantic::SOURCE_OF_TRUTH;
  if (semantic_str == "TRUTH_ADJACENT") return TruthSemantic::TRUTH_ADJACENT;
  if (semantic_str == "ADVISORY") return TruthSemantic::ADVISORY;
  return std::nullopt;
}

bool ArtifactClassifier::isValidCombination(ArtifactClass artifact_class,
                                            TruthSemantic truth_semantic) {
  // Validation rules based on DISTRIBUTED_TENSOR_SHARDING.md and EPIC 3 documentation
  switch (artifact_class) {
    case ArtifactClass::PRIMARY:
      // Primary artifacts must be source-of-truth or truth-adjacent
      return truth_semantic == TruthSemantic::SOURCE_OF_TRUTH ||
             truth_semantic == TruthSemantic::TRUTH_ADJACENT;

    case ArtifactClass::DERIVED:
      // Derived artifacts must be truth-adjacent or advisory
      // (never source-of-truth since they are generated)
      return truth_semantic == TruthSemantic::TRUTH_ADJACENT ||
             truth_semantic == TruthSemantic::ADVISORY;

    case ArtifactClass::EPHEMERAL:
      // Ephemeral artifacts can be any semantic depending on their role
      // (temporary contractions might be advisory, session summaries might be truth-adjacent)
      return true;

    case ArtifactClass::ADVISORY_ONLY:
      // Advisory-only artifacts must have advisory semantic
      return truth_semantic == TruthSemantic::ADVISORY;

    default:
      return false;
  }
}

bool ArtifactClassifier::isAdvisoryOnly(ArtifactClass artifact_class,
                                        TruthSemantic truth_semantic) {
  // An artifact is advisory-only if it cannot affect correctness
  if (artifact_class == ArtifactClass::ADVISORY_ONLY) {
    return true;  // By definition
  }
  if (truth_semantic == TruthSemantic::ADVISORY) {
    return true;  // Advisory semantics means no binding truth
  }
  return false;
}

bool ArtifactClassifier::isTruthBearing(ArtifactClass artifact_class,
                                        TruthSemantic truth_semantic) {
  // An artifact is truth-bearing if it affects correctness
  if (artifact_class == ArtifactClass::ADVISORY_ONLY) {
    return false;  // Explicitly not truth-bearing
  }
  if (truth_semantic == TruthSemantic::ADVISORY) {
    return false;  // Advisory semantics are not truth-bearing
  }
  return truth_semantic == TruthSemantic::SOURCE_OF_TRUTH ||
         truth_semantic == TruthSemantic::TRUTH_ADJACENT;
}

// ============================================================================
// InMemoryArtifactRegistry: Registry Implementation
// ============================================================================

bool InMemoryArtifactRegistry::registerArtifact(const ArtifactMetadata& metadata) {
  // Validate classification
  if (!ArtifactClassifier::isValidCombination(metadata.artifact_class,
                                              metadata.truth_semantic)) {
    return false;
  }

  // Ensure artifact_id is unique
  if (metadata_map_.find(metadata.artifact_id) != metadata_map_.end()) {
    return false;
  }

  // Ensure initial state is CREATED
  if (metadata.lifecycle_state != LifecycleState::CREATED &&
      metadata.lifecycle_state != LifecycleState::ACTIVE) {
    return false;
  }

  metadata_map_[metadata.artifact_id] = metadata;
  return true;
}

std::optional<ArtifactMetadata> InMemoryArtifactRegistry::lookup(
    const std::string& artifact_id) const {
  auto it = metadata_map_.find(artifact_id);
  if (it == metadata_map_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool InMemoryArtifactRegistry::transitionState(const std::string& artifact_id,
                                               LifecycleState new_state) {
  auto it = metadata_map_.find(artifact_id);
  if (it == metadata_map_.end()) {
    return false;
  }

  // Validate transition
  if (!ArtifactLifecyclePolicy::isValidTransition(it->second.lifecycle_state, new_state)) {
    return false;
  }

  // Perform transition
  it->second.lifecycle_state = new_state;
  it->second.updated_at_unix_sec = time(nullptr);
  return true;
}

std::vector<std::string> InMemoryArtifactRegistry::listByClass(
    ArtifactClass artifact_class) const {
  std::vector<std::string> result;
  for (const auto& [id, metadata] : metadata_map_) {
    if (metadata.artifact_class == artifact_class) {
      result.push_back(id);
    }
  }
  return result;
}

std::vector<std::string> InMemoryArtifactRegistry::listBySemantic(
    TruthSemantic semantic) const {
  std::vector<std::string> result;
  for (const auto& [id, metadata] : metadata_map_) {
    if (metadata.truth_semantic == semantic) {
      result.push_back(id);
    }
  }
  return result;
}

std::vector<std::string> InMemoryArtifactRegistry::listByState(LifecycleState state) const {
  std::vector<std::string> result;
  for (const auto& [id, metadata] : metadata_map_) {
    if (metadata.lifecycle_state == state) {
      result.push_back(id);
    }
  }
  return result;
}

size_t InMemoryArtifactRegistry::count() const {
  return metadata_map_.size();
}

bool InMemoryArtifactRegistry::remove(const std::string& artifact_id) {
  auto it = metadata_map_.find(artifact_id);
  if (it == metadata_map_.end()) {
    return false;
  }
  metadata_map_.erase(it);
  return true;
}

void InMemoryArtifactRegistry::clear() {
  metadata_map_.clear();
}

}  // namespace distributed_tensor
}  // namespace themis
