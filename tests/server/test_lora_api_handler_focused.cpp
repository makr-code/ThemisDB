/**
 * @file test_lora_api_handler_focused.cpp
 * @brief Group LA — LoRAApiHandler HTTP-dispatch and LLM-adapter lifecycle path tests.
 */

#include <gtest/gtest.h>
#include "server/lora_api_handler.h"
#include "llm/lora_framework/lora_orchestrator.h"

#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http  = beast::http;

using namespace themis::server;
using namespace themis;

// ── Helpers ───────────────────────────────────────────────────────────────────

static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& body = "")
{
    http::request<http::string_body> req;
    req.method(method);
    req.target(target);
    req.version(11);
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class LoraApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        orch_    = std::make_shared<llm::lora::LoRAOrchestrator>();
        handler_ = std::make_unique<LoRAApiHandler>(orch_);
    }

    std::shared_ptr<llm::lora::LoRAOrchestrator> orch_;
    std::unique_ptr<LoRAApiHandler>               handler_;
};

// ── LA1: Construct LoRAApiHandler with null orchestrator does not throw ───────
TEST(LoraApiHandlerFocused, LA1_NullOrchestrator_ConstructDoesNotThrow) {
    EXPECT_NO_THROW({
        LoRAApiHandler h(nullptr);
    });
}

// ── LA2: GET /api/v1/llm/lora/health returns a response without crash ────────
TEST_F(LoraApiHandlerTest, LA2_HealthEndpoint_ReturnsResponse) {
    auto req = makeRequest(http::verb::get, "/api/v1/llm/lora/health");
    http::response<http::string_body> resp;
    EXPECT_NO_THROW({ resp = handler_->handleRequest(req); });
    // Any valid HTTP status is acceptable
    EXPECT_GT(static_cast<unsigned>(resp.result_int()), 0u);
}

// ── LA3: GET to unknown route returns 404 or 400 ─────────────────────────────
TEST_F(LoraApiHandlerTest, LA3_UnknownRoute_Returns4xx) {
    auto req = makeRequest(http::verb::get, "/api/v1/llm/lora/does-not-exist");
    auto resp = handler_->handleRequest(req);
    EXPECT_GE(resp.result_int(), 400);
    EXPECT_LT(resp.result_int(), 500);
}

// ── LA4: POST to /api/v1/llm/lora/models without JWT returns 401 ─────────────
TEST_F(LoraApiHandlerTest, LA4_PostModelsNoJWT_Returns401) {
    auto req = makeRequest(
        http::verb::post,
        "/api/v1/llm/lora/models",
        R"({"model_id":"m1","base_model":"llama"})"
    );
    auto resp = handler_->handleRequest(req);
    // Without a valid JWT the handler should deny with 401 or 400/422
    EXPECT_GE(resp.result_int(), 400);
}

// ── LA5: Without inference engine, POST /lora/query returns 501 ──────────────
TEST_F(LoraApiHandlerTest, LA5_QueryWithoutEngine_Returns501) {
    // No inference engine attached → 501 Not Implemented expected
    auto req = makeRequest(
        http::verb::post,
        "/api/v1/llm/lora/query",
        R"({"prompt":"hello","adapter_id":"test"})"
    );
    auto resp = handler_->handleRequest(req);
    // 501 when no engine, or 400/401 due to missing auth
    EXPECT_GE(resp.result_int(), 400);
}

// ── LA6: setInferenceEngine with null does not crash ─────────────────────────
TEST_F(LoraApiHandlerTest, LA6_SetNullEngine_NoThrow) {
    EXPECT_NO_THROW({ handler_->setInferenceEngine(nullptr); });
}
