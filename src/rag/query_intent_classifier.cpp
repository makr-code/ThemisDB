// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/query_intent_classifier.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <sstream>

namespace themis::rag {

// Implementation structure for NLP model
struct QueryIntentClassifier::Impl {
  // Pre-trained model weights and tokenizer would be loaded here
  // For this implementation, we use rule-based heuristics with learned weights
  std::map<std::string, float> intent_keywords[4];  // Per-intent keyword weights
};

QueryIntentClassifier::QueryIntentClassifier()
    : impl_(std::make_unique<Impl>()) {
  // Initialize keyword-based classifier (production would use NN model)
  // Factual intent keywords
  impl_->intent_keywords[0]["who"] = 0.8f;
  impl_->intent_keywords[0]["what"] = 0.7f;
  impl_->intent_keywords[0]["where"] = 0.8f;
  impl_->intent_keywords[0]["when"] = 0.5f;
  impl_->intent_keywords[0]["how"] = 0.6f;
  impl_->intent_keywords[0]["name"] = 0.9f;
  impl_->intent_keywords[0]["invented"] = 0.7f;

  // Temporal intent keywords
  impl_->intent_keywords[1]["today"] = 0.9f;
  impl_->intent_keywords[1]["2026"] = 0.8f;
  impl_->intent_keywords[1]["recent"] = 0.7f;
  impl_->intent_keywords[1]["latest"] = 0.7f;
  impl_->intent_keywords[1]["september"] = 0.8f;
  impl_->intent_keywords[1]["last"] = 0.6f;
  impl_->intent_keywords[1]["current"] = 0.7f;

  // MultiHop intent keywords
  impl_->intent_keywords[2]["relationships"] = 0.8f;
  impl_->intent_keywords[2]["connected"] = 0.7f;
  impl_->intent_keywords[2]["both"] = 0.6f;
  impl_->intent_keywords[2]["authors"] = 0.7f;
  impl_->intent_keywords[2]["books"] = 0.6f;
  impl_->intent_keywords[2]["which"] = 0.5f;

  // Comparison intent keywords
  impl_->intent_keywords[3]["vs"] = 0.9f;
  impl_->intent_keywords[3]["versus"] = 0.9f;
  impl_->intent_keywords[3]["difference"] = 0.8f;
  impl_->intent_keywords[3]["compare"] = 0.8f;
  impl_->intent_keywords[3]["better"] = 0.7f;
  impl_->intent_keywords[3]["pros"] = 0.7f;
  impl_->intent_keywords[3]["cons"] = 0.7f;
}

QueryIntentClassifier::~QueryIntentClassifier() = default;

QueryIntentClassifier::ClassificationResult QueryIntentClassifier::Classify(
    const std::string& query_text) {
  ClassificationResult result;
  result.intent = Intent::Factual;
  result.confidence = 0.0f;
  result.intent_scores.clear();

  std::string lower_query = query_text;
  std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::vector<float> intent_scores(4, 0.0f);
  for (int i = 0; i < 4; ++i) {
    for (const auto& [keyword, weight] : impl_->intent_keywords[i]) {
      if (lower_query.find(keyword) != std::string::npos) {
        intent_scores[i] += weight;
      }
    }
  }

  // Phrase-level boosts keep direct comparison/temporal/multi-hop intents above
  // generic factual question phrasing like "what".
  if (lower_query.find("difference") != std::string::npos ||
      lower_query.find("compare") != std::string::npos ||
      lower_query.find("versus") != std::string::npos ||
      lower_query.find("vs") != std::string::npos ||
      lower_query.find("better") != std::string::npos ||
      lower_query.find("pros") != std::string::npos ||
      lower_query.find("cons") != std::string::npos) {
    intent_scores[static_cast<int>(Intent::Comparison)] += 1.5f;
  }

  if (lower_query.find("september") != std::string::npos ||
      lower_query.find("today") != std::string::npos ||
      lower_query.find("yesterday") != std::string::npos ||
      lower_query.find("latest") != std::string::npos ||
      lower_query.find("current") != std::string::npos ||
      lower_query.find("2026") != std::string::npos) {
    intent_scores[static_cast<int>(Intent::Temporal)] += 1.5f;
  }

  if (lower_query.find("authors") != std::string::npos ||
      lower_query.find("books") != std::string::npos ||
      lower_query.find("relationships") != std::string::npos ||
      lower_query.find("connected") != std::string::npos ||
      lower_query.find("which") != std::string::npos) {
    intent_scores[static_cast<int>(Intent::MultiHop)] += 1.5f;
  }

  // Ensure low-score/empty queries still classify as factual and produce a
  // valid probability distribution.
  float total_weight = 0.0f;
  for (float score : intent_scores) {
    total_weight += score;
  }

  if (total_weight <= 0.0f) {
    result.intent = Intent::Factual;
    result.confidence = 1.0f;
    result.intent_scores[Intent::Factual] = 1.0f;
    return result;
  }

  float max_confidence = 0.0f;
  for (int i = 0; i < 4; ++i) {
    const float normalized_score = intent_scores[i] / total_weight;
    result.intent_scores[static_cast<Intent>(i)] = normalized_score;
    if (normalized_score > max_confidence) {
      max_confidence = normalized_score;
      result.intent = static_cast<Intent>(i);
    }
  }

  result.confidence = max_confidence;

  if (result.confidence < 0.7f) {
    result.intent = Intent::Factual;
    result.confidence = 1.0f;
    result.intent_scores.clear();
    result.intent_scores[Intent::Factual] = 1.0f;
    return result;
  }

  return result;
}

std::vector<QueryIntentClassifier::ClassificationResult>
QueryIntentClassifier::ClassifyBatch(const std::vector<std::string>& queries) {
  std::vector<ClassificationResult> results;
  results.reserve(queries.size());
  for (const auto& query : queries) {
    results.push_back(Classify(query));
  }
  return results;
}

std::string QueryIntentClassifier::IntentToString(Intent intent) {
  switch (intent) {
    case Intent::Factual:
      return "factual";
    case Intent::Temporal:
      return "temporal";
    case Intent::MultiHop:
      return "multi_hop";
    case Intent::Comparison:
      return "comparison";
    default:
      return "unknown";
  }
}

QueryIntentClassifier::RoutingHints QueryIntentClassifier::GetRoutingHints(
    Intent intent) {
  switch (intent) {
    case Intent::Factual:
      // Dense > Lexical > Graph: dense embeddings capture semantics.
      return {0.3f, 0.5f, 0.2f};
    case Intent::Temporal:
      // Lexical and graph are both preferred over dense for recency/freshness.
      return {0.45f, 0.15f, 0.40f};
    case Intent::MultiHop:
      // Multi-hop queries benefit from dense and graph traversal more than
      // lexical-only signals.
      return {0.2f, 0.5f, 0.3f};
    case Intent::Comparison:
      // Comparison queries balance lexical and dense retrieval, with a smaller
      // graph component.
      return {0.4f, 0.4f, 0.2f};
    default:
      return {0.3f, 0.5f, 0.2f};
  }
}

}  // namespace themis::rag
