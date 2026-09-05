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
#include "src/distributed_tensor/include/tensor_delta_log.h"
#include <string>
#include <memory>
#include <map>
#include <cstdint>
#include <optional>
#include <vector>

namespace themis {
namespace distributed_tensor {

/// @brief Staleness detection and handling.
enum class StalenessLevel : uint8_t {
  /// Artifact is fresh and current.
  FRESH = 0,

  /// Artifact has small delta lag (acceptable).
  SLIGHTLY_STALE = 1,

  /// Artifact is significantly behind (warning level).
  MODERATELY_STALE = 2,

  /// Artifact is very far behind (critical).
  CRITICALLY_STALE = 3,
};

/// @brief Stale artifact metrics.
struct StaleArtifactMetrics {
  /// Artifact identifier.
  std::string artifact_id;

  /// Current staleness level.
  StalenessLevel staleness = StalenessLevel::FRESH;

  /// Delta lag in entries (sequence_end gap).
  uint64_t delta_lag = 0;

  /// Age of artifact in seconds since last update.
  int64_t age_seconds = 0;

  /// Delta backlog at detection time (entries not yet processed).
  uint64_t delta_backlog = 0;

  /// Worker throughput in deltas/second.
  double worker_throughput = 0.0;

  /// Delta arrival rate in deltas/second.
  double delta_arrival_rate = 0.0;

  /// Whether artifact should fallback to exact graph.
  bool should_fallback = false;

  /// Optional reason for staleness.
  std::string reason;
};

/// @brief Policy for stale artifact detection.
struct StalenessPolicy {
  /// Delta lag threshold for SLIGHTLY_STALE (entries).
  uint64_t lag_threshold_slightly = 100;

  /// Delta lag threshold for MODERATELY_STALE (entries).
  uint64_t lag_threshold_moderate = 1000;

  /// Delta lag threshold for CRITICALLY_STALE (entries).
  uint64_t lag_threshold_critical = 10000;

  /// Age threshold for SLIGHTLY_STALE (seconds).
  int64_t age_threshold_slightly_sec = 60;

  /// Age threshold for MODERATELY_STALE (seconds).
  int64_t age_threshold_moderate_sec = 300;

  /// Age threshold for CRITICALLY_STALE (seconds).
  int64_t age_threshold_critical_sec = 3600;

  /// Whether to trigger cascade invalidation on critical staleness.
  bool cascade_invalidate_on_critical = true;

  /// Fallback to exact graph when staleness reaches this level.
  StalenessLevel fallback_threshold = StalenessLevel::MODERATELY_STALE;
};

/// @brief Stale artifact detector.
///
/// Monitors artifact freshness and detects when artifacts fall behind
/// in delta processing. Provides escalation and fallback mechanisms.
///
class StaleArtifactDetector {
 public:
  /// Creates a detector with default policy.
  StaleArtifactDetector();

  virtual ~StaleArtifactDetector() = default;

  // Prevent copy/move
  StaleArtifactDetector(const StaleArtifactDetector&) = delete;
  StaleArtifactDetector& operator=(const StaleArtifactDetector&) = delete;
  StaleArtifactDetector(StaleArtifactDetector&&) = delete;
  StaleArtifactDetector& operator=(const StaleArtifactDetector&&) = delete;

  /// Analyzes an artifact for staleness.
  /// @param artifact_id Artifact to analyze
  /// @param manifest Current artifact manifest
  /// @param current_source_seq Current sequence number in exact graph
  /// @param worker_throughput Current worker throughput in deltas/sec
  /// @param delta_arrival_rate Current delta arrival rate in deltas/sec
  /// @return Staleness metrics
  virtual StaleArtifactMetrics analyzeArtifactStaleness(
      const std::string& artifact_id,
      const ArtifactManifest& manifest,
      uint64_t current_source_seq,
      double worker_throughput = 0.0,
      double delta_arrival_rate = 0.0);

  /// Determines if an artifact should fallback to exact graph.
  /// @param metrics Staleness metrics
  /// @return true if fallback is recommended, false otherwise
  virtual bool shouldFallback(const StaleArtifactMetrics& metrics);

  /// Determines if an artifact should be invalidated due to staleness.
  /// @param metrics Staleness metrics
  /// @return true if invalidation is recommended, false otherwise
  virtual bool shouldInvalidate(const StaleArtifactMetrics& metrics);

  /// Gets the current staleness policy.
  StalenessPolicy getPolicy() const;

  /// Sets the staleness detection policy.
  /// @param policy New policy to apply
  void setPolicy(const StalenessPolicy& policy);

  /// Tracks staleness history for an artifact.
  struct StalenessHistory {
    /// Artifact identifier.
    std::string artifact_id;

    /// Most recent staleness level.
    StalenessLevel most_recent = StalenessLevel::FRESH;

    /// Number of times artifact became moderately stale.
    uint64_t moderate_staleness_count = 0;

    /// Number of times artifact became critically stale.
    uint64_t critical_staleness_count = 0;

    /// Timestamp of last staleness change.
    int64_t last_change_unix_sec = 0;
  };

  /// Tracks staleness history for an artifact.
  /// @param artifact_id Artifact identifier
  /// @param metrics New staleness metrics
  virtual void updateStalenessHistory(const std::string& artifact_id,
                                      const StaleArtifactMetrics& metrics);

  /// Gets staleness history for an artifact.
  /// @param artifact_id Artifact identifier
  /// @return History if available, empty optional otherwise
  virtual std::optional<StalenessHistory> getStalenessHistory(const std::string& artifact_id);

  /// Gets overall detector statistics.
  struct Stats {
    /// Number of artifacts currently detected as stale.
    uint64_t artifacts_currently_stale = 0;

    /// Total number of staleness detections (cumulative).
    uint64_t total_staleness_detections = 0;

    /// Number of cascade invalidations triggered.
    uint64_t cascade_invalidations_triggered = 0;

    /// Average staleness duration in seconds.
    double average_staleness_duration_sec = 0.0;
  };

  /// Gets detector statistics.
  virtual Stats getStats() const;

 protected:
  StalenessPolicy policy_;
  std::map<std::string, StalenessHistory> history_;
  Stats stats_;

  /// Internal helper to classify staleness level based on metrics.
  StalenessLevel classifyStaleness(const StaleArtifactMetrics& metrics);

  /// Internal helper to get current time in seconds since epoch.
  int64_t getCurrentTimeUnixSec();
};

}  // namespace distributed_tensor
}  // namespace themis
