// Tests for SHORTEST_PATH parsing, execution limits, and request stop handling.

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "query/aql_parser.h"
#include "query/aql_runner.h"
#include "query/aql_translator.h"
#include "query/query_engine.h"
#include "query/request_context.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/error_registry.h"

using namespace themis;
using namespace themis::query;

namespace {

struct StopAtPollState {
    unsigned int polls = 0;
    unsigned int stop_at = 0;
};

bool stopAtPoll(const void* opaque) noexcept {
    auto* state = const_cast<StopAtPollState*>(static_cast<const StopAtPollState*>(opaque));
    ++state->polls;
    return state->stop_at != 0 && state->polls >= state->stop_at;
}

} // namespace

TEST(AQLShortestPathTest, ParseAndTranslateShortestPath) {
    const std::string aql = R"(
        FOR v IN 1..5 OUTBOUND "city:berlin" GRAPH "cities"
        SHORTEST_PATH TO "city:dresden"
        RETURN v
    )";
    AQLParser parser;
    const auto parsed = parser.parse(aql);
    ASSERT_TRUE(parsed.has_value()) << parsed.error().message();
    ASSERT_TRUE((*parsed)->traversal);
    ASSERT_TRUE((*parsed)->traversal->shortestPath);
    EXPECT_EQ((*parsed)->traversal->shortestPathTarget, "city:dresden");

    const auto translated = AQLTranslator::translate(*parsed);
    ASSERT_TRUE(translated.success) << translated.error_message;
    ASSERT_TRUE(translated.traversal.has_value());
    EXPECT_TRUE(translated.traversal->shortestPath);
    EXPECT_EQ(translated.traversal->endVertex, "city:dresden");
}

TEST(AQLShortestPathTest, ParserRejectsDepthAboveLimitAndOverlongDepth) {
    AQLParser parser;
    const auto at_limit = parser.parse(
        "FOR v IN 1..100 OUTBOUND \"city:berlin\" GRAPH \"cities\" "
        "SHORTEST_PATH TO \"city:dresden\" RETURN v");
    ASSERT_TRUE(at_limit.has_value()) << at_limit.error().message();

    const auto over_limit = parser.parse(
        "FOR v IN 1..101 OUTBOUND \"city:berlin\" GRAPH \"cities\" "
        "SHORTEST_PATH TO \"city:dresden\" RETURN v");
    EXPECT_FALSE(over_limit.has_value());

    const auto overlong = parser.parse(
        "FOR v IN 1..999999999999999999999999999999999999999999 OUTBOUND "
        "\"city:berlin\" GRAPH \"cities\" SHORTEST_PATH TO \"city:dresden\" RETURN v");
    EXPECT_FALSE(overlong.has_value());
}

class AQLShortestPathExecutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_aql_shortest_path_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::remove_all(db_path_);

        RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.memtable_size_mb = 32;
        config.block_cache_size_mb = 32;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        engine_ = std::make_unique<QueryEngine>(*db_, *secondary_index_, *graph_index_);
    }

    void TearDown() override {
        engine_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    void addEdge(const std::string& id, const std::string& from, const std::string& to, double weight = 1.0) {
        BaseEntity edge(id);
        edge.setField("id", id);
        edge.setField("_from", from);
        edge.setField("_to", to);
        edge.setField("_weight", weight);
        ASSERT_TRUE(graph_index_->addEdge(edge).ok);
    }

    const std::string aql_ =
        "FOR v IN 1..4 OUTBOUND \"A\" GRAPH \"default\" "
        "SHORTEST_PATH TO \"C\" RETURN v";
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
    std::unique_ptr<GraphIndexManager> graph_index_;
    std::unique_ptr<QueryEngine> engine_;
};

TEST_F(AQLShortestPathExecutionTest, EnforcesMaximumPathDepth) {
    addEdge("ab", "A", "B");
    addEdge("bc", "B", "C");

    const auto result = executeAql(
        "FOR v IN 1..1 OUTBOUND \"A\" GRAPH \"default\" "
        "SHORTEST_PATH TO \"C\" RETURN v",
        *engine_);
    ASSERT_TRUE(result) << result.error().message();
    ASSERT_TRUE(result->contains("paths"));
    EXPECT_TRUE((*result)["paths"].empty());
}

TEST_F(AQLShortestPathExecutionTest, FindsWithinLimitPathWhenCheaperRouteExceedsDepth) {
    addEdge("ax", "A", "X");
    addEdge("xb", "X", "B");
    addEdge("ab", "A", "B", 3.0);
    addEdge("bc", "B", "C");

    const auto result = executeAql(
        "FOR v IN 1..2 OUTBOUND \"A\" GRAPH \"default\" "
        "SHORTEST_PATH TO \"C\" RETURN v",
        *engine_);
    ASSERT_TRUE(result) << result.error().message();
    ASSERT_EQ((*result)["paths"].size(), 1u);
    EXPECT_EQ((*result)["paths"][0], nlohmann::json({"A", "B", "C"}));
}

TEST_F(AQLShortestPathExecutionTest, TemporalSearchFindsWithinLimitAlternatePath) {
    addEdge("ax", "A", "X", 1.0);
    addEdge("xy", "X", "Y", 1.0);
    addEdge("yc", "Y", "C", 1.0);
    addEdge("ab", "A", "B", 3.0);
    addEdge("bc", "B", "C", 3.0);

    RecursivePathQuery query;
    query.start_node = "A";
    query.end_node = "C";
    query.graph_id = "default";
    query.max_depth = 2;
    query.valid_from = "0";
    query.valid_to = "0";
    const auto result = engine_->executeRecursivePathQuery(query);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ((*result)[0], std::vector<std::string>({"A", "B", "C"}));
}

TEST_F(AQLShortestPathExecutionTest, TemporalSearchRejectsInvalidDepthLimit) {
    const auto [status, path] = graph_index_->dijkstraAtTime("A", "C", 0, -2);
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(path.path.empty());
}

TEST_F(AQLShortestPathExecutionTest, CyclesDoNotRepeatVerticesInShortestPath) {
    addEdge("ab", "A", "B");
    addEdge("ba", "B", "A");
    addEdge("bc", "B", "C");

    const auto result = executeAql(aql_, *engine_);
    ASSERT_TRUE(result) << result.error().message();
    ASSERT_EQ((*result)["paths"].size(), 1u);
    EXPECT_EQ((*result)["paths"][0], nlohmann::json({"A", "B", "C"}));
}

TEST_F(AQLShortestPathExecutionTest, ExpiredRequestReturnsTimeoutWithoutPartialPath) {
    const auto request = RequestContext::withTimeout(std::chrono::milliseconds{0});
    auto stop_reason = StopReason::None;

    const auto result = executeAql(aql_, *engine_, request, &stop_reason);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_TIMEOUT);
    EXPECT_EQ(stop_reason, StopReason::DeadlineExceeded);
}

TEST_F(AQLShortestPathExecutionTest, CancelledRequestReturnsCancellationWithoutPartialPath) {
    auto token = std::make_shared<QueryCancellationToken>();
    token->cancel();
    const RequestContext request(token);
    auto stop_reason = StopReason::None;

    const auto result = executeAql(aql_, *engine_, request, &stop_reason);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_CANCELLED);
    EXPECT_EQ(stop_reason, StopReason::Cancelled);
}

TEST_F(AQLShortestPathExecutionTest, CancellationAfterShortestPathDispatchDiscardsResult) {
    addEdge("ab", "A", "B");
    addEdge("bc", "B", "C");

    StopAtPollState baseline;
    const RequestContext unbounded_request(
        nullptr, std::nullopt, &stopAtPoll, &baseline);
    const auto completed = executeAql(aql_, *engine_, unbounded_request);
    ASSERT_TRUE(completed) << completed.error().message();
    ASSERT_GT(baseline.polls, 0u);

    StopAtPollState cancellation{0, baseline.polls};
    const RequestContext request(nullptr, std::nullopt, &stopAtPoll, &cancellation);
    auto stop_reason = StopReason::None;
    const auto result = executeAql(aql_, *engine_, request, &stop_reason);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_CANCELLED);
    EXPECT_EQ(stop_reason, StopReason::Cancelled);
}
