/**
 * @file test_llm_validation.cpp
 * @brief Unit tests for LLM input validation and error handling
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include "aql/llm_error_codes.h"

using namespace themis::aql;

class LLMValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }
    
    void TearDown() override {
        handler.reset();
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

// ============================================================================
// Prompt Validation Tests
// ============================================================================

TEST_F(LLMValidationTest, ValidatePrompt_Empty) {
    // Empty prompt should throw INVALID_PROMPT
    EXPECT_THROW({
        try {
            LLMValidator::validatePrompt("");
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_PROMPT);
            EXPECT_EQ(LLMException::getErrorCodeString(e.getErrorCode()), "LLM_INVALID_PROMPT");
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidatePrompt_TooLong) {
    // Create a prompt that exceeds max length
    std::string long_prompt(ValidationLimits::MAX_PROMPT_LENGTH + 1, 'a');
    
    EXPECT_THROW({
        try {
            LLMValidator::validatePrompt(long_prompt);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::PROMPT_TOO_LONG);
            EXPECT_EQ(LLMException::getErrorCodeString(e.getErrorCode()), "LLM_PROMPT_TOO_LONG");
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidatePrompt_Valid) {
    // Valid prompt should not throw
    EXPECT_NO_THROW(LLMValidator::validatePrompt("This is a valid prompt"));
    EXPECT_NO_THROW(LLMValidator::validatePrompt("a"));
    EXPECT_NO_THROW(LLMValidator::validatePrompt(std::string(1000, 'x')));
}

// ============================================================================
// Model/LoRA ID Validation Tests
// ============================================================================

TEST_F(LLMValidationTest, ValidateId_Empty) {
    // Empty ID should be allowed (uses default)
    EXPECT_NO_THROW(LLMValidator::validateId("", false));
    EXPECT_NO_THROW(LLMValidator::validateId("", true));
}

TEST_F(LLMValidationTest, ValidateId_Valid) {
    // Valid IDs with various formats
    EXPECT_NO_THROW(LLMValidator::validateId("model123", false));
    EXPECT_NO_THROW(LLMValidator::validateId("llama-2-7b", false));
    EXPECT_NO_THROW(LLMValidator::validateId("gpt_3.5_turbo", false));
    EXPECT_NO_THROW(LLMValidator::validateId("model.v1.2.3", false));
    EXPECT_NO_THROW(LLMValidator::validateId("legal-lora", true));
}

TEST_F(LLMValidationTest, ValidateId_InvalidCharacters) {
    // IDs with invalid characters should throw
    EXPECT_THROW({
        try {
            LLMValidator::validateId("model/path", false);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_MODEL_ID);
            throw;
        }
    }, LLMException);
    
    EXPECT_THROW({
        try {
            LLMValidator::validateId("model@version", false);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_MODEL_ID);
            throw;
        }
    }, LLMException);
    
    EXPECT_THROW({
        try {
            LLMValidator::validateId("model space", false);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_MODEL_ID);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidateId_TooLong) {
    // ID exceeding max length should throw
    std::string long_id(ValidationLimits::MAX_ID_LENGTH + 1, 'a');
    
    EXPECT_THROW({
        try {
            LLMValidator::validateId(long_id, false);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_MODEL_ID);
            throw;
        }
    }, LLMException);
    
    EXPECT_THROW({
        try {
            LLMValidator::validateId(long_id, true);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_LORA_ID);
            throw;
        }
    }, LLMException);
}

// ============================================================================
// Collection Validation Tests
// ============================================================================

TEST_F(LLMValidationTest, ValidateCollection_Empty) {
    // Empty collection should throw
    EXPECT_THROW({
        try {
            LLMValidator::validateCollection("");
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_COLLECTION);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidateCollection_TooLong) {
    // Collection name exceeding max length should throw
    std::string long_collection(ValidationLimits::MAX_COLLECTION_NAME_LENGTH + 1, 'a');
    
    EXPECT_THROW({
        try {
            LLMValidator::validateCollection(long_collection);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_COLLECTION);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidateCollection_Valid) {
    // Valid collection names
    EXPECT_NO_THROW(LLMValidator::validateCollection("documents"));
    EXPECT_NO_THROW(LLMValidator::validateCollection("my_collection"));
    EXPECT_NO_THROW(LLMValidator::validateCollection("collection123"));
}

// ============================================================================
// Top-K Validation Tests
// ============================================================================

TEST_F(LLMValidationTest, ValidateTopK_TooSmall) {
    // top_k below minimum should throw
    EXPECT_THROW({
        try {
            LLMValidator::validateTopK(0);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_OPTIONS);
            throw;
        }
    }, LLMException);
    
    EXPECT_THROW({
        try {
            LLMValidator::validateTopK(-1);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_OPTIONS);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidateTopK_TooLarge) {
    // top_k above maximum should throw
    EXPECT_THROW({
        try {
            LLMValidator::validateTopK(ValidationLimits::MAX_RAG_TOP_K + 1);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_OPTIONS);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ValidateTopK_Valid) {
    // Valid top_k values
    EXPECT_NO_THROW(LLMValidator::validateTopK(1));
    EXPECT_NO_THROW(LLMValidator::validateTopK(5));
    EXPECT_NO_THROW(LLMValidator::validateTopK(10));
    EXPECT_NO_THROW(LLMValidator::validateTopK(ValidationLimits::MAX_RAG_TOP_K));
}

// ============================================================================
// Error Code Tests
// ============================================================================

TEST_F(LLMValidationTest, ErrorCodeStrings) {
    // Verify error code string formatting
    EXPECT_EQ(LLMException::getErrorCodeString(LLMErrorCode::INVALID_PROMPT), "LLM_INVALID_PROMPT");
    EXPECT_EQ(LLMException::getErrorCodeString(LLMErrorCode::PROMPT_TOO_LONG), "LLM_PROMPT_TOO_LONG");
    EXPECT_EQ(LLMException::getErrorCodeString(LLMErrorCode::MODEL_NOT_FOUND), "LLM_MODEL_NOT_FOUND");
    EXPECT_EQ(LLMException::getErrorCodeString(LLMErrorCode::INFERENCE_FAILED), "LLM_INFERENCE_FAILED");
    EXPECT_EQ(LLMException::getErrorCodeString(LLMErrorCode::TIMEOUT), "LLM_TIMEOUT");
}

TEST_F(LLMValidationTest, SafeErrorMessages) {
    // Verify that safe messages mask internal details
    LLMException internal_error(LLMErrorCode::INTERNAL_ERROR, "Detailed stack trace info");
    std::string safe_msg = internal_error.getSafeMessage();
    
    // Safe message should NOT contain internal details
    EXPECT_EQ(safe_msg.find("stack trace"), std::string::npos);
    EXPECT_NE(safe_msg.find("internal error"), std::string::npos);
}

// ============================================================================
// Integration Tests with Handler
// ============================================================================

TEST_F(LLMValidationTest, ExecuteInfer_EmptyPrompt) {
    // Empty prompt should be caught and wrapped in LLMException
    EXPECT_THROW({
        try {
            handler->executeInfer("", "model", "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_PROMPT);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ExecuteInfer_InvalidModelId) {
    // Invalid model ID should be caught
    EXPECT_THROW({
        try {
            handler->executeInfer("test", "model/with/slash", "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_MODEL_ID);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ExecuteInfer_InvalidLoRAId) {
    // Invalid LoRA ID should be caught
    EXPECT_THROW({
        try {
            handler->executeInfer("test", "", "lora@invalid", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_LORA_ID);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ExecuteInfer_PromptTooLong) {
    // Oversized prompt should be caught
    std::string huge_prompt(ValidationLimits::MAX_PROMPT_LENGTH + 100, 'a');
    
    EXPECT_THROW({
        try {
            handler->executeInfer(huge_prompt, "", "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::PROMPT_TOO_LONG);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ExecuteRAG_EmptyCollection) {
    // Empty collection should be caught
    EXPECT_THROW({
        try {
            handler->executeRAG("query", "", 5, "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_COLLECTION);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMValidationTest, ExecuteRAG_InvalidTopK) {
    // Invalid top_k should be caught
    EXPECT_THROW({
        try {
            handler->executeRAG("query", "docs", 0, "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_OPTIONS);
            throw;
        }
    }, LLMException);
    
    EXPECT_THROW({
        try {
            handler->executeRAG("query", "docs", 1000, "", {});
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::INVALID_OPTIONS);
            throw;
        }
    }, LLMException);
}

// ============================================================================
// Correlation ID Tests
// ============================================================================

TEST_F(LLMValidationTest, CorrelationId) {
    // Test correlation ID support
    std::string correlation_id = "req-12345";
    LLMException ex(LLMErrorCode::INFERENCE_FAILED, "Test error", correlation_id);
    
    EXPECT_EQ(ex.getCorrelationId(), correlation_id);
    EXPECT_EQ(ex.getErrorCode(), LLMErrorCode::INFERENCE_FAILED);
}
