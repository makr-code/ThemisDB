/**
 * @file test_utils_llm_path_focused.cpp
 * @brief Group UL — ErrorRegistry LLM error-code path tests.
 *
 * The utils module registers all LLM error codes (range 2000-2xxx) in the
 * global ErrorRegistry. These tests verify that the registry correctly
 * categorises, describes, and returns LLM-specific error metadata.
 */

#include <gtest/gtest.h>
#include "utils/error_registry.h"

#include <string>
#include <vector>

using namespace themis;
using namespace themis::utils;

// ── UL1: ERR_LLM_MODEL_NOT_FOUND is registered and has non-empty message ─────
TEST(UtilsLlmPathFocused, UL1_LlmModelNotFound_Registered) {
    auto& reg = ErrorRegistry::getInstance();
    auto meta = reg.getError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    EXPECT_FALSE(meta.message_template.empty());
}

// ── UL2: ERR_LLM_INFERENCE_TIMEOUT is categorised as "LLM" ──────────────────
TEST(UtilsLlmPathFocused, UL2_LlmInferenceTimeout_CategoryIsLLM) {
    auto& reg  = ErrorRegistry::getInstance();
    auto meta  = reg.getError(ErrorCode::ERR_LLM_INFERENCE_TIMEOUT);
    EXPECT_EQ(meta.category, "LLM");
}

// ── UL3: ERR_LLM_GPU_OOM solution hint is non-empty ─────────────────────────
TEST(UtilsLlmPathFocused, UL3_LlmGpuOom_SolutionNonEmpty) {
    auto& reg  = ErrorRegistry::getInstance();
    auto meta  = reg.getError(ErrorCode::ERR_LLM_GPU_OOM);
    EXPECT_FALSE(meta.solution.empty());
}

// ── UL4: getErrorsByCategory("LLM") returns at least 5 entries ───────────────
TEST(UtilsLlmPathFocused, UL4_LlmCategory_AtLeastFiveEntries) {
    auto& reg  = ErrorRegistry::getInstance();
    auto llm_errors = reg.getErrorsByCategory("LLM");
    EXPECT_GE(llm_errors.size(), 5u);
}

// ── UL5: All LLM error codes in [2000, 2099] are in category "LLM" ───────────
TEST(UtilsLlmPathFocused, UL5_LlmErrors_AllInLlmCategory) {
    auto& reg = ErrorRegistry::getInstance();
    const std::vector<ErrorCode> llm_codes = {
        ErrorCode::ERR_LLM_MODEL_NOT_FOUND,
        ErrorCode::ERR_LLM_MODEL_LOAD_FAILED,
        ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED,
        ErrorCode::ERR_LLM_INFERENCE_TIMEOUT,
        ErrorCode::ERR_LLM_GPU_OOM,
    };
    for (auto code : llm_codes) {
        auto meta = reg.getError(code);
        EXPECT_EQ(meta.category, "LLM")
            << "Expected 'LLM' category for error code "
            << static_cast<int>(code);
    }
}

// ── UL6: getRecoveryHint shorthand returns same as getError().solution ───────
TEST(UtilsLlmPathFocused, UL6_GetSolution_MatchesMetadataSolution) {
    auto& reg = ErrorRegistry::getInstance();
    auto meta      = reg.getError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    auto solution  = reg.getRecoveryHint(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    EXPECT_EQ(meta.solution, solution);
}
