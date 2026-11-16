/**
 * @file test_api_fulltext_search_deadline.cpp
 * @brief Focused tests for cooperative deadlines on POST /search/fulltext (QRY-07).
 */

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "query/request_context.h"
#include "server/content_api_handler.h"
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
    const auto* state = static_cast<const PollState*>(opaque);
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class FulltextSearchDeadlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::current_path() / ".qry07-api-test-data" /
                   ("fulltext_search_" + std::to_string(suffix));
        std::filesystem::create_directories(db_path_.parent_path());

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        index_ = std::make_shared<themis::SecondaryIndexManager>(*db_);
        const auto index_status = index_->createFulltextIndex("articles", "content");
        ASSERT_TRUE(index_status.ok) << index_status.message;
        for (int i = 0; i < 128; ++i) {
            themis::BaseEntity entity("article-" + std::to_string(i));
            entity.setField("content", "timeoutprobe index cancellation coverage");
            const auto put_status = index_->put("articles", entity);
            ASSERT_TRUE(put_status.ok) << put_status.message;
        }
        handler_ = std::make_unique<themis::server::ContentApiHandler>(
            db_, nullptr, nullptr, nullptr, index_, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    static http::request<http::string_body> makeRequest(const nlohmann::json& body) {
        http::request<http::string_body> req{http::verb::post, "/search/fulltext", 11};
        req.body() = body.dump();
        req.prepare_payload();
        return req;
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::SecondaryIndexManager> index_;
    std::unique_ptr<themis::server::ContentApiHandler> handler_;
};


TEST_F(FulltextSearchDeadlineTest, EmptyResultsRemainDistinctFromDeadline) {
    const auto res = handler_->handleFulltextSearch(
        makeRequest({{"table", "articles"}, {"column", "content"}, {"query", "absenttoken"}}));
    ASSERT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(nlohmann::json::parse(res.body())["count"], 0);
}

TEST_F(FulltextSearchDeadlineTest, ExpiredDeadlineReturns504WithoutResults) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleFulltextSearch(
        makeRequest({{"table", "articles"}, {"column", "content"}, {"query", "timeoutprobe"}}),
        expired);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

TEST_F(FulltextSearchDeadlineTest, CancellationDuringIndexScanDiscardsPartialResults) {
    PollState state;
    state.stop_after = 3;  // Entry, token-stage, then the 64-entry scan checkpoint.
    const themis::query::RequestContext request(nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto res = handler_->handleFulltextSearch(
        makeRequest({{"table", "articles"}, {"column", "content"}, {"query", "timeoutprobe"}}),
        request);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("article-"), std::string::npos);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

} // namespace
