/**
 * @file test_embedded_llm_batch_contract.cpp
 * @brief Contract tests for EmbeddedLLM batch embeddings.
 */

#include <gtest/gtest.h>

#include "llm/embedded_llm.h"

#include <cmath>
#include <vector>

namespace themis { namespace llm { 
namespace {

float l2Norm(const std::vector<float>& values) {
    float sum = 0.0f;
    for (const float value : values) {
        sum += value * value;
    }
    return std::sqrt(sum);
}

} // namespace

TEST(EmbeddedLLMBatchContract, ReturnsOneEmbeddingPerInputText) {
    EmbeddedLLM llm;
    const std::vector<std::string> texts = {"alpha", "beta", "", "alpha beta"};

    const auto batch_embeddings = llm.embedBatch(texts);

    ASSERT_EQ(batch_embeddings.size(), texts.size());
    for (std::size_t index = 0; index < texts.size(); ++index) {
        const auto single_embedding = llm.embed(texts[index]);
        ASSERT_FALSE(batch_embeddings[index].empty());
        EXPECT_EQ(batch_embeddings[index], single_embedding);
        EXPECT_NEAR(l2Norm(batch_embeddings[index]), 1.0f, 1e-4f);
    }
}

TEST(EmbeddedLLMBatchContract, HandlesEmptyBatch) {
    EmbeddedLLM llm;

    EXPECT_TRUE(llm.embedBatch({}).empty());
}

TEST(EmbeddedLLMBatchContract, ReportsCorrectBackendMetadataWhenNoCallback) {
    EmbeddedLLM llm;

    InferenceRequest request;
    request.prompt = "What is 2+2?";
    request.request_id = "req-1";

    const auto response = llm.generateFull(request);
    const auto stats = llm.getStats();

    // Both stub and fail-closed modes report llm_enabled=false.
    EXPECT_FALSE(response.metadata.value("llm_enabled", true));
    EXPECT_FALSE(stats.value("llm_enabled", true));

#ifdef THEMIS_LLM_STUB_MODE
    // Stub/dev builds: deterministic fallback with success=true.
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.metadata.value("backend", std::string{}), "deterministic-fallback");
    EXPECT_EQ(stats.value("backend", std::string{}), "deterministic-fallback");
#else
    // Production builds (no THEMIS_LLM_STUB_MODE): fail-closed.
    EXPECT_FALSE(response.success);
    EXPECT_FALSE(response.error_message.empty());
    EXPECT_EQ(response.metadata.value("backend", std::string{}), "no-backend-fail-closed");
    EXPECT_EQ(stats.value("backend", std::string{}), "no-backend-fail-closed");
#endif
}
} } // namespace themis::llm
