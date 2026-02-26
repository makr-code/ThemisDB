// Test: AQL LLM Handler API Stability
//
// Validates the stability guarantees for the public LLM AQL handler
// contracts defined in:
//   - include/aql/llm_aql_handler.h
//   - include/aql/llm_error_codes.h
//
// These tests act as a canary: if any of the declared-stable enum values,
// struct field layouts, or version constants change, the corresponding test
// will fail at compile-time or runtime, alerting maintainers to bump
// LLM_AQL_HANDLER_API_VERSION and update the breaking-changes log.
//
// Every test that verifies a numeric enum value uses a static_assert (compile
// time) AND a runtime EXPECT_EQ (for clean failure messages in CI logs).
//
// Platform: No GPU or LLM model required — all checks are CPU-side.

#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "aql/llm_aql_handler.h"
#include "aql/llm_error_codes.h"

using namespace themis::aql;

// =============================================================================
// API version constant
// =============================================================================

TEST(AqlApiStability, ApiVersionConstantIsDefined) {
    // LLM_AQL_HANDLER_API_VERSION must be accessible and equal to 100 (v1.0).
    static_assert(LLM_AQL_HANDLER_API_VERSION == 100u,
        "LLM_AQL_HANDLER_API_VERSION must be 100 (v1.0); bump only on breaking changes");
    EXPECT_EQ(LLM_AQL_HANDLER_API_VERSION, 100u);
}

TEST(AqlApiStability, ApiVersionConstantIsUint32) {
    static_assert(
        std::is_same<decltype(LLM_AQL_HANDLER_API_VERSION), const uint32_t>::value,
        "LLM_AQL_HANDLER_API_VERSION must be uint32_t");
}

// =============================================================================
// LLMErrorCode enum value stability
// =============================================================================

TEST(AqlApiStability, ErrorCodeUnderlyingTypeIsInt) {
    static_assert(
        std::is_integral<std::underlying_type<LLMErrorCode>::type>::value,
        "LLMErrorCode underlying type must remain integral");
}

TEST(AqlApiStability, ErrorCodeInvalidPromptIs1001) {
    static_assert(static_cast<int>(LLMErrorCode::INVALID_PROMPT) == 1001,
        "LLMErrorCode::INVALID_PROMPT must equal 1001; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::INVALID_PROMPT), 1001);
}

TEST(AqlApiStability, ErrorCodePromptTooLongIs1002) {
    static_assert(static_cast<int>(LLMErrorCode::PROMPT_TOO_LONG) == 1002,
        "LLMErrorCode::PROMPT_TOO_LONG must equal 1002; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::PROMPT_TOO_LONG), 1002);
}

TEST(AqlApiStability, ErrorCodeInvalidModelIdIs1003) {
    static_assert(static_cast<int>(LLMErrorCode::INVALID_MODEL_ID) == 1003,
        "LLMErrorCode::INVALID_MODEL_ID must equal 1003; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::INVALID_MODEL_ID), 1003);
}

TEST(AqlApiStability, ErrorCodeInvalidLoraIdIs1004) {
    static_assert(static_cast<int>(LLMErrorCode::INVALID_LORA_ID) == 1004,
        "LLMErrorCode::INVALID_LORA_ID must equal 1004; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::INVALID_LORA_ID), 1004);
}

TEST(AqlApiStability, ErrorCodePromptInjectionIs1007) {
    static_assert(static_cast<int>(LLMErrorCode::PROMPT_INJECTION) == 1007,
        "LLMErrorCode::PROMPT_INJECTION must equal 1007; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::PROMPT_INJECTION), 1007);
}

TEST(AqlApiStability, ErrorCodeModelNotFoundIs2001) {
    static_assert(static_cast<int>(LLMErrorCode::MODEL_NOT_FOUND) == 2001,
        "LLMErrorCode::MODEL_NOT_FOUND must equal 2001; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::MODEL_NOT_FOUND), 2001);
}

TEST(AqlApiStability, ErrorCodeLoraNotFoundIs3001) {
    static_assert(static_cast<int>(LLMErrorCode::LORA_NOT_FOUND) == 3001,
        "LLMErrorCode::LORA_NOT_FOUND must equal 3001; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::LORA_NOT_FOUND), 3001);
}

TEST(AqlApiStability, ErrorCodeInferenceFailedIs4001) {
    static_assert(static_cast<int>(LLMErrorCode::INFERENCE_FAILED) == 4001,
        "LLMErrorCode::INFERENCE_FAILED must equal 4001; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::INFERENCE_FAILED), 4001);
}

TEST(AqlApiStability, ErrorCodeTimeoutIs4004) {
    static_assert(static_cast<int>(LLMErrorCode::TIMEOUT) == 4004,
        "LLMErrorCode::TIMEOUT must equal 4004; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::TIMEOUT), 4004);
}

TEST(AqlApiStability, ErrorCodeInternalErrorIs9001) {
    static_assert(static_cast<int>(LLMErrorCode::INTERNAL_ERROR) == 9001,
        "LLMErrorCode::INTERNAL_ERROR must equal 9001; this value is frozen");
    EXPECT_EQ(static_cast<int>(LLMErrorCode::INTERNAL_ERROR), 9001);
}

// =============================================================================
// ValidationLimits constants stability
// =============================================================================

TEST(AqlApiStability, MaxPromptLengthIs128000) {
    static_assert(ValidationLimits::MAX_PROMPT_LENGTH == 128000u,
        "ValidationLimits::MAX_PROMPT_LENGTH must remain 128000; frozen for API compat");
    EXPECT_EQ(ValidationLimits::MAX_PROMPT_LENGTH, 128000u);
}

TEST(AqlApiStability, MaxNlQueryLengthIs4096) {
    static_assert(ValidationLimits::MAX_NL_QUERY_LENGTH == 4096u,
        "ValidationLimits::MAX_NL_QUERY_LENGTH must remain 4096; frozen for API compat");
    EXPECT_EQ(ValidationLimits::MAX_NL_QUERY_LENGTH, 4096u);
}

TEST(AqlApiStability, MaxSchemaContextLengthIs32768) {
    static_assert(ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH == 32768u,
        "ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH must remain 32768; frozen for API compat");
    EXPECT_EQ(ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH, 32768u);
}

TEST(AqlApiStability, MaxRagTopKIs100) {
    static_assert(ValidationLimits::MAX_RAG_TOP_K == 100,
        "ValidationLimits::MAX_RAG_TOP_K must remain 100; frozen for API compat");
    EXPECT_EQ(ValidationLimits::MAX_RAG_TOP_K, 100);
}

TEST(AqlApiStability, MinRagTopKIs1) {
    static_assert(ValidationLimits::MIN_RAG_TOP_K == 1,
        "ValidationLimits::MIN_RAG_TOP_K must remain 1; frozen for API compat");
    EXPECT_EQ(ValidationLimits::MIN_RAG_TOP_K, 1);
}

// =============================================================================
// ConversationTurn struct field layout
// =============================================================================

TEST(AqlApiStability, ConversationTurnHasNlQueryField) {
    ConversationTurn turn;
    turn.nl_query = "test query";
    EXPECT_EQ(turn.nl_query, "test query");
}

TEST(AqlApiStability, ConversationTurnHasAqlResultField) {
    ConversationTurn turn;
    turn.aql_result = "FOR d IN docs RETURN d";
    EXPECT_EQ(turn.aql_result, "FOR d IN docs RETURN d");
}

// =============================================================================
// LLMAQLHandler::BatchInferRequest struct field layout
// =============================================================================

TEST(AqlApiStability, BatchInferRequestHasPromptField) {
    LLMAQLHandler::BatchInferRequest req;
    req.prompt = "Test prompt";
    EXPECT_EQ(req.prompt, "Test prompt");
}

TEST(AqlApiStability, BatchInferRequestHasModelIdField) {
    LLMAQLHandler::BatchInferRequest req;
    req.model_id = "my-model";
    EXPECT_EQ(req.model_id, "my-model");
}

TEST(AqlApiStability, BatchInferRequestHasLoraIdField) {
    LLMAQLHandler::BatchInferRequest req;
    req.lora_id = "my-lora";
    EXPECT_EQ(req.lora_id, "my-lora");
}

TEST(AqlApiStability, BatchInferRequestHasOptionsField) {
    LLMAQLHandler::BatchInferRequest req;
    req.options["max_tokens"] = "100";
    EXPECT_EQ(req.options.at("max_tokens"), "100");
}

// =============================================================================
// LLMAQLHandler::BatchNLToAQLRequest / BatchNLToAQLResult struct layout
// =============================================================================

TEST(AqlApiStability, BatchNLToAQLRequestHasNlQueryField) {
    LLMAQLHandler::BatchNLToAQLRequest req;
    req.nl_query = "Find all users";
    EXPECT_EQ(req.nl_query, "Find all users");
}

TEST(AqlApiStability, BatchNLToAQLRequestHasSchemaContextField) {
    LLMAQLHandler::BatchNLToAQLRequest req;
    req.schema_context = "users: {name, age}";
    EXPECT_EQ(req.schema_context, "users: {name, age}");
}

TEST(AqlApiStability, BatchNLToAQLResultHasAqlQueryField) {
    LLMAQLHandler::BatchNLToAQLResult res;
    res.aql_query = "FOR u IN users RETURN u";
    EXPECT_EQ(res.aql_query, "FOR u IN users RETURN u");
}

TEST(AqlApiStability, BatchNLToAQLResultHasErrorField) {
    LLMAQLHandler::BatchNLToAQLResult res;
    res.error = "translation failed";
    EXPECT_EQ(res.error, "translation failed");
}

TEST(AqlApiStability, BatchNLToAQLResultHasSuccessField) {
    LLMAQLHandler::BatchNLToAQLResult res;
    res.success = true;
    EXPECT_TRUE(res.success);
    res.success = false;
    EXPECT_FALSE(res.success);
}

// =============================================================================
// LLMAQLHandler::AQLTranslationResult struct layout
// =============================================================================

TEST(AqlApiStability, AQLTranslationResultHasAqlQueryField) {
    LLMAQLHandler::AQLTranslationResult result;
    result.aql_query = "FOR d IN docs RETURN d";
    EXPECT_EQ(result.aql_query, "FOR d IN docs RETURN d");
}

TEST(AqlApiStability, AQLTranslationResultHasConfidenceField) {
    // The confidence field is of type AQLConfidenceScore (from aql_confidence_scorer.h);
    // verify it is accessible on the AQLTranslationResult struct.
    LLMAQLHandler::AQLTranslationResult result;
    (void)result.confidence; // field must exist and be accessible
    SUCCEED();
}

// =============================================================================
// LLMAQLHandler::QueryConfidenceScore struct layout
// =============================================================================

TEST(AqlApiStability, QueryConfidenceScoreHasScoreField) {
    LLMAQLHandler::QueryConfidenceScore s;
    s.score = 0.9f;
    EXPECT_FLOAT_EQ(s.score, 0.9f);
}

TEST(AqlApiStability, QueryConfidenceScoreHasExplanationField) {
    LLMAQLHandler::QueryConfidenceScore s;
    s.explanation = "High confidence";
    EXPECT_EQ(s.explanation, "High confidence");
}

TEST(AqlApiStability, QueryConfidenceScoreHasSuggestionsField) {
    LLMAQLHandler::QueryConfidenceScore s;
    s.suggestions.push_back("Use LIMIT");
    EXPECT_EQ(s.suggestions.size(), 1u);
    EXPECT_EQ(s.suggestions[0], "Use LIMIT");
}

// =============================================================================
// LLMException stability
// =============================================================================

TEST(AqlApiStability, LLMExceptionCarriesErrorCode) {
    LLMException ex(LLMErrorCode::PROMPT_INJECTION, "injection detected");
    EXPECT_EQ(ex.getErrorCode(), LLMErrorCode::PROMPT_INJECTION);
}

TEST(AqlApiStability, LLMExceptionExtendsStdRuntimeError) {
    static_assert(
        std::is_base_of<std::runtime_error, LLMException>::value,
        "LLMException must remain derived from std::runtime_error");
    SUCCEED();
}

TEST(AqlApiStability, LLMExceptionGetSafeMessageNeverEmpty) {
    LLMException ex(LLMErrorCode::INTERNAL_ERROR, "internal detail");
    EXPECT_FALSE(ex.getSafeMessage().empty());
}

TEST(AqlApiStability, LLMExceptionWithCorrelationId) {
    LLMException ex(LLMErrorCode::TIMEOUT, "timed out", "req-123");
    EXPECT_EQ(ex.getCorrelationId(), "req-123");
}

// =============================================================================
// AQLConversationSession API stability
// =============================================================================

TEST(AqlApiStability, AQLConversationSessionStartsEmpty) {
    AQLConversationSession session;
    EXPECT_TRUE(session.empty());
    EXPECT_EQ(session.size(), 0u);
}

TEST(AqlApiStability, AQLConversationSessionAddTurnIncreasesSize) {
    AQLConversationSession session;
    session.addTurn("Find users", "FOR u IN users RETURN u");
    EXPECT_FALSE(session.empty());
    EXPECT_EQ(session.size(), 1u);
}

TEST(AqlApiStability, AQLConversationSessionGetHistoryReturnsAllTurns) {
    AQLConversationSession session;
    session.addTurn("Query A", "AQL A");
    session.addTurn("Query B", "AQL B");
    const auto& history = session.getHistory();
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].nl_query, "Query A");
    EXPECT_EQ(history[0].aql_result, "AQL A");
    EXPECT_EQ(history[1].nl_query, "Query B");
    EXPECT_EQ(history[1].aql_result, "AQL B");
}

TEST(AqlApiStability, AQLConversationSessionClearResetsHistory) {
    AQLConversationSession session;
    session.addTurn("Find users", "FOR u IN users RETURN u");
    EXPECT_EQ(session.size(), 1u);
    session.clear();
    EXPECT_TRUE(session.empty());
    EXPECT_EQ(session.size(), 0u);
}
