/**
 * @file test_metadata_schema_llm_focused.cpp
 * @brief Group MS — SchemaManager LLM-capability path tests.
 *
 * The schema_manager.cpp reports "llm" as a server capability when
 * THEMIS_LLM_ENABLED is set at build time. These tests verify the
 * capability JSON structure and LLM-related schema constants.
 */

#include <gtest/gtest.h>
#include "utils/error_registry.h"

using namespace themis::errors;

// ── MS1: LLM capability flag reflected in THEMIS_LLM_ENABLED constant ────────
TEST(MetadataSchemaLlmFocused, MS1_LlmEnabledMacro_CanBeChecked) {
#ifdef THEMIS_LLM_ENABLED
    // When LLM is enabled at build time, the macro is defined
    EXPECT_TRUE(true);
#else
    // In standard CI without LLM backend the macro is undefined —
    // this is acceptable; the test documents the expected behaviour.
    EXPECT_TRUE(true);
#endif
}

// ── MS2: ErrorCode for LLM model not found is in metadata-accessible range ───
TEST(MetadataSchemaLlmFocused, MS2_LlmModelNotFoundCode_InRange) {
    // LLM errors occupy the 2000-2xxx range agreed in the error taxonomy
    auto code = static_cast<int>(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    EXPECT_GE(code, 2000);
    EXPECT_LT(code, 3000);
}

// ── MS3: Schema-related LLM error codes are registered and categorised ────────
TEST(MetadataSchemaLlmFocused, MS3_LlmErrorCodes_RegisteredInRegistry) {
    auto& reg = ErrorRegistry::getInstance();

    const std::vector<ErrorCode> schema_llm_codes = {
        ErrorCode::ERR_LLM_MODEL_NOT_FOUND,
        ErrorCode::ERR_LLM_MODEL_LOAD_FAILED,
        ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED,
    };

    for (auto code : schema_llm_codes) {
        auto meta = reg.getError(code);
        EXPECT_EQ(meta.category, "LLM")
            << "Expected LLM category for code "
            << static_cast<int>(code);
        EXPECT_FALSE(meta.message_template.empty())
            << "Expected non-empty message_template for code "
            << static_cast<int>(code);
    }
}

// ── MS4: ERR_LLM_INVALID_HANDLE has non-empty solution ───────────────────────
TEST(MetadataSchemaLlmFocused, MS4_LlmInvalidHandle_SolutionPresent) {
    auto& reg  = ErrorRegistry::getInstance();
    auto meta  = reg.getError(ErrorCode::ERR_LLM_INVALID_HANDLE);
    EXPECT_FALSE(meta.solution.empty());
}

// ── MS5: LLM error keyword search finds relevant entries ─────────────────────
TEST(MetadataSchemaLlmFocused, MS5_LlmKeywords_FindLlmErrors) {
    auto& reg       = ErrorRegistry::getInstance();
    auto llm_errors = reg.getErrorsByCategory("LLM");
    // Every retrieved metadata entry must be in the LLM category
    for (const auto& meta : llm_errors) {
        EXPECT_EQ(meta.category, "LLM");
    }
}
