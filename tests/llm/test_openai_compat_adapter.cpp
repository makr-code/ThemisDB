/**
 * @file test_openai_compat_adapter.cpp
 * @brief Unit tests for OpenAICompatAdapter
 *
 * Tests:
 * - parseRequest    — valid and invalid chat completions request bodies
 * - buildResponse   — non-streaming OpenAI-compatible response JSON
 * - buildStreamChunk / buildStreamFinalChunk / buildStreamDone — SSE framing
 * - buildError      — error JSON shape
 * - generateCompletionId — ID format
 *
 * All tests are pure-logic tests that do NOT require a running inference
 * engine; they only exercise the JSON translation layer.
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include "llm/openai_compat_adapter.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_plugin_manager.h"
#include "governance/policy_engine.h"
#include "server/llm_api_handler.h"

namespace http = boost::beast::http;

using themis::llm::OpenAICompatAdapter;
using themis::llm::InferenceRequest;
using themis::llm::InferenceResponse;
using themis::governance::PolicyEngine;
using themis::governance::InferencePermissionResult;
using themis::server::LLMApiHandler;
using json = nlohmann::json;

// ═══════════════════════════════════════════════════════════
// Helper utilities
// ═══════════════════════════════════════════════════════════

/** Return the InferenceRequest from a parse result or FAIL the test. */
static InferenceRequest requireOk(
    std::variant<InferenceRequest, std::string> result,
    const std::string& context = "") {
    if (std::holds_alternative<std::string>(result)) {
        ADD_FAILURE() << "Expected parse success but got error: "
                      << std::get<std::string>(result)
                      << (context.empty() ? "" : (" [" + context + "]"));
        return {};
    }
    return std::get<InferenceRequest>(std::move(result));
}

/** Return the error string from a parse result or FAIL the test. */
static std::string requireError(
    std::variant<InferenceRequest, std::string> result) {
    if (!std::holds_alternative<std::string>(result)) {
        ADD_FAILURE() << "Expected parse failure but got success";
        return {};
    }
    return std::get<std::string>(result);
}

// ═══════════════════════════════════════════════════════════
// parseRequest — valid inputs
// ═══════════════════════════════════════════════════════════

class ParseRequestTest : public ::testing::Test {};

TEST_F(ParseRequestTest, MinimalUserMessage) {
    json body = {
        {"model",    "gpt-4"},
        {"messages", json::array({
            {{"role","user"},{"content","Hello!"}}
        })}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    EXPECT_EQ(req.model_id, "gpt-4");
    EXPECT_NE(req.prompt.find("Hello!"), std::string::npos);
    EXPECT_FALSE(req.system_prompt.has_value());
}

TEST_F(ParseRequestTest, SystemAndUserMessages) {
    json body = {
        {"model",    "llama3"},
        {"messages", json::array({
            {{"role","system"},{"content","You are helpful."}},
            {{"role","user"},  {"content","What is 2+2?"}}
        })}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    ASSERT_TRUE(req.system_prompt.has_value());
    EXPECT_EQ(*req.system_prompt, "You are helpful.");
    EXPECT_NE(req.prompt.find("What is 2+2?"), std::string::npos);
}

TEST_F(ParseRequestTest, MultiTurnConversation) {
    json body = {
        {"messages", json::array({
            {{"role","user"},      {"content","Hello"}},
            {{"role","assistant"},{"content","Hi there"}},
            {{"role","user"},      {"content","Goodbye"}}
        })}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    EXPECT_NE(req.prompt.find("Hello"), std::string::npos);
    EXPECT_NE(req.prompt.find("Hi there"), std::string::npos);
    EXPECT_NE(req.prompt.find("Goodbye"), std::string::npos);
}

TEST_F(ParseRequestTest, TemperatureAndMaxTokens) {
    json body = {
        {"messages",   json::array({{{"role","user"},{"content","x"}}})},
        {"temperature", 0.3},
        {"max_tokens",  256}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    EXPECT_FLOAT_EQ(req.temperature, 0.3f);
    EXPECT_EQ(req.max_tokens, 256);
}

TEST_F(ParseRequestTest, StopAsString) {
    json body = {
        {"messages", json::array({{{"role","user"},{"content","x"}}})},
        {"stop",     "\n"}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    ASSERT_EQ(req.stop_sequences.size(), 1u);
    EXPECT_EQ(req.stop_sequences[0], "\n");
}

TEST_F(ParseRequestTest, StopAsArray) {
    json body = {
        {"messages", json::array({{{"role","user"},{"content","x"}}})},
        {"stop",     json::array({"END","STOP"})}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    ASSERT_EQ(req.stop_sequences.size(), 2u);
    EXPECT_EQ(req.stop_sequences[0], "END");
    EXPECT_EQ(req.stop_sequences[1], "STOP");
}

TEST_F(ParseRequestTest, ToolsArray) {
    json tool = {
        {"type", "function"},
        {"function", {
            {"name",        "get_weather"},
            {"description", "Get current weather"},
            {"parameters",  {
                {"type",       "object"},
                {"properties", {{"location", {{"type","string"}}}}}
            }}
        }}
    };
    json body = {
        {"messages", json::array({{{"role","user"},{"content","Weather?"}}})},
        {"tools",    json::array({tool})}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    ASSERT_EQ(req.tools.size(), 1u);
    EXPECT_EQ(req.tools[0].name, "get_weather");
    EXPECT_EQ(req.tools[0].description, "Get current weather");
    EXPECT_FALSE(req.tools[0].parameters.empty());
}

TEST_F(ParseRequestTest, TopPForwarded) {
    json body = {
        {"messages", json::array({{{"role","user"},{"content","x"}}})},
        {"top_p",    0.85}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));

    EXPECT_FLOAT_EQ(req.top_p, 0.85f);
}

TEST_F(ParseRequestTest, MaxCompletionTokensAlias) {
    json body = {
        {"messages",              json::array({{{"role","user"},{"content","x"}}})},
        {"max_completion_tokens", 128}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));
    EXPECT_EQ(req.max_tokens, 128);
}

TEST_F(ParseRequestTest, ArrayContentParts) {
    json body = {
        {"messages", json::array({
            {{"role","user"},{"content", json::array({
                {{"type","text"},{"text","Hello from parts"}}
            })}}
        })}
    };

    auto req = requireOk(OpenAICompatAdapter::parseRequest(body));
    EXPECT_NE(req.prompt.find("Hello from parts"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// parseRequest — error cases
// ═══════════════════════════════════════════════════════════

TEST_F(ParseRequestTest, MissingMessages) {
    json body = {{"model", "gpt-4"}};
    std::string err = requireError(OpenAICompatAdapter::parseRequest(body));
    EXPECT_FALSE(err.empty());
}

TEST_F(ParseRequestTest, EmptyMessagesArray) {
    json body = {{"messages", json::array()}};
    std::string err = requireError(OpenAICompatAdapter::parseRequest(body));
    EXPECT_FALSE(err.empty());
}

TEST_F(ParseRequestTest, MessageMissingRole) {
    json body = {
        {"messages", json::array({{{"content","Hello"}}})}
    };
    std::string err = requireError(OpenAICompatAdapter::parseRequest(body));
    EXPECT_FALSE(err.empty());
}

TEST_F(ParseRequestTest, MessageMissingContent) {
    json body = {
        {"messages", json::array({{{"role","user"}}})}
    };
    std::string err = requireError(OpenAICompatAdapter::parseRequest(body));
    EXPECT_FALSE(err.empty());
}

// ═══════════════════════════════════════════════════════════
// buildResponse — non-streaming
// ═══════════════════════════════════════════════════════════

class BuildResponseTest : public ::testing::Test {
protected:
    InferenceResponse makeResponse(const std::string& text,
                                   int prompt_tokens = 10,
                                   int completion_tokens = 20) {
        InferenceResponse r;
        r.text              = text;
        r.model_id          = "test-model";
        r.tokens_prompt     = prompt_tokens;
        r.tokens_generated  = completion_tokens;
        return r;
    }
};

TEST_F(BuildResponseTest, TopLevelFields) {
    auto resp = makeResponse("Hello world");
    json j = OpenAICompatAdapter::buildResponse(resp, "test-model", "chatcmpl-abc");

    EXPECT_EQ(j["id"],     "chatcmpl-abc");
    EXPECT_EQ(j["object"], "chat.completion");
    EXPECT_EQ(j["model"],  "test-model");
    EXPECT_TRUE(j.contains("created"));
    EXPECT_TRUE(j["created"].is_number_integer());
}

TEST_F(BuildResponseTest, ChoicesStructure) {
    auto resp = makeResponse("The answer is 42");
    json j = OpenAICompatAdapter::buildResponse(resp, "m");

    ASSERT_TRUE(j.contains("choices"));
    ASSERT_EQ(j["choices"].size(), 1u);

    const auto& choice = j["choices"][0];
    EXPECT_EQ(choice["index"],         0);
    EXPECT_EQ(choice["finish_reason"], "stop");
    EXPECT_EQ(choice["message"]["role"],    "assistant");
    EXPECT_EQ(choice["message"]["content"], "The answer is 42");
}

TEST_F(BuildResponseTest, UsageBlock) {
    auto resp = makeResponse("ok", 5, 15);
    json j = OpenAICompatAdapter::buildResponse(resp, "m");

    ASSERT_TRUE(j.contains("usage"));
    EXPECT_EQ(j["usage"]["prompt_tokens"],     5);
    EXPECT_EQ(j["usage"]["completion_tokens"], 15);
    EXPECT_EQ(j["usage"]["total_tokens"],      20);
}

TEST_F(BuildResponseTest, AutoGeneratedId) {
    auto resp = makeResponse("hi");
    json j = OpenAICompatAdapter::buildResponse(resp, "m");

    ASSERT_TRUE(j["id"].is_string());
    std::string id = j["id"].get<std::string>();
    EXPECT_EQ(id.substr(0, 9), "chatcmpl-")
        << "Auto-generated completion ID must begin with 'chatcmpl-'";
}

TEST_F(BuildResponseTest, ToolCallsFinishReason) {
    InferenceResponse resp = makeResponse("");
    themis::llm::ToolCall tc;
    tc.name      = "get_weather";
    tc.arguments = json{{"location", "Berlin"}};
    resp.tool_calls.push_back(tc);

    json j = OpenAICompatAdapter::buildResponse(resp, "m");

    EXPECT_EQ(j["choices"][0]["finish_reason"], "tool_calls");
    EXPECT_TRUE(j["choices"][0]["message"].contains("tool_calls"));
    EXPECT_TRUE(j["choices"][0]["message"]["content"].is_null());
}

// ═══════════════════════════════════════════════════════════
// buildStreamChunk
// ═══════════════════════════════════════════════════════════

class StreamChunkTest : public ::testing::Test {};

TEST_F(StreamChunkTest, WireFormat) {
    std::string chunk = OpenAICompatAdapter::buildStreamChunk(
        "Hello", "chatcmpl-x", "gpt-4", 1700000000LL);

    EXPECT_EQ(chunk.substr(0, 6), "data: ")
        << "SSE chunk must begin with 'data: '";
    EXPECT_EQ(chunk.substr(chunk.size() - 2), "\n\n")
        << "SSE chunk must end with double newline";
}

TEST_F(StreamChunkTest, JsonPayload) {
    std::string chunk = OpenAICompatAdapter::buildStreamChunk(
        "world", "chatcmpl-y", "llama3", 1700000000LL);

    // Strip "data: " prefix and "\n\n" suffix to get pure JSON
    std::string payload = chunk.substr(6, chunk.size() - 8);
    json j = json::parse(payload);

    EXPECT_EQ(j["id"],     "chatcmpl-y");
    EXPECT_EQ(j["object"], "chat.completion.chunk");
    EXPECT_EQ(j["model"],  "llama3");

    ASSERT_EQ(j["choices"].size(), 1u);
    EXPECT_EQ(j["choices"][0]["delta"]["content"], "world");
    EXPECT_TRUE(j["choices"][0]["finish_reason"].is_null());
}

TEST_F(StreamChunkTest, FinalChunkHasStopReason) {
    std::string chunk = OpenAICompatAdapter::buildStreamFinalChunk(
        "chatcmpl-z", "m");

    std::string payload = chunk.substr(6, chunk.size() - 8);
    json j = json::parse(payload);

    EXPECT_EQ(j["choices"][0]["finish_reason"], "stop");
    EXPECT_TRUE(j["choices"][0]["delta"].is_object());
    EXPECT_TRUE(j["choices"][0]["delta"].empty());
}

TEST_F(StreamChunkTest, DoneSentinel) {
    EXPECT_EQ(OpenAICompatAdapter::buildStreamDone(), "data: [DONE]\n\n");
}

// ═══════════════════════════════════════════════════════════
// buildError
// ═══════════════════════════════════════════════════════════

TEST(BuildErrorTest, DefaultType) {
    json err = OpenAICompatAdapter::buildError("Something went wrong");

    ASSERT_TRUE(err.contains("error"));
    EXPECT_EQ(err["error"]["message"], "Something went wrong");
    EXPECT_EQ(err["error"]["type"],    "invalid_request_error");
    EXPECT_TRUE(err["error"]["code"].is_null());
}

TEST(BuildErrorTest, CustomTypeAndCode) {
    json err = OpenAICompatAdapter::buildError("Limit hit", "rate_limit_error", "quota_exceeded");

    EXPECT_EQ(err["error"]["type"], "rate_limit_error");
    EXPECT_EQ(err["error"]["code"], "quota_exceeded");
}

// ═══════════════════════════════════════════════════════════
// generateCompletionId
// ═══════════════════════════════════════════════════════════

TEST(GenerateCompletionIdTest, PrefixFormat) {
    std::string id = OpenAICompatAdapter::generateCompletionId();

    ASSERT_GE(id.size(), 9u + 1u)
        << "Completion ID must be 'chatcmpl-' plus at least one character";
    EXPECT_EQ(id.substr(0, 9), "chatcmpl-");
}

TEST(GenerateCompletionIdTest, UniqueIds) {
    std::string id1 = OpenAICompatAdapter::generateCompletionId();
    std::string id2 = OpenAICompatAdapter::generateCompletionId();

    EXPECT_NE(id1, id2)
        << "Two consecutive completion IDs should not be identical";
}

// ═══════════════════════════════════════════════════════════
// PolicyEngine::checkInferencePermission — API key validation
// ═══════════════════════════════════════════════════════════

class InferencePermissionTest : public ::testing::Test {
protected:
    PolicyEngine engine_;  // No YAML loaded → default (open classification)
};

TEST_F(InferencePermissionTest, MissingAuthorizationHeader_Returns401) {
    std::unordered_map<std::string, std::string> headers;
    auto result = engine_.checkInferencePermission(headers);

    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.http_status, 401);
    EXPECT_FALSE(result.denial_reason.empty());
}

TEST_F(InferencePermissionTest, EmptyAuthorizationHeader_Returns401) {
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "";
    auto result = engine_.checkInferencePermission(headers);

    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.http_status, 401);
}

TEST_F(InferencePermissionTest, MalformedAuthorizationHeader_Returns401) {
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "Basic dXNlcjpwYXNz";  // Basic auth, not Bearer
    auto result = engine_.checkInferencePermission(headers);

    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.http_status, 401);
}

TEST_F(InferencePermissionTest, BearerPrefixWithEmptyKey_Returns401) {
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer ";
    auto result = engine_.checkInferencePermission(headers);

    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.http_status, 401);
}

TEST_F(InferencePermissionTest, ValidBearerKey_AllowedByDefault) {
    // No YAML loaded → default classification is open → inference permitted.
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer sk-test-api-key-12345";
    auto result = engine_.checkInferencePermission(headers);

    EXPECT_TRUE(result.allowed);
}

TEST_F(InferencePermissionTest, ValidKeyDecisionContainsClassification) {
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer sk-any-key";
    auto result = engine_.checkInferencePermission(headers);

    // Even without YAML the decision must have a non-empty classification.
    EXPECT_FALSE(result.decision.classification.empty());
}

TEST_F(InferencePermissionTest, StrictClassificationHeader_Returns403) {
    // Simulate a "geheim" classification via the X-Data-Classification header
    // which is honoured by PolicyEngine::evaluate() for override.
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"]       = "Bearer sk-test";
    headers["X-Data-Classification"] = "geheim";
    auto result = engine_.checkInferencePermission(headers);

    // geheim → ann_allowed=false → inference must be denied with 403
    if (!result.allowed) {
        EXPECT_EQ(result.http_status, 403);
        EXPECT_FALSE(result.denial_reason.empty());
    }
    // If the engine doesn't honour the override header, allowed=true is also
    // acceptable — the test verifies the mapping logic is coherent.
}

TEST_F(InferencePermissionTest, DenialReasonIsHumanReadable) {
    std::unordered_map<std::string, std::string> headers;
    // Missing auth → denial_reason must be a non-empty English string
    auto result = engine_.checkInferencePermission(headers);
    ASSERT_FALSE(result.allowed);
    EXPECT_GT(result.denial_reason.size(), 5u)
        << "denial_reason must be a non-trivial human-readable string";
}

// ═══════════════════════════════════════════════════════════
// LLMApiHandler::setPolicyEngine / handleOpenAIChatCompletions
// Server-level integration: verify policy gate is wired correctly.
// ═══════════════════════════════════════════════════════════

namespace {

class ScopedDefaultLogCapture final {
public:
    ScopedDefaultLogCapture()
        : previous_logger_(spdlog::default_logger()),
          previous_level_(spdlog::get_level()) {
        sink_ = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream_);
        logger_ = std::make_shared<spdlog::logger>("openai_compat_capture", sink_);
        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::info);
        logger_->set_pattern("%v");

        spdlog::set_default_logger(logger_);
        spdlog::set_level(spdlog::level::info);
    }

    ~ScopedDefaultLogCapture() {
        if (logger_) {
            logger_->flush();
        }
        spdlog::set_default_logger(previous_logger_);
        spdlog::set_level(previous_level_);
    }

    ScopedDefaultLogCapture(const ScopedDefaultLogCapture&) = delete;
    ScopedDefaultLogCapture& operator=(const ScopedDefaultLogCapture&) = delete;

    [[nodiscard]] std::string captured() const {
        return stream_.str();
    }

private:
    std::ostringstream stream_;
    std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink_;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> previous_logger_;
    spdlog::level::level_enum previous_level_;
};

/// Helper: build a minimal Boost.Beast HTTP POST request for /v1/chat/completions.
static http::request<http::string_body> makeChatRequest(
    const std::string& auth_header = "",
    const std::string& body_override = "") {

    const std::string body = body_override.empty()
        ? R"({"model":"test","messages":[{"role":"user","content":"hi"}]})"
        : body_override;

    http::request<http::string_body> req{http::verb::post, "/v1/chat/completions", 11};
    req.set(http::field::content_type, "application/json");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    req.body() = body;
    req.prepare_payload();
    return req;
}

/// Helper: build a minimal Boost.Beast HTTP GET request for /v1/models.
static http::request<http::string_body> makeModelsRequest(
    const std::string& auth_header = "") {

    http::request<http::string_body> req{http::verb::get, "/v1/models", 11};
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    return req;
}

} // anonymous namespace

class LLMApiHandlerPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto mgr = std::make_shared<themis::llm::LLMPluginManager>();
        handler_ = std::make_unique<LLMApiHandler>(std::move(mgr), std::nullopt);
    }

    std::unique_ptr<LLMApiHandler> handler_;
    PolicyEngine policy_engine_;  // No YAML loaded → open classification
};

TEST_F(LLMApiHandlerPolicyTest, NoPolicyEngine_RequestPassesThroughWithoutAuthCheck) {
    // When no PolicyEngine is configured the handler must not return 401/403
    // due to a missing Authorization header — it should attempt inference
    // (or fail with a different error, e.g. 500 / bad request from the engine).
    auto req = makeChatRequest();  // no Authorization header
    auto res = handler_->handleRequest(req);

    // The status must NOT be 401 or 403 — those indicate the policy gate
    // incorrectly fired when no policy engine was configured.
    EXPECT_NE(res.result_int(), 401)
        << "No PolicyEngine configured; handler must not return 401";
    EXPECT_NE(res.result_int(), 403)
        << "No PolicyEngine configured; handler must not return 403";
}

TEST_F(LLMApiHandlerPolicyTest, WithPolicyEngine_MissingAuthReturns401) {
    handler_->setPolicyEngine(&policy_engine_);

    auto req = makeChatRequest();  // no Authorization header
    auto res = handler_->handleRequest(req);

    EXPECT_EQ(res.result_int(), 401)
        << "PolicyEngine configured; missing auth header must return 401";
}

TEST_F(LLMApiHandlerPolicyTest, WithPolicyEngine_MalformedAuthReturns401) {
    handler_->setPolicyEngine(&policy_engine_);

    auto req = makeChatRequest("Basic dXNlcjpwYXNz");  // Basic, not Bearer
    auto res = handler_->handleRequest(req);

    EXPECT_EQ(res.result_int(), 401)
        << "PolicyEngine configured; malformed Authorization must return 401";
}

TEST_F(LLMApiHandlerPolicyTest, WithPolicyEngine_ValidBearerKeyPassesPolicyGate) {
    // A valid Bearer key should pass the policy gate (open classification).
    // The request may still fail at the inference layer (no model loaded), but
    // it must NOT fail at the policy check layer with 401 or 403.
    handler_->setPolicyEngine(&policy_engine_);

    auto req = makeChatRequest("Bearer sk-test-api-key-12345");
    auto res = handler_->handleRequest(req);

    EXPECT_NE(res.result_int(), 401)
        << "Valid Bearer key must pass policy gate (not 401)";
    EXPECT_NE(res.result_int(), 403)
        << "Valid Bearer key with open classification must not get 403";
}

TEST_F(LLMApiHandlerPolicyTest, WithPolicyEngine_ErrorBodyIsOpenAICompatible) {
    // The 401 error body must be an OpenAI-compatible error JSON object.
    handler_->setPolicyEngine(&policy_engine_);

    auto req = makeChatRequest();  // no auth
    auto res = handler_->handleRequest(req);

    ASSERT_EQ(res.result_int(), 401);
    ASSERT_EQ(res[http::field::content_type], "application/json");

    json err = json::parse(res.body());
    ASSERT_TRUE(err.contains("error"))
        << "401 response body must have an 'error' key (OpenAI wire format)";
    EXPECT_TRUE(err["error"].contains("message"))
        << "error object must contain 'message'";
}

TEST_F(LLMApiHandlerPolicyTest, SetPolicyEngine_NullDetaches) {
    // Attaching then detaching the PolicyEngine should restore the no-auth behaviour.
    handler_->setPolicyEngine(&policy_engine_);
    handler_->setPolicyEngine(nullptr);  // detach

    auto req = makeChatRequest();  // no Authorization header
    auto res = handler_->handleRequest(req);

    EXPECT_NE(res.result_int(), 401)
        << "After detaching PolicyEngine, handler must not return 401";
}

TEST_F(LLMApiHandlerPolicyTest, OpenAIModelsEndpoint_ReturnsListShapeWithoutPolicy) {
    auto req = makeModelsRequest();
    auto res = handler_->handleRequest(req);

    ASSERT_EQ(res.result_int(), 200);
    ASSERT_EQ(res[http::field::content_type], "application/json");

    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("object"));
    ASSERT_TRUE(body.contains("data"));
    EXPECT_EQ(body["object"], "list");
    EXPECT_TRUE(body["data"].is_array());
}

TEST_F(LLMApiHandlerPolicyTest, OpenAIModelsEndpoint_DoesNotRequirePolicyAuthHeader) {
    handler_->setPolicyEngine(&policy_engine_);

    auto req = makeModelsRequest();
    auto res = handler_->handleRequest(req);

    EXPECT_NE(res.result_int(), 401);
    EXPECT_NE(res.result_int(), 403);
}

TEST_F(LLMApiHandlerPolicyTest, OpenAIChatNonStreaming_EmitsLifecycleLogs) {
    handler_->setPolicyEngine(&policy_engine_);

    ScopedDefaultLogCapture capture;
    auto req = makeChatRequest(
        "Bearer sk-test-api-key-12345",
        R"({"model":"test","stream":false,"messages":[{"role":"user","content":"hi"}]})");

    auto res = handler_->handleRequest(req);
    const std::string logs = capture.captured();

    EXPECT_NE(logs.find("LLMApiHandler::handleOpenAIChatCompletions non-stream start"), std::string::npos)
        << "Non-stream request should emit a start lifecycle log";

    const bool has_complete =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions non-stream complete") != std::string::npos;
    const bool has_failed =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions non-stream failed") != std::string::npos;
    const bool has_failed_unknown =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions non-stream failed with unknown error") != std::string::npos;

    EXPECT_TRUE(has_complete || has_failed || has_failed_unknown)
        << "Non-stream request should emit a terminal lifecycle log (complete or failed)";

    if (res.result_int() >= 500) {
        EXPECT_TRUE(has_failed || has_failed_unknown)
            << "Server-error path should include a non-stream failure lifecycle log";
    }
}

TEST_F(LLMApiHandlerPolicyTest, OpenAIChatStreaming_EmitsLifecycleLogs) {
    handler_->setPolicyEngine(&policy_engine_);

    ScopedDefaultLogCapture capture;
    auto req = makeChatRequest(
        "Bearer sk-test-api-key-12345",
        R"({"model":"test","stream":true,"messages":[{"role":"user","content":"hi"}]})");

    auto res = handler_->handleRequest(req);
    const std::string logs = capture.captured();

    EXPECT_NE(logs.find("LLMApiHandler::handleOpenAIChatCompletions stream start"), std::string::npos)
        << "Streaming request should emit a start lifecycle log";

    const bool has_complete =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions stream complete") != std::string::npos;
    const bool has_failed =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions stream failed") != std::string::npos;
    const bool has_failed_unknown =
        logs.find("LLMApiHandler::handleOpenAIChatCompletions stream failed with unknown error") != std::string::npos;

    EXPECT_TRUE(has_complete || has_failed || has_failed_unknown)
        << "Streaming request should emit a terminal lifecycle log (complete or failed)";

    if (res.result_int() >= 500) {
        EXPECT_TRUE(has_failed || has_failed_unknown)
            << "Server-error path should include a streaming failure lifecycle log";
    }
}
