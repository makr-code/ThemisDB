// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/adaptive_hybrid_router.h"

#include "rag/hybrid_retriever.h"
#include "rag/otel_span_emitter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

namespace themis::rag {

namespace {

std::string IntentToString(QueryIntentClassifier::Intent intent) {
  switch (intent) {
    case QueryIntentClassifier::Intent::Factual: return "factual";
    case QueryIntentClassifier::Intent::Temporal: return "temporal";
    case QueryIntentClassifier::Intent::MultiHop: return "multi_hop";
    case QueryIntentClassifier::Intent::Comparison: return "comparison";
    default: return "unknown";
  }
}

std::vector<Document> ApplyRetrievalBackendFallback(
    const QueryContext& context,
    const RoutingDecision& decision,
    const std::vector<Document>& lexical_docs,
    const std::vector<Document>& dense_docs,
    const std::vector<Document>& graph_docs) {
  std::map<std::string, float> fused_scores;
  for (size_t i = 0; i < lexical_docs.size(); ++i) {
    const auto& doc = lexical_docs[i];
    fused_scores[std::to_string(doc.id)] += decision.lexical_weight / (i + 1.0f);
  }
  for (size_t i = 0; i < dense_docs.size(); ++i) {
    const auto& doc = dense_docs[i];
    fused_scores[std::to_string(doc.id)] += decision.dense_weight / (i + 1.0f);
  }
  for (size_t i = 0; i < graph_docs.size(); ++i) {
    const auto& doc = graph_docs[i];
    fused_scores[std::to_string(doc.id)] += decision.graph_weight / (i + 1.0f);
  }

  std::vector<Document> results;
  for (const auto& [id_str, score] : fused_scores) {
    auto doc_id = static_cast<uint32_t>(std::stoul(id_str));
    Document doc{doc_id, context.query_text, score};
    results.push_back(doc);
  }
  std::sort(results.begin(), results.end(), [](const Document& a, const Document& b) {
    return a.retrieval_score > b.retrieval_score;
  });
  if (results.size() > 10) {
    results.erase(results.begin() + 10, results.end());
  }
  return results;
}

}  // namespace

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
  std::vector<Document> lexical_docs;
  std::vector<Document> dense_docs;
  std::vector<Document> graph_docs;

  const std::string normalized_query = context.query_text;
  const size_t candidate_count = 8;
  for (size_t i = 0; i < candidate_count; ++i) {
    const std::string suffix = "doc_" + std::to_string(i + 1);
    const float lexical_score = std::max(0.0f, 1.0f - static_cast<float>(i) * 0.09f);
    lexical_docs.push_back({static_cast<uint32_t>(i + 1), normalized_query + " " + suffix, lexical_score});
    const float dense_score = std::max(0.0f, 0.92f - static_cast<float>(i) * 0.08f);
    dense_docs.push_back({static_cast<uint32_t>(100 + i + 1), normalized_query + " semantic " + suffix, dense_score});
    const float graph_score = std::max(0.0f, 0.88f - static_cast<float>(i) * 0.07f);
    graph_docs.push_back({static_cast<uint32_t>(200 + i + 1), normalized_query + " graph " + suffix, graph_score});
  }

  std::vector<judge::RetrievedDocument> bm25_candidates;
  bm25_candidates.reserve(lexical_docs.size());
  for (const auto& doc : lexical_docs) {
    judge::RetrievedDocument item;
    item.id = std::to_string(doc.id);
    item.content = doc.content;
    item.similarity_score = doc.retrieval_score;
    bm25_candidates.push_back(item);
  }

  std::vector<judge::RetrievedDocument> vector_candidates;
  vector_candidates.reserve(dense_docs.size());
  for (const auto& doc : dense_docs) {
    judge::RetrievedDocument item;
    item.id = std::to_string(doc.id);
    item.content = doc.content;
    item.similarity_score = doc.retrieval_score;
    vector_candidates.push_back(item);
  }

  HybridRetriever retriever(HybridRetrieverConfig{ decision.lexical_weight,
                                                 decision.dense_weight,
                                                 true,
                                                 60.0,
                                                 10,
                                                 true });
  auto fused = retriever.fuse(bm25_candidates, vector_candidates);

  std::vector<Document> hybrid_results;
  hybrid_results.reserve(std::min<size_t>(fused.documents.size(), 10));
  for (const auto& doc : fused.documents) {
    try {
      const uint32_t doc_id = static_cast<uint32_t>(std::stoul(doc.id));
      hybrid_results.push_back({doc_id, doc.content, static_cast<float>(doc.similarity_score)});
    } catch (...) {
      hybrid_results.push_back({static_cast<uint32_t>(hybrid_results.size() + 1), doc.content, static_cast<float>(doc.similarity_score)});
    }
    if (hybrid_results.size() >= 10) {
      break;
    }
  }

  if (hybrid_results.empty()) {
    return ApplyRetrievalBackendFallback(
        context, decision, lexical_docs, dense_docs, graph_docs);
  }

  return hybrid_results;
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
  auto emitter = std::make_shared<OTELSpanEmitter>("RAG");
  auto span = emitter->StartSpan("rag.routing_decision");
  span->SetAttribute("rag.request_id", context.query_id);
  span->SetAttribute("rag.intent", IntentToString(context.intent.intent));
  span->SetAttribute("rag.lexical_weight", static_cast<uint64_t>(decision.lexical_weight * 1000000.0f));
  span->SetAttribute("rag.dense_weight", static_cast<uint64_t>(decision.dense_weight * 1000000.0f));
  span->SetAttribute("rag.graph_weight", static_cast<uint64_t>(decision.graph_weight * 1000000.0f));
  span->SetAttribute("rag.policy_version", decision.policy_version);
  span->SetAttribute("rag.decision_id", decision.decision_id);
  span->SetAttribute("rag.tenant_id", context.tenant_id);
  span->SetAttribute("rag.fallback", decision.policy_version == "fallback" ||
                                      decision.policy_version == "static_fallback");
  span->EndSpan();
  emitter->Flush();
}

}  // namespace themis::rag
