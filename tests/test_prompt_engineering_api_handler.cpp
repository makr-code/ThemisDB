/**
 * @file test_prompt_engineering_api_handler.cpp
 * @brief Unit tests for PromptEngineeringApiHandler
 */

#include <gtest/gtest.h>
#include "server/prompt_engineering_api_handler.h"
#include <memory>

using namespace themis;
using namespace themis::server;

class PromptEngineeringApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: These are null pointers for basic testing
        // In a real scenario, we'd use mocks or test doubles
        storage_ = nullptr;
        manager_ = nullptr;
        optimizer_ = nullptr;
        tracker_ = nullptr;
        orchestrator_ = nullptr;
        feedback_collector_ = nullptr;
        version_control_ = nullptr;
        integration_ = nullptr;
        auth_ = nullptr;

        handler_ = std::make_unique<PromptEngineeringApiHandler>(
            storage_,
            manager_,
            optimizer_,
            tracker_,
            orchestrator_,
            feedback_collector_,
            version_control_,
            integration_,
            auth_
        );
    }

    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<prompt_engineering::PromptManager> manager_;
    std::shared_ptr<prompt_engineering::PromptOptimizer> optimizer_;
    std::shared_ptr<prompt_engineering::PromptPerformanceTracker> tracker_;
    std::shared_ptr<prompt_engineering::SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<prompt_engineering::PromptVersionControl> version_control_;
    std::shared_ptr<prompt_engineering::PromptEngineeringIntegration> integration_;
    std::shared_ptr<AuthMiddleware> auth_;
    
    std::unique_ptr<PromptEngineeringApiHandler> handler_;
};

TEST_F(PromptEngineeringApiHandlerTest, Construction) {
    // Simply test that the handler can be constructed
    EXPECT_NE(handler_, nullptr);
}

TEST_F(PromptEngineeringApiHandlerTest, OptimizeWithoutOrchestrator) {
    // Create a request
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/prompt_engineering/optimize");
    req.body() = R"({"prompt_id": "test_prompt"})";
    req.prepare_payload();

    // Call handler (should return service unavailable since orchestrator is null)
    auto response = handler_->handleOptimize(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
    EXPECT_TRUE(response.body().find("SelfImprovementOrchestrator not available") != std::string::npos);
}

TEST_F(PromptEngineeringApiHandlerTest, ListABTestsWithoutOrchestrator) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/prompt_engineering/ab_tests");

    auto response = handler_->handleListABTests(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
}

TEST_F(PromptEngineeringApiHandlerTest, SubmitFeedbackWithoutCollector) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/prompt_engineering/feedback");
    req.body() = R"({
        "prompt_id": "test_prompt",
        "query": "test query",
        "response": "test response",
        "type": "USER_POSITIVE"
    })";
    req.prepare_payload();

    auto response = handler_->handleSubmitFeedback(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
    EXPECT_TRUE(response.body().find("FeedbackCollector not available") != std::string::npos);
}

TEST_F(PromptEngineeringApiHandlerTest, GetStatsWithNullComponents) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/prompt_engineering/stats");

    auto response = handler_->handleGetStats(req);
    
    // Should return 200 OK with empty/partial stats
    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response.body().find("{") != std::string::npos); // Valid JSON
}

TEST_F(PromptEngineeringApiHandlerTest, GetHistoryWithoutOrchestrator) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/prompt_engineering/history/test_prompt");

    auto response = handler_->handleGetHistory(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
}

    TEST_F(PromptEngineeringApiHandlerTest, GetABTestRejectsInvalidTestId) {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.method(boost::beast::http::verb::get);
        req.target("/api/v1/prompt_engineering/ab_tests/../bad");

        auto response = handler_->handleGetABTest(req);

        EXPECT_EQ(response.result(), boost::beast::http::status::bad_request);
        EXPECT_TRUE(response.body().find("Invalid test_id") != std::string::npos);
    }

    TEST_F(PromptEngineeringApiHandlerTest, GetHistoryRejectsInvalidPromptId) {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.method(boost::beast::http::verb::get);
        req.target("/api/v1/prompt_engineering/history/../bad");

        auto response = handler_->handleGetHistory(req);

        EXPECT_EQ(response.result(), boost::beast::http::status::bad_request);
        EXPECT_TRUE(response.body().find("Invalid prompt_id") != std::string::npos);
    }

TEST_F(PromptEngineeringApiHandlerTest, GetVersionsWithoutVersionControl) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/prompt_engineering/versions/test_prompt");

    auto response = handler_->handleGetVersions(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
    EXPECT_TRUE(response.body().find("PromptVersionControl not available") != std::string::npos);
}

TEST_F(PromptEngineeringApiHandlerTest, GetVersionsRejectsInvalidPromptId) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/prompt_engineering/versions/../bad");

    auto response = handler_->handleGetVersions(req);

    EXPECT_EQ(response.result(), boost::beast::http::status::bad_request);
    EXPECT_TRUE(response.body().find("Invalid prompt_id") != std::string::npos);
}

TEST_F(PromptEngineeringApiHandlerTest, RollbackWithoutOrchestrator) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/prompt_engineering/rollback");
    req.body() = R"({"prompt_id": "test_prompt"})";
    req.prepare_payload();

    auto response = handler_->handleRollback(req);
    
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
}

TEST_F(PromptEngineeringApiHandlerTest, OptimizeWithInvalidJSON) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/prompt_engineering/optimize");
    req.body() = "not valid json{";
    req.prepare_payload();

    // Even though orchestrator is null, should fail on JSON parsing first
    auto response = handler_->handleOptimize(req);
    
    // Will return service unavailable first since null check happens before parse
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
}

TEST_F(PromptEngineeringApiHandlerTest, FeedbackWithMissingPromptId) {
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/prompt_engineering/feedback");
    req.body() = R"({
        "query": "test query",
        "response": "test response",
        "type": "USER_POSITIVE"
    })";
    req.prepare_payload();

    auto response = handler_->handleSubmitFeedback(req);
    
    // Should return service unavailable since collector is null
    // (null check happens before validation)
    EXPECT_EQ(response.result(), boost::beast::http::status::service_unavailable);
}

// Note: Comprehensive integration tests with actual components
// would require a full test harness with mocked dependencies
