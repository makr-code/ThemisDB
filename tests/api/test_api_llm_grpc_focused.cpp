/**
 * @file test_api_llm_grpc_focused.cpp
 * @brief Group AG — gRPC service LLM error-code mapping path tests.
 *
 * The themisdb_grpc_service.cpp maps LLM-specific ErrorCodes to gRPC status
 * codes (DEADLINE_EXCEEDED for timeouts, RESOURCE_EXHAUSTED for OOM). These
 * tests verify that the LLM error code constants are in the expected ranges
 * and that the ErrorRegistry resolves them correctly — the same data used by
 * the gRPC mapping switch statement.
 */

#include <gtest/gtest.h>
#include "utils/error_registry.h"

using namespace themis;
using namespace themis::errors;

// ── AG1: ERR_LLM_INFERENCE_TIMEOUT maps to DEADLINE_EXCEEDED (code check) ────
TEST(ApiLlmGrpcFocused, AG1_LlmInferenceTimeout_CorrectCode) {
    // The gRPC service maps ERR_LLM_INFERENCE_TIMEOUT → DEADLINE_EXCEEDED.
    // Verify the error code is registered and categorised correctly.
    auto& reg = ErrorRegistry::getInstance();
    auto meta = reg.getError(ErrorCode::ERR_LLM_INFERENCE_TIMEOUT);
    EXPECT_EQ(meta.category, "LLM");
    EXPECT_FALSE(meta.solution.empty());
}

// ── AG2: ERR_LLM_GPU_OOM and ERR_LLM_RAM_OOM map to RESOURCE_EXHAUSTED ───────
TEST(ApiLlmGrpcFocused, AG2_LlmOomErrors_RegisteredAsLlm) {
    auto& reg = ErrorRegistry::getInstance();

    for (auto code : {ErrorCode::ERR_LLM_GPU_OOM, ErrorCode::ERR_LLM_RAM_OOM}) {
        auto meta = reg.getError(code);
        EXPECT_EQ(meta.category, "LLM")
            << "Expected LLM category for code "
            << static_cast<int>(code);
    }
}

// ── AG3: All LLM error codes are in the 2000-2999 range ──────────────────────
TEST(ApiLlmGrpcFocused, AG3_LlmErrors_InCodeRange) {
    auto& reg       = ErrorRegistry::getInstance();
    auto llm_errors = reg.getErrorsByCategory("LLM");
    ASSERT_FALSE(llm_errors.empty());

    for (const auto& meta : llm_errors) {
        int code_val = static_cast<int>(meta.code);
        EXPECT_GE(code_val, 2000) << "LLM error code below 2000: " << code_val;
        EXPECT_LT(code_val, 3000) << "LLM error code above 2999: " << code_val;
    }
}

// ── AG4: ERR_LLM_BATCH_SIZE_EXCEEDED is registered (API rate-limit path) ─────
TEST(ApiLlmGrpcFocused, AG4_LlmBatchSizeExceeded_Registered) {
    auto& reg  = ErrorRegistry::getInstance();
    auto meta  = reg.getError(ErrorCode::ERR_LLM_BATCH_SIZE_EXCEEDED);
    EXPECT_EQ(meta.category, "LLM");
    EXPECT_FALSE(meta.message_template.empty());
}

// ── AG5: getRecoveryHint for each LLM OOM code returns non-empty hint ───────
TEST(ApiLlmGrpcFocused, AG5_LlmOomErrors_SolutionNonEmpty) {
    auto& reg = ErrorRegistry::getInstance();

    for (auto code : {ErrorCode::ERR_LLM_GPU_OOM, ErrorCode::ERR_LLM_RAM_OOM}) {
        auto solution = reg.getRecoveryHint(code);
        EXPECT_FALSE(solution.empty())
            << "Empty solution for code " << static_cast<int>(code);
    }
}
