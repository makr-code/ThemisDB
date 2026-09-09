#include <gtest/gtest.h>

#include "query/fts_executor.h"

#include <chrono>
#include <filesystem>
#ifdef _WIN32
  #include <windows.h>
  #define GET_PID() static_cast<int>(GetCurrentProcessId())
#else
  #include <unistd.h>
  #define GET_PID() ::getpid()
#endif

namespace themis::query::fts {
namespace {

std::filesystem::path makeTempIndexPath() {
  const auto base = std::filesystem::temp_directory_path();
  const auto unique = base / ("themis_fts_test_" + std::to_string(GET_PID()) + "_" +
                              std::to_string(std::chrono::steady_clock::now()
                                                 .time_since_epoch()
                                                 .count()));
  return unique;
}

TEST(FtsExecutorTest, UpdateAndExecuteTermQuery) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({1U, "Database systems and indexing"});
  batch.additions.push_back({2U, "Graph systems without keyword"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makeTerm("database");
  auto result = executor.execute(query);
  ASSERT_TRUE(result.has_value());
  ASSERT_FALSE(result->empty());
  EXPECT_EQ((*result)[0].doc_id, 1U);
  EXPECT_GT((*result)[0].score, 0.0F);
}

TEST(FtsExecutorTest, ExecutesExactPhraseQueriesWithTokenPositions) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({1U, "graph database engine for analytics"});
  batch.additions.push_back({2U, "graph scalable database engine"});
  batch.additions.push_back({3U, "engine database graph"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makePhrase("graph database engine");
  auto result = executor.execute(query);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1U);
  EXPECT_EQ((*result)[0].doc_id, 1U);
}

TEST(FtsExecutorTest, ExecutesPhraseQueriesCaseInsensitively) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({11U, "Graph Database Engine with mixed case"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makePhrase("GRAPH DATABASE ENGINE");
  auto result = executor.execute(query);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1U);
  EXPECT_EQ((*result)[0].doc_id, 11U);
}

TEST(FtsExecutorTest, ExecutesProximityPhraseQueriesWhenGapIsWithinBudget) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({21U, "graph highly distributed storage engine"});
  batch.additions.push_back({22U, "graph storage replication distributed telemetry engine"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makePhrase("graph engine");
  query.proximity_distance = 3U;

  auto result = executor.execute(query);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1U);
  EXPECT_EQ((*result)[0].doc_id, 21U);
}

TEST(FtsExecutorTest, BooleanQueriesReusePhraseMatching) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({31U, "graph database engine analytics"});
  batch.additions.push_back({32U, "graph database cache analytics"});
  batch.additions.push_back({33U, "graph database cache"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode phrase = SearchNode::makePhrase("graph database");
  SearchNode term = SearchNode::makeTerm("analytics");
  SearchNode query =
      SearchNode::makeBoolean(SearchNodeType::AND, {phrase, term});

  auto result = executor.execute(query);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2U);
  EXPECT_EQ((*result)[0].doc_id, 31U);
  EXPECT_EQ((*result)[1].doc_id, 32U);
}

TEST(FtsExecutorTest, RespectsTimeoutFailClosed) {
  const auto path = makeTempIndexPath();
  FtsExecutor executor(path.string());

  IndexUpdateBatch batch;
  batch.additions.push_back({10U, "alpha beta gamma delta"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makeTerm("alpha");
  ExecutionOptions options;
  options.timeout = std::chrono::milliseconds(0);
  auto result = executor.execute(query, options);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), FtsError::EXECUTION_TIMEOUT);
}

TEST(FtsExecutorTest, ReturnsOutOfMemoryWhenPostingListCannotBeCached) {
  const auto path = makeTempIndexPath();
  IndexCache::Config cache_config;
  cache_config.max_size_mb = 0;
  cache_config.bloom_filter_size_bits = 256;
  FtsExecutor executor(path.string(), cache_config);

  IndexUpdateBatch batch;
  batch.additions.push_back({1U, "alpha beta"});
  batch.additions.push_back({2U, "alpha gamma"});
  ASSERT_TRUE(executor.updateIndex(batch).has_value());

  SearchNode query = SearchNode::makeTerm("alpha");
  auto result = executor.execute(query);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), FtsError::OUT_OF_MEMORY);
}

}  // namespace
}  // namespace themis::query::fts
