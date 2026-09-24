// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Classifies query intent for adaptive routing decisions.
/// 
/// Implements NLP-based query intent classification supporting four intent categories:
/// - Factual: Direct answer lookup (person, place, date)
/// - Temporal: Time-sensitive queries with recency requirements
/// - MultiHop: Queries requiring multi-step reasoning across documents
/// - Comparison: Comparative reasoning and analysis tasks
///
/// @details
/// Classification uses softmax-normalized intent scores with confidence thresholding.
/// Queries below the confidence threshold (0.7) fall back to static routing.
/// Pre-trained on open QA datasets (SQUAD, NQ, HotpotQA) with fine-tuning on
/// internal ThemisDB query logs.
///
/// @note Latency target: <5ms p99 (CPU-bound NLP inference)
class QueryIntentClassifier {
 public:
  /// @brief Query intent categories for adaptive routing.
  enum class Intent : uint8_t {
    Factual = 0,      ///< Direct answer lookup (e.g., "Who invented lightbulbs?")
    Temporal = 1,     ///< Time-sensitive (e.g., "COVID cases in September 2026")
    MultiHop = 2,     ///< Multi-step reasoning (e.g., "Authors of banned books")
    Comparison = 3    ///< Comparative reasoning (e.g., "Python vs Go")
  };

  /// @brief Classification result for a query.
  struct ClassificationResult {
    Intent intent;                                ///< Predicted intent category
    float confidence;                             ///< Confidence score [0, 1]
    std::map<Intent, float> intent_scores;       ///< Softmax scores per intent
    std::string query_id;                        ///< For correlation
  };

  /// @brief Default constructor; initializes NLP model from disk.
  QueryIntentClassifier();

  /// @brief Destructor.
  ~QueryIntentClassifier() = default;

  /// @brief Classify a single query.
  /// 
  /// @param query_text The query text to classify.
  /// @return ClassificationResult with intent, confidence, and per-intent scores.
  /// 
  /// @details
  /// - Tokenizes query using word-level tokenization
  /// - Encodes with pre-trained embeddings
  /// - Computes softmax over intent logits
  /// - Confidence below 0.7 returns Intent::Factual (safest fallback)
  ClassificationResult Classify(const std::string& query_text);

  /// @brief Classify a batch of queries (more efficient than sequential calls).
  ///
  /// @param queries Vector of query texts.
  /// @return Vector of ClassificationResult in same order as input.
  ///
  /// @details
  /// - Batches inference through NLP model (typically 32-query batches)
  /// - Reduces overhead vs sequential classification
  /// - Deterministic ordering: results[i] corresponds to queries[i]
  std::vector<ClassificationResult> ClassifyBatch(
      const std::vector<std::string>& queries);

  /// @brief Get human-readable label for intent.
  /// 
  /// @param intent The intent to label.
  /// @return String label: "factual", "temporal", "multi_hop", "comparison".
  static std::string IntentToString(Intent intent);

  /// @brief Routing hint for intent (recommended weight allocation).
  /// 
  /// @param intent The intent category.
  /// @return Recommended (lexical%, dense%, graph%) weights for hybrid routing.
  /// 
  /// @details
  /// - Factual: Dense > Lexical > Graph (dense embeddings capture semantics)
  /// - Temporal: Lexical + Graph > Dense (recency requires freshness signals)
  /// - MultiHop: Dense + Graph > Lexical (semantics + connectivity)
  /// - Comparison: Lexical ≈ Dense > Graph (keyword + semantic balance)
  struct RoutingHints {
    float lexical_weight;
    float dense_weight;
    float graph_weight;
  };
  static RoutingHints GetRoutingHints(Intent intent);

 private:
  // NLP model and state (implementation details hidden)
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace themis::rag
