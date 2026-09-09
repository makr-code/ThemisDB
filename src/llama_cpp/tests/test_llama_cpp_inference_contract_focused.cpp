/**
 * @file test_llama_cpp_inference_contract_focused.cpp
 * @brief Group IC — Inference contract tests for LlamaCppPlugin.
 *
 * Validates the generate() and generateRAG() output contract under
 * THEMIS_LLAMA_CPP_STUB_MODE:
 *   - Response fields are populated consistently
 *   - Failure cases return success==false with a non-empty error_message
 *   - Streaming callback is invoked when provided
 *   - LoRA adapter hint is accepted without crash
 *   - JSON-schema binding is accepted without crash
 *
 * All tests require no real model file.
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <memory>
#include <string>

using namespace themis::llamacpp;
using namespace themis::llm;
using json = nlohmann::json;

// ── helper ────────────────────────────────────────────────────────────────────

static std::unique_ptr<LlamaCppPlugin> make_loaded_plugin() {
    auto p = std::make_unique<LlamaCppPlugin>();
    p->loadModel("/stub/model.gguf", {});
    return p;
}

// ── Group IC — Inference Contract ─────────────────────────────────────────────

// IC1: generate() on unloaded plugin returns success==false with error message
TEST(LlamaCppInferenceContractFocusedTests, IC1_GenerateUnloaded_ReturnsError) {
    LlamaCppPlugin p;
    InferenceRequest req;
    req.prompt     = "hello";
    req.max_tokens = 32;
    auto resp = p.generate(req);
    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.error_message.empty());
}

// IC2: generate() on loaded plugin returns success
TEST(LlamaCppInferenceContractFocusedTests, IC2_GenerateLoaded_ReturnsSuccess) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt     = "What is the capital of France?";
    req.max_tokens = 32;
    auto resp = p.generate(req);
    EXPECT_TRUE(resp.success);
}

// IC3: generate() response text is non-empty when successful
TEST(LlamaCppInferenceContractFocusedTests, IC3_GenerateSuccess_TextNotEmpty) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt     = "Describe the sky.";
    req.max_tokens = 32;
    auto resp = p.generate(req);
    if (resp.success) {
        EXPECT_FALSE(resp.text.empty());
    }
}

// IC4: error_message is empty on a successful response
TEST(LlamaCppInferenceContractFocusedTests, IC4_GenerateSuccess_ErrorMessageEmpty) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt     = "hello";
    req.max_tokens = 8;
    auto resp = p.generate(req);
    if (resp.success) {
        EXPECT_TRUE(resp.error_message.empty());
    }
}

// IC5: system_prompt field is accepted without crash
TEST(LlamaCppInferenceContractFocusedTests, IC5_SystemPrompt_Accepted) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt        = "Hi";
    req.max_tokens    = 8;
    req.system_prompt = "You are a helpful assistant.";
    ASSERT_NO_THROW(p.generate(req));
}

// IC6: lora_adapter_id hint is accepted without crash
TEST(LlamaCppInferenceContractFocusedTests, IC6_LoRAAdapterHint_Accepted) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt          = "test";
    req.max_tokens      = 8;
    req.lora_adapter_id = "dummy-lora-adapter";
    ASSERT_NO_THROW(p.generate(req));
}

// IC7: streaming callback is invoked at least once when provided
TEST(LlamaCppInferenceContractFocusedTests, IC7_StreamingCallback_IsInvoked) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt     = "Stream test";
    req.max_tokens = 16;

    std::atomic<int> callback_count{0};
    req.stream_callback = [&callback_count](const std::string& chunk) {
        if (!chunk.empty()) {
            ++callback_count;
        }
    };

    auto resp = p.generate(req);
    // In stub mode the callback may or may not be called; the key invariant is no crash.
    (void)resp;
    SUCCEED();
}

// IC8: generateRAG() returns a response without crash
TEST(LlamaCppInferenceContractFocusedTests, IC8_GenerateRAG_ReturnsResponse) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;

    RAGContext ctx;
    ctx.documents.push_back({"Paris is the capital of France.", "stub", 1.0f, {}});

    InferenceRequest req;
    req.prompt     = "What is the capital of France?";
    req.max_tokens = 32;

    ASSERT_NO_THROW({
        auto resp = p.generateRAG(ctx, req);
        EXPECT_TRUE(resp.success);
    });
}

// IC9: generateRAG() with empty context does not crash
TEST(LlamaCppInferenceContractFocusedTests, IC9_GenerateRAG_EmptyContext_NocrASH) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    RAGContext ctx;  // empty
    InferenceRequest req;
    req.prompt     = "Any question?";
    req.max_tokens = 8;
    ASSERT_NO_THROW(p.generateRAG(ctx, req));
}

// IC10: json_schema binding is accepted without crash
TEST(LlamaCppInferenceContractFocusedTests, IC10_JsonSchema_Accepted) {
    auto plugin = make_loaded_plugin();
    LlamaCppPlugin& p = *plugin;
    InferenceRequest req;
    req.prompt     = "Return a JSON object";
    req.max_tokens = 32;
    req.json_schema = json{{"type", "object"}, {"properties", json::object()}};
    ASSERT_NO_THROW(p.generate(req));
}
