/**
 * @file test_api_fusion_search_deadline.cpp
 * @brief Focused tests for cooperative deadlines on POST /search/fusion (QRY-07).
 */

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "index/vector_index.h"
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
    auto* state = const_cast<PollState*>(static_cast<const PollState*>(opaque));
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class FusionSearchDeadlineTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::current_path() / ".qry07-api-test-data" /
                   ("fusion_search_" + std::to_string(suffix));
        std::filesystem::create_directories(db_path_.parent_path());

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*db_);
        const auto index_status = secondary_index_->createFulltextIndex("articles", "content");
        ASSERT_TRUE(index_status.ok) << index_status.message;
        for (int i = 0; i < 128; ++i) {
            themis::BaseEntity entity("article-" + std::to_string(i));
            entity.setField("content", "fusiondeadline indexed cancellation coverage");
            const auto put_status = secondary_index_->put("articles", entity);
            ASSERT_TRUE(put_status.ok) << put_status.message;
        }

        vector_index_ = std::make_shared<themis::VectorIndexManager>(*db_);
        handler_ = std::make_unique<themis::server::ContentApiHandler>(
            db_, nullptr, nullptr, nullptr, secondary_index_, vector_index_);
    }

    void TearDown() override {
        handler_.reset();
        vector_index_.reset();
        secondary_index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    static http::request<http::string_body> makeRequest(const nlohmann::json& body) {
        http::request<http::string_body> req{http::verb::post, "/search/fusion", 11};
        req.body() = body.dump();
        req.prepare_payload();
        return req;
    }

    static nlohmann::json textOnlyRequest(std::string query) {
        return {
            {"table", "articles"},
            {"text_column", "content"},
            {"text_query", std::move(query)},
            {"fusion_mode", "rrf"}
        };
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::unique_ptr<themis::server::ContentApiHandler> handler_;
};

TEST_F(FusionSearchDeadlineTest, EmptyTextResultsRemainSuccessful) {
    const auto res = handler_->handleFusionSearch(makeRequest(textOnlyRequest("notpresenttoken")));

    ASSERT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(nlohmann::json::parse(res.body())["count"], 0);
}

TEST_F(FusionSearchDeadlineTest, ExpiredDeadlineReturns504WithoutResults) {
    const themis::query::RequestContext expired(
        nullptr, themis::query::RequestContext::Clock::now() - std::chrono::milliseconds{1});
    const auto res = handler_->handleFusionSearch(
        makeRequest(textOnlyRequest("fusiondeadline")), expired);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

TEST_F(FusionSearchDeadlineTest, CancellationDuringTextStageDiscardsPartialResults) {
    PollState state;
    state.stop_after = 3;  // Entry, token stage, then the 64-entry prefix-scan poll.
    const themis::query::RequestContext request(nullptr, std::nullopt, &stopAfterPolls, &state);
    const auto res = handler_->handleFusionSearch(
        makeRequest(textOnlyRequest("fusiondeadline")), request);

    ASSERT_EQ(res.result(), http::status::gateway_timeout);
    EXPECT_EQ(res.body().find("article-"), std::string::npos);
    EXPECT_EQ(res.body().find("\"results\""), std::string::npos);
}

} // namespace
