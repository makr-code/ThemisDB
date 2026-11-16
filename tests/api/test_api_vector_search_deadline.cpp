/**
 * @file test_api_vector_search_deadline.cpp
 * @brief Focused tests for the cooperative deadline on POST /vector/search (QRY-07).
 *
 * Covers handler-level behavior only: client-disconnect cancellation is not
 * observable in synchronous dispatch and is not tested here.
 */

#include <gtest/gtest.h>

#include "index/vector_index.h"
#include "query/request_context.h"
#include "server/vector_api_handler.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace {

namespace http = boost::beast::http;
using themis::server::VectorApiHandler;

struct PollState {
    std::atomic<unsigned int> polls{0};
    unsigned int stop_after = 0;
};

bool stopAfterPolls(const void* opaque) noexcept {
    auto* state = const_cast<PollState*>(static_cast<const PollState*>(opaque));
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class VectorSearchDeadlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::current_path() / ".qry07-api-test-data" /
                   ("vector_search_" + std::to_string(suffix));
        std::filesystem::create_directories(db_path_.parent_path());

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        index_ = std::make_shared<themis::VectorIndexManager>(*db_);
        const auto init = index_->init("vectors", 1, themis::VectorIndexManager::Metric::L2);
        ASSERT_TRUE(init.ok) << init.message;
        for (int i = 0; i < 8; ++i) {
            themis::BaseEntity entity("pk-" + std::to_string(i));
            entity.setField("embedding", std::vector<float>{static_cast<float>(i)});
            const auto status = index_->addEntity(entity);
            ASSERT_TRUE(status.ok) << status.message;
        }
        handler_ = std::make_unique<VectorApiHandler>(db_, index_, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    static http::request<http::string_body> makeRequest(const nlohmann::json& body) {
        http::request<http::string_body> req{http::verb::post, "/vector/search", 11};
        req.body() = body.dump();
        req.prepare_payload();
        return req;
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::VectorIndexManager> index_;
    std::unique_ptr<VectorApiHandler> handler_;
};

} // namespace

TEST_F(VectorSearchDeadlineTest, DefaultOverloadIsUnboundedAndReturnsResults) {
    const auto res = handler_->handleSearch(makeRequest({{"vector", {0.0}}, {"k", 3}}));
    ASSERT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(nlohmann::json::parse(res.body())["count"], 3);
}

TEST_F(VectorSearchDeadlineTest, ContextWithoutDeadlineIsUnbounded) {
    const auto res = handler_->handleSearch(makeRequest({{"vector", {0.0}}, {"k", 3}}),
                                            themis::query::RequestContext{});
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(VectorSearchDeadlineTest, ExpiredDeadlineReturns504WithoutResults) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleSearch(makeRequest({{"vector", {0.0}}, {"k", 3}}), expired);
    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    const auto body = res.body();
    EXPECT_EQ(body.find("pk-"), std::string::npos);
    EXPECT_EQ(body.find("\"results\""), std::string::npos);
}

TEST_F(VectorSearchDeadlineTest, ExpiredDeadlineReturns504InCursorMode) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleSearch(
        makeRequest({{"vector", {0.0}}, {"k", 3}, {"use_cursor", true}}), expired);
    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"items\""), std::string::npos);
}

TEST_F(VectorSearchDeadlineTest, StopDuringSerializationDiscardsPartialOutput) {
    // Measure the exact poll count for KNN alone, then stop on the first
    // handler-side poll while it materializes the completed search results.
    PollState search_polls;
    const themis::query::RequestContext counting(
        nullptr, std::nullopt, &stopAfterPolls, &search_polls);
    auto [search_status, search_results] = index_->searchKnn(
        std::vector<float>{0.0f}, 8, nullptr, counting);
    ASSERT_TRUE(search_status.ok) << search_status.message;
    ASSERT_EQ(search_results.size(), 8u);
    ASSERT_GT(search_polls.polls.load(std::memory_order_relaxed), 0u);

    PollState state;
    state.stop_after = search_polls.polls.load(std::memory_order_relaxed) + 2;
    const themis::query::RequestContext ctx(nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto res = handler_->handleSearch(makeRequest({{"vector", {0.0}}, {"k", 8}}), ctx);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("pk-"), std::string::npos);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

TEST_F(VectorSearchDeadlineTest, ValidationStatusPrecedesDeadline) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleSearch(makeRequest({{"k", 3}}), expired);
    EXPECT_EQ(res.result(), http::status::bad_request);
}
