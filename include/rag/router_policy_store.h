// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rag/query_intent_classifier.h"

namespace themis::rag {

/// @brief Persistent, versioned routing policy store.
///
/// Stores adaptive routing policies (weight allocations) per intent with
/// version management, durable history tracking, rollback capability, and
/// query-level metrics aggregation. The store persists policy/version metadata
/// and aggregate health telemetry in RocksDB so routing decisions remain
/// recoverable across process restarts.
class RouterPolicyStore {
 public:
  /// @brief Routing policy specification.
  struct PolicySpec {
    uint32_t version;                    ///< Policy version number
    QueryIntentClassifier::Intent intent;///< Intent this policy applies to
    float lexical_weight;                ///< BM25 weight [0, 1]
    float dense_weight;                  ///< HNSW weight [0, 1]
    float graph_weight;                  ///< Graph weight [0, 1]
    std::string created_at;              ///< ISO 8601 timestamp
    float baseline_ndcg;                 ///< Baseline nDCG@10 for comparison
    float confidence;                    ///< Policy confidence [0, 1]
    std::string description;             ///< Human-readable notes
  };

  /// @brief Metrics snapshot for a policy.
  struct MetricsSnapshot {
    float mean_ndcg_at_10;              ///< Average nDCG@10 achieved
    float mean_latency_ms;              ///< Average query latency (ms)
    float mean_cost_usd;                ///< Average cost per query
    uint64_t sample_count;              ///< Number of queries evaluated
    std::string window_start;           ///< Start of evaluation window
    std::string window_end;             ///< End of evaluation window
  };

  /// @brief History entry for policy changes.
  struct HistoryEntry {
    uint32_t version;
    QueryIntentClassifier::Intent intent;
    int64_t timestamp_us;               ///< When change occurred
    std::string decision;               ///< "promoted" | "demoted" | "created"
    float metrics_delta;                ///< nDCG change vs previous
    std::string reason;                 ///< Why change was made
  };

  /// @brief Constructor.
  ///
  /// @param db_path Path to RocksDB directory. If empty, the store remains
  ///        non-persistent and only keeps in-memory state for the current
  ///        process lifetime.
  /// @throws std::runtime_error if a non-empty database path cannot be opened.
  explicit RouterPolicyStore(const std::string& db_path);

  /// @brief Destructor flushes any buffered state before closing RocksDB.
  ~RouterPolicyStore();

  /// @brief Get the currently active policy for an intent.
  ///
  /// @param intent Intent category.
  /// @return Current PolicySpec if one is active; otherwise the default policy for
  ///         the intent is returned when no persisted policy exists.
  std::optional<PolicySpec> GetCurrentPolicy(
      QueryIntentClassifier::Intent intent);

  /// @brief Get a specific policy version.
  ///
  /// @param intent Intent category.
  /// @param version Policy version number to recover.
  /// @return PolicySpec or std::nullopt if the version is not present.
  std::optional<PolicySpec> GetPolicyByVersion(
      QueryIntentClassifier::Intent intent,
      uint32_t version);

  /// @brief Create or update policy for intent (new version).
  ///
  /// @param new_policy Policy specification to persist.
  /// @return true if successful, false on write error.
  ///
  /// @details
  /// - Increments version number automatically
  /// - Previous policy remains accessible via GetPolicyByVersion()
  /// - Updates "active_policy_version" atomic record
  /// - Appends to policy_history for audit trail
  bool UpdatePolicy(const PolicySpec& new_policy);

  /// @brief Rollback policy to previous version.
  ///
  /// @param intent Intent to rollback.
  /// @param target_version Version to restore to (must be < current).
  /// @return true if successful, false on error or version not found.
  ///
  /// @details
  /// - Reads target_version from policy_history
  /// - Sets as new "active_policy_version"
  /// - Appends rollback entry to history
  bool RollbackPolicy(QueryIntentClassifier::Intent intent,
                     uint32_t target_version);

  /// @brief Record a query-level routing metric for a decision.
  ///
  /// @param decision_id Unique routing decision ID used to correlate the record.
  /// @param intent Intent that was routed.
  /// @param ndcg_at_10 Observed nDCG@10 for the decision.
  /// @param latency_ms Query latency in milliseconds.
  /// @param cost_usd Query cost in USD.
  /// @return true when the point-in-time metric and aggregate update were both
  ///         durably written; false on write failure or invalid inputs.
  /// @note A persistent decision record is written under the intent-specific
  ///       metrics namespace, and the active policy aggregate is updated in place.
  bool RecordMetrics(
      const std::string& decision_id,
      QueryIntentClassifier::Intent intent,
      float ndcg_at_10,
      float latency_ms,
      float cost_usd);

  /// @brief Get aggregated metrics for a policy and intent.
  ///
  /// @param intent Intent category.
  /// @param since_time Optional minimum time cutoff used to restrict the recent
  ///        history window.
  /// @return MetricsSnapshot or std::nullopt if no metrics are present for the
  ///         requested window.
  std::optional<MetricsSnapshot> GetMetrics(
      QueryIntentClassifier::Intent intent,
      const std::optional<std::chrono::system_clock::time_point>& since_time = 
          std::nullopt);

  /// @brief Get full version history for an intent.
  ///
  /// @param intent Intent category.
  /// @return Vector of HistoryEntry sorted by timestamp (oldest first). Each
  ///         entry includes the decision label, metrics delta, and rollback reason
  ///         persisted in RocksDB.
  std::vector<HistoryEntry> GetHistory(
      QueryIntentClassifier::Intent intent);

  /// @brief Get all active policies.
  ///
  /// @return Map of intent → current PolicySpec.
  std::map<QueryIntentClassifier::Intent, PolicySpec> GetAllPolicies();

 /// @brief Restore persisted state from RocksDB.
  ///
 /// @return true when the store could reopen successfully or when no persisted
 ///         state exists. The state is rehydrated from the on-disk keys used to
 ///         store active policy versions, policy history, and metrics aggregates.
 bool Load();

 /// @brief Health check: verify DB connectivity.
 ///
 /// @return true if DB is accessible.
 bool IsHealthy() const;

 private:
  void* db_;  // Opaque pointer to RocksDB handle (to avoid rocksdb dependency in header)
  std::string db_path_;

  // In-memory storage for initial implementation
  std::map<QueryIntentClassifier::Intent, std::vector<PolicySpec>> policy_history_;
  std::map<QueryIntentClassifier::Intent, PolicySpec> current_policies_;

  // Key construction helpers
  static std::string MakePolicyKey(QueryIntentClassifier::Intent intent,
                                   uint32_t version);
  static std::string MakeMetricsKey(QueryIntentClassifier::Intent intent);
  static std::string MakeHistoryKey(int64_t timestamp_us);
};

}  // namespace themis::rag
