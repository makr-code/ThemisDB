#include <gtest/gtest.h>

#include "query/bm25_scorer.h"

namespace themis::query::fts {
namespace {

TEST(FtsBm25ScorerTest, ComputesPositiveScoreForAnnotatedTerm) {
  BM25Scorer scorer;
  SearchNode query = SearchNode::makeTerm("database");
  query.document_frequency = 3U;
  query.document_term_frequency = 4U;
  query.document_length_tokens = 120U;

  IndexStatistics stats;
  stats.document_count = 100U;
  stats.average_doc_length = 90.0F;

  const float score = scorer.compute(7U, query, stats);
  EXPECT_GT(score, 0.0F);
}

TEST(FtsBm25ScorerTest, TfComponentZeroWhenTermFrequencyZero) {
  BM25Scorer scorer;
  EXPECT_FLOAT_EQ(scorer.computeTFComponent(0U, 100U, 100.0F), 0.0F);
}

TEST(FtsBm25ScorerTest, IdfZeroForInvalidInput) {
  EXPECT_FLOAT_EQ(BM25Scorer::computeIDF(0U, 100U), 0.0F);
  EXPECT_FLOAT_EQ(BM25Scorer::computeIDF(200U, 100U), 0.0F);
}

}  // namespace
}  // namespace themis::query::fts
