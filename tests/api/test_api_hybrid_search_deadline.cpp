/**
 * @file test_api_hybrid_search_deadline.cpp
 * @brief Focused tests for cooperative deadlines on POST /search/hybrid (QRY-07).
 */

#include <gtest/gtest.h>

#include "content/content_manager.h"
#include "query/request_context.h"
#include "server/content_api_handler.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
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

class HybridSearchDeadlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::current_path() / ".qry07-api-test-data" /
                   ("hybrid_search_" + std::to_string(suffix));
        std::filesystem::create_directories(db_path_.parent_path());

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        content_manager_ = std::make_shared<themis::content::ContentManager>(
            db_, nullptr, nullptr, nullptr);
        handler_ = std::make_unique<themis::server::ContentApiHandler>(
            db_, content_manager_, nullptr, nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        content_manager_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    static http::request<http::string_body> makeRequest() {
        http::request<http::string_body> req{http::verb::post, "/search/hybrid", 11};
        req.body() = nlohmann::json{{"query", "deadline coverage"}, {"k", 5}}.dump();
        req.prepare_payload();
        return req;
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::content::ContentManager> content_manager_;
    std::unique_ptr<themis::server::ContentApiHandler> handler_;
};

TEST_F(HybridSearchDeadlineTest, EmptyResultsRemainSuccessful) {
    const auto res = handler_->handleHybridSearch(makeRequest());

    ASSERT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(nlohmann::json::parse(res.body())["count"], 0);
}

TEST_F(HybridSearchDeadlineTest, ExpiredDeadlineReturns504WithoutResults) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleHybridSearch(makeRequest(), expired);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

TEST_F(HybridSearchDeadlineTest, ClientCancellationReturns504WithoutResults) {
    auto token = std::make_shared<themis::query::QueryCancellationToken>();
    token->cancel();
    const themis::query::RequestContext cancelled(token);
    const auto res = handler_->handleHybridSearch(makeRequest(), cancelled);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

TEST_F(HybridSearchDeadlineTest, CancellationDuringContentSearchReturns504) {
    PollState state;
    state.stop_after = 2;  // Handler dispatch, then the content-manager base-search checkpoint.
    const themis::query::RequestContext cancelled(nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto res = handler_->handleHybridSearch(makeRequest(), cancelled);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

} // namespace
