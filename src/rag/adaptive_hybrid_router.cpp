// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "adaptive_hybrid_router.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis::rag {

AdaptiveHybridRouter::AdaptiveHybridRouter(
    std::shared_ptr<RouterPolicyStore> policy_store)
    : policy_store_(policy_store) {}

AdaptiveHybridRouter::RoutingDecision AdaptiveHybridRouter::Route(
    const QueryContext& context) {
  RoutingDecision decision;
  decision.decision_id = next_decision_id_++;

  // Look up current policy for intent from RouterPolicyStore
  auto policy = policy_store_->GetCurrentPolicy(context.intent.intent);
  
  if (policy) {
    decision.lexical_weight = policy->lexical_weight;
    decision.dense_weight = policy->dense_weight;
    decision.graph_weight = policy->graph_weight;
    decision.policy_version = std::to_string(policy->version);
  } else {
    // Fallback to static defaults
    auto fallback = GetFallbackWeights();
    decision.lexical_weight = fallback.lexical_weight;
    decision.dense_weight = fallback.dense_weight;
    decision.graph_weight = fallback.graph_weight;
    decision.policy_version = "fallback";
  }

  // Validate weights
  if (!ValidateWeights(decision)) {
    // If validation fails, use fallback
    auto fallback = GetFallbackWeights();
    decision.lexical_weight = fallback.lexical_weight;
    decision.dense_weight = fallback.dense_weight;
    decision.graph_weight = fallback.graph_weight;
    decision.policy_version = "fallback";
  }

  // Emit telemetry span
  EmitRoutingSpan(context, decision);

  return decision;
}

std::vector<Document> AdaptiveHybridRouter::RetrieveAdaptive(
    const QueryContext& context,
    const RoutingDecision& decision) {
  // Call BM25, HNSW, and graph retrievers
  // (Implementation would integrate with actual retrieval backends)
  
  // Placeholder: return empty for now (actual integration would happen)
  std::vector<Document> hybrid_results;
  
  // TODO: Call actual retrieval backends
  // std::vector<Document> bm25_results = lexical_retriever_.Retrieve(...);
  // std::vector<Document> hnsw_results = dense_retriever_.Retrieve(...);
  // std::vector<Document> graph_results = graph_retriever_.Retrieve(...);
  
  // Fuse via Reciprocal Rank Fusion (RRF)
  // std::map<std::string, float> rrf_scores;
  // for (size_t i = 0; i < bm25_results.size(); ++i) {
  //   rrf_scores[bm25_results[i].id] += decision.lexical_weight / (i + 1);
  // }
  // ... same for dense and graph results
  
  // Sort by RRF score and return top-10
  
  return hybrid_results;  // Top-10 results
}

std::vector<Document> AdaptiveHybridRouter::RouteAndRetrieve(
    const QueryContext& context) {
  auto decision = Route(context);
  return RetrieveAdaptive(context, decision);
}

AdaptiveHybridRouter::RoutingDecision
AdaptiveHybridRouter::GetFallbackWeights() {
  return {
      0.3f,  // lexical_weight
      0.5f,  // dense_weight
      0.2f,  // graph_weight
      "static_fallback",
      0
  };
}

bool AdaptiveHybridRouter::ValidateWeights(const RoutingDecision& decision,
                                           float tolerance) {
  float sum = decision.lexical_weight + decision.dense_weight +
              decision.graph_weight;
  return std::abs(sum - 1.0f) <= tolerance && 
         decision.lexical_weight >= 0.0f && 
         decision.dense_weight >= 0.0f && 
         decision.graph_weight >= 0.0f;
}

void AdaptiveHybridRouter::EmitRoutingSpan(const QueryContext& context,
                                          const RoutingDecision& decision) {
  // Emit OpenTelemetry span
  // Attributes:
  //   - rag.request_id
  //   - rag.intent (factual|temporal|multi_hop|comparison)
  //   - rag.lexical_weight
  //   - rag.dense_weight
  //   - rag.graph_weight
  //   - rag.policy_version
  //   - rag.decision_id
  
  // TODO: Integrate with OpenTelemetry SDK
  // otel::trace::Tracer tracer = ...;
  // auto span = tracer->StartSpan("rag.routing_decision");
  // span->SetAttribute("rag.request_id", context.query_id);
  // ... set other attributes
}

}  // namespace themis::rag
