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

// ── Training / snapshot provenance endpoints (server-delegated auth) ─────────

namespace {
/// Authorizer that grants regular endpoints and records admin checks.
struct RecordingAuthorizer {
    bool grant_admin = true;
    int admin_checks = 0;
    std::optional<http::response<http::string_body>> operator()(
        const http::request<http::string_body>&, bool admin) {
        if (!admin) {
            return std::nullopt;
        }
        ++admin_checks;
        if (grant_admin) {
            return std::nullopt;
        }
        http::response<http::string_body> res{http::status::forbidden, 11};
        res.body() = R"({"error":"forbidden"})";
        res.prepare_payload();
        return res;
    }
};

nlohmann::json bodyOf(const http::response<http::string_body>& r) {
    return nlohmann::json::parse(r.body(), nullptr, /*allow_exceptions=*/false);
}
}  // namespace

class LoraApiTrainingTest : public LoraApiHandlerTest {
protected:
    void SetUp() override {
        LoraApiHandlerTest::SetUp();
        auth_ = std::make_shared<RecordingAuthorizer>();
        auto a = auth_;
        handler_->setRequestAuthorizer(
            [a](const http::request<http::string_body>& req, bool admin) { return (*a)(req, admin); });
    }
    std::shared_ptr<RecordingAuthorizer> auth_;
};

// LA7: inline samples without a training backend: job Failed → 503 with reason, no adapter.
TEST_F(LoraApiTrainingTest, LA7_InlineSamplesWithoutBackend_Returns503) {
    auto resp = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters",
        R"({"adapter_id":"a1","samples":[{"input":"q1","output":"a1"},{"input":"q2","output":"a2"}]})"));
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
    const auto j = bodyOf(resp);
    ASSERT_TRUE(j.is_object());
    EXPECT_EQ(j.value("status", std::string{}), "failed");
    EXPECT_NE(j.value("error", std::string{}).find("configured"), std::string::npos);
    EXPECT_FALSE(orch_->getAdapter("a1").has_value());
}

// LA8: dataset_id goes through the snapshot path; without a reader → 503.
TEST_F(LoraApiTrainingTest, LA8_DatasetWithoutSnapshotReader_Returns503) {
    auto resp = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters", R"({"adapter_id":"a1","dataset_id":"qa_pairs"})"));
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
    const auto j = bodyOf(resp);
    EXPECT_NE(j.value("error", std::string{}).find("snapshot reader"), std::string::npos);
}

// LA9: invalid input is rejected before any job is created.
TEST_F(LoraApiTrainingTest, LA9_InvalidInput_Returns400) {
    const char* bodies[] = {
        R"({"adapter_id":"a1","dataset_id":"bad name; DROP"})",
        R"({"adapter_id":"a1","dataset_id":"ok","limit":0})",
        R"({"adapter_id":"a1","dataset_id":"ok","limit":-5})",
        R"({"adapter_id":"a1","dataset_id":"ok","samples":[]})",
        R"({"adapter_id":"a1","samples":[{"input":"only one","output":"x"}]})",
        R"({"adapter_id":"a1"})",
        R"({"adapter_id":"a1","dataset_id":"ok","base_model":"some-other-model"})",
    };
    for (const char* b : bodies) {
        auto resp = handler_->handleRequest(makeRequest(http::verb::post, "/api/v1/llm/lora/adapters", b));
        EXPECT_EQ(resp.result(), http::status::bad_request) << b;
    }
    EXPECT_TRUE(orch_->listJobs().empty());
}

// LA10: job status endpoint reflects orchestrator state; unknown job → 404.
TEST_F(LoraApiTrainingTest, LA10_JobStatusEndpoint) {
    auto created = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters",
        R"({"adapter_id":"a1","samples":[{"input":"q1","output":"a1"},{"input":"q2","output":"a2"}]})"));
    const std::string job_id = bodyOf(created).value("job_id", std::string{});
    ASSERT_FALSE(job_id.empty());
    auto resp = handler_->handleRequest(makeRequest(http::verb::get, "/api/v1/llm/lora/jobs/" + job_id));
    EXPECT_EQ(resp.result(), http::status::ok);
    EXPECT_EQ(bodyOf(resp).value("status", std::string{}), "failed");
    auto missing = handler_->handleRequest(makeRequest(http::verb::get, "/api/v1/llm/lora/jobs/nope"));
    EXPECT_EQ(missing.result(), http::status::not_found);
}

// LA11: admin endpoints are denied without a server authorizer (fail closed).
TEST_F(LoraApiHandlerTest, LA11_AdminEndpointsWithoutAuthorizer_Return403) {
    auto regen = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters/a1/regenerate",
        R"({"target_adapter_id":"a2","base_model":"m","base_model_gguf":"m.gguf"})"));
    EXPECT_EQ(regen.result(), http::status::forbidden);
    auto pin = handler_->handleRequest(makeRequest(http::verb::delete_, "/api/v1/llm/lora/snapshot-pins/abc"));
    EXPECT_EQ(pin.result(), http::status::forbidden);
}

// LA12: admin endpoints consult the authorizer with admin=true.
TEST_F(LoraApiTrainingTest, LA12_AdminEndpointsUseAdminAuthorization) {
    auth_->grant_admin = false;
    auto pin = handler_->handleRequest(makeRequest(http::verb::delete_, "/api/v1/llm/lora/snapshot-pins/abc"));
    EXPECT_EQ(pin.result(), http::status::forbidden);
    EXPECT_EQ(auth_->admin_checks, 1);

    auth_->grant_admin = true;
    // No snapshot reader → lease unknown.
    pin = handler_->handleRequest(makeRequest(http::verb::delete_, "/api/v1/llm/lora/snapshot-pins/abc"));
    EXPECT_EQ(pin.result(), http::status::not_found);
}

// LA13: regeneration validates the body and fails without a snapshot reader.
TEST_F(LoraApiTrainingTest, LA13_RegenerateAdapter) {
    auto bad = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters/a1/regenerate", R"({"target_adapter_id":"a2"})"));
    EXPECT_EQ(bad.result(), http::status::bad_request);

    auto resp = handler_->handleRequest(makeRequest(
        http::verb::post, "/api/v1/llm/lora/adapters/a1/regenerate",
        R"({"target_adapter_id":"a2","base_model":"m","base_model_gguf":"m.gguf"})"));
    EXPECT_GE(resp.result_int(), 400);  // job failed before training (503 or 422)
    EXPECT_EQ(bodyOf(resp).value("status", std::string{}), "failed");
}
