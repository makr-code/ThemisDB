/*
 * Focused validation tests for FeedbackAPIHandler input hardening.
 */

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

#include <boost/beast/http.hpp>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "llm/lora_framework/lora_feedback_storage.h"
#include "performance/phase3/bao.h"
#include "performance/workload_adaptive_optimizer.h"
#include "prompt_engineering/feedback_collector.h"
#include "rag/continuous_learning_orchestrator.h"
#include "server/feedback_api_handler.h"
#include "storage/rocksdb_wrapper.h"

namespace http = boost::beast::http;
using json = nlohmann::json;

namespace themis { 
namespace server { 
namespace {

http::request<http::string_body> makeRequest(
    const http::verb method,
    const std::string& target,
    const std::string& body = "")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

class FeedbackApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() /
                    std::filesystem::path("themis_feedback_api_handler_test");
        std::filesystem::remove_all(temp_dir_);
        std::filesystem::create_directories(temp_dir_);

        themis::RocksDBWrapper::Config db_config;
        db_config.db_path = temp_dir_.string();
        db_config.enable_blobdb = false;

        db_ = std::make_shared<themis::RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_->open());

        themis::llm::lora::FeedbackStorageService::Config storage_config;
        storage_config.db = db_;
        storage_config.enable_graph_links = false;
        storage_ = std::make_shared<themis::llm::lora::FeedbackStorageService>(storage_config);
        handler_ = std::make_unique<FeedbackAPIHandler>(storage_);
        live_feedback_collector_ = std::make_shared<themis::prompt_engineering::FeedbackCollector>();
        orchestrator_ = std::make_shared<themis::rag::learning::ContinuousLearningOrchestrator>(
            themis::rag::learning::ContinuousLearningConfig{});
        orchestrator_->wireLiveSignalProviders(
            std::make_shared<themis::performance::phase3::BaoOptimizer>(),
            std::make_shared<themis::performance::WorkloadAdaptiveOptimizer>(),
            live_feedback_collector_);
        handler_->setLiveFeedbackCollector(live_feedback_collector_);
        handler_->setLearningOrchestrator(orchestrator_);
    }

    void TearDown() override {
        handler_.reset();
        storage_.reset();
        db_.reset();
        std::filesystem::remove_all(temp_dir_);
    }

    json makeValidFeedbackBody() const {
        return json{
            {"adapter_id", "adapter-123"},
            {"user_id", "user-123"},
            {"rating", 4},
            {"feedback_text", "useful"},
            {"prompt", "hello"},
            {"response", "world"}
        };
    }

    std::filesystem::path temp_dir_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<themis::llm::lora::FeedbackStorageService> storage_;
    std::shared_ptr<themis::prompt_engineering::FeedbackCollector> live_feedback_collector_;
    std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> orchestrator_;
    std::unique_ptr<FeedbackAPIHandler> handler_;
};

TEST_F(FeedbackApiHandlerTest, CreateRejectsInvalidAdapterId) {
    auto body = makeValidFeedbackBody();
    body["adapter_id"] = "../escape";

    const auto res = handler_->handleCreateFeedback(
        makeRequest(http::verb::post, "/api/feedback", body.dump()));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, GetRejectsInvalidFeedbackId) {
    const auto res = handler_->handleGetFeedback(
        makeRequest(http::verb::get, "/api/feedback/../bad"),
        "../bad");

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, ListRejectsInvalidAdapterFilter) {
    const auto res = handler_->handleListFeedback(
        makeRequest(http::verb::get, "/api/feedback?adapter_id=../bad"));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, ListRejectsMalformedLimit) {
    const auto res = handler_->handleListFeedback(
        makeRequest(http::verb::get, "/api/feedback?limit=abc"));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, AdapterEndpointRejectsInvalidAdapterId) {
    const auto res = handler_->handleGetAdapterFeedback(
        makeRequest(http::verb::get, "/api/feedback/adapter/../bad"),
        "../bad");

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, StatisticsRejectInvalidAdapterId) {
    const auto res = handler_->handleGetStatistics(
        makeRequest(http::verb::get, "/api/feedback/stats?adapter_id=bad%0d%0aX-Test:1"));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(FeedbackApiHandlerTest, CreateMirrorsIntoLiveCollectorAndTriggersLoop4) {
    const auto res = handler_->handleCreateFeedback(
        makeRequest(http::verb::post, "/api/feedback", makeValidFeedbackBody().dump()));

    ASSERT_EQ(res.result(), http::status::created);
    ASSERT_EQ(live_feedback_collector_->newEntryCount(), 1u);

    const auto context = json::parse(orchestrator_->serializeLoopContext());
    ASSERT_TRUE(context.contains("loops"));
    ASSERT_FALSE(context["loops"].empty());
    const auto loop4_it = std::find_if(
        context["loops"].begin(),
        context["loops"].end(),
        [](const json& item) { return item.value("phase", "") == "LOOP_4_RLAIF"; });
    ASSERT_NE(loop4_it, context["loops"].end());
    const auto& loop4 = *loop4_it;
    EXPECT_EQ(loop4.value("signal_source", ""), "live");
    EXPECT_DOUBLE_EQ(loop4.value("signal_value", -1.0), 1.0);
}

} // namespace

}
} // namespace themis::server