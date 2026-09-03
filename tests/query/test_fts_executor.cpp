#include <gtest/gtest.h>

#include "query/fts_executor.h"

#include <chrono>
#include <filesystem>
#include <unistd.h>

namespace themis::query::fts {
namespace {

std::filesystem::path makeTempIndexPath() {
  const auto base = std::filesystem::temp_directory_path();
  const auto unique = base / ("themis_fts_test_" + std::to_string(::getpid()) + "_" +
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

}  // namespace
}  // namespace themis::query::fts
