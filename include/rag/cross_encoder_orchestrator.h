// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

// Forward declarations
struct Document;

/// @brief Orchestrates batch cross-encoder re-ranking with efficiency optimization.
///
/// Manages cross-encoder model lifecycle, batches requests for throughput,
/// and caches embeddings to reduce redundant inference.
///
/// @details
/// - Loads cross-encoder model on demand (lazy initialization)
/// - Accumulates queries into batches (timeout: 100ms, size: 32)
/// - Encodes query + documents together for efficiency
/// - Caches document embeddings (1-hour soft TTL)
/// - Target cache hit rate: ≥20% (saves reranking cost)
///
/// Batching strategy:
/// - Window: 100ms or 32 queries (whichever first)
/// - Fallback: Return top-k from hybrid retrieval on model load failure
///
/// @note Latency target: <50ms p99 per query (with batching/caching)
class CrossEncoderOrchestrator {
 public:
  /// @brief Reranker configuration.
  struct RerankerConfig {
    std::string model_name;        ///< e.g., "cross-encoder/ms-marco-MiniLMv2-L12-H384"
    uint32_t batch_size;           ///< Batch accumulation size (default 32)
    float confidence_threshold;    ///< Min score to include in output (default 0.5)
  };

  /// @brief Single query re-ranking request.
  struct RerankerQuery {
    std::string query_id;
    std::string query_text;
    std::vector<Document> documents;  ///< k=50 hybrid results
  };

  /// @brief Re-ranking result for single query.
  struct RerankedResults {
    std::vector<Document> reranked_docs;  ///< Top-k results (typically k=10)
    std::vector<float> relevance_scores;   ///< Confidence scores for each result
    uint64_t latency_ms;                  ///< Time to rerank
    uint32_t tokens_used;                 ///< Token count for billing
    bool used_cache;                      ///< Was cache hit?
  };

  /// @brief Constructor.
  ///
  /// @param config Reranker configuration (model name, batch size, etc.).
  /// @throws std::invalid_argument if config is invalid.
  explicit CrossEncoderOrchestrator(const RerankerConfig& config);

  /// @brief Destructor (unloads model if loaded).
  ~CrossEncoderOrchestrator();

  /// @brief Re-rank single query.
  ///
  /// @param query_text Query text.
  /// @param docs Documents to rerank (k=50).
  /// @return Top-k re-ranked results with scores.
  ///
  /// @details
  /// - Encodes query and documents
  /// - Computes cross-encoder scores
  /// - Filters by confidence_threshold
  /// - Returns top-10 (default k=10)
  /// - Caches embeddings for future use
  RerankedResults Rerank(const std::string& query_text,
                        const std::vector<Document>& docs);

  /// @brief Batch re-rank (more efficient than sequential calls).
  ///
  /// @param queries Vector of queries to rerank.
  /// @return Results in same order as input.
  ///
  /// @details
  /// - Batches inference through cross-encoder model
  /// - Encodes all queries + documents together
  /// - Returns results in original request order
  /// - Each result has independent latency tracking
  std::vector<RerankedResults> ReankBatch(
      const std::vector<RerankerQuery>& queries);

  /// @brief Cache statistics.
  struct CacheStats {
    uint64_t total_lookups;
    uint64_t cache_hits;
    float hit_rate;  ///< cache_hits / total_lookups
    uint64_t cache_size_bytes;
  };

  /// @brief Get cache statistics.
  ///
  /// @return Current cache performance metrics.
  CacheStats GetCacheStats() const;

  /// @brief Clear embedding cache (typically called daily).
  void ClearCache();

  /// @brief Model loading status.
  ///
  /// @return true if model is loaded in memory.
  bool IsModelLoaded() const;

  /// @brief Unload model from memory (frees resources).
  void UnloadModel();

  /// @brief Get model name currently configured.
  ///
  /// @return Model name string.
  std::string GetModelName() const;

 private:
  RerankerConfig config_;
  
  // Model state (implementation details hidden)
  struct ModelImpl;
  std::unique_ptr<ModelImpl> model_;

  // Embedding cache (doc_id → cached_embedding)
  struct CacheEntry {
    std::vector<float> embedding;
    std::chrono::steady_clock::time_point accessed_at;
  };
  std::map<std::string, CacheEntry> embedding_cache_;
  
  // Cache statistics
  uint64_t cache_hits_ = 0;
  uint64_t cache_lookups_ = 0;

  // Model lazy-loading
  bool EnsureModelLoaded();
  
  // Cache management
  void EvictStaleEntries();  // Remove entries older than 1 hour
};

}  // namespace themis::rag
