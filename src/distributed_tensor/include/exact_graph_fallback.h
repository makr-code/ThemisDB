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

class ExactGraphFallbackPolicy {
 public:
  ExactGraphFallbackPolicy() = default;
  /**
   * @brief Exact Graph Fallback Policy.
   * @return Return value.
   */
  virtual ~ExactGraphFallbackPolicy() = default;

  // Prevent copy/move
  ExactGraphFallbackPolicy(const ExactGraphFallbackPolicy&) = delete;
  ExactGraphFallbackPolicy& operator=(const ExactGraphFallbackPolicy&) = delete;
  ExactGraphFallbackPolicy(ExactGraphFallbackPolicy&&) = delete;
  ExactGraphFallbackPolicy& operator=(ExactGraphFallbackPolicy&&) = delete;

  /**
   * @brief Can Use Artifact.
   * @param[in] manifest Input parameter.
   * @param[in] query_residual_tolerance Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool canUseArtifact(const ArtifactManifest& manifest,
                              double query_residual_tolerance,
                              int64_t now_unix_sec) const;

  /**
   * @brief Requires Fallback For State.
   * @param[in] manifest Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool requiresFallbackForState(const ArtifactManifest& manifest) const;

  /**
   * @brief Requires Fallback For Semantics.
   * @param[in] manifest Input parameter.
   * @param[in] query_requires_truth Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool requiresFallbackForSemantics(const ArtifactManifest& manifest,
                                            bool query_requires_truth) const;

  /**
   * @brief Requires Fallback For Residual.
   * @param[in] manifest Input parameter.
   * @param[in] query_residual_tolerance Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool requiresFallbackForResidual(const ArtifactManifest& manifest,
                                           double query_residual_tolerance) const;

  /**
   * @brief Requires Fallback For Freshness.
   * @param[in] manifest Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @param[in] query_max_age_ms Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool requiresFallbackForFreshness(const ArtifactManifest& manifest,
                                            int64_t now_unix_sec,
                                            int64_t query_max_age_ms) const;

  struct FallbackMetrics {
    uint64_t total_fallback_decisions = 0;
    uint64_t fallback_due_to_state = 0;
    uint64_t fallback_due_to_semantics = 0;
    uint64_t fallback_due_to_residual = 0;
    uint64_t fallback_due_to_freshness = 0;
    uint64_t artifacts_used_successfully = 0;
  };

  /**
   * @brief Get Metrics.
   * @return Return value.
   */
  virtual FallbackMetrics getMetrics() const;

  /**
   * @brief Record Fallback.
   * @param[in] reason Input parameter.
   */
  virtual void recordFallback(const std::string& reason);

 protected:
  FallbackMetrics metrics_;
};

}  // namespace distributed_tensor
}  // namespace themis
