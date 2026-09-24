// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "query_intent_classifier.h"
#include "rocksdb/db.h"

namespace themis::rag {

/// @brief Persistent, versioned routing policy store backed by RocksDB.
///
/// Stores adaptive routing policies (weight allocations) per intent with
/// version management, history tracking, and rollback capability.
///
/// @details
/// Schema (RocksDB column families):
/// - default CF: active policies and metrics
///   - "active_policy_version" → uint32 (current version number)
///   - "policy:{intent}:{version}" → PolicySpec JSON
///   - "policy_metrics:{intent}:{version}" → MetricsSnapshot JSON
/// - policy_history CF: version history with big-endian timestamps
///   - be64(timestamp_us) → HistoryEntry JSON
///
/// All writes are atomic per intent (transaction isolation).
/// History is append-only and immutable.
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
  /// @param db_path Path to RocksDB directory.
  /// @throws std::runtime_error if DB open fails.
  explicit RouterPolicyStore(const std::string& db_path);

  /// @brief Destructor (closes RocksDB).
  ~RouterPolicyStore();

  /// @brief Get current active policy for intent.
  ///
  /// @param intent Intent category.
  /// @return Current PolicySpec if exists, std::nullopt if not.
  std::optional<PolicySpec> GetCurrentPolicy(
      QueryIntentClassifier::Intent intent);

  /// @brief Get specific policy version.
  ///
  /// @param intent Intent category.
  /// @param version Policy version number.
  /// @return PolicySpec or std::nullopt if version doesn't exist.
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

  /// @brief Record metrics for evaluation feedback loop.
  ///
  /// @param decision_id Unique routing decision ID (for correlation).
  /// @param intent Intent that was routed.
  /// @param ndcg_at_10 Observed nDCG@10.
  /// @param latency_ms Query latency in milliseconds.
  /// @param cost_usd Query cost in USD.
  /// @return true if recorded, false on write error.
  ///
  /// @details
  /// - Metrics stored in "metrics:{intent}:{version}:aggregated"
  /// - Aggregated hourly by (intent, active_policy_version)
  /// - Used by offline feedback loop to detect improvements
  bool RecordMetrics(
      const std::string& decision_id,
      QueryIntentClassifier::Intent intent,
      float ndcg_at_10,
      float latency_ms,
      float cost_usd);

  /// @brief Get aggregated metrics for policy.
  ///
  /// @param intent Intent category.
  /// @param since_time Window start (optional).
  /// @return MetricsSnapshot or std::nullopt if no metrics yet.
  std::optional<MetricsSnapshot> GetMetrics(
      QueryIntentClassifier::Intent intent,
      const std::optional<std::chrono::system_clock::time_point>& since_time = 
          std::nullopt);

  /// @brief Get full version history for intent.
  ///
  /// @param intent Intent category.
  /// @return Vector of HistoryEntry sorted by timestamp (oldest first).
  std::vector<HistoryEntry> GetHistory(
      QueryIntentClassifier::Intent intent);

  /// @brief Get all active policies.
  ///
  /// @return Map of intent → current PolicySpec.
  std::map<QueryIntentClassifier::Intent, PolicySpec> GetAllPolicies();

  /// @brief Health check: verify DB connectivity.
  ///
  /// @return true if DB is accessible.
  bool IsHealthy() const;

 private:
  rocksdb::DB* db_;
  std::string db_path_;

  // Key construction helpers
  static std::string MakePolicyKey(QueryIntentClassifier::Intent intent,
                                   uint32_t version);
  static std::string MakeMetricsKey(QueryIntentClassifier::Intent intent);
  static std::string MakeHistoryKey(int64_t timestamp_us);
};

}  // namespace themis::rag
