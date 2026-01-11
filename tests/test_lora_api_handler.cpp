#include <gtest/gtest.h>
#include "server/lora_api_handler.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include <nlohmann/json.hpp>

using namespace themis::server;
using namespace themis::llm::lora;
using json = nlohmann::json;

namespace {

// Helper to create HTTP request
http::request<http::string_body> createRequest(
    http::verb method,
    std::string_view target,
    const std::string& body = "") {
    
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

} // anonymous namespace

class LoRAApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create orchestrator with minimal config
        LoRAOrchestrator::Config config;
        orchestrator_ = std::make_shared<LoRAOrchestrator>(config);
        
        // Create handler without JWT validation for testing
        handler_ = std::make_unique<LoRAApiHandler>(orchestrator_, std::nullopt);
    }
    
    std::shared_ptr<LoRAOrchestrator> orchestrator_;
    std::unique_ptr<LoRAApiHandler> handler_;
};

TEST_F(LoRAApiHandlerTest, HealthEndpointReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/health");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
    
    // Parse response body
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("error"));
    EXPECT_EQ(body["error"], "Unauthorized");
}

TEST_F(LoRAApiHandlerTest, StatsEndpointReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/stats");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
    
    // Parse response body
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("error"));
    EXPECT_EQ(body["error"], "Unauthorized");
}

TEST_F(LoRAApiHandlerTest, InvalidEndpointReturnsUnauthorized) {
    // Invalid endpoint still checks auth first
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/invalid");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, CreateAdapterWithoutBodyReturnsUnauthorized) {
    // Without JWT token, should return 401 before checking body
    auto req = createRequest(http::verb::post, "/api/v1/llm/lora/adapters");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, ListAdaptersReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/adapters");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, QueryEndpointReturnsUnauthorized) {
    // Without JWT token, should return 401
    json query_body = {
        {"adapter_id", "test_adapter"},
        {"prompt", "test prompt"}
    };
    
    auto req = createRequest(http::verb::post, "/api/v1/llm/lora/query", query_body.dump());
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, LoadAdapterReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::post, "/api/v1/llm/lora/adapters/test_adapter/load");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, UnloadAdapterReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::post, "/api/v1/llm/lora/adapters/test_adapter/unload");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, GetAdapterStatusReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/adapters/test_adapter/status");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, GetAdapterReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::get, "/api/v1/llm/lora/adapters/test_adapter");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, UpdateAdapterReturnsUnauthorized) {
    // Without JWT token, should return 401
    json update_body = {
        {"additional_training_data", {
            {"dataset_id", "new_dataset"}
        }}
    };
    
    auto req = createRequest(http::verb::put, "/api/v1/llm/lora/adapters/test_adapter", update_body.dump());
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, DeleteAdapterReturnsUnauthorized) {
    // Without JWT token, should return 401
    auto req = createRequest(http::verb::delete_, "/api/v1/llm/lora/adapters/test_adapter");
    auto res = handler_->handleRequest(req);
    
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(LoRAApiHandlerTest, ModelManagementReturnsUnauthorized) {
    // Test model endpoints without JWT
    auto req1 = createRequest(http::verb::post, "/api/v1/llm/models");
    auto res1 = handler_->handleRequest(req1);
    EXPECT_EQ(res1.result(), http::status::unauthorized);
    
    auto req2 = createRequest(http::verb::get, "/api/v1/llm/models");
    auto res2 = handler_->handleRequest(req2);
    EXPECT_EQ(res2.result(), http::status::unauthorized);
    
    auto req3 = createRequest(http::verb::get, "/api/v1/llm/models/test_model");
    auto res3 = handler_->handleRequest(req3);
    EXPECT_EQ(res3.result(), http::status::unauthorized);
    
    auto req4 = createRequest(http::verb::delete_, "/api/v1/llm/models/test_model");
    auto res4 = handler_->handleRequest(req4);
    EXPECT_EQ(res4.result(), http::status::unauthorized);
}

// Note: Tests with JWT authentication would require setting up JWTValidator
// with valid configuration, which would be done in integration tests

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
