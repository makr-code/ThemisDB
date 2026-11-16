/**
 * @file test_api_aql_fulltext_deadline.cpp
 * @brief Focused tests for cooperative deadlines on AQL full-text retrieval (QRY-07).
 */

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "query/request_context.h"
#include "server/query_api_handler.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace {

namespace http = boost::beast::http;

struct PollState {
    std::atomic<unsigned int> polls{0};
    unsigned int stop_after = 0;
};

bool stopAfterPolls(const void* opaque) noexcept {
    auto* state = const_cast<PollState*>(static_cast<const PollState*>(opaque));
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class AqlFulltextDeadlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::current_path() / ".qry07-api-test-data" /
                   ("aql_fulltext_" + std::to_string(suffix));
        std::filesystem::create_directories(db_path_.parent_path());

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        index_ = std::make_shared<themis::SecondaryIndexManager>(*db_);
        const auto index_status = index_->createFulltextIndex("articles", "content");
        ASSERT_TRUE(index_status.ok) << index_status.message;
        const auto category_index_status = index_->createIndex("articles", "category");
        ASSERT_TRUE(category_index_status.ok) << category_index_status.message;
        const auto status_index_status = index_->createIndex("articles", "status");
        ASSERT_TRUE(status_index_status.ok) << status_index_status.message;
        for (int i = 0; i < 128; ++i) {
            themis::BaseEntity entity("article-" + std::to_string(i));
            entity.setField("content", "deadlineprobe fulltext cancellation coverage");
            entity.setField("category", i % 2 == 0 ? "even" : "odd");
            entity.setField("status", "published");
            const auto put_status = index_->put("articles", entity);
            ASSERT_TRUE(put_status.ok) << put_status.message;
        }

        handler_ = std::make_unique<themis::server::QueryApiHandler>(
            db_, index_, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, false, false);
    }

    void TearDown() override {
        handler_.reset();
        index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    static http::request<http::string_body> makeRequest(std::string query) {
        http::request<http::string_body> req{http::verb::post, "/query/aql", 11};
        req.body() = nlohmann::json{{"query", std::move(query)}}.dump();
        req.prepare_payload();
        return req;
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::SecondaryIndexManager> index_;
    std::unique_ptr<themis::server::QueryApiHandler> handler_;
};

TEST_F(AqlFulltextDeadlineTest, IndexedSearchReturnsResultsAndNoMatchRemainsEmptySuccess) {
    const auto found = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") RETURN d"));
    ASSERT_EQ(found.result(), http::status::ok) << found.body();
    EXPECT_GT(nlohmann::json::parse(found.body())["count"].get<size_t>(), 0u);

    const auto filtered = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") "
                    "AND d.category == \"even\" AND d.status == \"published\" RETURN d"));
    ASSERT_EQ(filtered.result(), http::status::ok) << filtered.body();
    EXPECT_EQ(nlohmann::json::parse(filtered.body())["count"], 64);

    const auto absent = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"absenttoken\") RETURN d"));
    ASSERT_EQ(absent.result(), http::status::ok) << absent.body();
    EXPECT_EQ(nlohmann::json::parse(absent.body())["count"], 0);
}

TEST_F(AqlFulltextDeadlineTest, ExpiredServerDeadlineReturns504WithoutResults) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto response = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") RETURN d"),
        expired);

    ASSERT_EQ(response.result(), http::status::gateway_timeout);
    EXPECT_EQ(response.body().find("article-"), std::string::npos);
}

TEST_F(AqlFulltextDeadlineTest, ClientCancellationReturns504WithoutPartialResults) {
    auto token = std::make_shared<themis::query::QueryCancellationToken>();
    token->cancel();
    const themis::query::RequestContext cancelled(token);
    const auto response = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") RETURN d"),
        cancelled);

    ASSERT_EQ(response.result(), http::status::gateway_timeout);
    EXPECT_EQ(response.body().find("article-"), std::string::npos);
}

TEST_F(AqlFulltextDeadlineTest, CancellationDuringPrefixScanDiscardsPartialResults) {
    PollState state;
    state.stop_after = 6;  // Handler, engine stages, then the 64-entry index-scan poll.
    const themis::query::RequestContext cancelled(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto response = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") RETURN d"),
        cancelled);

    ASSERT_EQ(response.result(), http::status::gateway_timeout);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
    EXPECT_EQ(response.body().find("article-"), std::string::npos);
}

TEST_F(AqlFulltextDeadlineTest, StructuralPredicateExecutionHonorsCancellation) {
    PollState state;
    state.stop_after = 26;  // After text retrieval, at structural-index intersection.
    const themis::query::RequestContext cancelled(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto response = handler_->handleQueryAql(
        makeRequest("FOR d IN articles FILTER FULLTEXT(d.content, \"deadlineprobe\") "
                    "AND d.category == \"even\" AND d.status == \"published\" RETURN d"),
        cancelled);

    ASSERT_EQ(response.result(), http::status::gateway_timeout);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
    EXPECT_EQ(response.body().find("article-"), std::string::npos);
}

} // namespace
