#include "query/bm25_scorer.h"

#include <algorithm>
#include <cmath>

namespace themis::query::fts {
namespace {

float accumulateScore(const SearchNode& node,
                      const IndexStatistics& stats,
                      const BM25Scorer& scorer) {
  switch (node.type) {
    case SearchNodeType::TERM:
    case SearchNodeType::PHRASE: {
      const uint32_t total_docs = std::max<uint32_t>(1U, stats.document_count);
      const uint32_t df = node.document_frequency.value_or(1U);
      const uint32_t tf = node.document_term_frequency.value_or(1U);
      const uint32_t doc_len = node.document_length_tokens.value_or(
          static_cast<uint32_t>(std::max(1.0f, stats.average_doc_length)));
      const float idf = BM25Scorer::computeIDF(df, total_docs);
      return idf * scorer.computeTFComponent(tf, doc_len, stats.average_doc_length) * node.boost;
    }
    case SearchNodeType::AND:
    case SearchNodeType::OR: {
      float total = 0.0f;
      for (const auto& child : node.children) {
        total += accumulateScore(child, stats, scorer);
      }
      return total;
    }
    case SearchNodeType::NOT:
      return 0.0f;
  }
  return 0.0f;
}

}  // namespace

BM25Scorer::BM25Scorer() : BM25Scorer(Config{}) {}

BM25Scorer::BM25Scorer(const Config& config) : config_(config) {}

float BM25Scorer::compute(uint64_t /*doc_id*/,
                          const SearchNode& query,
                          const IndexStatistics& index_stats) const {
  if (index_stats.document_count == 0) {
    return 0.0f;
  }
  return std::max(0.0f, accumulateScore(query, index_stats, *this));
}

float BM25Scorer::computeIDF(uint32_t doc_freq, uint32_t total_docs) {
  if (doc_freq == 0 || total_docs == 0 || doc_freq > total_docs) {
    return 0.0f;
  }
  const float numerator = static_cast<float>(total_docs - doc_freq) + 0.5f;
  const float denominator = static_cast<float>(doc_freq) + 0.5f;
  if (denominator <= 0.0f || numerator <= 0.0f) {
    return 0.0f;
  }
  return std::max(0.0f, std::log1pf(numerator / denominator));
}

float BM25Scorer::computeTFComponent(uint32_t term_freq,
                                     uint32_t doc_length,
                                     float avg_doc_length) const {
  if (term_freq == 0) {
    return 0.0f;
  }

  const float safe_avg_len = std::max(1.0f, avg_doc_length);
  const float length_norm =
      1.0f - config_.b + config_.b * (static_cast<float>(std::max<uint32_t>(1U, doc_length)) /
                                      safe_avg_len);
  const float tf = static_cast<float>(term_freq);
  const float denominator = tf + config_.k1 * length_norm;
  if (denominator <= 0.0f) {
    return 0.0f;
  }
  return (tf * (config_.k1 + 1.0f)) / denominator;
}

}  // namespace themis::query::fts
