// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rag/common_types.h"
#include "rag/query_intent_classifier.h"
#include "rag/router_policy_store.h"

namespace themis::rag {

/// @brief Adaptive hybrid retrieval routing based on learned per-intent weights.
///
/// Routes queries to lexical (BM25), dense (HNSW), and graph retrievers with
/// adaptive weights learned per query intent. Weights are stored in RouterPolicyStore
/// and updated via offline feedback loop.
///
/// @details
/// - Request-time: Classify intent → lookup weights → execute hybrid retrieval
/// - Weights must sum to 1.0 ± 0.01 for valid routing decisions
/// - Fallback (on policy miss): Static defaults (lexical=0.3, dense=0.5, graph=0.2)
/// - Telemetry: Emits OpenTelemetry span with routing decision details
///
/// @note Latency target: <5ms p99 (policy lookup + weight computation)
class AdaptiveHybridRouter {
 public:
  /// @brief Routing decision with weight allocation.
  struct RoutingDecision {
    float lexical_weight;           ///< BM25 weight [0, 1]
    float dense_weight;             ///< HNSW weight [0, 1]
    float graph_weight;             ///< Graph weight [0, 1]
    std::string policy_version;     ///< Version of policy applied
    uint64_t decision_id;           ///< Unique ID for telemetry correlation
  };

  /// @brief Query-time context for routing.
  struct QueryContext {
    std::string query_text;
    QueryIntentClassifier::ClassificationResult intent;
    std::string tenant_id;
    std::string query_id;           ///< For logging/debugging
  };

  /// @brief Constructor.
  /// 
  /// @param policy_store Shared PolicyStore for weight lookups.
  explicit AdaptiveHybridRouter(
      std::shared_ptr<RouterPolicyStore> policy_store);

  /// @brief Destructor.
  ~AdaptiveHybridRouter() = default;

  /// @brief Compute routing decision for query.
  ///
  /// @param context Query context with intent classification result.
  /// @return RoutingDecision with weight allocation and policy version.
  ///
  /// @details
  /// - Looks up current policy for intent from RouterPolicyStore
  /// - Validates weights sum to 1.0 ± 0.01
  /// - Emits OpenTelemetry span: "rag.routing_decision"
  /// - On policy miss: uses static fallback defaults
  RoutingDecision Route(const QueryContext& context);

  /// @brief Execute hybrid retrieval with adaptive weights.
  ///
  /// @param context Query context.
  /// @param decision Routing decision from Route().
  /// @return Top-10 fused results using RRF with allocated weights.
  ///
  /// @details
  /// - Calls BM25 retriever (k=50) with lexical_weight
  /// - Calls HNSW retriever (k=50) with dense_weight
  /// - Calls graph retriever (k=50) with graph_weight
  /// - Fuses via Reciprocal Rank Fusion (RRF): score = Σ (weight / rank)
  /// - Returns top-10 by fused score
  /// - Emits telemetry span: "rag.hybrid_retrieval"
  std::vector<Document> RetrieveAdaptive(
      const QueryContext& context,
      const RoutingDecision& decision);

  /// @brief Combined: Route + Retrieve in one call.
  ///
  /// @param context Query context.
  /// @return Top-10 results using adaptive routing decision.
  ///
  /// @details
  /// Convenience method: calls Route() then RetrieveAdaptive().
  std::vector<Document> RouteAndRetrieve(const QueryContext& context);

  /// @brief Get current fallback weights (used on policy miss).
  /// 
  /// @return Static default weights.
  static RoutingDecision GetFallbackWeights();

  /// @brief Validate weights sum to 1.0 ± tolerance.
  /// 
  /// @param decision Routing decision to validate.
  /// @param tolerance Maximum absolute deviation from 1.0 (default 0.01).
  /// @return true if weights are valid, false otherwise.
  static bool ValidateWeights(const RoutingDecision& decision,
                              float tolerance = 0.01f);

 private:
  std::shared_ptr<RouterPolicyStore> policy_store_;
  uint64_t next_decision_id_ = 0;

  // OpenTelemetry span emission
  void EmitRoutingSpan(const QueryContext& context,
                      const RoutingDecision& decision);
};

}  // namespace themis::rag
