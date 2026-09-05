/// @file stale_artifact_detector.cc
/// @brief Implementation of stale artifact detector
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/stale_artifact_detector.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace themis {
namespace distributed_tensor {

StaleArtifactDetector::StaleArtifactDetector() {
  // Initialize with default policy
  policy_.lag_threshold_slightly = 100;
  policy_.lag_threshold_moderate = 1000;
  policy_.lag_threshold_critical = 10000;
  policy_.age_threshold_slightly_sec = 60;
  policy_.age_threshold_moderate_sec = 300;
  policy_.age_threshold_critical_sec = 3600;
  policy_.cascade_invalidate_on_critical = true;
  policy_.fallback_threshold = StalenessLevel::MODERATELY_STALE;
}

int64_t StaleArtifactDetector::getCurrentTimeUnixSec() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

StaleArtifactMetrics StaleArtifactDetector::analyzeArtifactStaleness(
    const std::string& artifact_id,
    const ArtifactManifest& manifest,
    uint64_t current_source_seq,
    double worker_throughput,
    double delta_arrival_rate) {
  StaleArtifactMetrics metrics;
  metrics.artifact_id = artifact_id;

  // Calculate delta lag
  if (current_source_seq >= manifest.source_seq_end) {
    metrics.delta_lag = current_source_seq - manifest.source_seq_end;
  } else {
    metrics.delta_lag = 0;  // Artifact is caught up
  }

  // Calculate artifact age
  int64_t now = getCurrentTimeUnixSec();
  metrics.age_seconds = now - manifest.last_rebuild_at_unix_sec;
  if (metrics.age_seconds < 0) {
    metrics.age_seconds = 0;
  }

  // Estimate delta backlog (rough estimate based on lag)
  metrics.delta_backlog = metrics.delta_lag;

  // Store throughput information
  metrics.worker_throughput = worker_throughput;
  metrics.delta_arrival_rate = delta_arrival_rate;

  // Classify staleness
  metrics.staleness = classifyStaleness(metrics);

  // Determine if fallback is needed
  metrics.should_fallback = shouldFallback(metrics);

  // Generate reason for staleness
  if (metrics.staleness == StalenessLevel::FRESH) {
    metrics.reason = "artifact is fresh";
  } else if (metrics.delta_lag > 0) {
    metrics.reason = "delta lag: " + std::to_string(metrics.delta_lag);
  } else if (metrics.age_seconds > 0) {
    metrics.reason = "age: " + std::to_string(metrics.age_seconds) + "s";
  }

  return metrics;
}

StalenessLevel StaleArtifactDetector::classifyStaleness(const StaleArtifactMetrics& metrics) {
  // Check based on age first (higher priority)
  if (metrics.age_seconds >= policy_.age_threshold_critical_sec) {
    return StalenessLevel::CRITICALLY_STALE;
  }
  if (metrics.age_seconds >= policy_.age_threshold_moderate_sec) {
    return StalenessLevel::MODERATELY_STALE;
  }
  if (metrics.age_seconds >= policy_.age_threshold_slightly_sec) {
    return StalenessLevel::SLIGHTLY_STALE;
  }

  // Check based on delta lag
  if (metrics.delta_lag >= policy_.lag_threshold_critical) {
    return StalenessLevel::CRITICALLY_STALE;
  }
  if (metrics.delta_lag >= policy_.lag_threshold_moderate) {
    return StalenessLevel::MODERATELY_STALE;
  }
  if (metrics.delta_lag >= policy_.lag_threshold_slightly) {
    return StalenessLevel::SLIGHTLY_STALE;
  }

  // Check if worker is falling behind (delta arrival > worker throughput)
  if (metrics.delta_arrival_rate > 0.0 && metrics.worker_throughput > 0.0) {
    double ratio = metrics.delta_arrival_rate / metrics.worker_throughput;
    if (ratio > 2.0) {  // Arriving 2x faster than being processed
      return StalenessLevel::MODERATELY_STALE;
    }
    if (ratio > 1.5) {  // Arriving 1.5x faster
      return StalenessLevel::SLIGHTLY_STALE;
    }
  }

  return StalenessLevel::FRESH;
}

bool StaleArtifactDetector::shouldFallback(const StaleArtifactMetrics& metrics) {
  if (metrics.staleness >= policy_.fallback_threshold) {
    return true;
  }

  // Also consider if worker is significantly behind
  if (metrics.delta_arrival_rate > 0.0 && metrics.worker_throughput > 0.0) {
    double ratio = metrics.delta_arrival_rate / metrics.worker_throughput;
    if (ratio > 2.5) {  // Arriving significantly faster
      return true;
    }
  }

  return false;
}

bool StaleArtifactDetector::shouldInvalidate(const StaleArtifactMetrics& metrics) {
  // Only invalidate on critical staleness if policy enables it
  if (policy_.cascade_invalidate_on_critical && metrics.staleness == StalenessLevel::CRITICALLY_STALE) {
    return true;
  }

  return false;
}

StalenessPolicy StaleArtifactDetector::getPolicy() const {
  return policy_;
}

void StaleArtifactDetector::setPolicy(const StalenessPolicy& policy) {
  policy_ = policy;
}

void StaleArtifactDetector::updateStalenessHistory(const std::string& artifact_id,
                                                   const StaleArtifactMetrics& metrics) {
  auto it = history_.find(artifact_id);

  if (it == history_.end()) {
    // Create new history entry
    StalenessHistory history;
    history.artifact_id = artifact_id;
    history.most_recent = metrics.staleness;
    history.last_change_unix_sec = getCurrentTimeUnixSec();

    if (metrics.staleness == StalenessLevel::MODERATELY_STALE) {
      history.moderate_staleness_count = 1;
    } else if (metrics.staleness == StalenessLevel::CRITICALLY_STALE) {
      history.critical_staleness_count = 1;
    }

    history_[artifact_id] = history;
  } else {
    // Update existing history
    StalenessHistory& history = it->second;

    if (metrics.staleness != history.most_recent) {
      history.most_recent = metrics.staleness;
      history.last_change_unix_sec = getCurrentTimeUnixSec();

      if (metrics.staleness == StalenessLevel::MODERATELY_STALE) {
        history.moderate_staleness_count++;
      } else if (metrics.staleness == StalenessLevel::CRITICALLY_STALE) {
        history.critical_staleness_count++;
      }
    }
  }

  // Update overall stats
  if (metrics.staleness != StalenessLevel::FRESH) {
    if (stats_.artifacts_currently_stale == 0 ||
        metrics.staleness == StalenessLevel::CRITICALLY_STALE) {
      // Update if this is first staleness or it's critical
    }
    stats_.artifacts_currently_stale++;
  } else {
    if (stats_.artifacts_currently_stale > 0) {
      stats_.artifacts_currently_stale--;
    }
  }
  stats_.total_staleness_detections++;
}

std::optional<StaleArtifactDetector::StalenessHistory> StaleArtifactDetector::getStalenessHistory(
    const std::string& artifact_id) {
  auto it = history_.find(artifact_id);

  if (it == history_.end()) {
    return std::nullopt;
  }

  return it->second;
}

StaleArtifactDetector::Stats StaleArtifactDetector::getStats() const {
  return stats_;
}

}  // namespace distributed_tensor
}  // namespace themis
