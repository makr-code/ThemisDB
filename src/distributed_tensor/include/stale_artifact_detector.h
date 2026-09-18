/// @file stale_artifact_detector.h
/// @brief Stale artifact detection and monitoring
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines stale artifact detection for monitoring when
/// artifacts fall behind in delta processing and need special handling.
///
/// ## Design
///
/// Stale detection monitors:
/// - Delta lag (sequence_end gap between exact graph and artifact)
/// - Age of artifact (last update timestamp)
/// - Delta backlog growth rate
/// - Worker throughput vs delta arrival rate
///
/// Actions on staleness:
/// - Mark artifact STALE in manifest
/// - Alert planner to prefer exact graph fallback
/// - Escalate to worker for priority processing
/// - Trigger cascade invalidation if policy violated

#pragma once

#include "artifact_manifest.h"
#include "tensor_delta_log.h"
#include <string>
#include <memory>
#include <map>
#include <cstdint>
#include <optional>
#include <vector>

namespace themis {
namespace distributed_tensor {

enum class StalenessLevel : uint8_t {
  FRESH = 0,

  SLIGHTLY_STALE = 1,

  MODERATELY_STALE = 2,

  CRITICALLY_STALE = 3,
};

struct StaleArtifactMetrics {
  std::string artifact_id;

  StalenessLevel staleness = StalenessLevel::FRESH;

  uint64_t delta_lag = 0;

  int64_t age_seconds = 0;

  uint64_t delta_backlog = 0;

  double worker_throughput = 0.0;

  double delta_arrival_rate = 0.0;

  bool should_fallback = false;

  std::string reason;
};

struct StalenessPolicy {
  uint64_t lag_threshold_slightly = 100;

  uint64_t lag_threshold_moderate = 1000;

  uint64_t lag_threshold_critical = 10000;

  int64_t age_threshold_slightly_sec = 60;

  int64_t age_threshold_moderate_sec = 300;

  int64_t age_threshold_critical_sec = 3600;

  bool cascade_invalidate_on_critical = true;

  StalenessLevel fallback_threshold = StalenessLevel::MODERATELY_STALE;
};

class StaleArtifactDetector {
 public:
  StaleArtifactDetector();

  /**
   * @brief Stale Artifact Detector.
   * @return Return value.
   */
  virtual ~StaleArtifactDetector() = default;

  // Prevent copy/move
  StaleArtifactDetector(const StaleArtifactDetector&) = delete;
  StaleArtifactDetector& operator=(const StaleArtifactDetector&) = delete;
  StaleArtifactDetector(StaleArtifactDetector&&) = delete;
  StaleArtifactDetector& operator=(const StaleArtifactDetector&&) = delete;

  virtual StaleArtifactMetrics analyzeArtifactStaleness(
      const std::string& artifact_id,
      const ArtifactManifest& manifest,
      uint64_t current_source_seq,
      double worker_throughput = 0.0,
      double delta_arrival_rate = 0.0);

  /**
   * @brief Should Fallback.
   * @param[in] metrics Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldFallback(const StaleArtifactMetrics& metrics);

  /**
   * @brief Should Invalidate.
   * @param[in] metrics Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldInvalidate(const StaleArtifactMetrics& metrics);

  /**
   * @brief Look up a retention policy by name.
   * @return Pointer to the stored policy on success, or an error if it is missing.
   */
  StalenessPolicy getPolicy() const;

  /**
   * @brief Set Policy.
   * @param[in] policy Input parameter.
   */
  void setPolicy(const StalenessPolicy& policy);

  struct StalenessHistory {
    std::string artifact_id;

    StalenessLevel most_recent = StalenessLevel::FRESH;

    uint64_t moderate_staleness_count = 0;

    uint64_t critical_staleness_count = 0;

    int64_t last_change_unix_sec = 0;
  };

  /**
   * @brief Update Staleness History.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] metrics Input parameter.
   */
  virtual void updateStalenessHistory(const std::string& artifact_id,
                                      const StaleArtifactMetrics& metrics);

  /**
   * @brief Get Staleness History.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  virtual std::optional<StalenessHistory> getStalenessHistory(const std::string& artifact_id);

  struct Stats {
    uint64_t artifacts_currently_stale = 0;

    uint64_t total_staleness_detections = 0;

    uint64_t cascade_invalidations_triggered = 0;

    double average_staleness_duration_sec = 0.0;
  };

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  virtual Stats getStats() const;

 protected:
  StalenessPolicy policy_;
  std::map<std::string, StalenessHistory> history_;
  Stats stats_;

  /**
   * @brief Classify Staleness.
   * @param[in] metrics Input parameter.
   * @return Return value.
   */
  StalenessLevel classifyStaleness(const StaleArtifactMetrics& metrics);

  /**
   * @brief Get Current Time Unix Sec.
   * @return Return value.
   */
  int64_t getCurrentTimeUnixSec();
};

}  // namespace distributed_tensor
}  // namespace themis
