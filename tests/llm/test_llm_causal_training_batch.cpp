// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>

#include "llm/llama_wrapper.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

TEST(LlamaCausalTrainingBatchTest, ReturnsFinitePerTokenHiddenStatesAndLogits) {
    const char* model_path = std::getenv("THEMIS_TEST_MODEL_PATH");
    if (!model_path || !std::filesystem::is_regular_file(model_path)) {
        GTEST_SKIP() << "Set THEMIS_TEST_MODEL_PATH to a TinyLlama GGUF model";
    }

    themis::llm::LlamaWrapper::Config config;
    config.n_ctx = 128;
    config.n_batch = 128;
    config.n_gpu_layers = 0;
    themis::llm::LlamaWrapper wrapper(config);
    ASSERT_TRUE(wrapper.loadModel(model_path));

    const auto tokens = wrapper.tokenizeForBridge("ThemisDB stores documents.", true);
    ASSERT_GE(tokens.size(), 2u);
    const auto forward = wrapper.forwardCausalBatchForTraining(tokens);

    EXPECT_EQ(forward.token_count, tokens.size());
    EXPECT_GT(forward.hidden_size, 0u);
    EXPECT_GT(forward.vocabulary_size, 0u);
    EXPECT_EQ(forward.hidden_states.size(), forward.token_count * forward.hidden_size);
    EXPECT_EQ(forward.logits.size(), forward.token_count * forward.vocabulary_size);
    EXPECT_TRUE(std::all_of(forward.hidden_states.begin(), forward.hidden_states.end(),
                            [](float value) { return std::isfinite(value); }));
    EXPECT_TRUE(std::all_of(forward.logits.begin(), forward.logits.end(),
                            [](float value) { return std::isfinite(value); }));
}