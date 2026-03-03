/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_openai_compat_adapter.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:01:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     419                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8f8969876  2026-02-27  feat(llm): OpenAI-compatible /v1/chat/completions passthr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

#include "llm/openai_compat_adapter.h"
#include "llm/llm_plugin_interface.h"

using themis::llm::OpenAICompatAdapter;
using themis::llm::InferenceRequest;
using themis::llm::InferenceResponse;
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
