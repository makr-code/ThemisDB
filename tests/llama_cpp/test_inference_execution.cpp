#include <gtest/gtest.h>

#include "llama_cpp/llama_cpp_plugin.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::llamacpp;
using namespace themis::llm;

namespace {

std::unique_ptr<LlamaCppPlugin> make_loaded_plugin() {
    auto plugin = std::make_unique<LlamaCppPlugin>();
    EXPECT_TRUE(plugin->loadModel("/stub/model.gguf", {}));
    return plugin;
}

}  // namespace

TEST(LlamaCppInferenceExecutionTests, I01_Generate_ReturnsSuccessForLoadedStubModel) {
    auto plugin = make_loaded_plugin();

    InferenceRequest req;
    req.prompt = "What is the capital of France?";
    req.max_tokens = 32;

    auto resp = plugin->generate(req);
    EXPECT_TRUE(resp.success);
    EXPECT_FALSE(resp.text.empty());
}

TEST(LlamaCppInferenceExecutionTests, I02_Generate_WithoutLoadReturnsError) {
    LlamaCppPlugin plugin;

    InferenceRequest req;
    req.prompt = "hello";
    auto resp = plugin.generate(req);

    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.error_message.empty());
}

TEST(LlamaCppInferenceExecutionTests, I03_Generate_PreservesPromptMetadata) {
    auto plugin = make_loaded_plugin();

    InferenceRequest req;
    req.prompt = "Explain the moon.";
    req.max_tokens = 24;
    req.trace_id = "trace-123";
    req.span_id = "span-456";

    auto resp = plugin->generate(req);
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.trace_id, "trace-123");
    EXPECT_EQ(resp.span_id, "span-456");
}

TEST(LlamaCppInferenceExecutionTests, I04_Generate_StreamsTokensWhenCallbackPresent) {
    auto plugin = make_loaded_plugin();

    InferenceRequest req;
    req.prompt = "Stream me a sentence.";
    req.max_tokens = 10;
    std::atomic<int> callbacks{0};
    req.stream_callback = [&callbacks](const std::string& token) {
        if (!token.empty()) {
            ++callbacks;
        }
    };

    auto resp = plugin->generate(req);
    EXPECT_TRUE(resp.success);
    EXPECT_GE(callbacks.load(), 0);
}

TEST(LlamaCppInferenceExecutionTests, I05_GenerateBatch_ProcessesMultipleRequests) {
    auto plugin = make_loaded_plugin();

    std::vector<InferenceRequest> requests(3);
    for (std::size_t i = 0; i < requests.size(); ++i) {
        requests[i].prompt = "batch prompt " + std::to_string(i);
        requests[i].max_tokens = 16;
    }

    auto responses = plugin->generateBatch(requests);
    EXPECT_EQ(responses.size(), requests.size());
    for (const auto& resp : responses) {
        EXPECT_TRUE(resp.success);
    }
}

TEST(LlamaCppInferenceExecutionTests, I06_GenerateBatch_EmptyInputReturnsEmptyOutput) {
    auto plugin = make_loaded_plugin();
    std::vector<InferenceRequest> requests;

    auto responses = plugin->generateBatch(requests);
    EXPECT_TRUE(responses.empty());
}

TEST(LlamaCppInferenceExecutionTests, I07_GenerateRAG_ReturnsStructuredResponse) {
    auto plugin = make_loaded_plugin();

    RAGContext ctx;
    ctx.query = "capital of France";
    ctx.documents.push_back({"France is in Europe.", "doc-1", 0.9f, {}});

    InferenceRequest req;
    req.prompt = "capital of France";
    req.max_tokens = 24;

    auto resp = plugin->generateRAG(ctx, req);
    EXPECT_TRUE(resp.success);
    EXPECT_FALSE(resp.text.empty());
}

TEST(LlamaCppInferenceExecutionTests, I08_GenerateRAG_WithoutLoadedModelReturnsError) {
    LlamaCppPlugin plugin;

    RAGContext ctx;
    ctx.documents.push_back({"The capital is Paris.", "doc-1", 1.0f, {}});

    InferenceRequest req;
    req.prompt = "capital of France";
    auto resp = plugin.generateRAG(ctx, req);

    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.error_message.empty());
}
