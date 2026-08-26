/**
 * @file test_search_edge_cases_retrieval.cpp
 * @brief Current retrieval contract smoke tests.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "search/hybrid_search.h"
#include "search/search_error_codes.h"

namespace themis::search {
namespace {

class RetrieverEdgeCasesTest : public ::testing::Test {
 protected:
  struct MockRetriever {
    bool bm25_available = true;
    bool vector_available = true;
    size_t bm25_results = 0;
    size_t vector_results = 0;

    SearchErrorCode retrieve_bm25(const std::string& query, size_t k,
                                 std::vector<HybridSearch::Result>& results) {
      if (!bm25_available) {
        return SearchErrorCode::BM25_BACKEND_UNAVAILABLE;
      }
      results.resize(std::min(k, bm25_results));
      for (size_t i = 0; i < results.size(); ++i) {
        results[i].document_id = "bm25_" + std::to_string(i);
        results[i].bm25_score = 100.0 - (i * 5.0);
      }
      return SearchErrorCode::SUCCESS;
    }

    SearchErrorCode retrieve_vector(const std::string& query, size_t k,
                                   std::vector<HybridSearch::Result>& results) {
      if (!vector_available) {
        return SearchErrorCode::VECTOR_BACKEND_UNAVAILABLE;
      }
      results.resize(std::min(k, vector_results));
      for (size_t i = 0; i < results.size(); ++i) {
        results[i].document_id = "vec_" + std::to_string(i);
        results[i].vector_score = 95.0 - (i * 4.0);
      }
      return SearchErrorCode::SUCCESS;
    }
  };
};

TEST_F(RetrieverEdgeCasesTest, RET_01_EmptyBM25Results) {
  MockRetriever retriever;
  retriever.bm25_results = 0;
  retriever.vector_results = 100;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_bm25("test query", 10, results);

  EXPECT_EQ(results.size(), 0u);
  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
}

TEST_F(RetrieverEdgeCasesTest, RET_02_EmptyVectorResults) {
  MockRetriever retriever;
  retriever.bm25_results = 100;
  retriever.vector_results = 0;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_vector("test query", 10, results);

  EXPECT_EQ(results.size(), 0u);
  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
}

TEST_F(RetrieverEdgeCasesTest, RET_03_BothBackendsUnavailable) {
  MockRetriever retriever;
  retriever.bm25_available = false;
  retriever.vector_available = false;

  std::vector<HybridSearch::Result> bm25_results;
  std::vector<HybridSearch::Result> vec_results;
  auto bm25_status = retriever.retrieve_bm25("test", 10, bm25_results);
  auto vec_status = retriever.retrieve_vector("test", 10, vec_results);

  EXPECT_EQ(bm25_status, SearchErrorCode::BM25_BACKEND_UNAVAILABLE);
  EXPECT_EQ(vec_status, SearchErrorCode::VECTOR_BACKEND_UNAVAILABLE);
  EXPECT_EQ(bm25_results.size(), 0u);
  EXPECT_EQ(vec_results.size(), 0u);
}

TEST_F(RetrieverEdgeCasesTest, RET_06_PartialBackendDegradation) {
  MockRetriever retriever;
  retriever.bm25_available = true;
  retriever.vector_available = true;
  retriever.bm25_results = 50;
  retriever.vector_results = 5;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_vector("test", 10, results);

  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
  EXPECT_EQ(results.size(), 5u);
}

TEST_F(RetrieverEdgeCasesTest, RET_09_BoundaryK1) {
  MockRetriever retriever;
  retriever.bm25_results = 100;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_bm25("test", 1, results);

  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(RetrieverEdgeCasesTest, RET_11_BoundaryMaxK) {
  MockRetriever retriever;
  retriever.vector_results = 1000;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_vector("test", std::numeric_limits<size_t>::max(), results);

  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
  EXPECT_EQ(results.size(), 1000u);
}

TEST_F(RetrieverEdgeCasesTest, RET_12_BoundaryK0) {
  MockRetriever retriever;
  retriever.bm25_results = 100;

  std::vector<HybridSearch::Result> results;
  auto status = retriever.retrieve_bm25("test", 0, results);

  EXPECT_EQ(status, SearchErrorCode::SUCCESS);
  EXPECT_EQ(results.size(), 0u);
}

}  // namespace
}  // namespace themis::search
