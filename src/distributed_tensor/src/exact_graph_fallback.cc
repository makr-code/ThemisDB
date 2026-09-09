/// @file exact_graph_fallback.cc
/// @brief Implementation of exact graph fallback policy
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/exact_graph_fallback.h"

namespace themis {
namespace distributed_tensor {

bool ExactGraphFallbackPolicy::canUseArtifact(const ArtifactManifest& manifest,
                                              double query_residual_tolerance,
                                              int64_t now_unix_sec) const {
  // Check all fallback conditions
  if (requiresFallbackForState(manifest)) {
    return false;
  }
  if (requiresFallbackForSemantics(manifest, query_residual_tolerance > 0)) {
    return false;
  }
  if (requiresFallbackForResidual(manifest, query_residual_tolerance)) {
    return false;
  }
  if (requiresFallbackForFreshness(manifest, now_unix_sec, 0)) {
    return false;
  }
  return true;
}

bool ExactGraphFallbackPolicy::requiresFallbackForState(const ArtifactManifest& manifest) const {
  // Only ACTIVE and STALE states are usable
  return manifest.lifecycle_state != LifecycleState::ACTIVE && manifest.lifecycle_state != LifecycleState::STALE;
}

bool ExactGraphFallbackPolicy::requiresFallbackForSemantics(const ArtifactManifest& manifest,
                                                            bool query_requires_truth) const {
  // If query requires truth and artifact is advisory-only, need fallback
  if (query_requires_truth && manifest.advisory_only) {
    return true;
  }
  return false;
}

bool ExactGraphFallbackPolicy::requiresFallbackForResidual(const ArtifactManifest& manifest,
                                                           double query_residual_tolerance) const {
  // If query has residual tolerance requirement and artifact exceeds it
  if (query_residual_tolerance > 0.0 && manifest.residual > query_residual_tolerance) {
    return true;
  }
  return false;
}

bool ExactGraphFallbackPolicy::requiresFallbackForFreshness(const ArtifactManifest& manifest,
                                                            int64_t now_unix_sec,
                                                            int64_t query_max_age_ms) const {
  // If query requires specific freshness and artifact is too stale
  if (query_max_age_ms > 0) {
    if (manifest.artifact_age_ms > static_cast<uint64_t>(query_max_age_ms)) {
      return true;
    }
  }
  return false;
}

ExactGraphFallbackPolicy::FallbackMetrics ExactGraphFallbackPolicy::getMetrics() const { return metrics_; }

void ExactGraphFallbackPolicy::recordFallback(const std::string& reason) { metrics_.total_fallback_decisions++; }

}  // namespace distributed_tensor
}  // namespace themis
