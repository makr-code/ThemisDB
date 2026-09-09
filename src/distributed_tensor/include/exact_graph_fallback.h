/// @file exact_graph_fallback.h
/// @brief Exact graph fallback policy and integration for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines policies for fallback to exact graph state when tensor
/// artifacts are stale, invalid, or don't meet query accuracy requirements.
///
/// ## Design Philosophy
///
/// The exact graph is always authoritative. Tensor artifacts are advisory-only.
/// Fallback occurs when:
/// - Artifact is not in ACTIVE state
/// - Advisory-only semantics mean artifact cannot guarantee correctness
/// - Query accuracy requirements cannot be met by artifact
/// - Residual is too high for the query's tolerance
///

#pragma once

#include "artifact_manifest.h"
#include <string>
#include <memory>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @brief Exact graph fallback policy for query planning.
///
/// Determines whether a tensor artifact can be used for a query or whether
/// fallback to exact graph is required.
///
class ExactGraphFallbackPolicy {
 public:
  ExactGraphFallbackPolicy() = default;
  virtual ~ExactGraphFallbackPolicy() = default;

  // Prevent copy/move
  ExactGraphFallbackPolicy(const ExactGraphFallbackPolicy&) = delete;
  ExactGraphFallbackPolicy& operator=(const ExactGraphFallbackPolicy&) = delete;
  ExactGraphFallbackPolicy(ExactGraphFallbackPolicy&&) = delete;
  ExactGraphFallbackPolicy& operator=(ExactGraphFallbackPolicy&&) = delete;

  /// Checks if artifact is suitable for use (no fallback needed).
  /// @param manifest Artifact manifest
  /// @param query_residual_tolerance Maximum acceptable residual for query
  /// @param now_unix_sec Current time
  /// @return true if artifact can be used, false if exact fallback required
  virtual bool canUseArtifact(const ArtifactManifest& manifest,
                              double query_residual_tolerance,
                              int64_t now_unix_sec) const;

  /// Determines if artifact must use exact fallback due to lifecycle state.
  /// @param manifest Artifact manifest
  /// @return true if fallback required
  virtual bool requiresFallbackForState(const ArtifactManifest& manifest) const;

  /// Determines if artifact must use exact fallback due to advisory-only semantics.
  /// @param manifest Artifact manifest
  /// @param query_requires_truth true if query requires truth-bearing guarantees
  /// @return true if fallback required
  virtual bool requiresFallbackForSemantics(const ArtifactManifest& manifest,
                                            bool query_requires_truth) const;

  /// Determines if artifact must use exact fallback due to residual threshold.
  /// @param manifest Artifact manifest
  /// @param query_residual_tolerance Maximum acceptable residual
  /// @return true if fallback required
  virtual bool requiresFallbackForResidual(const ArtifactManifest& manifest,
                                           double query_residual_tolerance) const;

  /// Determines if artifact must use exact fallback due to freshness.
  /// @param manifest Artifact manifest
  /// @param now_unix_sec Current time
  /// @param query_max_age_ms Maximum acceptable artifact age (0 = any age OK)
  /// @return true if fallback required
  virtual bool requiresFallbackForFreshness(const ArtifactManifest& manifest,
                                            int64_t now_unix_sec,
                                            int64_t query_max_age_ms) const;

  /// Collects metrics about fallback decisions for observability.
  struct FallbackMetrics {
    uint64_t total_fallback_decisions = 0;
    uint64_t fallback_due_to_state = 0;
    uint64_t fallback_due_to_semantics = 0;
    uint64_t fallback_due_to_residual = 0;
    uint64_t fallback_due_to_freshness = 0;
    uint64_t artifacts_used_successfully = 0;
  };

  /// Returns fallback metrics.
  virtual FallbackMetrics getMetrics() const;

  /// Records a fallback decision for metrics.
  virtual void recordFallback(const std::string& reason);

 protected:
  FallbackMetrics metrics_;
};

}  // namespace distributed_tensor
}  // namespace themis
