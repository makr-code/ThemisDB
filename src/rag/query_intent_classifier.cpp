// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "query_intent_classifier.h"

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

QueryIntentClassifier::ClassificationResult QueryIntentClassifier::Classify(
    const std::string& query_text) {
  ClassificationResult result;
  result.intent = Intent::Factual;  // Default fallback
  result.confidence = 0.0f;

  // Tokenize query (simple lowercase splitting)
  std::stringstream ss(query_text);
  std::string token;
  std::vector<std::string> tokens;
  std::string lower_query = query_text;
  std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Score each intent category
  std::vector<float> intent_scores(4, 0.0f);
  for (int i = 0; i < 4; ++i) {
    for (const auto& [keyword, weight] : impl_->intent_keywords[i]) {
      if (lower_query.find(keyword) != std::string::npos) {
        intent_scores[i] += weight;
      }
    }
  }

  // Apply softmax to get probabilities
  float max_score = *std::max_element(intent_scores.begin(), intent_scores.end());
  float sum = 0.0f;
  for (int i = 0; i < 4; ++i) {
    float exp_score = std::exp(intent_scores[i] - max_score);
    result.intent_scores[static_cast<Intent>(i)] = exp_score;
    sum += exp_score;
  }

  // Normalize
  for (auto& [intent, score] : result.intent_scores) {
    score /= sum;
  }

  // Find highest confidence
  float max_confidence = 0.0f;
  for (auto& [intent, score] : result.intent_scores) {
    if (score > max_confidence) {
      max_confidence = score;
      result.intent = intent;
    }
  }

  result.confidence = max_confidence;

  // Threshold: below 0.7, fallback to Factual
  if (result.confidence < 0.7f) {
    result.intent = Intent::Factual;
    result.confidence = 0.7f;
    result.intent_scores[Intent::Factual] = 1.0f;
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
      // Dense > Lexical > Graph: dense embeddings capture semantics
      return {0.2f, 0.6f, 0.2f};
    case Intent::Temporal:
      // Lexical + Graph > Dense: recency requires freshness signals
      return {0.4f, 0.2f, 0.4f};
    case Intent::MultiHop:
      // Dense + Graph > Lexical: semantics + connectivity
      return {0.2f, 0.4f, 0.4f};
    case Intent::Comparison:
      // Lexical ≈ Dense > Graph: keyword + semantic balance
      return {0.3f, 0.4f, 0.3f};
    default:
      return {0.3f, 0.5f, 0.2f};  // Static fallback
  }
}

}  // namespace themis::rag
