// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rag/common_types.h"

namespace themis::rag {

// Forward declarations
class IngestionLatencyMonitor;

/// @brief Staleness-aware retrieval routing with freshness metadata.
///
/// Routes queries to retrievers based on index freshness, with fallback
/// to read-only replicas when primary is too stale. Attaches staleness
/// metadata to results for downstream quality adjustment.
///
/// @details
/// Routing decisions:
/// - If primary p95 staleness < target: route to primary only
/// - If primary p95 > target, < 2x target: route with freshness penalty
/// - If primary p95 > 2x target: fall back to replicas (if available)
/// - If all too stale: route to any available (mark as degraded)
///
/// Staleness metadata:
/// - max_staleness_ms: Latest staleness measurement
/// - is_degraded: true if fallback route used
/// - freshness_confidence: 0.0-1.0 quality adjustment factor
///
/// @code
/// auto router = std::make_unique<StalenessAwareRouter>(monitor);
/// auto decision = router->Route(query, available_shards);
/// // decision.use_shards: which shards to query
/// // decision.staleness_metadata: attach to response
/// @endcode
class StalenessAwareRouter {
 public:
  /// @brief Routing decision with staleness context.
  struct RoutingDecision {
    std::vector<std::string> primary_shards;    ///< Primary replicas to query
    std::vector<std::string> fallback_shards;   ///< Fallback replicas (if primary too stale)
    bool use_fallback;                          ///< true if primary exceeded threshold
    uint64_t max_staleness_ms;                  ///< Latest staleness observed
    float freshness_confidence;                 ///< Quality adjustment [0, 1]
    std::string rationale;                      ///< Why this decision was made
  };

  /// @brief Staleness metadata for response.
  struct StalenessMetadata {
    uint64_t max_staleness_ms;  ///< Latest staleness (ms)
    bool is_degraded;           ///< true if fallback route used
    std::string freshness_note; ///< Human-readable note
  };

  /// @brief Constructor with monitor.
  /// @param monitor Shared latency monitor instance.
  explicit StalenessAwareRouter(
      std::shared_ptr<IngestionLatencyMonitor> monitor);

  /// @brief Route query based on freshness.
  /// @param query_text User query (for logging).
  /// @param available_shards Available primary shard IDs.
  /// @return Routing decision with staleness context.
  RoutingDecision Route(
      const std::string& query_text,
      const std::vector<std::string>& available_shards);

  /// @brief Route with explicit replica list.
  /// @param query_text User query.
  /// @param primary_shards Primary replica IDs.
  /// @param fallback_shards Fallback replica IDs (if primary too stale).
  /// @return Routing decision.
  RoutingDecision RouteWithFallback(
      const std::string& query_text,
      const std::vector<std::string>& primary_shards,
      const std::vector<std::string>& fallback_shards);

  /// @brief Get staleness metadata for response.
  /// @return Metadata to include in response envelope.
  StalenessMetadata GetStalenessMetadata();

  /// @brief Check if primary is "healthy" (below target).
  /// @return true if all primaries within SLA.
  bool IsPrimaryHealthy();

  /// @brief Check if primary is "degraded" (1x-2x target).
  /// @return true if staleness significant but not critical.
  bool IsPrimaryDegraded();

  /// @brief Check if primary is "critical" (>2x target).
  /// @return true if primary should be avoided.
  bool IsPrimaryCritical();

  /// @brief Set staleness thresholds (default from monitor target).
  /// @param target_ms Target staleness (ms).
  /// @param degraded_threshold_ms Degraded threshold (ms).
  /// @param critical_threshold_ms Critical threshold (ms).
  void SetStalenessThresholds(
      uint64_t target_ms,
      uint64_t degraded_threshold_ms,
      uint64_t critical_threshold_ms);

 private:
  std::shared_ptr<IngestionLatencyMonitor> monitor_;
  uint64_t staleness_target_ms_;
  uint64_t staleness_degraded_ms_;  // 1.5x target
  uint64_t staleness_critical_ms_;  // 2x target
  uint64_t last_staleness_ms_;
};

}  // namespace themis::rag
